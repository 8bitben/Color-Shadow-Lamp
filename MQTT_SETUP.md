# MQTT Home Assistant Integration Setup

This guide explains how to set up your Color Shadow Lamp to work with Home Assistant via MQTT.

## Prerequisites

1. Home Assistant with MQTT integration installed
2. MQTT broker running (can be the Home Assistant add-on)
3. Your WiFi network credentials

## Configuration Steps

### 1. Update Configuration File

First, create your private configuration file:

1. Copy `include/secrets.h.template` to `include/secrets.h`
2. Edit `include/secrets.h` with your network settings:

```cpp
// WiFi Configuration
#define WIFI_SSID "YourWiFiNetworkName"
#define WIFI_PASSWORD "YourWiFiPassword"

// MQTT Broker Configuration
#define MQTT_SERVER "192.168.1.100"  // Your Home Assistant IP address
#define MQTT_USER ""                 // Leave empty if no authentication required
#define MQTT_PASSWORD ""             // Leave empty if no authentication required
```

**Note**: The `secrets.h` file is ignored by Git to keep your credentials safe. Device configuration settings like `DEVICE_NAME` and `DEVICE_ID` remain in `include/config.h`.

### 2. Flash the Updated Firmware

After updating the configuration, build and upload the firmware to your device.

### 3. Switch to MQTT Mode

Use the button on your device to cycle through modes until you reach "MQTT" mode. The device will:

1. Connect to your WiFi network
2. Connect to the MQTT broker
3. Automatically register with Home Assistant via MQTT Discovery

## Home Assistant Integration

Once the device is in MQTT mode and connected, it will automatically appear in Home Assistant as a new light entity. You can:

- Turn the light on/off
- Control individual RGB color channels
- Set brightness levels
- Use it in automations and scenes

## MQTT Topics

The device uses the following MQTT topics (replace `{device_id}` with your actual device ID):

- **Command Topic**: `homeassistant/light/{device_id}/set`
- **State Topic**: `homeassistant/light/{device_id}/state`
- **Availability Topic**: `homeassistant/light/{device_id}/availability`
- **Config Topic**: `homeassistant/light/{device_id}/config`

## Manual MQTT Control

You can also control the device manually via MQTT by publishing to the command topic:

### Turn On/Off
```json
{"state": "ON"}
{"state": "OFF"}
```

### Set Color
```json
{"state": "ON", "color": {"r": 255, "g": 0, "b": 0}}
```

### Set Brightness
```json
{"state": "ON", "brightness": 128}
```

### Combined Command
```json
{"state": "ON", "color": {"r": 100, "g": 200, "b": 50}, "brightness": 200}
```

## Troubleshooting

### Device Not Appearing in Home Assistant
1. Check that MQTT Discovery is enabled in Home Assistant
2. Verify WiFi and MQTT broker connectivity
3. Check the Home Assistant logs for MQTT messages
4. Ensure the device ID is unique

### Connection Issues
1. Verify WiFi credentials in `include/secrets.h`
2. Check MQTT broker IP address and port in `include/secrets.h`
3. Ensure MQTT broker is accessible from the device's network
4. Check if MQTT authentication is required

### LED Control Issues
1. Verify the device is receiving commands (check serial output)
2. Test with manual MQTT commands
3. Check Home Assistant entity state

## Operation Modes

The device has multiple operation modes accessible via the button:
- **RGB**: Manual control via potentiometers
- **LTT**: Linus Tech Tips colors
- **POWERCON**: Power consumption testing
- **WIFI**: Web interface control
- **MQTT**: Home Assistant integration (new)
- **OFF**: Device off

Press the button to cycle through modes. The current mode is displayed in the serial output.
