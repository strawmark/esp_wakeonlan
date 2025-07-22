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

void wakePC(int index) {
    WOL.setRepeat(3, 100);
    WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());
    WOL.sendMagicPacket(MACAddress[index], WAKEONLAN_PORT);
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(SSID, NET_PASSWORD);
    
    while (WiFi.status() != WL_CONNECTED) {}
    
    // Server routes

    // 404 Not Found
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Error 404");
    });

    // Index page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
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
            return request->redirect("/");
        } else {
            return request->send(400, "text/plain", "Device parameter missing");
        }
    });

    server.begin();
}

void loop() {}
