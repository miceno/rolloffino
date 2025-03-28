void setup_serial() {
  // Serial.setDebugOutput(true);
  Serial.begin(BAUD_RATE);  // Baud rate to match that in the driver
  while (!Serial) {
    delay(10);
  }
  delay(10);
  Serial.println();
}

void serial_loop(Motor *m) {
  while (Serial.available() <= 0) {
    for (int cnt = 0; cnt < 60; cnt++) {
      if (Serial.available() > 0)
        break;
      else
        delay(100);
    }
  }
  parseCommand(m);  // Some input detected
}
