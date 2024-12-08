/*
 * DEBUG
 */

#include <Arduino.h>

void setup_debug() {
  Debug.timestampOn();
  Debug.formatTimestampOn();
  Debug.newlineOn();
  Debug.setDebugLevel(DEFAULT_LOG_LEVEL);
}

// easy to use helper-function for non-blocking timing
bool TimePeriodIsOver(unsigned long &startOfPeriod, unsigned long TimePeriod) {
  unsigned long currentMillis = millis();
  if (currentMillis - startOfPeriod >= TimePeriod) {
    // more time than TimePeriod has elapsed since last time if-condition was true
    startOfPeriod = currentMillis;  // a new period starts right here so set new starttime
    return true;
  } else return false;  // actual TimePeriod is NOT yet over
}
