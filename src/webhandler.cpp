#include "webhandler.h"
#include "schedule.h"
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
    server.on("/schedule/edit", HTTP_PUT, handleEditScheduleById);
    server.on("/schedule/delete", HTTP_DELETE, handleDeleteScheduleById);
    server.on("/schedule/sort", HTTP_POST, handleSortSchedule);
    server.on("/schedule-debug", HTTP_GET, handleScheduleDebug);
    
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
        } else if (millis() - wifiStartTime > 10000) { // 10 second timeout
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
        Serial.println("=== ADD SCHEDULE REQUEST ===");
        String body = server.arg("plain");
        Serial.printf("Request body: %s\n", body.c_str());
        
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            Serial.printf("JSON parse error: %s\n", error.c_str());
            server.send(400, "text/plain", "Invalid JSON");
            return;
        }
        
        uint8_t hour = doc["hour"] | 0;
        uint8_t minute = doc["minute"] | 0;
        String description = doc["description"] | "";
        
        Serial.printf("Parsed data: hour=%d, minute=%d, description='%s'\n", 
                     hour, minute, description.c_str());
        
        if (hour > 23 || minute > 59) {
            Serial.printf("Invalid time: %02d:%02d\n", hour, minute);
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid time\"}");
            return;
        }
        
        if (description.length() == 0) {
            Serial.println("Empty description");
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Description cannot be empty\"}");
            return;
        }
        
        Serial.printf("Calling addScheduleEntry(%d, %d, '%s')\n", hour, minute, description.c_str());
        
        if (addScheduleEntry(hour, minute, description)) {
            Serial.println("Schedule added successfully, sending 200 response");
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(200, "application/json", "{\"success\":true,\"message\":\"Schedule added\"}");
        } else {
            Serial.println("addScheduleEntry returned false, sending 400 response");
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Failed to add schedule\"}");
        }
        
        Serial.println("=== END ADD SCHEDULE REQUEST ===");
    } else {
        Serial.printf("Invalid method for ADD: %s\n", server.method() == HTTP_GET ? "GET" : "PUT");
        server.send(405, "text/plain", "Method Not Allowed");
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
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(200, "application/json", "{\"success\":true,\"message\":\"Schedule updated\"}");
        } else {
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Failed to update schedule\"}");
        }
    }
}

void handleEditScheduleById() {
    if (server.method() == HTTP_PUT) {
        Serial.println("=== EDIT SCHEDULE REQUEST ===");
        String body = server.arg("plain");
        Serial.printf("Request body: %s\n", body.c_str());
        
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            Serial.printf("JSON parse error: %s\n", error.c_str());
            server.send(400, "text/plain", "Invalid JSON");
            return;
        }
        
        uint32_t id = doc["id"] | 0;
        Serial.printf("Parsed ID: %u\n", id);
        
        if (id == 0) {
            Serial.println("Error: Invalid ID (0 or parsing failed)");
            server.send(400, "text/plain", "Invalid ID");
            return;
        }
        
        uint8_t hour = doc["hour"] | 0;
        uint8_t minute = doc["minute"] | 0;
        String description = doc["description"] | "";
        bool enabled = doc["enabled"] | true;
        
        Serial.printf("Updating schedule ID %u: %02d:%02d - %s (enabled: %s)\n", 
                     id, hour, minute, description.c_str(), enabled ? "true" : "false");
        
        if (editScheduleEntry(id, hour, minute, description, enabled)) {
            Serial.printf("Schedule ID %u updated successfully\n", id);
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(200, "application/json", "{\"success\":true,\"message\":\"Schedule updated\"}");
        } else {
            Serial.printf("Failed to update schedule ID %u\n", id);
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Failed to update schedule\"}");
        }
        
        Serial.println("=== END EDIT SCHEDULE REQUEST ===");
    } else {
        Serial.printf("Invalid method for EDIT by ID: %s\n", server.method() == HTTP_GET ? "GET" : "POST");
        server.send(405, "text/plain", "Method Not Allowed");
    }
}

