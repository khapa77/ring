#include <Arduino.h>
#include <WiFi.h>
#include "AudioOutputI2S.h"
#include "AudioFileSourceHTTPStream.h"
#include "AudioGeneratorMP3.h"

const char* ssid = "ASUS";
const char* password = "password";
const char* radioURL = "http://vis.media-ice.musicradio.com/CapitalMP3";

#define I2S_BCLK 26
#define I2S_LRC 25
#define I2S_DOUT 22

AudioOutputI2S *output;
AudioFileSourceHTTPStream *file;
AudioGeneratorMP3 *decoder;

void setup() {
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  output = new AudioOutputI2S();
  output->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  output->SetGain(0.1);

  file = new AudioFileSourceHTTPStream(radioURL);
  decoder = new AudioGeneratorMP3();
  decoder->begin(file, output);
}

void loop() {
  if (decoder->isRunning()) {
    if (!decoder->loop()) {
      decoder->stop();
      delay(1000);
      decoder->begin(file, output);
    }
  } else {
    delay(1000);
    decoder->begin(file, output);
  }
  
  delay(1);
}
