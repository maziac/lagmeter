#ifndef __Utilities_H__
#define __Utilities_H__

#include <LiquidCrystal.h>



// Sets the ADC clock (the lowest 3 bits)
#define SET_ADC_CLOCK(bits)   (_SFR_BYTE(ADCSRA) = (_SFR_BYTE(ADCSRA) & 0b11111000) | bits)


// Defines for the available LCD keys.
enum {
	LCD_KEY_NONE,
	LCD_KEY_SELECT,
	LCD_KEY_LEFT,
	LCD_KEY_RIGHT,
	LCD_KEY_UP,
	LCD_KEY_DOWN
};

// Threshold for keys. Below a key is pressed, above no key is pressed.
#define LCD_KEY_PRESS_THRESHOLD   800


extern bool abortAll;
extern LiquidCrystal lcd;

int getLcdKey();
void waitLcdKeyRelease();
bool isAbort();
void waitMs(int waitTime);
void Error(const __FlashStringHelper* area, const __FlashStringHelper* error);
char* secsToString(unsigned long time);
char* longToString(unsigned long value);

#endif
