#ifndef __rolloff_linear_actuator_sensor__
#define __rolloff_linear_actuator_sensor__


/*
 * Get switch value
 * NO contacts used and configured with a pull up resistor to the Arduino input pin. Logical 1 input when switch opened. GG
 * When switch closes The LOW voltage logical 0 is applied to the input pin. GG
 * The off or on value is to be sent to the host in the ACK response
 */
void getSwitch(int id, char* value) {
  if (digitalRead(id) == OPEN_CONTACT)
    strcpy(value, "OFF");
  else
    strcpy(value, "ON");
  DEBUG_VERBOSE("gs:id=%d,v=%s", id, value);  // DEBUG
}

bool isSwitchOn(int id) {
  char switch_value[16 + 1];
  getSwitch(id, switch_value);
  if (strcmp(switch_value, "ON") == 0) {
    return true;
  }
  return false;
}


#endif