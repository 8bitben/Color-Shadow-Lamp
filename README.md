# Color Shadow Lamp, w/ MQTT Home Assistant Integration

## Information About This Fork
This fork of the Color Shadow Lamp firmware contains only two modes:
- MQTT, for use with Home Assistant
- RGB, manual control via the device knobs

In MQTT mode, the power limit is set to 80%. In RGB mode, the power limit is set to 30%.


## Setup Instructions

### Configuration
Before building the project, you need to set up your private configuration:

1. Copy `include/secrets.h.template` to `include/secrets.h`
2. Edit `include/secrets.h` with your actual WiFi and MQTT credentials:
   - WiFi SSID and password
   - MQTT server IP address
   - MQTT username and password

The `secrets.h` file is ignored by Git to keep your credentials safe.

See MQTT_SETUP.md file for more setup details.

### Programming via USB
To upload code via USB: 
1. Hold the button down
2. Apply 12V
3. Plug in USB
4. Upload code
5. You can release the button once the code starts uploading 
6. Power cycle
