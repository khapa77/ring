# ESP32 Gong/Ring System

A complete ESP32-based gong scheduling system with LoRa communication, MP3 playback, and web interface management.

## Features

- **WiFi Management**: Automatic fallback to Access Point mode if WiFi connection fails
- **Schedule Management**: Add, edit, delete, and manage gong schedules with persistent storage
- **MP3 Playback**: Play gong sounds using the MP3-TF-16P module
- **LoRa Communication**: Send and receive gong triggers via LoRa (XL1278-SMT)
- **Web Interface**: Modern Bootstrap-based web interface for schedule management
- **API Endpoints**: RESTful API for programmatic control
- **Offline Operation**: Works in AP mode when WiFi is unavailable

## Hardware Requirements

- **ESP32-D0WD-V3** development board
- **XL1278-SMT LoRa module** (433 MHz)
- **MP3-TF-16P audio module**
- **MicroSD card** (for MP3 storage)
- **Speaker/headphones** for audio output

## Pin Connections

### LoRa Module (XL1278-SMT)
- **CS**: GPIO5
- **RST**: GPIO14  
- **DIO0**: GPIO2
- **SCK**: GPIO18
- **MISO**: GPIO19
- **MOSI**: GPIO23

### MP3 Module (MP3-TF-16P)
- **RX**: GPIO17 (ESP32 TX)
- **TX**: GPIO16 (ESP32 RX)
- **BUSY**: GPIO18

## Software Setup

### 1. Install PlatformIO

```bash
# Install PlatformIO Core
pip install platformio

# Or install PlatformIO IDE extension in VS Code
```

### 2. Clone and Build

```bash
git clone <repository-url>
cd ring
platformio run
```

### 3. Upload to ESP32

```bash
platformio run --target upload
```

### 4. Monitor Serial Output

```bash
platformio device monitor
```

## Configuration

### WiFi Settings

Edit `webhandler.cpp` to set your WiFi credentials:

```cpp
const char* sta_ssid = "YOUR_WIFI_SSID";
const char* sta_password = "YOUR_WIFI_PASSWORD";
```

### LoRa Settings

Modify `lorahandler.cpp` for your frequency and power requirements:

```cpp
#define LORA_FREQUENCY 433E6  // Change to your region's frequency
#define LORA_TX_POWER 20      // Adjust power (0-20 dBm)
```

## Usage

### Web Interface

1. **Connect to WiFi**: The ESP32 will attempt to connect to your configured WiFi network
2. **Access Point Mode**: If WiFi fails, connect to "GonggonG" network (password: "vipassana")
3. **Open Browser**: Navigate to the ESP32's IP address (usually 192.168.4.1 in AP mode)

### Schedule Management

- **Add Schedule**: Set hour, minute, and description for gong times
- **Edit Schedule**: Modify existing schedule entries
- **Delete Schedule**: Remove unwanted schedule entries
- **Enable/Disable**: Toggle individual schedule entries on/off

### Manual Control

- **Play Locally**: Trigger gong sound immediately on this device
- **Send via LoRa**: Broadcast gong trigger to other LoRa devices
- **Refresh**: Update schedule display from server

## API Endpoints

### GET /schedule
Returns the current schedule as JSON array.

**Response:**
```json
[
  {
    "id": 1,
    "hour": 6,
    "minute": 0,
    "enabled": true,
    "description": "Morning meditation"
  }
]
```

### POST /schedule
Add a new schedule entry.

**Request Body:**
```json
{
  "hour": 6,
  "minute": 0,
  "description": "Morning meditation"
}
```

### PUT /schedule
Edit an existing schedule entry.

**Request Body:**
```json
{
  "id": 1,
  "hour": 6,
  "minute": 30,
  "description": "Morning meditation (updated)"
}
```

### DELETE /schedule?id={id}
Delete a schedule entry by ID.

### POST /play
Trigger local gong playback.

### POST /play-lora
Send gong trigger via LoRa.

## LoRa Message Format

Messages are sent with a type header and JSON payload:

```
Type:Payload
```

**Example:**
```
1:{"type":"gong","timestamp":1234567890,"device":"ESP32_Gong"}
```

**Message Types:**
- `1`: Gong trigger
- `2`: Schedule synchronization
- `3`: Status/health check

## File Structure

```
ring/
├── data/
│   └── index.html          # Web interface
├── src/
│   ├── main.cpp            # Main application logic
│   ├── webhandler.cpp      # WiFi and web server
│   ├── lorahandler.cpp     # LoRa communication
│   ├── mp3handler.cpp      # MP3 playback control
│   └── schedule.cpp        # Schedule management
├── include/
│   ├── webhandler.h        # Web handler declarations
│   ├── lorahandler.h       # LoRa handler declarations
│   ├── mp3handler.h        # MP3 handler declarations
│   └── schedule.h          # Schedule declarations
├── platformio.ini          # PlatformIO configuration
└── README.md               # This file
```

## Troubleshooting

### Common Issues

1. **WiFi Connection Fails**
   - Check SSID and password in `webhandler.cpp`
   - Verify WiFi network availability
   - System will automatically switch to AP mode

2. **LoRa Not Working**
   - Verify pin connections
   - Check frequency settings for your region
   - Ensure proper power supply

3. **MP3 Not Playing**
   - Check audio connections
   - Verify MP3 files are on SD card
   - Check BUSY pin connection

4. **Web Interface Not Loading**
   - Check if SPIFFS is properly initialized
   - Verify `index.html` is in `data/` folder
   - Check serial monitor for error messages

### Serial Debug Output

Enable debug output by setting in `platformio.ini`:

```ini
build_flags = -DCORE_DEBUG_LEVEL=3
```

## Development

### Adding New Features

1. **New API Endpoints**: Add handlers in `webhandler.cpp`
2. **New LoRa Message Types**: Extend message handling in `lorahandler.cpp`
3. **Additional Audio Controls**: Extend `mp3handler.cpp`

### Testing

- Use PlatformIO's built-in testing framework
- Test LoRa communication with multiple devices
- Verify schedule persistence across reboots

## License

This project is open source. Please check the license file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Support

For issues and questions:
1. Check the troubleshooting section
2. Review serial monitor output
3. Open an issue on the repository
4. Check ESP32 and LoRa documentation

---

**Note**: This system is designed for meditation centers, temples, or personal use. Ensure compliance with local regulations for LoRa frequency usage.
