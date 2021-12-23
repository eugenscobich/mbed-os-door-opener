#include "OledController.h"
#include "mbed.h"

void OledController::println(const char *str) {

  // strncpy(displayLine1, displayLine2, 21);
  // strncpy(displayLine2, displayLine3, 21);
  strncpy(displayLine3, displayLine4, 21);
  strncpy(displayLine4, displayLine5, 21);
  strncpy(displayLine5, displayLine6, 21);
  strncpy(displayLine6, displayLine7, 21);
  strncpy(displayLine7, displayLine8, 21);
  strncpy(displayLine8, str, 21);
  isDisplayOn = true;
  timeout.detach();
  timeout.attach(mbed::callback(this, &OledController::timeoutHandler), 300s);
}

void OledController::printLeftDoorDetails(int count, int current, char *state) {
  sprintf(displayLine1, "%3d | %4d | %8s", count, current, state);
}

void OledController::printRightDoorDetails(int count, int current, char *state) {
  sprintf(displayLine2, "%3d | %4d | %8s", count, current, state);
}

void OledController::timeoutHandler() {
  isDisplayOn = false;
}

void OledController::displayLines() {
  oled.clearDisplay();
  if (isDisplayOn) {
    oled.setTextCursor(0, 0);
    oled.printf("%s", displayLine1);
    oled.setTextCursor(0, 8);
    oled.printf("%s", displayLine2);
    oled.setTextCursor(0, 16);
    oled.printf("%s", displayLine3);
    oled.setTextCursor(0, 24);
    oled.printf("%s", displayLine4);
    oled.setTextCursor(0, 32);
    oled.printf("%s", displayLine5);
    oled.setTextCursor(0, 40);
    oled.printf("%s", displayLine6);
    oled.setTextCursor(0, 48);
    oled.printf("%s", displayLine7);
    oled.setTextCursor(0, 56);
    oled.printf("%s", displayLine8);
  }
  oled.display();
}

Adafruit_SSD1306_I2c *OledController::getOled() { return &oled; }
