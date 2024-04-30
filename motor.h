#include <sys/_intsup.h>
#ifndef __motor_h__
#define __motor_h__

enum cmd_input {
  CMD_NONE,
  CMD_OPEN,
  CMD_CLOSE,
  CMD_STOP,
  CMD_LOCK,
  CMD_AUXSET,
  CMD_CONNECT,
  CMD_DISCONNECT,
  CMD_DISABLE
};


class Motor {
public:
  virtual void runCommand(int command_input, char* value);

  virtual void motor_off() = 0;

  virtual void motor_on() = 0;

  virtual void stopCommand() = 0;

  virtual void connectCommand() = 0;

  virtual void openCommand() = 0;

  virtual void closeCommand() = 0;

  virtual bool isRoofMoving() = 0;
  virtual bool isStopAllowed() = 0;
  virtual void check_roof_turn_off_relays() = 0;
};

class TA6586 : public Motor {
public:
  unsigned long MotionStartTime = 0;
  unsigned long MotionStopTime = 0;
public:
  bool isRoofMoving();
  bool isStopAllowed();
  void check_roof_turn_off_relays();

  void motor_off();

  void motor_on();

  void stopCommand();

  void connectCommand();

  void openCommand();

  void closeCommand();

};
#endif