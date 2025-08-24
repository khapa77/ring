#include "webhandler.h"
#include <WiFi.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* sta_ssid = "YOUR_HOME_SSID";
const char* sta_password = "YOUR_HOME_PASSWORD";
const char* ap_ssid = "GonggonG";
const char* ap_password = "vipassana";

// Web server instance
WebServer server(WEB_SERVER_PORT);

// WiFi state
bool apMode = false;
unsigned long wifiStartTime = 0;

void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(sta_ssid, sta_password);
    wifiStartTime = millis();
    
    Serial.println("Connecting to WiFi...");
    Serial.printf("SSID: %s\n", sta_ssid);
}

void setupWebServer() {
    // Set up API endpoints
    server.on("/", HTTP_GET, handleRoot);
    server.on("/schedule", HTTP_GET, handleSchedule);
    server.on("/schedule", HTTP_POST, handleAddSchedule);
    server.on("/schedule", HTTP_PUT, handleEditSchedule);
    server.on("/schedule", HTTP_DELETE, handleDeleteSchedule);
    server.on("/play", HTTP_POST, handlePlay);
    server.on("/play-lora", HTTP_POST, handlePlayLoRa);
    
    // Handle not found
    server.onNotFound(handleNotFound);
    
    // Enable CORS
    server.enableCORS(true);
    
    // Start server
    server.begin();
    Serial.println("Web server started");
}

void loopWebServer() {
    // Check WiFi connection status
    if (!apMode) {
        if (WiFi.status() == WL_CONNECTED) {
            // WiFi connected, handle web server
            server.handleClient();
        } else if (millis() - wifiStartTime > WIFI_TIMEOUT) {
            // WiFi connection failed, switch to AP mode
            Serial.println("WiFi connection failed, switching to AP mode");
            setupAPMode();
        }
    } else {
        // AP mode, handle web server
        server.handleClient();
    }
}

void setupAPMode() {
    WiFi.mode(WIFI_AP);
    bool result = WiFi.softAP(ap_ssid, ap_password);
    
    if (result) {
        apMode = true;
        Serial.printf("AP started: %s\n", ap_ssid);
        Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("AP start failed!");
    }
}

void handleRoot() {
    // Serve the main HTML page
    if (SPIFFS.exists("/index.html")) {
        File file = SPIFFS.open("/index.html", "r");
        server.streamFile(file, "text/html");
        file.close();
    } else {
        server.send(200, "text/html", "<h1>ESP32 Gong Scheduler</h1><p>index.html not found</p>");
    }
}

void handleSchedule() {
    if (server.method() == HTTP_GET) {
        // Return schedule as JSON
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "application/json", getScheduleJSON());
    }
}

void handleAddSchedule() {
    if (server.method() == HTTP_POST) {
        String body = server.arg("plain");
        
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            server.send(400, "text/plain", "Invalid JSON");
            return;
        }
        
        uint8_t hour = doc["hour"] | 0;
        uint8_t minute = doc["minute"] | 0;
        String description = doc["description"] | "";
        
        if (addScheduleEntry(hour, minute, description)) {
            server.send(200, "application/json", "{\"success\":true,\"message\":\"Schedule added\"}");
        } else {
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Failed to add schedule\"}");
        }
    }
}

void handleEditSchedule() {
    if (server.method() == HTTP_PUT) {
        String body = server.arg("plain");
        
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            server.send(400, "text/plain", "Invalid JSON");
            return;
        }
        
        uint32_t id = doc["id"] | 0;
        uint8_t hour = doc["hour"] | 0;
        uint8_t minute = doc["minute"] | 0;
        String description = doc["description"] | "";
        
        if (editScheduleEntry(id, hour, minute, description)) {
            server.send(200, "application/json", "{\"success\":true,\"message\":\"Schedule updated\"}");
        } else {
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Failed to update schedule\"}");
        }
    }
}

void handleDeleteSchedule() {
    if (server.method() == HTTP_DELETE) {
        String idStr = server.arg("id");
        uint32_t id = idStr.toInt();
        
        if (deleteScheduleEntry(id)) {
            server.send(200, "application/json", "{\"success\":true,\"message\":\"Schedule deleted\"}");
        } else {
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Failed to delete schedule\"}");
        }
    }
}

void handlePlay() {
    if (server.method() == HTTP_POST) {
        playGong();
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Gong played locally\"}");
    }
}

void handlePlayLoRa() {
    if (server.method() == HTTP_POST) {
        sendGongLoRa();
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Gong sent via LoRa\"}");
    }
}

void handleNotFound() {
    server.send(404, "text/plain", "Not found");
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String getWiFiStatus() {
    if (apMode) {
        return "AP Mode: " + String(ap_ssid);
    } else if (WiFi.status() == WL_CONNECTED) {
        return "Connected: " + WiFi.localIP().toString();
    } else {
        return "Connecting...";
    }
}
