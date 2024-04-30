

// #include <ESP8266WiFi.h>
#include "motor.h"
#include "config.h"
#include "Arduino_DebugUtils.h"
#include "functions.h"

////////////////////////////////////////////////////////////////////////////////
// Action command received

// Here after pin associations resolved and request action known
// Default action is to set the associated relay to the requested state "ON" or "OFF" and
// send acknowledgement to the host.
// target is the name associated with the relay "OPEN", "CLOSE", "STOP", "LOCK", "AUXSET".
// It will be used when  sending the acknowledgement to the host. Find out if a particular
// command is being processed using if (strcmp(target, "OPEN") == 0) {do something}
//
// relay: pin id of the relay
// value: How to set the relay "ON" or "OFF"
//
//
void Motor::runCommand(int command_input, char* value) {
  DEBUG_DEBUG("runCommand %d, %s", command_input, value);  // DEBUG

  switch (command_input) {
    case CMD_STOP:
      stopCommand();
      break;
    case CMD_CONNECT:
      connectCommand();
      break;
    /*
  // AUX Set
  if (command_input == CMD_AUXSET) {

    if (strncmp(value, "ON", 2)) {
      digitalWrite(FUNC_AUX, LOW);
    }
    if (strncmp(value, "OFF", 3)) {
      digitalWrite(FUNC_AUX, HIGH);
    }
  } else
  */
    case CMD_OPEN:
      openCommand();
      break;
    case CMD_CLOSE:
      closeCommand();
      break;
  }                // switch
}
