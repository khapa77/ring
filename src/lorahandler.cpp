#include "lorahandler.h"
#include <SPI.h>
#include <LoRa.h>

// External callback for gong trigger
void (*onGongTrigger)() = nullptr;

// Forward declarations for message handlers
void handleGongMessage(const String& content);
void handleScheduleMessage(const String& content);
void handleStatusMessage(const String& content);

void setupLoRa() {
    // Initialize SPI for LoRa
    SPI.begin(18, 19, 23, LORA_SS_PIN); // SCK, MISO, MOSI, SS
    
    // Configure LoRa pins
    LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
    
    // Initialize LoRa
    if (!LoRa.begin(LORA_FREQUENCY)) {
        Serial.println("LoRa initialization failed!");
        return;
    }
    
    // Configure LoRa parameters
    LoRa.setSyncWord(LORA_SYNC_WORD);
    LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
    LoRa.setSignalBandwidth(LORA_BANDWIDTH);
    LoRa.setCodingRate4(LORA_CODING_RATE);
    LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
    
    Serial.println("LoRa module initialized");
}

void loopLoRa() {
    // Check for incoming messages
    if (isLoRaMessageAvailable()) {
        String message = receiveLoRaMessage();
        if (message.length() > 0) {
            onLoRaMessageReceived(message);
        }
    }
}

void sendGongLoRa() {
    // Create JSON message for gong trigger
    DynamicJsonDocument doc(256);
    doc["type"] = "gong";
    doc["timestamp"] = millis();
    doc["device"] = "ESP32_Gong";
    
    String message;
    serializeJson(doc, message);
    
    sendLoRaMessage(message, MSG_TYPE_GONG);
}

void sendLoRaMessage(const String& message, uint8_t type) {
    // Add message type header
    String fullMessage = String(type, HEX) + ":" + message;
    
    // Send the message
    LoRa.beginPacket();
    LoRa.print(fullMessage);
    LoRa.endPacket();
    
    Serial.printf("LoRa message sent (Type: 0x%02X): %s\n", type, message.c_str());
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
        Serial.printf("LoRa message received: %s\n", message.c_str());
    }
    
    return message;
}

void onLoRaMessageReceived(const String& message) {
    // Parse message type and content
    int colonIndex = message.indexOf(':');
    if (colonIndex == -1) {
        Serial.println("Invalid LoRa message format");
        return;
    }
    
    String typeStr = message.substring(0, colonIndex);
    String content = message.substring(colonIndex + 1);
    
    uint8_t type = strtol(typeStr.c_str(), NULL, 16);
    
    Serial.printf("Processing LoRa message type: 0x%02X, content: %s\n", type, content.c_str());
    
    switch (type) {
        case MSG_TYPE_GONG:
            handleGongMessage(content);
            break;
        case MSG_TYPE_SCHEDULE:
            handleScheduleMessage(content);
            break;
        case MSG_TYPE_STATUS:
            handleStatusMessage(content);
            break;
        default:
            Serial.printf("Unknown message type: 0x%02X\n", type);
            break;
    }
}

void handleGongMessage(const String& content) {
    // Parse JSON content
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, content);
    
    if (error) {
        Serial.println("Failed to parse gong message JSON");
        return;
    }
    
    // Check if this is a gong trigger
    if (doc.containsKey("type") && doc["type"] == "gong") {
        Serial.println("Gong message received via LoRa - triggering local playback");
        
        // Trigger local gong playback
        if (onGongTrigger) {
            onGongTrigger();
        }
    }
}

void handleScheduleMessage(const String& content) {
    // Handle schedule synchronization messages
    Serial.println("Schedule message received via LoRa");
    // TODO: Implement schedule sync logic
}

void handleStatusMessage(const String& content) {
    // Handle status/health check messages
    Serial.println("Status message received via LoRa");
    // TODO: Implement status handling logic
}
