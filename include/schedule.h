#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// Schedule entry structure
struct ScheduleEntry {
    uint8_t hour;
    uint8_t minute;
    bool enabled;
    String description;
    uint32_t id;
};

// Schedule management functions
void setupSchedule();
void checkSchedule();
bool addScheduleEntry(uint8_t hour, uint8_t minute, const String& description);
bool deleteScheduleEntry(uint32_t id);
bool editScheduleEntry(uint32_t id, uint8_t hour, uint8_t minute, const String& description, bool enabled = true);
String getScheduleJSON();
void loadScheduleFromSPIFFS();
bool saveScheduleToSPIFFS();
void loadDefaultSchedules();
void triggerGong();

// External callback for gong trigger
extern void (*onGongTrigger)();
