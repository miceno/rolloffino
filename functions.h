#ifndef __functions_h
#define __functions_h

void setup_serial();
void serial_loop();
void checkConnection();
bool isStopAllowed();
void motor_off();
void motor_on();

void stopCommand();
void openCommand();
void closeCommand();
void connectCommand();

void getSwitch(int id, char* value);
bool isSwitchOn(int id);
void getStatus(int sw, char* value);

void sendAck(char* val);
void sendNak(const char* errorMsg);
int read_data(char* inpBuf, int offset);
char* safeStrTok(char* input, const char* delimiter, char* output);
bool parseCommand();
bool is_data_available();
void receiveCommand();

void setup_debug();

#endif