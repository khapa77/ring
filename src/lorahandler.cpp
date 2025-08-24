#include "lorahandler.h"
#include <SPI.h>
#include <LoRa.h>
#include <stdarg.h>

// External callback for gong trigger
void (*onGongTrigger)() = nullptr;

// Global LoRa context
LoRaContext loraContext = {
    .state = LORA_IDLE,
    .stateStartTime = 0,
    .maxRetries = LORA_MAX_RETRIES,
    .ackTimeout = LORA_ACK_TIMEOUT,
    .retryDelay = LORA_RETRY_DELAY,
    .lastMessageId = 0,
    .pendingMessage = {},
    .isInitialized = false,
    .errorCount = 0,
    .successCount = 0,
    .memoryErrors = 0,
    .lastMemoryCheck = 0,
    .logLevel = LORA_LOG_INFO,
    .logCount = 0
};

// Forward declarations for message handlers
void handleGongMessage(const char* content);
void handleScheduleMessage(const char* content);
void handleStatusMessage(const char* content);
void handleAckMessage(const char* content);

// Helper functions
uint32_t generateMessageId();
bool validateLoRaPacket(const char* message, uint16_t length);
void changeLoRaState(LoRaState newState);
bool sendLoRaPacket(const char* message, uint16_t length);
void safeStringCopy(char* dest, const char* src, size_t maxLen);