void handleDeleteSchedule() {
    if (server.method() == HTTP_DELETE) {
        String idStr = server.arg("id");
        Serial.printf("DELETE request received for schedule ID: '%s'\n", idStr.c_str());
        
        if (idStr.length() == 0) {
            Serial.println("Error: No ID provided in DELETE request");
            server.send(400, "application/json", "{\"success\":false,\"message\":\"No ID provided\"}");
            return;
        }
        
        uint32_t id = idStr.toInt();
        Serial.printf("Parsed ID: %u\n", id);
        
        if (id == 0) {
            Serial.println("Error: Invalid ID (0 or parsing failed)");
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid ID\"}");
            return;
        }
        
        if (deleteScheduleEntry(id)) {
            Serial.printf("Schedule ID %u deleted successfully\n", id);
            server.send(200, "application/json", "{\"success\":true,\"message\":\"Schedule deleted\"}");
        } else {
            Serial.printf("Failed to delete schedule ID %u\n", id);
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Failed to delete schedule\"}");
        }
    } else {
        Serial.printf("Invalid method for DELETE: %s\n", server.method() == HTTP_GET ? "GET" : "POST");
        server.send(405, "text/plain", "Method Not Allowed");
    }
}

void handleDeleteScheduleById() {
    if (server.method() == HTTP_DELETE) {
        Serial.println("=== DELETE SCHEDULE REQUEST ===");
        String body = server.arg("plain");
        Serial.printf("Request body: %s\n", body.c_str());
        
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            Serial.printf("JSON parse error: %s\n", error.c_str());
            server.send(400, "text/plain", "Invalid JSON");
            return;
        }
        
        uint32_t id = doc["id"] | 0;
        Serial.printf("Parsed ID: %u\n", id);
        
        if (id == 0) {
            Serial.println("Error: Invalid ID (0 or parsing failed)");
            server.send(400, "text/plain", "Invalid ID");
            return;
        }
        
        Serial.printf("Attempting to delete schedule ID %u\n", id);
        
        if (deleteScheduleEntry(id)) {
            Serial.printf("Schedule ID %u deleted successfully\n", id);
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(200, "application/json", "{\"success\":true,\"message\":\"Schedule deleted\"}");
        } else {
            Serial.printf("Failed to delete schedule ID %u\n", id);
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Failed to delete schedule\"}");
        }
        
        Serial.println("=== END DELETE SCHEDULE REQUEST ===");
    } else {
        Serial.printf("Invalid method for DELETE by ID: %s\n", server.method() == HTTP_GET ? "GET" : "POST");
        server.send(405, "text/plain", "Method Not Allowed");
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

void handleScheduleDebug() {
    if (server.method() == HTTP_GET) {
        Serial.println("=== SCHEDULE DEBUG INFO ===");
        
        // Get schedule JSON
        String scheduleJson = getScheduleJSON();
        Serial.printf("Schedule JSON: %s\n", scheduleJson.c_str());
        
        // Create debug response
        DynamicJsonDocument doc(2048);
        doc["timestamp"] = millis();
        doc["schedule_count"] = 0; // Will be filled by schedule module
        
        // Add schedule entries
        JsonArray array = doc.createNestedArray("schedules");
        // Note: We can't directly access scheduleEntries here, so we'll parse the JSON
        
        DynamicJsonDocument scheduleDoc(2048);
        DeserializationError error = deserializeJson(scheduleDoc, scheduleJson);
        
        if (!error) {
            JsonArray scheduleArray = scheduleDoc.as<JsonArray>();
            for (JsonObject entry : scheduleArray) {
                JsonObject debugEntry = array.createNestedObject();
                debugEntry["id"] = entry["id"];
                debugEntry["hour"] = entry["hour"];
                debugEntry["minute"] = entry["minute"];
                debugEntry["enabled"] = entry["enabled"];
                debugEntry["description"] = entry["description"];
            }
            doc["schedule_count"] = array.size();
        } else {
            doc["parse_error"] = "Failed to parse schedule JSON";
        }
        
        // Add SPIFFS info
        doc["spiffs_total"] = SPIFFS.totalBytes();
        doc["spiffs_used"] = SPIFFS.usedBytes();
        doc["spiffs_free"] = SPIFFS.totalBytes() - SPIFFS.usedBytes();
        
        String result;
        serializeJson(doc, result);
        
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "application/json", result);
        
        Serial.println("=== END SCHEDULE DEBUG ===");
    }
}

void handleSortSchedule() {
    if (server.method() == HTTP_POST) {
        Serial.println("=== MANUAL SORT SCHEDULE REQUEST ===");
        
        // Perform manual sorting
        sortScheduleByTime();
        
        Serial.println("Manual sort completed, sending response");
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Schedule sorted by time\"}");
        
        Serial.println("=== END MANUAL SORT REQUEST ===");
    } else {
        Serial.printf("Invalid method for SORT: %s\n", server.method() == HTTP_GET ? "GET" : "PUT");
        server.send(405, "text/plain", "Method Not Allowed");
    }
}
