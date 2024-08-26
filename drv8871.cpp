#ifndef __rolloff_linear_actuator_DRV8871__
#define __rolloff_linear_actuator_DRV8871__

#include <ESP8266WiFi.h>
#include "motor.h"

const char* DRV8871_VERSION_ID = "V1.3-esp-wifi-magnet-DRV8871";

const char* DRV8871::getVersion() {
  return DRV8871_VERSION_ID;
}

#endif