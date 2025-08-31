#include <SPI.h>
#include <LoRa.h>

// Пины для LoRa модуля
#define LORA_SCK     14
#define LORA_MISO    12
#define LORA_MOSI    13
#define LORA_SS      15
#define LORA_RST     2
#define LORA_DIO0    4

// Пин для светодиода (рекомендую GPIO 23 - свободный и удобный)
#define LED_PIN      23

// Частота для России (433 МГц)
#define BAND 433E6

void controlLED(const String& command) {
  if (command == "ON") {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED turned ON");
  } else if (command == "OFF") {
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED turned OFF");
  }
}

void parsePacket(String packet) {
  // Ищем команду LED в пакете
  int ledIndex = packet.indexOf("LED:");
  if (ledIndex != -1) {
    int endIndex = packet.indexOf("|", ledIndex);
    if (endIndex == -1) endIndex = packet.length();
    
    String ledCommand = packet.substring(ledIndex + 4, endIndex);
    controlLED(ledCommand);
  }
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;
  
  String packet = "";
  
  // Чтение пакета
  while (LoRa.available()) {
    packet += (char)LoRa.read();
  }
  
  Serial.print("Received: ");
  Serial.println(packet);
  
  // Парсинг пакета и управление светодиодом
  parsePacket(packet);
  
  // Вывод RSSI и SNR
  Serial.print("RSSI: ");
  Serial.print(LoRa.packetRssi());
  Serial.print(" dBm | SNR: ");
  Serial.print(LoRa.packetSnr());
  Serial.println(" dB");
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  // Настройка пина светодиода
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("LoRa Receiver with LED control");

  // Настройка SPI и LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  
  if (!LoRa.begin(BAND)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  
  // Установка параметров приема
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  
  // Регистрация callback для приема пакетов
  LoRa.onReceive(onReceive);
  LoRa.receive();
  
  Serial.println("LoRa receiver ready! Waiting for LED commands...");
}

void loop() {
  // Основной цикл пустой - обработка в прерываниях
  delay(100);
}
