#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <SPIFFS.h>

// Web server configuration
#define WEB_SERVER_PORT 80
#define WIFI_TIMEOUT 60000  // 1 minute
#define WIFI_CONFIG_FILE "/wifi.conf"

// WiFi credentials structure
struct WiFiConfig {
    char ssid[32];
    char password[64];
    bool configured;
};

// Function declarations
void setupWiFi();
void setupWebServer();
void loopWebServer();
void handleRoot();
void handleSchedule();
void handleAddSchedule();
void handleDeleteSchedule();
void handleEditSchedule();
void handlePlay();
void handlePlayLoRa();
void handleWiFiConfig();
void handleWiFiSave();
void handleNotFound();
void setupAPMode();
bool isWiFiConnected();
String getWiFiStatus();

// WiFi configuration functions
bool loadWiFiConfig();
bool saveWiFiConfig(const String& ssid, const String& password);
void resetWiFiConfig();

// External functions
extern void playGong();
extern void sendGongLoRa();
extern String getScheduleJSON();
extern bool addScheduleEntry(uint8_t hour, uint8_t minute, const String& description);
extern bool deleteScheduleEntry(uint32_t id);
extern bool editScheduleEntry(uint32_t id, uint8_t hour, uint8_t minute, const String& description);
