#ifndef STATE_H
#define STATE_H

#include <Arduino.h>
#include "LEDController.h"

enum class OperationMode {
    RGB,
    LTT,
    POWERCON,
    MQTT,
    WIFI,
    OFF,
};

class StateHandler {
private:
    static constexpr int BUTTON_PIN = 9;
    static constexpr int DEBOUNCE_TIME = 50;
    
    OperationMode currentMode;
    unsigned long lastButtonPress;
    bool buttonWasPressed;
    LEDController &ledController;

public:
    StateHandler(LEDController &controller)
        : currentMode(OperationMode::MQTT), lastButtonPress(0), buttonWasPressed(false), ledController(controller) {}

    void begin() {
        currentMode = OperationMode::MQTT;
        pinMode(BUTTON_PIN, INPUT);
        Serial.println("Initial mode: MQTT");
    }

    OperationMode getCurrentMode() const {
        return currentMode;
    }

    void setMode(OperationMode mode) {
        currentMode = mode;
        Serial.print("Mode set to: ");
        switch(mode) {
            case OperationMode::RGB: Serial.println("RGB"); break;
//            case OperationMode::LTT: Serial.println("LTT"); break;
//            case OperationMode::POWERCON: Serial.println("POWERCON"); break;
            case OperationMode::MQTT: Serial.println("MQTT"); break;
//            case OperationMode::WIFI: Serial.println("WIFI"); break;
//            case OperationMode::OFF: Serial.println("OFF"); break;
        }
    }

    void update() {
        bool buttonIsPressed = (digitalRead(BUTTON_PIN) == 0);
        
        if (buttonIsPressed && !buttonWasPressed) {
            unsigned long now = millis();
            if (now - lastButtonPress >= DEBOUNCE_TIME) {
                lastButtonPress = now;
                
                switch (currentMode) {
                    case OperationMode::OFF:
                        currentMode = OperationMode::RGB;
                        Serial.println("Mode changed to: RGB");
                        break;
                    case OperationMode::RGB:
                        currentMode = OperationMode::LTT;
                        Serial.println("Mode changed to: LTT");
                        break;
                    case OperationMode::LTT:
                        currentMode = OperationMode::POWERCON;
                        Serial.println("Mode changed to: POWERCON");
                        break;
                    case OperationMode::POWERCON:
                        currentMode = OperationMode::MQTT;
                        Serial.println("Mode changed to: MQTT");
                        break;
                    case OperationMode::MQTT:
                        currentMode = OperationMode::WIFI;
                        Serial.println("Mode changed to: WIFI");
                        break;
                    case OperationMode::WIFI:
                        currentMode = OperationMode::OFF;
                        Serial.println("Mode changed to: OFF");
                        break;
                }
            }
        }
        buttonWasPressed = buttonIsPressed;
    }
};

#endif