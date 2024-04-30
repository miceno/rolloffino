#ifndef __rolloff_linear_actuator_comm__
#define __rolloff_linear_actuator_comm__ 1

void setup_serial() {
  Serial.begin(BAUD_RATE);  // Baud rate to match that in the driver
  while (!Serial)
    ;
}

void serial_loop() {
  while (Serial.available() <= 0) {
    for (int cnt = 0; cnt < 60; cnt++) {
      if (Serial.available() > 0)
        break;
      else
        delay(100);
    }
  }
  receiveCommand();  // Some input detected
}



#endif
