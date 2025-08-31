#include <SPI.h>
#include <LoRa.h>

// Пины для LoRa модуля
#define LORA_SCK     14
#define LORA_MISO    12
#define LORA_MOSI    13
#define LORA_SS      15
#define LORA_RST     2
#define LORA_DIO0    4

// Частота для России (433 МГц)
#define BAND 433E6

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println("LoRa Transmitter");

  // Настройка SPI и LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  
  if (!LoRa.begin(BAND)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  
  // Установка параметров передачи
  LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  
  Serial.println("LoRa transmitter ready!");
}

void loop() {
  static bool ledState = false;
  ledState = !ledState; // Меняем состояние светодиода
  
  Serial.println("Sending packet...");
  
  // Отправка пакета с командой для светодиода
  LoRa.beginPacket();
  LoRa.print("LED:");
  LoRa.print(ledState ? "ON" : "OFF");
  LoRa.print("|TIME:");
  LoRa.print(millis());
  LoRa.print("|RSSI:");
  LoRa.print(LoRa.rssi());
  LoRa.endPacket();
  
  Serial.print("Sent command: LED ");
  Serial.println(ledState ? "ON" : "OFF");
  
  delay(3000);  // Отправка каждые 3 секунды
}
