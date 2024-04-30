#ifndef __rolloff_linear_actuator_utils__
#define __rolloff_linear_actuator_utils__


/*
 * DEBUG
 */

void setup_debug() {
  Debug.timestampOn();
  Debug.formatTimestampOn();
  Debug.newlineOn();
  Debug.setDebugLevel(DEFAULT_LOG_LEVEL);
}

#endif