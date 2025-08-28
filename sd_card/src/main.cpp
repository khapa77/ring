#include <Arduino.h>
#include "AudioOutputI2S.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include <SD.h>
#include <SPI.h>

// Пины для MAX98257A
#define I2S_BCLK 26
#define I2S_LRC 25
#define I2S_DOUT 22

// Пины для SD-карты
#define SD_CS 16
#define SD_MOSI 17
#define SD_SCK 5
#define SD_MISO 18

// Глобальные объекты
AudioOutputI2S *output;
AudioFileSourceSD *sdFile;
AudioGeneratorMP3 *decoder;

// Прототипы функций
void startPlayback();
void listFiles();

void setup() {
  Serial.begin(115200);
  
  // Инициализация SD-карты
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card initialization failed!");
    while(1) delay(1000); // Останавливаем если SD не работает
  } else {
    Serial.println("SD Card initialized successfully");
  }

  // Настройка аудиовыхода
  output = new AudioOutputI2S();
  output->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  output->SetGain(1); // Громкость 0.1

  // Запуск воспроизведения с SD-карты
  startPlayback();
}

// Функция для вывода списка файлов на SD-карте
void listFiles() {
  Serial.println("Listing files on SD card:");
  File root = SD.open("/");
  while (File file = root.openNextFile()) {
    Serial.print("  ");
    Serial.print(file.name());
    Serial.print(" (");
    Serial.print(file.size());
    Serial.println(" bytes)");
    file.close();
  }
  root.close();
}

// Запуск воспроизведения
void startPlayback() {
  Serial.println("Starting playback from SD card...");
  
  // Останавливаем предыдущее воспроизведение
  if (decoder && decoder->isRunning()) {
    decoder->stop();
    delete decoder;
    decoder = nullptr;
  }
  if (sdFile) {
    delete sdFile;
    sdFile = nullptr;
  }
  
  // Проверяем наличие файла
  if (!SD.exists("/0001.mp3")) {
    Serial.println("File 0001.mp3 not found on SD card!");
    listFiles();
    Serial.println("Waiting 5 seconds before retry...");
    delay(5000);
    return;
  }
  
  // Создаем объекты для воспроизведения
  sdFile = new AudioFileSourceSD("/0001.mp3");
  decoder = new AudioGeneratorMP3();
  
  // Начинаем воспроизведение
  if (decoder->begin(sdFile, output)) {
    Serial.println("Playback started successfully");
  } else {
    Serial.println("Failed to start MP3 decoder");
  }
}

void loop() {
  if (decoder && decoder->isRunning()) {
    if (!decoder->loop()) {
      // Файл завершен, перезапускаем
      Serial.println("Playback finished, restarting...");
      decoder->stop();
      delay(1000);
      startPlayback();
    }
  } else {
    // Если воспроизведение не запущено, пытаемся перезапустить
    delay(2000);
    startPlayback();
  }
  
  delay(1);
}
