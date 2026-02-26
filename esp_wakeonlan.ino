#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <WakeOnLan.h>
#include <WiFiUdp.h>


#include "config.h"

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

    // Server routes
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");    // Serve index.html for any unknown path

    server.onNotFound([](AsyncWebServerRequest *request) {
        File f = LittleFS.open("/index.html", "r");
        if (!f) {
            request->send(404, "text/plain", "File not found");
            return;
        }
        request->send(LittleFS, "/index.html", "text/html");
    });

    // Index page route
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        File f = LittleFS.open("/index.html", "r");
        if (!f) {
            request->send(404, "text/plain", "File not found");
            return;
        }
        request->send(LittleFS, "/index.html", "text/html");
    });

    // REST API: Wake on LAN
    server.on("/api/wake",HTTP_POST,[](AsyncWebServerRequest *request){},nullptr,[](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        static String body; // body container
        
        if (index == 0) {
            body.clear(); // Clear before appending the new HTTP Request
        }
        
        body += (char*)data;

        if (index + len == total) {
            DynamicJsonDocument doc(200);
            DeserializationError err = deserializeJson(doc, body.c_str());

            if (err) {
                return request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            }
            
            int device_index = doc["device"] | -1; // Treat missing device key as an invalid device value

            if (device_index < 0 || device_index >= (int)devices.size()) {
                return request->send(400, "application/json", "{\"error\":\"Invalid device index or device parameter not found\"}");
            }

            wakePC(device_index);
            
            JsonObject resp = doc.to<JsonObject>();
            resp["status"] = "ok";
            String output;
            serializeJson(resp, output);
            request->send(200, "application/json", output);
        }
    });

    // REST API: Status
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument resp;
        resp["sending"] = sending;
        resp["currentIndex"] = currentIndex;
        resp["packetsSent"] = packetsSent;
        resp["lastSend"] = lastSend;
        String output;
        serializeJson(resp, output);
        return request->send(200, "application/json", output);
    });

    // REST API: Devices list
    server.on("/api/devices", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(200);
        JsonArray arr = doc.createNestedArray();
        for (int i=0; i < (int)devices.size(); ++i){
            JsonObject obj = arr.createNestedObject();
            obj["index"] = i;
            obj["name"] = devices[i].name;
            obj["mac"] = devices[i].mac;  
        }
        String output;
        serializeJson(doc, output);
        return request->send(200, "application/json", output);
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
