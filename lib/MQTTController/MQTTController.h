#ifndef MQTTCONTROLLER_H
#define MQTTCONTROLLER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "LEDController.h"
#include "config.h"

class MQTTController {
private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    LEDController &ledController;
    
    // Configuration - update config.h file with your settings
    const char* wifi_ssid = WIFI_SSID;
    const char* wifi_password = WIFI_PASSWORD;
    const char* mqtt_server = MQTT_SERVER;
    const int mqtt_port = MQTT_PORT;
    const char* mqtt_user = MQTT_USER;
    const char* mqtt_password = MQTT_PASSWORD;
    
    // Device identification
    const char* device_name = DEVICE_NAME;
    const char* device_id = DEVICE_ID;
    
    // MQTT topics
    String command_topic;
    String state_topic;
    String availability_topic;
    String config_topic;
    
    // Power limit: 80% of full PWM (2047) to prevent overheating
    static const int MAX_PWM = 1638;

    // State tracking
    int current_red = 255;   // Start with white color
    int current_green = 255;
    int current_blue = 255;
    bool is_on = false;
    
    unsigned long lastReconnectAttempt = 0;
    unsigned long lastHeartbeat = 0;
    const unsigned long RECONNECT_INTERVAL = 5000;
    const unsigned long HEARTBEAT_INTERVAL = 30000;

    int connectionAttempts = 0;
    bool initialConnectionFailed = false;
    
    void setupTopics() {
        String base = "homeassistant/light/" + String(device_id);
        command_topic = base + "/set";
        state_topic = base + "/state";
        availability_topic = base + "/availability";
        config_topic = base + "/config";
    }
    
    void publishDiscoveryConfig() {
        StaticJsonDocument<1024> doc; // Increased size for better safety
        
        doc["name"] = device_name;
        doc["unique_id"] = device_id;
        doc["default_entity_id"] = "light." + String(device_id);
        doc["state_topic"] = state_topic;
        doc["command_topic"] = command_topic;
        doc["availability_topic"] = availability_topic;
        doc["schema"] = "json";
        doc["brightness"] = true;
        doc["supported_color_modes"] = JsonArray(); // New HA format
        doc["supported_color_modes"].add("rgb");
        doc["optimistic"] = false;
        doc["retain"] = true;
        doc["brightness_scale"] = 255;
        
        // Device info for proper grouping in HA
        JsonObject device = doc["device"];
        JsonArray identifiers = device["identifiers"];
        identifiers.add(device_id);
        device["name"] = device_name;
        device["model"] = "Color Shadow LED Lamp";
        device["manufacturer"] = "RCTESTFLIGHT";
        device["sw_version"] = "1.0.0";
        device["hw_version"] = "1.0";
        device["suggested_area"] = "Living Room"; // Optional: suggest an area
        device["configuration_url"] = "http://" + WiFi.localIP().toString(); // Optional: link to device
        
        String config_payload;
        serializeJson(doc, config_payload);
        
        Serial.println("=== MQTT Discovery Config ===");
        Serial.printf("Topic: %s\n", config_topic.c_str());
        Serial.printf("Payload: %s\n", config_payload.c_str());
        Serial.println("=============================");
        
        bool result = mqttClient.publish(config_topic.c_str(), config_payload.c_str(), true);
        Serial.printf("Discovery config published: %s\n", result ? "SUCCESS" : "FAILED");
        
        if (!result) {
            Serial.printf("MQTT client state: %d\n", mqttClient.state());
            Serial.printf("MQTT buffer size might be too small for payload size: %d\n", config_payload.length());
        }
    }
    
    void publishState() {
        StaticJsonDocument<256> doc;
        
        doc["state"] = is_on ? "ON" : "OFF";
        doc["color_mode"] = "rgb"; // Tell HA we're in RGB mode
        
        if (is_on) {
            // Calculate brightness as the maximum of the RGB values
            int max_rgb = max(max(current_red, current_green), current_blue);
            doc["brightness"] = map(max_rgb, 0, MAX_PWM, 0, 255);
            
            // Always include color information in RGB format
            JsonObject color = doc.createNestedObject("color");
            color["r"] = map(current_red, 0, MAX_PWM, 0, 255);
            color["g"] = map(current_green, 0, MAX_PWM, 0, 255);
            color["b"] = map(current_blue, 0, MAX_PWM, 0, 255);
        } else {
            // When off, still report brightness as 0 but include last color
            doc["brightness"] = 0;
            JsonObject color = doc.createNestedObject("color");
            color["r"] = map(current_red, 0, MAX_PWM, 0, 255);
            color["g"] = map(current_green, 0, MAX_PWM, 0, 255);
            color["b"] = map(current_blue, 0, MAX_PWM, 0, 255);
        }
        
        String state_payload;
        serializeJson(doc, state_payload);
        
        Serial.printf("Publishing state: %s\n", state_payload.c_str());
        bool result = mqttClient.publish(state_topic.c_str(), state_payload.c_str(), true);
        Serial.printf("State published: %s\n", result ? "SUCCESS" : "FAILED");
    }
    
