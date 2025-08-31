#pragma once

#include <Arduino.h>

// System information
#define SYSTEM_NAME "ESP32 Gong Scheduler"
#define SYSTEM_VERSION "1.0.0"
#define SYSTEM_BUILD_DATE __DATE__ " " __TIME__

// Debug configuration
#define DEBUG_SERIAL_ENABLED true
#define DEBUG_SERIAL_BAUD 115200

// Timing intervals (in milliseconds)
#define SCHEDULE_CHECK_INTERVAL 1000    // Check schedule every second
#define STATUS_PRINT_INTERVAL 30000     // Print status every 30 seconds
#define WATCHDOG_RESET_INTERVAL 1000    // Reset watchdog every second

// WiFi configuration
#define WIFI_TIMEOUT 60000              // 1 minute timeout for WiFi connection
#define WEB_SERVER_PORT 80

// LoRa configuration
#define LORA_FREQUENCY 433E6            // 433 MHz
#define LORA_SYNC_WORD 0x12
#define LORA_SPREADING_FACTOR 7
#define LORA_BANDWIDTH 125E3
#define LORA_CODING_RATE 5

// LoRa pin definitions for ESP32
#define LORA_SS_PIN 5                   // ESP32 GPIO5 -> LoRa CS
#define LORA_RST_PIN 14                 // ESP32 GPIO14 -> LoRa RST
#define LORA_DIO0_PIN 2                 // ESP32 GPIO2 -> LoRa DIO0

// MP3/Audio configuration
#define SD_CS_PIN 16                    // SD card CS pin
#define AUDIO_VOLUME 21                  // Audio volume (0-30)

// Message types
#define MSG_TYPE_GONG 0x01
#define MSG_TYPE_SCHEDULE 0x02
#define MSG_TYPE_STATUS 0x03

// File paths
#define SCHEDULE_FILE "/schedule.json"
#define WIFI_CONFIG_FILE "/wifi_config.json"
