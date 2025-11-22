#include <Arduino.h>
#include "LEDController.h"
#include "LTTController.h"
#include "WiFiManager.h"
#include "MQTTController.h"
#include "state.h"

const int RED_PIN = 5;
const int GREEN_PIN = 6;
const int BLUE_PIN = 7;

const int POT_RED_PIN = 4;
const int POT_GREEN_PIN = 3;
const int POT_BLUE_PIN = 0;

const int MOVING_AVERAGE_SIZE = 8; // Size of the moving average window

int pot1Values[MOVING_AVERAGE_SIZE] = {0};
int pot2Values[MOVING_AVERAGE_SIZE] = {0};
int pot3Values[MOVING_AVERAGE_SIZE] = {0};
int potIndex = 0;

int lastPot1 = 0;
int lastPot2 = 0;
int lastPot3 = 0;

//unsigned long potTimer = 0;
//const int potInterval = 50; // Check every 50ms
//bool potChanged = false;

LEDController ledController(
    RED_PIN, GREEN_PIN, BLUE_PIN,
    0, 1, 2);

LTTController lttController(ledController);
WiFiManager wifiManager(ledController);
MQTTController mqttController(ledController);
StateHandler stateHandler(ledController);

int readAveragedADC(int pin, int samples = 4)
{
  int32_t sum = 0;
  for (int i = 0; i < samples; i++)
  {
    sum += analogReadMilliVolts(pin);
  }
  return sum / samples;
}

int calculateMovingAverage(int *values, int size)
{
  int sum = 0;
  for (int i = 0; i < size; i++)
  {
    sum += values[i];
  }
  return sum / size;
}

void blinkRedLight() {
  // Blink red light three times
  for (int i = 0; i < 3; i++) {
    ledController.setPWMDirectly(2047, 0, 0); // Red on
    delay(300);
    ledController.setPWMDirectly(0, 0, 0); // Off
    delay(300);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Color Shadow Lamp starting up...");
  ledController.begin();
  stateHandler.begin();

  analogSetAttenuation(ADC_2_5db);
  analogSetPinAttenuation(POT_RED_PIN, ADC_2_5db);
  analogSetPinAttenuation(POT_GREEN_PIN, ADC_2_5db);
  analogSetPinAttenuation(POT_BLUE_PIN, ADC_2_5db);
}

void loop()
{
  static unsigned long lastUpdate = 0;
  const unsigned long UPDATE_INTERVAL = 20;

  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdate >= UPDATE_INTERVAL)
  {
    lastUpdate = currentMillis;
    stateHandler.update();

    if (stateHandler.getCurrentMode() == OperationMode::WIFI)
    {
      wifiManager.update();
    }
    else if (stateHandler.getCurrentMode() == OperationMode::MQTT)
    {
      mqttController.update();
    }

    int pot1 = readAveragedADC(POT_RED_PIN);
    int pot2 = readAveragedADC(POT_GREEN_PIN);
    int pot3 = readAveragedADC(POT_BLUE_PIN);

    pot1 = map(constrain(pot1, 5, 950), 5, 950, 0, 2047); // Left pot (meant for Red)
    pot2 = map(constrain(pot2, 5, 950), 5, 950, 0, 2047); // Middle pot (meant for Green)
    pot3 = map(constrain(pot3, 5, 950), 5, 950, 0, 2047); // Right pot (meant for Blue)

    // Update moving average arrays
    pot1Values[potIndex] = pot1;
    pot2Values[potIndex] = pot2;
    pot3Values[potIndex] = pot3;
    potIndex = (potIndex + 1) % MOVING_AVERAGE_SIZE;

    // Calculate moving averages
    pot1 = calculateMovingAverage(pot1Values, MOVING_AVERAGE_SIZE);
    pot2 = calculateMovingAverage(pot2Values, MOVING_AVERAGE_SIZE);
    pot3 = calculateMovingAverage(pot3Values, MOVING_AVERAGE_SIZE);

    //Serial.print("pot1: ");
    //Serial.print(pot1);
    //Serial.print(", pot2: ");
    //Serial.print(pot2);
    //Serial.print(", pot3: ");
    //Serial.println(pot3);

    static bool wasInWiFiMode = false;
    static bool wasInMQTTMode = false;
    static bool wasInRGBMode = false;
    static bool mqttFailureHandled = false;
    bool isInWiFiMode = stateHandler.getCurrentMode() == OperationMode::WIFI;
    bool isInMQTTMode = stateHandler.getCurrentMode() == OperationMode::MQTT;
    bool isInRGBMode = stateHandler.getCurrentMode() == OperationMode::RGB;

    if (isInWiFiMode && !wasInWiFiMode)
    {
      wifiManager.begin();
      ledController.checkAndUpdatePowerLimit();
    }
    else if (!isInWiFiMode && wasInWiFiMode)
    {
      wifiManager.stop();
      ledController.setPWMDirectly(0, 0, 0);
      ledController.checkAndUpdatePowerLimit();
    }

    if (isInMQTTMode && !wasInMQTTMode)
    {
      ledController.setMQTTModePowerLimit();
      mqttController.begin();
      mqttFailureHandled = false;
    }
    else if (!isInMQTTMode && wasInMQTTMode)
    {
      mqttController.stop();
      ledController.setPWMDirectly(0, 0, 0);
    }

    if (isInRGBMode && !wasInRGBMode)
    {
      ledController.setRGBModePowerLimit();
    }

    // Check for MQTT connection failure and fallback to RGB mode
    if (isInMQTTMode && !mqttFailureHandled && mqttController.hasInitialConnectionFailed())
    {
      Serial.println("MQTT connection failed! Blinking red light and falling back to RGB mode...");
      blinkRedLight();
      stateHandler.setMode(OperationMode::RGB);
      mqttController.stop();
      mqttFailureHandled = true;
      isInMQTTMode = false;
    }

    wasInWiFiMode = isInWiFiMode;
    wasInMQTTMode = isInMQTTMode;
    wasInRGBMode = isInRGBMode;

    switch (stateHandler.getCurrentMode())
    {
    case OperationMode::RGB:
      // pot1 is LEFT (physically red), pot3 is RIGHT (physically blue)
      // since the LED pins are now swapped in begin(), we need to swap these too
      ledController.setPWMDirectly(pot3, pot2, pot1); // Swap values to match physical layout
      break;
    case OperationMode::MQTT:
      // LED control happens via MQTT
      break;

    // Temporarily disabled modes:
    // case OperationMode::LTT:
    //   lttController.updateLTT(pot1, pot2, pot3);
    //   break;
    // case OperationMode::POWERCON:
    //   {
    //     float powerLimit = map(pot2, 0, 2047, 50, 1000) / 1000.0f;
    //     ledController.setPowerLimit(powerLimit);
    //     ledController.setPWMForced(2047, 2047, 2047);
    //   }
    //   break;
    // case OperationMode::OFF:
    // case OperationMode::WIFI:
    //   break;
    }
  }
  delay(2);
}