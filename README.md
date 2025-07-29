# Color Shadow Lamp

## Setup Instructions

### Configuration
Before building the project, you need to set up your private configuration:

1. Copy `include/secrets.h.template` to `include/secrets.h`
2. Edit `include/secrets.h` with your actual WiFi and MQTT credentials:
   - WiFi SSID and password
   - MQTT server IP address
   - MQTT username and password

The `secrets.h` file is ignored by Git to keep your credentials safe.

### Programming via USB
To upload code via USB: 
1. Hold the button down
2. Apply 12V
3. Plug in USB
4. Upload code
5. You can release the button once the code starts uploading 
6. Power cycle
