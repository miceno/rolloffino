#ifndef _my_oled_console_h_
#define _my_oled_console_h_

/*
 * Power mode of the console
 * PWC_DISABLE: it will be on forever
 * PWC_ENABLE: it will switch off after some timeout
*/
enum power_mode_console {
  PWC_DISABLE,
  PWC_ENABLE
};

class OledConsole {
  public:
    power_mode_console power_mode;
    int timeout = 0;
    int lastTimeWrite = 0;
  public:
    OledConsole(unsigned long timeout, power_mode_console mode);
    void print(const char *);
    void apply_power_mode();
};

void oled_loop(OledConsole *oconsole);
void oled_setup();

#endif