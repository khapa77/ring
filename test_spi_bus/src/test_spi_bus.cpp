#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "config.h"

// SPI объект для LoRa (VSPI)
SPIClass vspi(VSPI);

void testLoRaOnly() {
    Serial.println("\n=== LoRa Diagnostic Test ===");
    Serial.println("Testing LoRa module on VSPI...");
    
    // Инициализируем VSPI для LoRa (стандартные пины VSPI: SCK=18, MISO=19, MOSI=23)
    vspi.begin();
    Serial.println("VSPI for LoRa initialized");
    
    // Configure CS pin
    pinMode(LORA_SS_PIN, OUTPUT);
    digitalWrite(LORA_SS_PIN, HIGH);
    
    // Configure RST and DIO0 pins
    pinMode(LORA_RST_PIN, OUTPUT);
    pinMode(LORA_DIO0_PIN, INPUT);
    
    // Test LoRa on VSPI
    LoRa.setSPI(vspi);
    LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
    
    if (LoRa.begin(433E6)) {
        Serial.println("✓ LoRa module initialized successfully");
        Serial.printf("  Frequency: %.1f MHz\n", 433E6 / 1e6);
        
        // Test transmission
        LoRa.beginPacket();
        LoRa.print("LoRa test message");
        int result = LoRa.endPacket();
        Serial.printf("  Packet transmission: %s\n", result ? "SUCCESS" : "FAILED");
        Serial.printf("  Packet RSSI: %d\n", LoRa.packetRssi());
        
        // Test reception
        Serial.println("  Listening for packets (3 seconds)...");
        unsigned long startTime = millis();
        while (millis() - startTime < 3000) {
            int packetSize = LoRa.parsePacket();
            if (packetSize) {
                Serial.print("  Received: '");
                while (LoRa.available()) {
                    Serial.print((char)LoRa.read());
                }
                Serial.println("'");
                break;
            }
            delay(10);
        }
        
    } else {
        Serial.println("✗ LoRa module initialization failed!");
        Serial.println("  Check wiring:");
        Serial.println("  LoRa NSS  -> GPIO 16");
        Serial.println("  LoRa SCK  -> GPIO 18");
        Serial.println("  LoRa MISO -> GPIO 19");
        Serial.println("  LoRa MOSI -> GPIO 23");
        Serial.println("  LoRa RST  -> GPIO 17");
        Serial.println("  LoRa DIO0 -> GPIO 34");
        Serial.println("  LoRa VCC  -> 3.3V");
        Serial.println("  LoRa GND  -> GND");
    }
    
    digitalWrite(LORA_SS_PIN, HIGH);
}

void testSDCardSimple() {
    Serial.println("\n=== SD Card Basic Test ===");
    Serial.println("Note: SD card test requires separate sketch due to library conflicts");
    Serial.println("Pins for SD card (HSPI):");
    Serial.println("  SD CS   -> GPIO 21");
    Serial.println("  SD SCK  -> GPIO 14");
    Serial.println("  SD MISO -> GPIO 32");
    Serial.println("  SD MOSI -> GPIO 13");
    Serial.println("  SD VCC  -> 5V");
    Serial.println("  SD GND  -> GND");
}
