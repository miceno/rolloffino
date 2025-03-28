#ifndef __rolloffino_utils_h__
#define __rolloffino_utils_h__

#define MILLIS(x)    (x * 1000L)
void setup_serial();
void serial_loop(Motor* m);

void getSwitch(int id, char* value);
bool isSwitchOn(int id);
// void getStatus(int sw, char* value);

void sendAck(char* val);
void sendNak(const char* errorMsg);
int read_data(char* inpBuf, int offset);
char* safeStrTok(char* input, const char* delimiter, char* output);
bool receiveCommand();
bool is_data_available();
void parseCommand(Motor* m);

void setup_debug();
bool TimePeriodIsOver(unsigned long& startOfPeriod, unsigned long TimePeriod);


#endif