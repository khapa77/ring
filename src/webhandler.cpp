#include "webhandler.h"
#include <WiFi.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ap_ssid = "GonggonG";
const char* ap_password = "vipassana";

// WiFi configuration
WiFiConfig wifiConfig;
bool apMode = false;
unsigned long wifiStartTime = 0;

// Web server instance
WebServer server(WEB_SERVER_PORT);

void setupWiFi() {
    // Load WiFi configuration from SPIFFS
    if (loadWiFiConfig() && wifiConfig.configured) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifiConfig.ssid, wifiConfig.password);
        wifiStartTime = millis();
        
        Serial.println("Connecting to WiFi...");
        Serial.printf("SSID: %s\n", wifiConfig.ssid);
    } else {
        Serial.println("No WiFi configuration found, starting AP mode");
        setupAPMode();
    }
}

void setupWebServer() {
    // Set up API endpoints
    server.on("/", HTTP_GET, handleRoot);
    server.on("/schedule", HTTP_GET, handleSchedule);
    server.on("/schedule", HTTP_POST, handleAddSchedule);
    server.on("/schedule", HTTP_PUT, handleEditSchedule);
    server.on("/schedule", HTTP_DELETE, handleDeleteSchedule);
    
    // Add specific ID-based routes for better REST API support
    server.on("/schedule/", HTTP_PUT, handleEditScheduleById);
    server.on("/schedule/", HTTP_DELETE, handleDeleteScheduleById);
    
    server.on("/play", HTTP_POST, handlePlay);
    server.on("/play-lora", HTTP_POST, handlePlayLoRa);
    server.on("/wifi-config", HTTP_GET, handleWiFiConfig);
    server.on("/wifi-save", HTTP_POST, handleWiFiSave);
    server.on("/wifi-reset", HTTP_POST, handleWiFiReset);
    server.on("/wifi-status", HTTP_GET, handleWiFiStatus);
    
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
        bool enabled = doc["enabled"] | true;
        
        if (editScheduleEntry(id, hour, minute, description, enabled)) {
            server.send(200, "application/json", "{\"success\":true,\"message\":\"Schedule updated\"}");
        } else {
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Failed to update schedule\"}");
        }
    }
}