// Logging helper
const char* getLogLevelString(LoRaLogLevel level) {
    switch (level) {
        case LORA_LOG_DEBUG: return "DEBUG";
        case LORA_LOG_INFO:  return "INFO";
        case LORA_LOG_WARN:  return "WARN";
        case LORA_LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void setupLoRa() {
    logLoRa(LORA_LOG_INFO, "Initializing LoRa module...");
    
    // Check available memory before initialization
    if (ESP.getFreeHeap() < 10240) { // Need at least 10KB
        logLoRaFormatted(LORA_LOG_ERROR, "Insufficient memory for LoRa initialization! Free: %d bytes", ESP.getFreeHeap());
        loraContext.memoryErrors++;
        return;
    }
    
    // Initialize SPI for LoRa
    SPI.begin(18, 19, 23, LORA_SS_PIN); // SCK, MISO, MOSI, SS
    logLoRa(LORA_LOG_DEBUG, "SPI initialized for LoRa");
    
    // Configure LoRa pins
    LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
    
    // Initialize LoRa with retry
    int retryCount = 0;
    while (retryCount < 3 && !LoRa.begin(LORA_FREQUENCY)) {
        logLoRaFormatted(LORA_LOG_WARN, "LoRa initialization attempt %d failed!", retryCount + 1);
        delay(1000);
        retryCount++;
    }
    
    if (retryCount >= 3) {
        logLoRa(LORA_LOG_ERROR, "LoRa initialization failed after 3 attempts!");
        loraContext.isInitialized = false;
        changeLoRaState(LORA_ERROR);
        return;
    }
    
    // Configure LoRa parameters
    LoRa.setSyncWord(LORA_SYNC_WORD);
    LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
    LoRa.setSignalBandwidth(LORA_BANDWIDTH);
    LoRa.setCodingRate4(LORA_CODING_RATE);
    LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
    
    // Enable CRC
    LoRa.enableCrc();
    
    loraContext.isInitialized = true;
    changeLoRaState(LORA_IDLE);
    loraContext.lastMemoryCheck = millis();
    logLoRaFormatted(LORA_LOG_INFO, "LoRa module initialized successfully at %d MHz", LORA_FREQUENCY / 1000000);
}

void loopLoRa() {
    if (!loraContext.isInitialized) {
        return;
    }
    
    // Check memory periodically
    if (millis() - loraContext.lastMemoryCheck >= MEMORY_CHECK_INTERVAL) {
        if (!checkLoRaMemory()) {
            cleanupLoRaMemory();
        }
        loraContext.lastMemoryCheck = millis();
    }
    
    // Handle state machine
    switch (loraContext.state) {
        case LORA_IDLE:
            // Check for incoming messages
            if (isLoRaMessageAvailable()) {
                changeLoRaState(LORA_RECEIVING);
            }
            break;
            
        case LORA_SENDING:
            // Send pending message
            if (sendLoRaPacket(loraContext.pendingMessage.content, loraContext.pendingMessage.contentLength)) {
                if (loraContext.pendingMessage.requiresAck) {
                                    logLoRaFormatted(LORA_LOG_DEBUG, "Message sent, waiting for ACK (ID: 0x%08X)", 
                       loraContext.pendingMessage.messageId);
                    changeLoRaState(LORA_WAITING_ACK);
                } else {
                    loraContext.successCount++;
                                    logLoRaFormatted(LORA_LOG_DEBUG, "Message sent successfully (ID: 0x%08X)", 
                       loraContext.pendingMessage.messageId);
                    changeLoRaState(LORA_IDLE);
                }
            } else {
                // Send failed, retry or give up
                if (loraContext.pendingMessage.retryCount < loraContext.maxRetries) {
                    loraContext.pendingMessage.retryCount++;
                    loraContext.pendingMessage.timestamp = millis();
                    logLoRaFormatted(LORA_LOG_WARN, "Retrying LoRa message, attempt %d/%d (ID: 0x%08X)", 
                           loraContext.pendingMessage.retryCount, loraContext.maxRetries,
                           loraContext.pendingMessage.messageId);
                    changeLoRaState(LORA_IDLE); // Will retry on next loop
                } else {
                    loraContext.errorCount++;
                    logLoRaFormatted(LORA_LOG_ERROR, "LoRa message failed after %d retries (ID: 0x08X)", 
                           loraContext.maxRetries, loraContext.pendingMessage.messageId);
                    changeLoRaState(LORA_IDLE);
                }
            }
            break;
            
        case LORA_WAITING_ACK:
            // Check for ACK or timeout
            if (isLoRaMessageAvailable()) {
                String ackMessage = receiveLoRaMessage();
                if (validateLoRaPacket(ackMessage.c_str(), ackMessage.length())) {
                    handleAckMessage(ackMessage.c_str());
                    loraContext.successCount++;
                    logLoRaFormatted(LORA_LOG_DEBUG, "ACK received for message ID: 0x%08X", 
                           loraContext.pendingMessage.messageId);
                    changeLoRaState(LORA_IDLE);
                }
            } else if (millis() - loraContext.stateStartTime > loraContext.ackTimeout) {
                // ACK timeout, retry or give up
                if (loraContext.pendingMessage.retryCount < loraContext.maxRetries) {
                    loraContext.pendingMessage.retryCount++;
                    logLoRaFormatted(LORA_LOG_WARN, "ACK timeout, retrying message, attempt %d/%d (ID: 0x%08X)", 
                           loraContext.pendingMessage.retryCount, loraContext.maxRetries,
                           loraContext.pendingMessage.messageId);
                    changeLoRaState(LORA_SENDING);
                } else {
                    loraContext.errorCount++;
                    logLoRaFormatted(LORA_LOG_ERROR, "ACK timeout after %d retries (ID: 0x%08X)", 
                           loraContext.maxRetries, loraContext.pendingMessage.messageId);
                    changeLoRaState(LORA_IDLE);
                }
            }
            break;
            
        case LORA_RECEIVING: {
            // Process received message
            String message = receiveLoRaMessage();
            if (validateLoRaPacket(message.c_str(), message.length())) {
                logLoRa(LORA_LOG_DEBUG, "Valid packet received, processing...");
                onLoRaMessageReceived(message);
            } else {
                logLoRaFormatted(LORA_LOG_WARN, "Invalid LoRa packet received: %s", message.c_str());
                loraContext.errorCount++;
            }
            changeLoRaState(LORA_IDLE);
            break;
        }
            
        case LORA_ERROR:
            // Try to recover from error
            if (millis() - loraContext.stateStartTime > 5000) { // Wait 5 seconds
                logLoRaFormatted(LORA_LOG_INFO, "Attempting LoRa recovery...");
                if (LoRa.begin(LORA_FREQUENCY)) {
                    loraContext.isInitialized = true;
                    changeLoRaState(LORA_IDLE);
                    logLoRaFormatted(LORA_LOG_INFO, "LoRa recovery successful");
                } else {
                    logLoRaFormatted(LORA_LOG_ERROR, "LoRa recovery failed");
                }
            }
            break;
    }
    
    // Check for state timeout
    if (loraContext.state != LORA_IDLE && 
        millis() - loraContext.stateStartTime > LORA_STATE_TIMEOUT) {
        logLoRaFormatted(LORA_LOG_ERROR, "LoRa state timeout in state %d", loraContext.state);
        loraContext.errorCount++;
        changeLoRaState(LORA_ERROR);
    }
}

void sendGongLoRa() {
    logLoRaFormatted(LORA_LOG_INFO, "Sending gong trigger via LoRa");
    
    // Create JSON message for gong trigger
    DynamicJsonDocument doc(LORA_MAX_JSON_SIZE);
    doc["type"] = "gong";
    doc["timestamp"] = millis();
    doc["device"] = "ESP32_Gong";
    
    char messageBuffer[LORA_MAX_MESSAGE_SIZE];
    serializeJson(doc, messageBuffer, sizeof(messageBuffer));
    
    sendLoRaMessageWithRetry(messageBuffer, MSG_TYPE_GONG, true);
}

void sendLoRaMessage(const char* message, uint8_t type, bool requireAck) {
    if (!loraContext.isInitialized) {
        logLoRaFormatted(LORA_LOG_ERROR, "LoRa not initialized, cannot send message");
        return;
    }
    
    // Check message length
    uint16_t messageLen = strlen(message);
    if (messageLen >= LORA_MAX_MESSAGE_SIZE) {
        logLoRaFormatted(LORA_LOG_ERROR, "Message too long for LoRa buffer: %d bytes (max: %d)", 
               messageLen, LORA_MAX_MESSAGE_SIZE - 1);
        loraContext.memoryErrors++;
        return;
    }
    
    // Prepare message for sending
    safeStringCopy(loraContext.pendingMessage.content, message, LORA_MAX_MESSAGE_SIZE);
    loraContext.pendingMessage.type = type;
    loraContext.pendingMessage.retryCount = 0;
    loraContext.pendingMessage.timestamp = millis();
    loraContext.pendingMessage.messageId = generateMessageId();
    loraContext.pendingMessage.requiresAck = requireAck;
    loraContext.pendingMessage.contentLength = messageLen;
    
    // Create full message with header
    char fullMessage[LORA_MAX_MESSAGE_SIZE];
    snprintf(fullMessage, sizeof(fullMessage), "%02X:%08X:%s", 
             type, loraContext.pendingMessage.messageId, message);
    
    safeStringCopy(loraContext.pendingMessage.content, fullMessage, LORA_MAX_MESSAGE_SIZE);
    loraContext.pendingMessage.contentLength = strlen(fullMessage);
    
    logLoRaFormatted(LORA_LOG_DEBUG, "Prepared LoRa message (Type: 0x%02X, ID: 0x%08X, ACK: %s)", 
           type, loraContext.pendingMessage.messageId, requireAck ? "Yes" : "No");
    
    changeLoRaState(LORA_SENDING);
}

bool sendLoRaMessageWithRetry(const char* message, uint8_t type, bool requireAck) {
    if (loraContext.state != LORA_IDLE) {
        logLoRaFormatted(LORA_LOG_WARN, "LoRa busy in state %d, cannot send message", loraContext.state);
        return false;
    }
    
    sendLoRaMessage(message, type, requireAck);
    return true;
}

bool isLoRaMessageAvailable() {
    return LoRa.parsePacket() > 0;
}

String receiveLoRaMessage() {
    String message = "";
    
    // Read the message
    while (LoRa.available()) {
        message += (char)LoRa.read();
    }
    
    if (message.length() > 0) {
        logLoRaFormatted(LORA_LOG_DEBUG, "LoRa message received: %s", message.c_str());
    }
    
    return message;
}

void onLoRaMessageReceived(const String& message) {
    // Parse message type, ID and content
    int firstColon = message.indexOf(':');
    int secondColon = message.indexOf(':', firstColon + 1);
    
    if (firstColon == -1 || secondColon == -1) {
        logLoRaFormatted(LORA_LOG_ERROR, "Invalid LoRa message format: %s", message.c_str());
        return;
    }
    
    String typeStr = message.substring(0, firstColon);
    String idStr = message.substring(firstColon + 1, secondColon);
    String content = message.substring(secondColon + 1);
    
    uint8_t type = strtol(typeStr.c_str(), NULL, 16);
    uint32_t messageId = strtol(idStr.c_str(), NULL, 16);
    
    logLoRaFormatted(LORA_LOG_DEBUG, "Processing LoRa message type: 0x%02X, ID: 0x%08X, content: %s", 
           type, messageId, content.c_str());
    
    switch (type) {
        case MSG_TYPE_GONG:
            handleGongMessage(content.c_str());
            break;
        case MSG_TYPE_SCHEDULE:
            handleScheduleMessage(content.c_str());
            break;
        case MSG_TYPE_STATUS:
            handleStatusMessage(content.c_str());
            break;
        default:
            logLoRaFormatted(LORA_LOG_WARN, "Unknown message type: 0x%02X", type);
            break;
    }
}

void handleGongMessage(const char* content) {
    logLoRaFormatted(LORA_LOG_INFO, "Processing gong message: %s", content);
    
    // Parse JSON content
    DynamicJsonDocument doc(LORA_MAX_JSON_SIZE);
    DeserializationError error = deserializeJson(doc, content);
    
    if (error) {
        logLoRaFormatted(LORA_LOG_ERROR, "Failed to parse gong message JSON: %s", error.c_str());
        return;
    }
    
    // Check if this is a gong trigger
    if (doc.containsKey("type") && doc["type"] == "gong") {
        logLoRa(LORA_LOG_INFO, "Gong message received via LoRa - triggering local playback");
        
        // Trigger local gong playback
        if (onGongTrigger) {
            onGongTrigger();
        }
    }
}

void handleScheduleMessage(const char* content) {
    logLoRaFormatted(LORA_LOG_INFO, "Schedule message received via LoRa: %s", content);
    // TODO: Implement schedule sync logic
}

void handleStatusMessage(const char* content) {
    logLoRaFormatted(LORA_LOG_INFO, "Status message received via LoRa: %s", content);
    // TODO: Implement status handling logic
}

void handleAckMessage(const char* content) {
    logLoRaFormatted(LORA_LOG_DEBUG, "ACK message received via LoRa: %s", content);
    // TODO: Implement ACK handling logic
}

// Helper functions implementation
uint32_t generateMessageId() {
    return ++loraContext.lastMessageId;
}

bool validateLoRaPacket(const char* message, uint16_t length) {
    // Basic validation - check minimum length and format
    if (length < 5) { // At least "0:0:"
        return false;
    }
    
    // Check if it has at least two colons
    const char* firstColon = strchr(message, ':');
    if (!firstColon) return false;
    
    const char* secondColon = strchr(firstColon + 1, ':');
    if (!secondColon) return false;
    
    // Validate message type
    char typeStr[3] = {0};
    strncpy(typeStr, message, firstColon - message);
    uint8_t type = strtol(typeStr, NULL, 16);
    if (type < MSG_TYPE_GONG || type > MSG_TYPE_STATUS) {
        return false;
    }
    
    return true;
}

void changeLoRaState(LoRaState newState) {
    LoRaState oldState = loraContext.state;
    loraContext.state = newState;
    loraContext.stateStartTime = millis();
    logLoRaFormatted(LORA_LOG_DEBUG, "LoRa state changed: %d -> %d", oldState, newState);
}

bool sendLoRaPacket(const char* message, uint16_t length) {
    LoRa.beginPacket();
    LoRa.write((uint8_t*)message, length);
    bool result = LoRa.endPacket();
    
    if (result) {
        logLoRaFormatted(LORA_LOG_DEBUG, "LoRa packet sent successfully: %s", message);
    } else {
        logLoRa(LORA_LOG_ERROR, "LoRa packet send failed");
    }
    
    return result;
}

void safeStringCopy(char* dest, const char* src, size_t maxLen) {
    if (dest && src) {
        strncpy(dest, src, maxLen - 1);
        dest[maxLen - 1] = '\0';
    }
}

// Memory management functions
bool checkLoRaMemory() {
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < 5120) { // Less than 5KB free
        logLoRaFormatted(LORA_LOG_WARN, "Low memory warning: %d bytes free", freeHeap);
        loraContext.memoryErrors++;
        return false;
    }
    return true;
}

void cleanupLoRaMemory() {
    // Force garbage collection if possible
    yield();
    
    // Clear any pending messages if memory is critically low
    if (ESP.getFreeHeap() < 2048) {
        if (loraContext.state == LORA_IDLE) {
            memset(&loraContext.pendingMessage, 0, sizeof(LoRaMessage));
            logLoRa(LORA_LOG_WARN, "Cleared pending LoRa message due to low memory");
        }
    }
}

// Logging functions implementation
void setLoRaLogLevel(LoRaLogLevel level) {
    loraContext.logLevel = level;
    logLoRaFormatted(LORA_LOG_INFO, "LoRa log level set to: %s", getLogLevelString(level));
}

void logLoRa(LoRaLogLevel level, const char* message) {
    if (level >= loraContext.logLevel) {
        Serial.printf("[LoRa][%s] %s\n", getLogLevelString(level), message);
        loraContext.logCount++;
    }
}

void logLoRaFormatted(LoRaLogLevel level, const char* format, ...) {
    if (level >= loraContext.logLevel) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        Serial.printf("[LoRa][%s] %s\n", getLogLevelString(level), buffer);
        loraContext.logCount++;
    }
}

uint32_t getLoRaLogCount() {
    return loraContext.logCount;
}

// Public interface functions
bool isLoRaInitialized() {
    return loraContext.isInitialized;
}

LoRaState getLoRaState() {
    return loraContext.state;
}

uint32_t getLoRaErrorCount() {
    return loraContext.errorCount;
}

uint32_t getLoRaSuccessCount() {
    return loraContext.successCount;
}

uint32_t getLoRaMemoryErrors() {
    return loraContext.memoryErrors;
}

void resetLoRaErrors() {
    loraContext.errorCount = 0;
    loraContext.successCount = 0;
    loraContext.memoryErrors = 0;
    loraContext.logCount = 0;
}
