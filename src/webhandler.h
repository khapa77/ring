#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <SPIFFS.h>

// Web server configuration
#define WEB_SERVER_PORT 80
#define WIFI_TIMEOUT 60000  // 1 minute

// WiFi credentials
extern const char* sta_ssid;
extern const char* sta_password;
extern const char* ap_ssid;
extern const char* ap_password;

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
void handleNotFound();
void setupAPMode();
bool isWiFiConnected();
String getWiFiStatus();

// External functions
extern void playGong();
extern void sendGongLoRa();
extern String getScheduleJSON();
extern bool addScheduleEntry(uint8_t hour, uint8_t minute, const String& description);
extern bool deleteScheduleEntry(uint32_t id);
extern bool editScheduleEntry(uint32_t id, uint8_t hour, uint8_t minute, const String& description);