    void handleCommand(String payload) {
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
            Serial.println("Failed to parse MQTT command");
            return;
        }
        
        Serial.printf("Handling MQTT command: %s\n", payload.c_str());
        
        // Handle state command
        if (doc.containsKey("state")) {
            String state = doc["state"];
            is_on = (state == "ON");
            Serial.printf("MQTT: State set to %s\n", is_on ? "ON" : "OFF");
        }
        
        // Handle color mode (HA might send this)
        if (doc.containsKey("color_mode")) {
            String color_mode = doc["color_mode"];
            Serial.printf("MQTT: Color mode: %s\n", color_mode.c_str());
            // We only support RGB mode, so this is just for logging
        }
        
        // Handle RGB color - this takes priority over brightness
        if (doc.containsKey("color")) {
            JsonObject color = doc["color"].as<JsonObject>();
            if (color.containsKey("r") && color.containsKey("g") && color.containsKey("b")) {
                int r = constrain((int)color["r"], 0, 255);
                int g = constrain((int)color["g"], 0, 255);
                int b = constrain((int)color["b"], 0, 255);
                
                current_red = map(r, 0, 255, 0, MAX_PWM);
                current_green = map(g, 0, 255, 0, MAX_PWM);
                current_blue = map(b, 0, 255, 0, MAX_PWM);
                
                Serial.printf("MQTT: Color set to R=%d G=%d B=%d (PWM: %d,%d,%d)\n", 
                             r, g, b, current_red, current_green, current_blue);
                
                // If setting color, automatically turn on if not explicitly off
                if (!doc.containsKey("state")) {
                    is_on = true;
                }
            }
        }
        // Handle brightness (affects all channels proportionally)
        else if (doc.containsKey("brightness")) {
            int brightness = constrain((int)doc["brightness"], 0, 255);
            
            // If we have existing color ratios, maintain them
            if (current_red > 0 || current_green > 0 || current_blue > 0) {
                int max_current = max(max(current_red, current_green), current_blue);
                if (max_current > 0) {
                    float scale = map(brightness, 0, 255, 0, MAX_PWM) / (float)max_current;
                    current_red = (int)(current_red * scale);
                    current_green = (int)(current_green * scale);
                    current_blue = (int)(current_blue * scale);
                }
            } else {
                // No existing color, set to white at specified brightness
                int pwm_val = map(brightness, 0, 255, 0, MAX_PWM);
                current_red = current_green = current_blue = pwm_val;
            }
            
            Serial.printf("MQTT: Brightness set to %d (PWM: %d,%d,%d)\n", 
                         brightness, current_red, current_green, current_blue);
            
            // If setting brightness, automatically turn on if not explicitly off
            if (!doc.containsKey("state")) {
                is_on = true;
            }
        }
        
        // Apply the changes to the LED controller
        if (is_on) {
            ledController.setPWMDirectly(current_red, current_green, current_blue);
            Serial.printf("LEDs set to: R=%d G=%d B=%d\n", current_red, current_green, current_blue);
        } else {
            ledController.setPWMDirectly(0, 0, 0);
            Serial.println("LEDs turned OFF");
        }
        
        // Publish updated state back to HA
        publishState();
    }
    
    static void mqttCallback(char* topic, byte* payload, unsigned int length) {
        // This is a static callback, so we need to access the instance
        // We'll store a static pointer to the current instance
        String message;
        for (unsigned int i = 0; i < length; i++) {
            message += (char)payload[i];
        }
        
        Serial.printf("MQTT message received on %s: %s\n", topic, message.c_str());
        
        // Find the instance and call the handler
        if (current_instance) {
            current_instance->handleCommand(message);
        }
    }
    
    static MQTTController* current_instance;

