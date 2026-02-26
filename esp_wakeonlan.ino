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

bool sending = false;
int currentIndex = -1;
int packetsSent = 0;
unsigned long lastSend = 0;
const int interval = 2000; //ms
const int maxPackets = 5;

void wakePC(int index) {
    Serial.print("Wake avviato da web per ");
    Serial.println(MACAddress[index]);

    WOL.setRepeat(3, 100);
    WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());

    // inizializza stato
    currentIndex = index;
    packetsSent = 0;
    sending = true;
    lastSend = millis();
}

void setup() {
    // Mount LittleFS
    if (!LittleFS.begin()) {
        Serial.println("LittleFS non trovato, esegui l'upload dei file e riprova");
        return;
    }

    Serial.begin(115200);
    WiFi.begin(SSID, NET_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {}
    
    // Connection info on serial
    Serial.print("Connesso a ");
    Serial.println(SSID);
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // Server routes
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");    // Serve index.html for any unknown path (SPA fallback)
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

            if (device_index < 0 || device_index >= sizeof(MACAddress) / sizeof(MACAddress[0])) {
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

    server.begin();
}

void loop() {
    // Non blocking packets management
    if (sending && millis() - lastSend >= interval) {
        packetsSent++;
        Serial.printf("Mando pacchetto %d\n", packetsSent);
        WOL.sendMagicPacket(MACAddress[currentIndex], WAKEONLAN_PORT);
        lastSend = millis();

        if (packetsSent >= maxPackets) {
            sending = false;
            Serial.println("Terminato");
        }
    }
}
