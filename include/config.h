// Configuration file for Color Shadow Lamp
// Public configuration - sensitive values are in secrets.h

#ifndef CONFIG_H
#define CONFIG_H

// Include private configuration (create from secrets.h.template)
#include "secrets.h"

// MQTT Broker Configuration (non-sensitive)
#define MQTT_PORT 1883

// Device Configuration
#define DEVICE_NAME "Color Shadow Lamp"
#define DEVICE_ID "color_shadow_lamp_01"  // Must be unique if you have multiple devices

// Home Assistant MQTT Discovery Configuration
// These topics follow the Home Assistant MQTT Light integration format
// Base topic: homeassistant/light/{device_id}/
// Command topic: homeassistant/light/{device_id}/set
// State topic: homeassistant/light/{device_id}/state
// Config topic: homeassistant/light/{device_id}/config

#endif
