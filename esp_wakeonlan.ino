#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <WakeOnLan.h>
#include <WiFiUdp.h>

#include "rest_api.h"

WiFiUDP UDP;
WakeOnLan WOL(UDP);
AsyncWebServer server(80);
AsyncEventSource events("/events");

bool sending = false;
int currentIndex = -1;
int packetsSent = 0;
unsigned long lastSend = 0;
const int interval = 2000; //ms
const int maxPackets = 5;

std::vector<Device> devices;

void loadDevicesFromFS() {
    File f = LittleFS.open("/devices.json", "r");
    
    if (!f) {
        Serial.println("devices.json: file non trovato");
        return;
    }
    
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err){
        Serial.println("devices.json: errore di parsing");
        return;
    }

    devices.clear();

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
        Device device;
        device.index = obj["index"].as<int>();
        device.name = obj["name"].as<String>();
        device.mac = obj["mac"].as<String>();
        devices.push_back(device);
    }

    Serial.printf("Caricati %d dispositivi da devices.json\n", (int)devices.size());
}

void saveDevicesToFS() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();  

    for (const Device& dev : devices) {
        JsonObject obj = arr.add<JsonObject>();
        obj["index"] = dev.index;
        obj["name"] = dev.name; 
        obj["mac"] = dev.mac;
    }   

    File f = LittleFS.open("/devices.json", "w");

    if (!f) {
        Serial.println("devices.json: impossibile scrivere il file");
        return;
    }

    serializeJson(doc, f);
    f.close();
    
    Serial.println("Dati salvati");

    loadDevicesFromFS(); // Refresh device list
}

// SSE broadcast function
void sendStatus(){
    JsonDocument resp;
    resp["sending"] = sending;
    resp["currentIndex"] = currentIndex;
    resp["packetsSent"] = packetsSent;
    resp["lastSend"] = lastSend;

    String output;
    serializeJson(resp, output);
    events.send(output, "status");
}

void wakePC(int index) {
    Serial.print("Wake avviato da web per ");
    Serial.println(devices[index].name);

    WOL.setRepeat(3, 100);
    WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());

    // inizializza stato
    currentIndex = index;
    packetsSent = 0;
    sending = true;
    lastSend = millis();

    // Broadcast new status via SSE
    sendStatus();
}

void setup() {
    Serial.begin(115200);

    // Mount LittleFS
    if (!LittleFS.begin()) {
        Serial.println("LittleFS non trovato, esegui l'upload dei file e riprova");
        return;
    }

    loadDevicesFromFS();
    
    WiFi.begin(SSID, NET_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {}
    
    // Connection info on serial
    Serial.print("Connesso a ");
    Serial.println(SSID);
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    setupRestAPI(server);

    // Index page route
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        File f = LittleFS.open("/index.html", "r");
        if (!f) {
            request->send(404, "text/plain", "File not found");
            return;
        }
        request->send(LittleFS, "/index.html", "text/html");

    });
    
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");    // Serve index.html for any unknown path
    
    server.onNotFound([](AsyncWebServerRequest *request) {
        File f = LittleFS.open("/index.html", "r");
        if (!f) {
            request->send(404, "text/plain", "File not found");
            return;
        }
        request->send(LittleFS, "/index.html", "text/html");
    });
    
    // Register SSE endpoint
    server.addHandler(&events);
    server.begin();
}

void loop() {
    // Non blocking packets management
    if (sending && millis() - lastSend >= interval) {
        ++packetsSent;

        Serial.print("Mando pacchetto ");
        Serial.print(packetsSent);
        Serial.print(" verso ");
        Serial.println(devices[currentIndex].mac);
        
        WOL.sendMagicPacket(devices[currentIndex].mac, WAKEONLAN_PORT);
        lastSend = millis();

        // Broadcast status after each packet
        sendStatus();

        if (packetsSent >= maxPackets) {
            sending = false;
            Serial.println("Terminato");
            sendStatus();
        }
    }
}
