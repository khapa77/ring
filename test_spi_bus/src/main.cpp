#include <Arduino.h>

// Объявление функции теста
void testLoRaOnly();
void testSDCardSimple();

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("Starting Diagnostic Tests...");
    testLoRaOnly();
    testSDCardSimple();
    
    Serial.println("\n=== Test Complete ===");
}

void loop() {
    delay(1000);
}
