#ifndef _OLED_CONTROLLER_H_
#define _OLED_CONTROLLER_H_

#include "Adafruit_SSD1306.h"
#include "mbed.h"


class OledController {
public:
  OledController(PinName sda, PinName scl)
      : i2c(sda, scl), oled(i2c, D4, SSD_I2C_ADDRESS, 64, 128){};

  void println(const char* str);

  void printLeftDoorDetails(int count, int current, char* state);
  void printRightDoorDetails(int count, int current, char* state);
  
  void displayLines();

  Adafruit_SSD1306_I2c* getOled();

protected:
  I2C i2c;
  Adafruit_SSD1306_I2c oled;

  char displayLine1[22];
  char displayLine2[22];
  char displayLine3[22];
  char displayLine4[22];
  char displayLine5[22];
  char displayLine6[22];
  char displayLine7[22];
  char displayLine8[22];

  Timeout timeout;

  void timeoutHandler();
  bool isDisplayOn;
};

#endif