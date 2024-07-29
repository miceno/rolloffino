#include <sys/_intsup.h>
#ifndef __motor_h__
#define __motor_h__

#include "oled_console.h"
#include <cstddef>

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
  OledConsole *oledConsole = NULL;
public:
  virtual void runCommand(int command_input, char *value);
  virtual void printOledConsole(const char *msg);

  /* Pure abstract methods */
  virtual void motorOff() = 0;

  virtual void motorOn() = 0;

  virtual void stopCommand() = 0;

  virtual void connectCommand() = 0;

  virtual void openCommand() = 0;

  virtual void closeCommand() = 0;

  virtual bool isRoofMoving() = 0;
  virtual bool isStopAllowed() = 0;
  virtual void checkRoofMovement() = 0;
  virtual const char *getVersion() = 0;
};

class TA6586 : public Motor {
public:
  unsigned long MotionStartTime = 0;
  unsigned long MotionStopTime = 0;
public:
  bool isRoofMoving();
  bool isStopAllowed();
  void checkRoofMovement();

  void motorOff();

  void motorOn();

  void stopCommand();

  void connectCommand();

  void openCommand();

  void closeCommand();
  const char *getVersion();
};

class DRV8871 : public TA6586 {
  const char *getVersion();
};


#endif