#pragma once

#include <Arduino.h>

// System configuration
#define SYSTEM_NAME "ESP32_Gong_System"
#define SYSTEM_VERSION "2.0.0"
#define SYSTEM_BUILD_DATE __DATE__ " " __TIME__

// WiFi configuration
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"
#define WIFI_TIMEOUT 10000  // 10 seconds
#define WIFI_RETRY_ATTEMPTS 3

// LoRa configuration
#define LORA_FREQUENCY 433E6  // 433 MHz
#define LORA_SYNC_WORD 0x12
#define LORA_SPREADING_FACTOR 7
#define LORA_BANDWIDTH 125E3
#define LORA_CODING_RATE 5
#define LORA_TX_POWER 20
#define LORA_CRC_ENABLED true

// LoRa pins for ESP32
#define LORA_SS_PIN 5    // ESP32 GPIO5 -> LoRa CS
#define LORA_RST_PIN 14  // ESP32 GPIO14 -> LoRa RST
#define LORA_DIO0_PIN 2  // ESP32 GPIO2 -> LoRa DIO0

// LoRa message configuration
#define LORA_MAX_MESSAGE_SIZE 256
#define LORA_MAX_JSON_SIZE 512
#define LORA_MAX_RETRIES 3
#define LORA_ACK_TIMEOUT 2000  // 2 seconds
#define LORA_RETRY_DELAY 1000  // 1 second
#define LORA_STATE_TIMEOUT 10000 // 10 seconds max in any state

// Memory management
#define MIN_FREE_HEAP_LORA 10240  // 10KB minimum for LoRa
#define LOW_MEMORY_THRESHOLD 5120 // 5KB warning threshold
#define CRITICAL_MEMORY_THRESHOLD 2048 // 2KB critical threshold
#define MEMORY_CHECK_INTERVAL 30000 // Check memory every 30 seconds

// Timing configuration
#define SCHEDULE_CHECK_INTERVAL 1000  // Check schedule every second
#define STATUS_PRINT_INTERVAL 10000   // Print status every 10 seconds
#define WATCHDOG_RESET_INTERVAL 5000  // Reset watchdog every 5 seconds

// Logging configuration
#define DEFAULT_LOG_LEVEL 1  // INFO level by default
#define MAX_LOG_MESSAGE_SIZE 256

// Web server configuration
#define WEB_SERVER_PORT 80
#define WEB_SERVER_TIMEOUT 5000
#define MAX_HTTP_REQUEST_SIZE 2048

// MP3 player configuration
#define MP3_VOLUME_DEFAULT 20
#define MP3_VOLUME_MAX 30
#define MP3_VOLUME_MIN 0

// Schedule configuration
#define MAX_SCHEDULE_ENTRIES 100
#define SCHEDULE_FILE_PATH "/schedule.json"

// Power management (future use)
#define DEEP_SLEEP_ENABLED false
#define DEEP_SLEEP_INTERVAL 300000  // 5 minutes
#define LIGHT_SLEEP_ENABLED false
#define LIGHT_SLEEP_TIMEOUT 10000   // 10 seconds

// Security configuration (future use)
#define WEB_AUTH_ENABLED false
#define WEB_USERNAME "admin"
#define WEB_PASSWORD "password"
#define RATE_LIMIT_ENABLED false
#define MAX_REQUESTS_PER_MINUTE 60

// Debug configuration
#define DEBUG_SERIAL_ENABLED true
#define DEBUG_SERIAL_BAUD 115200
#define DEBUG_LOGGING_ENABLED true
#define DEBUG_MEMORY_MONITORING true
