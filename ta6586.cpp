#include <ESP8266WiFi.h>
#include "motor.h"
#include "config.h"
#include "Arduino_DebugUtils.h"
#include "utils.h"

const char* TA6586_VERSION_ID = "V1.5-esp-wifimanager-magnet-ta6586";

// Check if roof has fully opened or fully closed and turn off relay if so! GG
bool TA6586::isRoofMoving() {
  return bool(MotionStopTime != 0);
}
void TA6586::checkRoofMovement() {

  if (!isRoofMoving()) {
    if (MotionStartTime != 0) {
      if (TimePeriodIsOver(MotionStartTime, ROOF_MOVEMENT_MIN_TIME_MILLIS)) {
        DEBUG_INFO("ROOF_MOVEMENT_MIN_TIME timeout");
        if (isSwitchOn(SWITCH_OPENED) || isSwitchOn(SWITCH_CLOSED)) {
          DEBUG_INFO("Sensors say roof is open or closed: it is not moving...");
        }
        MotionStopTime = millis();
      }
    }
  } else {
    // Roof is moving
    // Add some delay for complete roof opening or closure
    if (TimePeriodIsOver(MotionStopTime, ROOF_MOTION_END_DELAY_MILLIS)) {
      DEBUG_INFO("ROOF_MOTION_END_DELAY: STOP");
      stopCommand();
      MotionStopTime = 0;
    }
    MotionStartTime = 0;
  }
}

void TA6586::motorOff() {
  DEBUG_INFO("Motor OFF");
  // Make sure motors are stopped

  // Set both to high to set motor brake.
  // HIGH means also that LED_BUILTIN will also switch off in case we are using
  // LED_BUILTIN for the motor.
  if (FUNC_DIRECTION_A != PIN_UNDEFINED) {
    digitalWrite(FUNC_DIRECTION_A, HIGH);
    digitalWrite(FUNC_ACTIVATION_A, HIGH);
  }

  // Set both to high to set motor brake.
  if (FUNC_DIRECTION_B != PIN_UNDEFINED) {
    digitalWrite(FUNC_DIRECTION_B, HIGH);
    digitalWrite(FUNC_ACTIVATION_B, HIGH);
  }
  MotionStartTime = 0;
  MotionStopTime = 0;
}

void TA6586::motorOn() {
  DEBUG_INFO("Motor ON");
}

void TA6586::stopCommand() {
  motorOff();  // Disable the motor
  // digitalWrite(FUNC_BLINKER, LOW);
}

void TA6586::connectCommand() {
  // Do nothing
}

void TA6586::openCommand() {
  DEBUG_INFO("OPEN");

  // Blink when opening roof
  // digitalWrite(FUNC_BLINKER, HIGH);

  // Activate the motor
  motorOn();

  if (FUNC_DIRECTION_A != PIN_UNDEFINED) {
    // Move motor: first set pwm and then activate direction.
    // Use PWM to allow different speeds on each motor.
    analogWrite(FUNC_ACTIVATION_A, map(MOTOR_A_SPEED_FACTOR_OPENING, 0, 100, 0, 255));
    // Set actuator voltage leads to open actuator
    digitalWrite(FUNC_DIRECTION_A, LOW);
  }

  if (FUNC_DIRECTION_B != PIN_UNDEFINED) {

    // Use PWM to allow different speeds on each motor.
    analogWrite(FUNC_ACTIVATION_B, map(MOTOR_B_SPEED_FACTOR_OPENING, 0, 100, 0, 255));
    // Set actuator voltage leads to open actuator
    digitalWrite(FUNC_DIRECTION_B, LOW);
  }
  MotionStartTime = millis();
  MotionStopTime = 0;
}

void TA6586::closeCommand() {
  DEBUG_INFO("CLOSE");

  // Blink when closing roof
  // digitalWrite(FUNC_BLINKER, HIGH);

  // Activate the motor
  motorOn();
  if (FUNC_DIRECTION_A != PIN_UNDEFINED) {
    // Move motor: first set pwm and then activate direction.
    // Use PWM to allow different speeds on each motor.
    analogWrite(FUNC_DIRECTION_A, map(MOTOR_A_SPEED_FACTOR_CLOSING, 0, 100, 0, 255));
    // Set actuator voltage leads to close actuator
    digitalWrite(FUNC_ACTIVATION_A, LOW);
  }
  if (FUNC_DIRECTION_B != PIN_UNDEFINED) {

    // Use PWM to allow different speeds on each motor.
    analogWrite(FUNC_DIRECTION_B, map(MOTOR_B_SPEED_FACTOR_CLOSING, 0, 100, 0, 255));
    // Set actuator voltage leads to close actuator
    digitalWrite(FUNC_ACTIVATION_B, LOW);
  }
  MotionStartTime = millis();
  MotionStopTime = 0;
}

const char* TA6586::getVersion() {
  return TA6586_VERSION_ID;
}

bool TA6586::isStopAllowed() {
  return true;
}
