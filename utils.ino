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
bool TimePeriodIsOver(unsigned long& startOfPeriod, unsigned long TimePeriod) {
  unsigned long currentMillis = millis();
  if (currentMillis - startOfPeriod >= TimePeriod) {
    // more time than TimePeriod has elapsed since last time if-condition was true
    startOfPeriod = currentMillis;  // a new period starts right here so set new starttime
    return true;
  } else return false;  // actual TimePeriod is NOT yet over
}

/*
 * Get switch value
 * NO contacts used and configured with a pull up resistor to the Arduino input pin. Logical 1 input when switch opened. GG
 * When switch closes The LOW voltage logical 0 is applied to the input pin. GG
 * The off or on value is to be sent to the host in the ACK response
 */
void getSwitch(int id, char* value) {
  if (digitalRead(id) == OPEN_CONTACT) {
    strcpy(value, "OFF");
  } else {
    strcpy(value, "ON");
  }
  DEBUG_VERBOSE("gs:id=%d,v=%s", id, value);
}

bool isSwitchOn(int id) {
  char switch_value[16 + 1];
  getSwitch(id, switch_value);
  if (strcmp(switch_value, "ON") == 0) {
    return true;
  }
  return false;
}
