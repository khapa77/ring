#include "mp3handler.h"
#include <HardwareSerial.h>

// Use Hardware Serial 2 for MP3 communication
HardwareSerial MP3Serial(2);

// MP3 command packet structure
struct MP3Command {
    uint8_t start;
    uint8_t version;
    uint8_t length;
    uint8_t command;
    uint8_t data;
    uint8_t checksum;
};

void setupMP3() {
    // Initialize Hardware Serial 2 for MP3 communication
    MP3Serial.begin(9600, SERIAL_8N1, MP3_RX_PIN, MP3_TX_PIN);
    
    // Configure BUSY pin as input with pull-up
    pinMode(MP3_BUSY_PIN, INPUT_PULLUP);
    
    // Set initial volume (0-30)
    setVolume(20);
    
    Serial.println("MP3 module initialized");
}

void sendMP3Command(uint8_t command, uint8_t data = 0) {
    MP3Command cmd;
    cmd.start = 0x7E;
    cmd.version = 0xFF;
    cmd.length = 0x06;
    cmd.command = command;
    cmd.data = data;
    cmd.checksum = 0xFF - (cmd.version + cmd.length + cmd.command + cmd.data);
    
    MP3Serial.write((uint8_t*)&cmd, sizeof(cmd));
    
    Serial.printf("MP3 Command sent: 0x%02X, Data: 0x%02X\n", command, data);
}

void playGong() {
    // Play track 1 (assuming gong sound is stored as first track)
    playTrack(1);
}

void playTrack(uint8_t trackNumber) {
    if (trackNumber < 1 || trackNumber > 3000) {
        Serial.println("Invalid track number");
        return;
    }
    
    sendMP3Command(MP3_CMD_PLAY_TRACK, trackNumber);
}

void setVolume(uint8_t volume) {
    if (volume > 30) {
        volume = 30;
    }
    
    sendMP3Command(MP3_CMD_SET_VOL, volume);
    Serial.printf("MP3 volume set to: %d\n", volume);
}

void stopPlayback() {
    sendMP3Command(MP3_CMD_STOP);
    Serial.println("MP3 playback stopped");
}

bool isPlaying() {
    // BUSY pin is LOW when playing, HIGH when stopped
    return !digitalRead(MP3_BUSY_PIN);
}

void loopMP3() {
    // Handle any incoming MP3 responses if needed
    if (MP3Serial.available()) {
        String response = MP3Serial.readString();
        Serial.printf("MP3 Response: %s\n", response.c_str());
    }
}

// Alternative implementation using SoftwareSerial if Hardware Serial 2 is not available
#ifdef USE_SOFTWARE_SERIAL
#include <SoftwareSerial.h>
SoftwareSerial MP3SerialSoft(MP3_RX_PIN, MP3_TX_PIN);

void setupMP3Soft() {
    MP3SerialSoft.begin(9600);
    pinMode(MP3_BUSY_PIN, INPUT_PULLUP);
    setVolume(20);
    Serial.println("MP3 module initialized (Software Serial)");
}

void sendMP3CommandSoft(uint8_t command, uint8_t data = 0) {
    MP3Command cmd;
    cmd.start = 0x7E;
    cmd.version = 0xFF;
    cmd.length = 0x06;
    cmd.command = command;
    cmd.data = data;
    cmd.checksum = 0xFF - (cmd.version + cmd.length + cmd.command + cmd.data);
    
    MP3SerialSoft.write((uint8_t*)&cmd, sizeof(cmd));
    
    Serial.printf("MP3 Command sent (Soft): 0x%02X, Data: 0x%02X\n", command, data);
}
#endif
