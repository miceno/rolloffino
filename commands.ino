#ifndef __rolloff_linear_actuator_commands__
#define __rolloff_linear_actuator_commands__

void checkConnection() {
  runCommand(CMD_CONNECT, value);
  // sendAck(value);
}


////////////////////////////////////////////////////////////////////////////////
// Abort movement command received, test to see if abort is allowed.
// If not return false and an error message will be returned to the host. If yes then return true.
// If either fully open or fully closed switches are on then deny the request by returning false.
// If neither switch is on then if there is a specific button (relay) assigned that can stop movement then return true
// to allow it to do so.
//
// This implementation assumes a one button setup and one which does not know if the roof is still moving or
// has hit something and already stopped. Before taking action see how long it has been since movement was initiated.
// If it is longer than the estimate to open or close the roof, assume motion has already stopped. In this case avoid
// emulating the single button push because that would set the roof moving again. If it seems that the roof
// could be moving then return true.
//
// Returning true will cause the Abort request to appear in the commandReceived routine where it will activate
// the requested relay.
//
bool isStopAllowed() {

#ifdef TA6586
  return true;
#elif DRV8871
  return true;
#else
  unsigned long timeNow = millis();

  // If the roof is either fully opened or fully closed, ignore the request.
  if (isSwitchOn(SWITCH_OPENED) || isSwitchOn(SWITCH_CLOSED)) {
    return false;
  }

  // If time since last open or close request is longer than the time for the roof travel return false
  if ((timeNow - timeMove) >= ROOF_OPEN_MILLI) {
    return false;
  } else

  // Stop will be attempted
  {
    return true;
  }
#endif
}

/*
 * Commands
 *
*/

void motor_off() {
  // Disable current to motors
  digitalWrite(MOTOR_ENABLE_A, LOW);
  digitalWrite(MOTOR_ENABLE_B, LOW);

  // Make sure motors are stopped
  digitalWrite(FUNC_DIRECTION_A, LOW);   // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_A, LOW);  // Set actuator in motion

  digitalWrite(FUNC_DIRECTION_B, LOW);   // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_B, LOW);  // Set actuator in motion
}

void motor_on() {
  // Make sure motors are stopped
  digitalWrite(FUNC_DIRECTION_A, HIGH);  // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_A, LOW);  // Set actuator in motion

  digitalWrite(FUNC_DIRECTION_B, HIGH);  // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_B, LOW);  // Set actuator in motion

  // Enable current to motors
  digitalWrite(MOTOR_ENABLE_A, HIGH);
  digitalWrite(MOTOR_ENABLE_B, HIGH);
}

void stopCommand() {
  motor_off();  // Disable the motor
  // digitalWrite(FUNC_BLINKER, LOW);
}

void connectCommand() {
  stopCommand();
}

void openCommand() {
  // digitalWrite(FUNC_BLINKER, HIGH);  // Blink when opening roof

  motor_on();                             // Activate the motor
  digitalWrite(FUNC_DIRECTION_A, LOW);    // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_A, HIGH);  // Set actuator in motion

  digitalWrite(FUNC_DIRECTION_B, LOW);    // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_B, HIGH);  // Set actuator in motion

  MotionStartTime = millis();
}

void closeCommand() {
  // digitalWrite(FUNC_BLINKER, HIGH);  // Blink when closing roof

  motor_on();                            // Activate the motor
  digitalWrite(FUNC_DIRECTION_A, HIGH);  // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_A, LOW);  // Set actuator in motion

  digitalWrite(FUNC_DIRECTION_B, HIGH);  // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_B, LOW);  // Set actuator in motion

  MotionStartTime = millis();
}

#endif
