#ifndef __rolloff_linear_actuator_ta6586__
#define __rolloff_linear_actuator_ta6586__

#include <ESP8266WiFi.h>
#include "motor.h"
#include "config.h"
#include "Arduino_DebugUtils.h"
#include "functions.h"
#include "oled_console.h"

const char* TA6586_VERSION_ID = "V1.3-esp-wifi-magnet-ta6586";

// Check if roof has fully opened or fully closed and turn off relay if so! GG
bool TA6586::isRoofMoving() {
  return bool(MotionStopTime != 0);
}
void TA6586::checkRoofMovement() {

  if (!isRoofMoving()) {
    if (MotionStartTime != 0) {
      if ((millis() - MotionStartTime) > ROOF_MOVEMENT_MIN_TIME_MILLIS) {
        if (isSwitchOn(SWITCH_OPENED) || isSwitchOn(SWITCH_CLOSED)) {
          DEBUG_INFO("Sensors say roof is open or closed: it is not moving...");  // DEBUG
        }
        MotionStopTime = millis();
      }
    }
  } else {
    // Roof is moving
    // Add some delay for complete roof opening or closure
    if ((millis() - MotionStopTime) > ROOF_MOTION_END_DELAY_MILLIS) {
      DEBUG_INFO("STOP");  // DEBUG
      oledConsole->print("STOP");
      stopCommand();
      MotionStopTime = 0;
    }
    MotionStartTime = 0;
  }
}

void TA6586::motorOff() {
  DEBUG_INFO("Motor OFF");
  oledConsole->print("Motor OFF");
  // Make sure motors are stopped

  // Set both to high to disable motor. 
  // HIGH means also that LED_BUILTIN will also switch off in case we are using
  // LED_BUILTIN for the motor.
  digitalWrite(FUNC_DIRECTION_A, HIGH);   
  digitalWrite(FUNC_ACTIVATION_A, HIGH);  

  // Set both to high to disable motor.
  digitalWrite(FUNC_DIRECTION_B, HIGH);
  digitalWrite(FUNC_ACTIVATION_B, HIGH);
  MotionStartTime = 0;
  MotionStopTime = 0;
}

void TA6586::motorOn() {
  DEBUG_INFO("Motor ON");
  // Make sure motors starts
  oledConsole->print("Motor ON");
}

void TA6586::stopCommand() {
  motorOff();  // Disable the motor
  // digitalWrite(FUNC_BLINKER, LOW);
}

void TA6586::connectCommand() {
  // Do nothing
}

void TA6586::openCommand() {
  oledConsole->print("OPEN");

  // digitalWrite(FUNC_BLINKER, HIGH);  // Blink when opening roof
  motorOn();                              // Activate the motor
  analogWrite(FUNC_DIRECTION_A, map(MOTOR_A_SPEED_FACTOR_OPENING, 0, 100, 0, 255));    // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_A, LOW);  // Set actuator in motion

  digitalWrite(FUNC_DIRECTION_B, map(MOTOR_B_SPEED_FACTOR_OPENING, 0, 100, 0, 255));    // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_B, LOW);  // Set actuator in motion

  MotionStartTime = millis();
  MotionStopTime = 0;
}

void TA6586::closeCommand() {
  oledConsole->print("CLOSE");
  // digitalWrite(FUNC_BLINKER, HIGH);  // Blink when closing roof
  motorOn();                             // Activate the motor
  digitalWrite(FUNC_DIRECTION_A, LOW);  // Set actuator voltage leads to open actuator
  analogWrite(FUNC_ACTIVATION_A, map(MOTOR_A_SPEED_FACTOR_CLOSING, 0, 100, 0, 255));  // Set actuator in motion

  digitalWrite(FUNC_DIRECTION_B, LOW);  // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_B, map(MOTOR_B_SPEED_FACTOR_CLOSING, 0, 100, 0, 255));  // Set actuator in motion

  MotionStartTime = millis();
  MotionStopTime = 0;
}

const char* TA6586::getVersion() {
  return TA6586_VERSION_ID;
}

bool TA6586::isStopAllowed() {
  return true;
}

#endif