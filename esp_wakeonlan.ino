#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WakeOnLan.h>
#include <WiFiUdp.h>

#include "webpages.h"
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
    Serial.begin(115200);
    WiFi.begin(SSID, NET_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {}
    
    // Connection info on serial
    Serial.print("Connesso a ");
    Serial.println(SSID);
    Serial.print("IP: ");
    Serial.print(WiFi.localIP());

    // Server routes

    // Index instead of 404 Not Found 
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send_P(200,"text/html",INDEX_HTML,NULL);
    });
    // Index page
    server.on(root, HTTP_GET, [](AsyncWebServerRequest *request) {
        if(!request->authenticate(USERNAME,ESP_PASSWORD))
            return request->requestAuthentication();
        request->send_P(200,"text/html",INDEX_HTML,NULL);
    });
    // Wake on LAN endpoint
    server.on("/wake", HTTP_POST, [](AsyncWebServerRequest *request) {
        if(!request->authenticate(USERNAME,ESP_PASSWORD))
            return request->requestAuthentication();
        if (request->hasParam("device", true)) {
            int device_index = request->getParam("device", true)->value().toInt();
            if (device_index < 0 || device_index >= sizeof(MACAddress) / sizeof(MACAddress[0]))
                return request->send(400, "text/plain", "Invalid device index");
            wakePC(device_index);
            return request->redirect(root);
        } else {
            return request->send(400, "text/plain", "Device parameter missing");
        }
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
