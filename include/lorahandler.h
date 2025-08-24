#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

// Logging levels
enum LoRaLogLevel {
    LORA_LOG_DEBUG = 0,
    LORA_LOG_INFO = 1,
    LORA_LOG_WARN = 2,
    LORA_LOG_ERROR = 3
};

// Message types
#define MSG_TYPE_GONG 0x01
#define MSG_TYPE_SCHEDULE 0x02
#define MSG_TYPE_STATUS 0x03

// LoRa state machine states
enum LoRaState {
    LORA_IDLE,
    LORA_SENDING,
    LORA_WAITING_ACK,
    LORA_RECEIVING,
    LORA_ERROR
};

// LoRa message structure with retry support
struct LoRaMessage {
    char content[LORA_MAX_MESSAGE_SIZE];
    uint8_t type;
    uint8_t retryCount;
    uint32_t timestamp;
    uint32_t messageId;
    bool requiresAck;
    uint16_t contentLength;
};

// LoRa context for state management
struct LoRaContext {
    LoRaState state;
    uint32_t stateStartTime;
    uint8_t maxRetries;
    uint32_t ackTimeout;
    uint32_t retryDelay;
    uint32_t lastMessageId;
    LoRaMessage pendingMessage;
    bool isInitialized;
    uint32_t errorCount;
    uint32_t successCount;
    uint32_t memoryErrors;
    uint32_t lastMemoryCheck;
    LoRaLogLevel logLevel;
    uint32_t logCount;
};

// Function declarations
void setupLoRa();
void loopLoRa();
void sendGongLoRa();
void sendLoRaMessage(const char* message, uint8_t type = MSG_TYPE_GONG, bool requireAck = false);
bool isLoRaMessageAvailable();
String receiveLoRaMessage();
void onLoRaMessageReceived(const String& message);

// New functions for error handling and state management
bool isLoRaInitialized();
LoRaState getLoRaState();
uint32_t getLoRaErrorCount();
uint32_t getLoRaSuccessCount();
uint32_t getLoRaMemoryErrors();
void resetLoRaErrors();
bool sendLoRaMessageWithRetry(const char* message, uint8_t type, bool requireAck = false);

// Memory management functions
bool checkLoRaMemory();
void cleanupLoRaMemory();

// Logging functions
void setLoRaLogLevel(LoRaLogLevel level);
void logLoRa(LoRaLogLevel level, const char* message);
void logLoRaFormatted(LoRaLogLevel level, const char* format, ...);
uint32_t getLoRaLogCount();

// External callback for gong trigger
extern void (*onGongTrigger)();

// Global LoRa context
extern LoRaContext loraContext;
