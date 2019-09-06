#include "Utilities.h"
#include "Arduino.h"

// Is set if a function is (forcefully) left.
// E.g. because of a keypress (aboort.
// The menu starts at the beginning afterwards.
bool abortAll = true;


// LCD pin configuration.
LiquidCrystal lcd(19, 17, 18, 4, 5, 6, 7); 



// Returns the pressed key (or LCD_KEY_NONE).
int getLcdKey() {
  int x = analogRead(0);
  // Return immediately if nothing is pressed
  if (x >= 800) 
    return LCD_KEY_NONE;
  
  waitLcdKeyRelease();

  // Return
  if (x < 60)
    return LCD_KEY_RIGHT;
  if (x < 200) 
    return LCD_KEY_UP;
  if (x < 400) 
    return LCD_KEY_DOWN;
  if (x < 600) 
    return LCD_KEY_LEFT;
  return LCD_KEY_SELECT;
}


// Waits on release of key press.
void waitLcdKeyRelease() {
  // Some key has been pressed
  delay(100);	// poor man's debounce
  // Wait until released
  while(analogRead(0) < 800);
  delay(100); // debounce
}


// isAbort: returns true if any key is pressed
// and sets 'abortAll' to true.
bool isAbort() {
  if(!abortAll)
    if(getLcdKey() != LCD_KEY_NONE) 
        abortAll = true;
  return abortAll;
}


// Waits for a certain time or abort (keypress).
void waitMs(int waitTime) {
  int time;
  int startTime = millis();
  do {
  	time = millis() - startTime;
    if(isAbort())
      return;
  } while(time < waitTime);
}


// Called if an error occurs.
// Prints error and waits on a key press.
// Then aborts.
void Error(const __FlashStringHelper* area, const __FlashStringHelper* error) {
  lcd.clear();
  lcd.print(area);
  lcd.setCursor(0,1);
  lcd.print(error);
  while(getLcdKey() == LCD_KEY_NONE);
  abortAll = true;
}
