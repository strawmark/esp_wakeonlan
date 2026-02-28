#include "rest_api.h"
#include <ArduinoJson.h>

// Extern from esp_wakeonlan.ino

extern std::vector<Device> devices;

extern bool sending;
extern int currentIndex;
extern int packetsSent;
extern unsigned long lastSend;

extern void wakePC(int index);
extern void saveDevicesToFS();

extern AsyncEventSource events;

int getValidIndex(String idStr, size_t maxLimit) {
    if (idStr.length() == 0) return -1;
    
    for (char c : idStr) {
        if (!isDigit(c)) return -1;
    }
    
    int i = idStr.toInt();
    if (i >= 0 && (size_t)i < maxLimit) {
        return i;
    }
    return -1;
}

// REST API routes

void setupRestAPI(AsyncWebServer &server) {
    
    //Get status of the sending process
    server.on("/api/wake/status", HTTP_GET,[](AsyncWebServerRequest *request) {
        String output;
        JsonDocument resp;

        resp["sending"] = sending;
        resp["currentIndex"] = currentIndex;
        resp["packetsSent"] = packetsSent;
        resp["lastSend"] = lastSend;
        serializeJson(resp, output);
        
        request->send(200, "application/json", output);
    });

    // Delete device from the list
    server.on("/api/devices*", HTTP_DELETE,[](AsyncWebServerRequest *request) {
        String idStr = request->url().substring(13);
        int i = getValidIndex(idStr, devices.size());

        if (i == -1) {
            request->send(404, "application/json", "{\"error\":\"device not found\"}");
            return;
        }

        devices.erase(devices.begin() + i);
        saveDevicesToFS();
        events.send("update", "devices-changed", millis());

        request->send(200, "application/json", "{\"status\":\"success\", \"index\":" + String(i) + "}");
    });
    
    // POST /api/devices
    server.on("/api/devices*", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Requests without body (/wake)
        String url = request->url();

        if (url.endsWith("/wake")) {
            String idStr = url.substring(13, url.length() - 5);
            int i = getValidIndex(idStr, devices.size());

            if (i == -1) {
                request->send(404, "application/json", "{\"error\":\"device not found\"}");
                return;
            }

            wakePC(i);
            request->send(200, "application/json", "{\"status\":\"success\", \"id\":" + String(i) + "}");
        }
        
    }, 
    nullptr, // Other POST requests on /api/devices/* with body
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        
        String url = request->url();

        // Only process if it's the root path (adding a device)
        if (url == "/api/devices" || url == "/api/devices/") {
            static String body;
            if (index == 0) { body.clear(); }
            body += (char*)data;

            if (index + len == total) {
                JsonDocument doc;
                if (deserializeJson(doc, body)) {
                    request->send(400, "application/json", "{\"error\":\"invalid JSON\"}");
                    return;
                }

                if (!doc.containsKey("name") || !doc.containsKey("mac")) {
                    request->send(400, "application/json", "{\"error\":\"missing fields\"}");
                    return;
                }
                
                Device newDev;
                newDev.index = devices.size(); //doc["index"].as<int>();
                newDev.name = doc["name"].as<String>();
                newDev.mac = doc["mac"].as<String>();
                devices.push_back(newDev);
                saveDevicesToFS();
                
                events.send("update", "devices-changed", millis());
                request->send(201, "application/json", "{\"status\":\"success\"}");
            }
        }
    });
    
    server.on("/api/devices*", HTTP_GET, [](AsyncWebServerRequest *request) {
        String url = request->url();

        if (url == "/api/devices" || url == "/api/devices/"){  // root url: get device list
            JsonDocument doc;
            JsonArray arr = doc.to<JsonArray>();

            for (size_t i = 0; i < devices.size(); i++) {
                JsonObject obj = arr.add<JsonObject>();
                obj["index"] = (int)i;
                obj["name"] = devices[i].name;
                obj["mac"] = devices[i].mac;
            }

            String output;
            serializeJson(doc, output);
            request->send(200, "application/json", output);
            return;
        } else {
            String idStr = url.substring(13);
            int i = getValidIndex(idStr, devices.size());

            if (i == -1) {
                request->send(404, "application/json", "{\"error\":\"device not found\"}");
                return;
            }

            JsonDocument doc;
            doc["index"] = i;
            doc["name"] = devices[i].name;
            doc["mac"] = devices[i].mac;
            String output;
            serializeJson(doc, output);
            request->send(200, "application/json", output);
        }
    });
}