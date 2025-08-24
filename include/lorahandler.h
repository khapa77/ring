#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// LoRa pin definitions for ESP32
#define LORA_SS_PIN 5    // ESP32 GPIO5 -> LoRa CS
#define LORA_RST_PIN 14  // ESP32 GPIO14 -> LoRa RST
#define LORA_DIO0_PIN 2  // ESP32 GPIO2 -> LoRa DIO0

// LoRa configuration
#define LORA_FREQUENCY 433E6  // 433 MHz
#define LORA_SYNC_WORD 0x12
#define LORA_SPREADING_FACTOR 7
#define LORA_BANDWIDTH 125E3
#define LORA_CODING_RATE 5

// Message types
#define MSG_TYPE_GONG 0x01
#define MSG_TYPE_SCHEDULE 0x02
#define MSG_TYPE_STATUS 0x03

// Function declarations
void setupLoRa();
void loopLoRa();
void sendGongLoRa();
void sendLoRaMessage(const String& message, uint8_t type = MSG_TYPE_GONG);
bool isLoRaMessageAvailable();
String receiveLoRaMessage();
void onLoRaMessageReceived(const String& message);

// External callback for gong trigger
extern void (*onGongTrigger)();
