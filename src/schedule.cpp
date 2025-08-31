#include "schedule.h"
#include <SPIFFS.h>
#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define MAX_SCHEDULE_ENTRIES 20
#define SCHEDULE_FILE "/schedule.json"
#define GONG_CONFIG_FILE "/gong.conf"

ScheduleEntry scheduleEntries[MAX_SCHEDULE_ENTRIES];
uint8_t scheduleCount = 0;
uint32_t nextScheduleId = 1;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// External callback for gong trigger
extern void (*onGongTrigger)();

void setupSchedule() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed");
        return;
    }
    
    loadScheduleFromSPIFFS();
    
    // If no schedules exist, load defaults from gong.conf
    if (scheduleCount == 0) {
        loadDefaultSchedules();
    }
    
    // Initialize NTP client
    timeClient.begin();
    timeClient.setTimeOffset(0); // Will be set based on timezone
    timeClient.setUpdateInterval(60000); // Update every minute
    
    Serial.println("Schedule module initialized");
}

void checkSchedule() {
    timeClient.update();
    
    if (!timeClient.isTimeSet()) {
        return; // Wait for NTP sync
    }
    
    // Get current time from NTP
    unsigned long epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime((time_t *)&epochTime);
    
    for (uint8_t i = 0; i < scheduleCount; i++) {
        if (scheduleEntries[i].enabled && 
            scheduleEntries[i].hour == ptm->tm_hour && 
            scheduleEntries[i].minute == ptm->tm_min) {
            
            Serial.printf("Schedule triggered: %02d:%02d - %s\n", 
                        scheduleEntries[i].hour, 
                        scheduleEntries[i].minute, 
                        scheduleEntries[i].description.c_str());
            
            triggerGong();
        }
    }
}

bool addScheduleEntry(uint8_t hour, uint8_t minute, const String& description) {
    if (scheduleCount >= MAX_SCHEDULE_ENTRIES) {
        return false;
    }
    
    if (hour > 23 || minute > 59) {
        return false;
    }
    
    ScheduleEntry& entry = scheduleEntries[scheduleCount];
    entry.hour = hour;
    entry.minute = minute;
    entry.description = description;
    entry.enabled = true;
    entry.id = nextScheduleId++;
    
    scheduleCount++;
    saveScheduleToSPIFFS();
    
    Serial.printf("Added schedule: %02d:%02d - %s (ID: %u)\n", 
                 hour, minute, description.c_str(), entry.id);
    
    return true;
}

bool deleteScheduleEntry(uint32_t id) {
    for (uint8_t i = 0; i < scheduleCount; i++) {
        if (scheduleEntries[i].id == id) {
            // Shift remaining entries
            for (uint8_t j = i; j < scheduleCount - 1; j++) {
                scheduleEntries[j] = scheduleEntries[j + 1];
            }
            scheduleCount--;
            saveScheduleToSPIFFS();
            
            Serial.printf("Deleted schedule ID: %u\n", id);
            return true;
        }
    }
    return false;
}

bool editScheduleEntry(uint32_t id, uint8_t hour, uint8_t minute, const String& description, bool enabled) {
    if (hour > 23 || minute > 59) {
        return false;
    }
    
    for (uint8_t i = 0; i < scheduleCount; i++) {
        if (scheduleEntries[i].id == id) {
            scheduleEntries[i].hour = hour;
            scheduleEntries[i].minute = minute;
            scheduleEntries[i].description = description;
            scheduleEntries[i].enabled = enabled;
            saveScheduleToSPIFFS();
            
            Serial.printf("Edited schedule ID: %u to %02d:%02d - %s (enabled: %s)\n", 
                        id, hour, minute, description.c_str(), enabled ? "true" : "false");
            return true;
        }
    }
    return false;
}

String getScheduleJSON() {
    DynamicJsonDocument doc(2048);
    JsonArray array = doc.to<JsonArray>();
    
    for (uint8_t i = 0; i < scheduleCount; i++) {
        JsonObject entry = array.createNestedObject();
        entry["id"] = scheduleEntries[i].id;
        entry["hour"] = scheduleEntries[i].hour;
        entry["minute"] = scheduleEntries[i].minute;
        entry["enabled"] = scheduleEntries[i].enabled;
        entry["description"] = scheduleEntries[i].description;
    }
    
    String result;
    serializeJson(doc, result);
    return result;
}

void loadScheduleFromSPIFFS() {
    if (!SPIFFS.exists(SCHEDULE_FILE)) {
        Serial.println("No schedule file found, starting with empty schedule");
        return;
    }
    
    File file = SPIFFS.open(SCHEDULE_FILE, "r");
    if (!file) {
        Serial.println("Failed to open schedule file for reading");
        return;
    }
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.println("Failed to parse schedule file");
        return;
    }
    
    scheduleCount = 0;
    JsonArray array = doc.as<JsonArray>();
    
    for (JsonObject entry : array) {
        if (scheduleCount >= MAX_SCHEDULE_ENTRIES) break;
        
        ScheduleEntry& sched = scheduleEntries[scheduleCount];
        sched.id = entry["id"] | 0;
        sched.hour = entry["hour"] | 0;
        sched.minute = entry["minute"] | 0;
        sched.enabled = entry["enabled"] | true;
        sched.description = entry["description"] | "";
        
        if (sched.id >= nextScheduleId) {
            nextScheduleId = sched.id + 1;
        }
        
        scheduleCount++;
    }
    
    Serial.printf("Loaded %d schedule entries\n", scheduleCount);
}

void loadDefaultSchedules() {
    if (!SPIFFS.exists(GONG_CONFIG_FILE)) {
        Serial.println("No gong.conf file found for default schedules");
        return;
    }
    
    File file = SPIFFS.open(GONG_CONFIG_FILE, "r");
    if (!file) {
        Serial.println("Failed to open gong.conf file for reading");
        return;
    }
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.println("Failed to parse gong.conf file");
        return;
    }
    
    if (doc.containsKey("default_schedules")) {
        JsonArray array = doc["default_schedules"];
        
        for (JsonObject entry : array) {
            if (scheduleCount >= MAX_SCHEDULE_ENTRIES) break;
            
            ScheduleEntry& sched = scheduleEntries[scheduleCount];
            sched.id = nextScheduleId++;
            sched.hour = entry["hour"] | 0;
            sched.minute = entry["minute"] | 0;
            sched.enabled = entry["enabled"] | true;
            sched.description = entry["description"] | "";
            
            scheduleCount++;
        }
        
        Serial.printf("Loaded %d default schedule entries from gong.conf\n", scheduleCount);
        
        // Save the default schedules to the schedule file
        saveScheduleToSPIFFS();
    }
}

void saveScheduleToSPIFFS() {
    File file = SPIFFS.open(SCHEDULE_FILE, "w");
    if (!file) {
        Serial.println("Failed to open schedule file for writing");
        return;
    }
    
    DynamicJsonDocument doc(2048);
    JsonArray array = doc.to<JsonArray>();
    
    for (uint8_t i = 0; i < scheduleCount; i++) {
        JsonObject entry = array.createNestedObject();
        entry["id"] = scheduleEntries[i].id;
        entry["hour"] = scheduleEntries[i].hour;
        entry["minute"] = scheduleEntries[i].minute;
        entry["enabled"] = scheduleEntries[i].enabled;
        entry["description"] = scheduleEntries[i].description;
    }
    
    serializeJson(doc, file);
    file.close();
    
    Serial.println("Schedule saved to SPIFFS");
}

void triggerGong() {
    if (onGongTrigger) {
        onGongTrigger();
    }
}
