#include "ssd1306.h"
#include "ssd1306_console.h"
#include "oled_console.h"

Ssd1306Console  console;


void oled_init(){
int scl = D6;
int sca = D5;
int address = 0;
  /* Replace the line below with ssd1306_128x32_i2c_init() if you need to use 128x32 display */
  ssd1306_128x64_i2c_initEx(scl, sca, address);
}

void oled_setup()
{
  /* Replace the line below with the display initialization function, you want to use */
  oled_init();
  console.begin();
  ssd1306_clearScreen();
  /* Set font to use with console */
  ssd1306_setFixedFont(ssd1306xled_font6x8);

}

OledConsole::OledConsole(unsigned long timeout, power_mode_console mode){
  this->timeout = (unsigned long) (timeout * 1e3);
  this->power_mode = mode;
}


void OledConsole::log(const char *message){
  ssd1306_displayOn();
  console.println(message);
  console.print("\n");
  lastTimeWrite = millis();
}

void OledConsole::apply_power_mode(){
  if ((millis()-lastTimeWrite) > timeout) {
    ssd1306_displayOff();
    lastTimeWrite = 0;
  }
}


void oled_loop(OledConsole *oconsole){
  oconsole->apply_power_mode();
}
