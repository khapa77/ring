#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// Sound types for gong
enum SoundType {
    SOUND_SHORT = 1,      // 0001.wav - Short
    SOUND_LONG = 2,       // 0002.wav - Long  
    SOUND_ONE = 3,        // 0003.wav - One
    SOUND_ORIGINAL = 4    // 0004.wav - Original
};

// Schedule entry structure
struct ScheduleEntry {
    uint8_t hour;
    uint8_t minute;
    bool enabled;
    String description;
    uint32_t id;
    SoundType soundType;  // 🆕 Тип звука для будильника
};

// Schedule management functions
void setupSchedule();
void checkSchedule();
bool addScheduleEntry(uint8_t hour, uint8_t minute, const String& description, SoundType soundType);
bool deleteScheduleEntry(uint32_t id);
bool editScheduleEntry(uint32_t id, uint8_t hour, uint8_t minute, const String& description, bool enabled, SoundType soundType);
String getScheduleJSON();
void loadScheduleFromSPIFFS();
bool saveScheduleToSPIFFS();
void loadDefaultSchedules();
void triggerGong();
void sortScheduleByTime();
void autoSortSchedule();

// Utility functions
String getSoundTypeName(SoundType soundType);  // 🆕 Получение названия звука

// External callback for gong trigger
extern void (*onGongTrigger)();
