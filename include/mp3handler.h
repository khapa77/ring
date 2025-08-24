#pragma once

#include <Arduino.h>
#include "schedule.h"  // 🆕 Для доступа к SoundType

// MP3-TF-16P pin definitions
#define MP3_RX_PIN 16  // ESP32 GPIO16 -> MP3 TX
#define MP3_TX_PIN 17  // ESP32 GPIO17 -> MP3 RX
#define MP3_BUSY_PIN 4 // ESP32 GPIO4 -> MP3 BUSY (изменено с GPIO18 для устранения конфликта с LoRa)

// MP3 commands
#define MP3_CMD_PLAY 0x01
#define MP3_CMD_PAUSE 0x02
#define MP3_CMD_STOP 0x0E
#define MP3_CMD_NEXT 0x03
#define MP3_CMD_PREV 0x04
#define MP3_CMD_VOL_UP 0x05
#define MP3_CMD_VOL_DOWN 0x06
#define MP3_CMD_SET_VOL 0x07
#define MP3_CMD_PLAY_TRACK 0x12

// Function declarations
void setupMP3();
void playGong();
void playTrack(uint8_t trackNumber);
void playSoundByType(SoundType soundType);  // 🆕 Воспроизведение звука по типу
void setVolume(uint8_t volume);
void stopPlayback();
bool isPlaying();
void loopMP3();

// Utility functions
String getSoundTypeName(SoundType soundType);  // 🆕 Получение названия звука
SoundType getSoundTypeByNumber(uint8_t trackNumber);  // 🆕 Получение типа звука по номеру трека
