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

// REST API routes

void setupRestAPI(AsyncWebServer &server) {
    
    // POST /api/wake : Wake a device
    server.on("/api/wake", HTTP_POST,[](AsyncWebServerRequest *request){},nullptr,[](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        static String body;

        if (index == 0){body.clear();} // Reset body on new request

        body += (char*)data;

        if (index + len == total) {

            JsonDocument doc;
            if (deserializeJson(doc, body)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            int device_index = doc["device"] | -1;  // Treat missing device key as an invalid device value

            if (device_index < 0 || device_index >= (int)devices.size()) {
                request->send(400, "application/json", "{\"error\":\"Invalid device index or device parameter not found\"}");
                return;
            }

            wakePC(device_index);

            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    });

    // GET /api/wake/status : Get status of the sending process
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

    // GET /api/devices/ : Get devices list
    server.on("/api/devices", HTTP_GET,[](AsyncWebServerRequest *request) {
        String output;    
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();

        for (int i = 0; i < (int)devices.size(); i++) {
            JsonObject obj = arr.add<JsonObject>();
            obj["index"] = i;
            obj["name"] = devices[i].name;
            obj["mac"] = devices[i].mac;
        }
        serializeJson(doc, output);

        request->send(200, "application/json", output);
    });

    // POST /api/devices : Add a new device
    server.on("/api/devices", HTTP_POST,[](AsyncWebServerRequest *request){},nullptr,[](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        static String body;
        JsonDocument doc;
        if (index == 0){body.clear();} // Reset body on new request

        body += (char*)data;

        if (index + len == total) {

            if (deserializeJson(doc, body)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            if (!doc.containsKey("index") || !doc.containsKey("name") || !doc.containsKey("mac")) {
                request->send(400, "application/json", "{\"error\":\"Missing fields\"}");
                return;
            }
            
            // Add the device to the list and save it permanently
            Device newDev;
            newDev.index = doc["index"].as<int>();
            newDev.name = doc["name"].as<String>();
            newDev.mac = doc["mac"].as<String>();
            devices.push_back(newDev);
            saveDevicesToFS();

            // Send SSE Event to prompt a list refresh
            events.send("update", "devices-changed", millis());

            request->send(201, "application/json", "{\"status\":\"success\"}");
        }
    });
}