#include <Arduino.h>
#include <SPIFFS.h>
#include "config.h"
#include "webhandler.h"
#include "lorahandler.h"
#include "mp3handler.h"
#include "schedule.h"

// Global state
unsigned long lastScheduleCheck = 0;
unsigned long lastStatusPrint = 0;
unsigned long lastWatchdogReset = 0;

void setup() {
    if (DEBUG_SERIAL_ENABLED) {
        Serial.begin(DEBUG_SERIAL_BAUD);
        Serial.println("\n=== " SYSTEM_NAME " v" SYSTEM_VERSION " ===");
        Serial.println("Build: " SYSTEM_BUILD_DATE);
    }
    
    // Initialize SPIFFS first
    if (!SPIFFS.begin(true)) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("SPIFFS initialization failed!");
        }
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
    
    if (DEBUG_SERIAL_ENABLED) {
        Serial.println("System initialization complete!");
    }
}

void loop() {
    unsigned long currentTime = millis();
    
    // Handle web server
    loopWebServer();
    
    // Handle LoRa communication
    loopLoRa();
    
    // Handle MP3 module
    loopMP3();
    
    // Check schedule periodically
    if (currentTime - lastScheduleCheck >= SCHEDULE_CHECK_INTERVAL) {
        checkSchedule();
        lastScheduleCheck = currentTime;
    }
    
    // Print system status periodically
    if (currentTime - lastStatusPrint >= STATUS_PRINT_INTERVAL) {
        if (DEBUG_SERIAL_ENABLED) {
            // printSystemStatus(); // TODO: Implement this function
        }
        lastStatusPrint = currentTime;
    }
    
    // Reset watchdog periodically
    if (currentTime - lastWatchdogReset >= WATCHDOG_RESET_INTERVAL) {
        // Feed the watchdog
        yield();
        lastWatchdogReset = currentTime;
    }
    
    // Small non-blocking delay using yield() instead of delay()
    yield();
}

// Additional utility functions
void printSystemStatus() {
    Serial.println("\n--- System Status ---");
    Serial.printf("System: %s v%s\n", SYSTEM_NAME, SYSTEM_VERSION);
    Serial.printf("WiFi: %s\n", getWiFiStatus().c_str());
    
    if (isLoRaInitialized()) {
        Serial.printf("LoRa: Initialized (State: %d, Errors: %d, Success: %d, Memory Errors: %d, Logs: %d)\n", 
                     getLoRaState(), getLoRaErrorCount(), getLoRaSuccessCount(), getLoRaMemoryErrors(), getLoRaLogCount());
    } else {
        Serial.println("LoRa: Not initialized");
    }
    
    Serial.printf("MP3: Initialized\n");
    Serial.printf("Schedule: %d entries\n", 0); // TODO: Get actual count
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("SPIFFS: %d bytes used\n", SPIFFS.usedBytes());
    Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
    Serial.println("-------------------");
}
