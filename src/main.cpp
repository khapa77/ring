#include <Arduino.h>
#include <SPIFFS.h>
#include "webhandler.h"
#include "lorahandler.h"
#include "mp3handler.h"
#include "schedule.h"

// Global state
unsigned long lastScheduleCheck = 0;
const unsigned long SCHEDULE_CHECK_INTERVAL = 1000; // Check every second

void setup() {
    delay(10000);
    Serial.begin(115200);
    Serial.println("\n=== ESP32 Gong/Ring System ===");
    
    // Initialize SPIFFS first
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed!");
        return;
    }
    
    // Initialize all modules
    setupWiFi();
    setupWebServer();
    setupLoRa();
    setupMP3();
    setupSchedule();
    
    // Set up callbacks
    onGongTrigger = playGong;
    
    Serial.println("System initialization complete!");
}

void loop() {
    // Handle web server
    loopWebServer();
    
    // Handle LoRa communication
    loopLoRa();
    
    // Handle MP3 module
    loopMP3();
    
    // Check schedule periodically
    if (millis() - lastScheduleCheck >= SCHEDULE_CHECK_INTERVAL) {
        checkSchedule();
        lastScheduleCheck = millis();
    }
    
    // Small delay to prevent watchdog issues
    delay(10);
}

// Additional utility functions
void printSystemStatus() {
    Serial.println("\n--- System Status ---");
    Serial.printf("WiFi: %s\n", getWiFiStatus().c_str());
    Serial.printf("LoRa: Initialized\n");
    Serial.printf("MP3: Initialized\n");
    Serial.printf("Schedule: %d entries\n", 0); // TODO: Get actual count
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("SPIFFS: %d bytes used\n", SPIFFS.usedBytes());
    Serial.println("-------------------");
}
