#ifndef __Utilities_H__
#define __Utilities_H__

#include <LiquidCrystal.h>


// Defines for the available LCD keys.
enum {
  LCD_KEY_NONE,
  LCD_KEY_SELECT,
  LCD_KEY_LEFT,
  LCD_KEY_RIGHT,
  LCD_KEY_UP,
  LCD_KEY_DOWN
};


extern bool abortAll;
extern LiquidCrystal lcd;

int getLcdKey();
void waitLcdKeyRelease();
bool isAbort();
void waitMs(int waitTime);
void Error(const __FlashStringHelper* area, const __FlashStringHelper* error);

#endif