void handleEditScheduleById() {
    if (server.method() == HTTP_PUT) {
        String uri = server.uri();
        // Extract ID from URI like "/schedule/123"
        int idStart = uri.lastIndexOf('/') + 1;
        String idStr = uri.substring(idStart);
        uint32_t id = idStr.toInt();
        
        if (id == 0) {
            server.send(400, "text/plain", "Invalid ID");
            return;
        }
        
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
        bool enabled = doc["enabled"] | true;
        
        if (editScheduleEntry(id, hour, minute, description, enabled)) {
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

void handleDeleteScheduleById() {
    if (server.method() == HTTP_DELETE) {
        String uri = server.uri();
        // Extract ID from URI like "/schedule/123"
        int idStart = uri.lastIndexOf('/') + 1;
        String idStr = uri.substring(idStart);
        uint32_t id = idStr.toInt();
        
        if (id == 0) {
            server.send(400, "text/plain", "Invalid ID");
            return;
        }
        
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

void handleWiFiConfig() {
    if (server.method() == HTTP_GET) {
        // Return current WiFi configuration
        DynamicJsonDocument doc(256);
        doc["ssid"] = wifiConfig.ssid;
        doc["configured"] = wifiConfig.configured;
        doc["connected"] = WiFi.status() == WL_CONNECTED;
        doc["ap_mode"] = apMode;
        
        String result;
        serializeJson(doc, result);
        server.send(200, "application/json", result);
    }
}

void handleWiFiSave() {
    if (server.method() == HTTP_POST) {
        String body = server.arg("plain");
        
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            server.send(400, "text/plain", "Invalid JSON");
            return;
        }
        
        String ssid = doc["ssid"] | "";
        String password = doc["password"] | "";
        
        if (ssid.length() == 0) {
            server.send(400, "application/json", "{\"success\":false,\"message\":\"SSID cannot be empty\"}");
            return;
        }
        
        if (saveWiFiConfig(ssid, password)) {
            server.send(200, "application/json", "{\"success\":true,\"message\":\"WiFi configuration saved. Device will restart to connect.\"}");
            
            // Restart after a short delay to apply new WiFi settings
            delay(1000);
            ESP.restart();
        } else {
            server.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save WiFi configuration\"}");
        }
    }
}

void handleWiFiReset() {
    if (server.method() == HTTP_POST) {
        resetWiFiConfig();
        server.send(200, "application/json", "{\"success\":true,\"message\":\"WiFi configuration reset. Device will restart.\"}");
        
        // Restart after a short delay
        delay(1000);
        ESP.restart();
    }
}

void handleWiFiStatus() {
    if (server.method() == HTTP_GET) {
        DynamicJsonDocument doc(256);
        doc["ssid"] = wifiConfig.ssid;
        doc["connected"] = WiFi.status() == WL_CONNECTED;
        doc["ap_mode"] = apMode;
        
        if (WiFi.status() == WL_CONNECTED) {
            doc["ip"] = WiFi.localIP().toString();
        }
        
        String result;
        serializeJson(doc, result);
        server.send(200, "application/json", result);
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

// WiFi configuration functions
bool loadWiFiConfig() {
    // First try to load from gong.conf
    if (SPIFFS.exists("/gong.conf")) {
        File file = SPIFFS.open("/gong.conf", "r");
        if (file) {
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, file);
            file.close();
            
            if (!error && doc.containsKey("wifi")) {
                JsonObject wifi = doc["wifi"];
                strlcpy(wifiConfig.ssid, wifi["ssid"] | "", sizeof(wifiConfig.ssid));
                strlcpy(wifiConfig.password, wifi["password"] | "", sizeof(wifiConfig.password));
                wifiConfig.configured = wifi["configured"] | false;
                
                Serial.printf("WiFi config loaded from gong.conf: SSID=%s, configured=%s\n", 
                             wifiConfig.ssid, wifiConfig.configured ? "true" : "false");
                return true;
            }
        }
    }
    
    // Fallback to wifi.conf
    if (SPIFFS.exists(WIFI_CONFIG_FILE)) {
        File file = SPIFFS.open(WIFI_CONFIG_FILE, "r");
        if (!file) {
            Serial.println("Failed to open WiFi config file");
            return false;
        }
        
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        
        if (error) {
            Serial.println("Failed to parse WiFi config file");
            return false;
        }
        
        strlcpy(wifiConfig.ssid, doc["ssid"] | "", sizeof(wifiConfig.ssid));
        strlcpy(wifiConfig.password, doc["password"] | "", sizeof(wifiConfig.password));
        wifiConfig.configured = doc["configured"] | false;
        
        Serial.printf("WiFi config loaded from wifi.conf: SSID=%s, configured=%s\n", 
                     wifiConfig.ssid, wifiConfig.configured ? "true" : "false");
        
        return true;
    }
    
    Serial.println("No WiFi config file found");
    return false;
}

bool saveWiFiConfig(const String& ssid, const String& password) {
    File file = SPIFFS.open(WIFI_CONFIG_FILE, "w");
    if (!file) {
        Serial.println("Failed to open WiFi config file for writing");
        return false;
    }
    
    DynamicJsonDocument doc(512);
    doc["ssid"] = ssid;
    doc["password"] = password;
    doc["configured"] = true;
    
    serializeJson(doc, file);
    file.close();
    
    // Update local config
    strlcpy(wifiConfig.ssid, ssid.c_str(), sizeof(wifiConfig.ssid));
    strlcpy(wifiConfig.password, password.c_str(), sizeof(wifiConfig.password));
    wifiConfig.configured = true;
    
    Serial.printf("WiFi config saved: SSID=%s\n", ssid.c_str());
    return true;
}

void resetWiFiConfig() {
    if (SPIFFS.exists(WIFI_CONFIG_FILE)) {
        SPIFFS.remove(WIFI_CONFIG_FILE);
    }
    
    wifiConfig.configured = false;
    wifiConfig.ssid[0] = '\0';
    wifiConfig.password[0] = '\0';
    
    Serial.println("WiFi configuration reset");
}
