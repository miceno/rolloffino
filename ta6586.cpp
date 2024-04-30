#ifndef __rolloff_linear_actuator_ta6586__
#define __rolloff_linear_actuator_ta6586__

#include <ESP8266WiFi.h>
#include "motor.h"
#include "config.h"
#include "Arduino_DebugUtils.h"
#include "functions.h"

const char* TA6586_VERSION_ID = "V1.3-esp-wifi-magnet-ta6586";

// Check if roof has fully opened or fully closed and turn off relay if so! GG
bool TA6586::isRoofMoving(){
  return bool(MotionStopTime != 0);
}
void TA6586::checkRoofMovement() {

  if (!isRoofMoving()) {
    if (MotionStartTime != 0) {
      if ((millis() - MotionStartTime) > ROOF_MOVEMENT_MIN_TIME_MILLIS) {
        if (isSwitchOn(SWITCH_OPENED) || isSwitchOn(SWITCH_CLOSED)) {
          DEBUG_INFO("Roof is not moving...");  // DEBUG
          MotionStopTime = millis();
        }
      }
    }
  } else {
    // Roof is moving  
    // Add some delay for complete roof opening or closure
    if ((millis() - MotionStopTime) > ROOF_MOTION_END_DELAY_MILLIS) {
      DEBUG_INFO("Stop...");  // DEBUG
      stopCommand();
      MotionStopTime = 0;
    }
    MotionStartTime = 0;
  }
}

void TA6586::motorOff() {
  // Disable current to motors
  digitalWrite(MOTOR_ENABLE_A, LOW);
  digitalWrite(MOTOR_ENABLE_B, LOW);

  // Make sure motors are stopped
  digitalWrite(FUNC_DIRECTION_A, LOW);   // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_A, LOW);  // Set actuator in motion

  digitalWrite(FUNC_DIRECTION_B, HIGH);   // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_B, HIGH);  // Set actuator in motion
}

void TA6586::motorOn() {
  // Make sure motors are stopped
  digitalWrite(FUNC_DIRECTION_A, HIGH);  // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_A, LOW);  // Set actuator in motion

  digitalWrite(FUNC_DIRECTION_B, HIGH);  // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_B, LOW);  // Set actuator in motion

  // Enable current to motors
  digitalWrite(MOTOR_ENABLE_A, HIGH);
  digitalWrite(MOTOR_ENABLE_B, HIGH);
}

void TA6586::stopCommand() {
  motorOff();  // Disable the motor
  // digitalWrite(FUNC_BLINKER, LOW);
}

void TA6586::connectCommand() {
  // Do nothing
}

void TA6586::openCommand() {
  // digitalWrite(FUNC_BLINKER, HIGH);  // Blink when opening roof

  motorOn();                             // Activate the motor
  digitalWrite(FUNC_DIRECTION_A, LOW);    // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_A, HIGH);  // Set actuator in motion

  digitalWrite(FUNC_DIRECTION_B, LOW);    // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_B, HIGH);  // Set actuator in motion

  MotionStartTime = millis();
}

void TA6586::closeCommand() {
  // digitalWrite(FUNC_BLINKER, HIGH);  // Blink when closing roof

  motorOn();                            // Activate the motor
  digitalWrite(FUNC_DIRECTION_A, HIGH);  // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_A, LOW);  // Set actuator in motion

  digitalWrite(FUNC_DIRECTION_B, HIGH);  // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_B, LOW);  // Set actuator in motion

  MotionStartTime = millis();
}

const char *TA6586::getVersion(){
  return TA6586_VERSION_ID;
}

bool TA6586::isStopAllowed() {
  return true;
}

#endif