public:
    MQTTController(LEDController &controller) 
        : mqttClient(wifiClient), ledController(controller) {
        current_instance = this;
        setupTopics();
    }
    
    void begin() {
        Serial.println("Starting MQTT mode...");

        connectionAttempts = 0;
        initialConnectionFailed = false;

        // Connect to WiFi
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifi_ssid, wifi_password);

        Serial.printf("Connecting to WiFi network: %s\n", wifi_ssid);
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 30) {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("\nFailed to connect to WiFi");
            initialConnectionFailed = true;
            return;
        }

        Serial.println();
        Serial.printf("Connected to WiFi. IP address: %s\n", WiFi.localIP().toString().c_str());

        // Setup MQTT
        Serial.printf("Connecting to MQTT broker: %s:%d\n", mqtt_server, mqtt_port);
        mqttClient.setServer(mqtt_server, mqtt_port);
        mqttClient.setCallback(mqttCallback);
        mqttClient.setBufferSize(1024); // Increase buffer size for larger payloads

        // Debug topic information
        Serial.println("=== MQTT Topics ===");
        Serial.printf("Command: %s\n", command_topic.c_str());
        Serial.printf("State: %s\n", state_topic.c_str());
        Serial.printf("Availability: %s\n", availability_topic.c_str());
        Serial.printf("Config: %s\n", config_topic.c_str());
        Serial.println("==================");

        // Initial connection attempts (2 attempts max)
        for (int i = 0; i < 2 && !mqttClient.connected(); i++) {
            reconnect();
            if (!mqttClient.connected() && i < 1) {
                delay(2000);
            }
        }

        if (!mqttClient.connected()) {
            Serial.println("Failed to connect to MQTT after 2 attempts");
            initialConnectionFailed = true;
        }
    }
    
    void reconnect() {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected, attempting reconnection...");
            WiFi.reconnect();
            return;
        }
        
        if (!mqttClient.connected()) {
            Serial.print("Attempting MQTT connection...");
            
            String client_id = String(device_id) + "_" + String(random(0xffff), HEX);
            Serial.printf("Client ID: %s\n", client_id.c_str());
            
            bool connected;
            if (strlen(mqtt_user) > 0) {
                Serial.printf("Connecting with credentials: %s\n", mqtt_user);
                connected = mqttClient.connect(client_id.c_str(), mqtt_user, mqtt_password, 
                                             availability_topic.c_str(), 1, true, "offline");
            } else {
                Serial.println("Connecting without credentials");
                connected = mqttClient.connect(client_id.c_str(), availability_topic.c_str(), 
                                             1, true, "offline");
            }
            
            if (connected) {
                Serial.println("MQTT connected successfully!");
                
                // Publish availability
                bool avail_result = mqttClient.publish(availability_topic.c_str(), "online", true);
                Serial.printf("Availability published: %s\n", avail_result ? "SUCCESS" : "FAILED");
                
                // Subscribe to command topic
                bool sub_result = mqttClient.subscribe(command_topic.c_str());
                Serial.printf("Subscribed to commands: %s\n", sub_result ? "SUCCESS" : "FAILED");
                
                // Small delay before discovery config
                delay(100);
                
                // Test basic publishing first
                String test_topic = "homeassistant/test/" + String(device_id);
                bool test_result = mqttClient.publish(test_topic.c_str(), "test_message", false);
                Serial.printf("Test publish result: %s\n", test_result ? "SUCCESS" : "FAILED");
                
                // Publish discovery config
                publishDiscoveryConfig();
                
                // Small delay before state
                delay(100);
                
                // Publish initial state
                publishState();
                
                Serial.println("MQTT setup complete - device should appear in HA");
            } else {
                Serial.printf("MQTT connection failed, rc=%d\n", mqttClient.state());
                switch(mqttClient.state()) {
                    case -4: Serial.println("Connection timeout"); break;
                    case -3: Serial.println("Connection lost"); break;
                    case -2: Serial.println("Connect failed"); break;
                    case -1: Serial.println("Disconnected"); break;
                    case 1: Serial.println("Bad protocol"); break;
                    case 2: Serial.println("Bad client ID"); break;
                    case 3: Serial.println("Unavailable"); break;
                    case 4: Serial.println("Bad credentials"); break;
                    case 5: Serial.println("Unauthorized"); break;
                }
                Serial.println("Retrying in 5 seconds");
            }
        }
    }
    
    void update() {
        unsigned long now = millis();
        
        // Handle MQTT loop
        if (mqttClient.connected()) {
            mqttClient.loop();
            
            // Send periodic heartbeat
            if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
                mqttClient.publish(availability_topic.c_str(), "online", true);
                lastHeartbeat = now;
            }
        } else {
            // Attempt reconnection
            if (now - lastReconnectAttempt >= RECONNECT_INTERVAL) {
                lastReconnectAttempt = now;
                reconnect();
            }
        }
    }
    
    void stop() {
        if (mqttClient.connected()) {
            mqttClient.publish(availability_topic.c_str(), "offline", true);
            mqttClient.disconnect();
        }
        WiFi.disconnect();
        Serial.println("MQTT controller stopped");
    }
    
    bool isConnected() {
        return WiFi.status() == WL_CONNECTED && mqttClient.connected();
    }

    bool hasInitialConnectionFailed() {
        return initialConnectionFailed;
    }
};

// Static member definition
MQTTController* MQTTController::current_instance = nullptr;

#endif
