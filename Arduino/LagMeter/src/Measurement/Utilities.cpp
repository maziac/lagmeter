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
	if (x >= LCD_KEY_PRESS_THRESHOLD)
		return LCD_KEY_NONE;

	waitLcdKeyRelease();

	// Return
	if (x < 60)
		return LCD_KEY_RIGHT;
	if (x < 200)
		return LCD_KEY_UP;
	if (x < 400)
		return LCD_KEY_DOWN;
	if (x < 650)
		return LCD_KEY_LEFT;
	return LCD_KEY_SELECT;
}


// Waits on release of key press.
void waitLcdKeyRelease() {
	// Some key has been pressed
	delay(100);	// poor man's debounce
	// Wait until released
	while (analogRead(0) < LCD_KEY_PRESS_THRESHOLD);
	delay(100); // debounce
}


// isAbort: returns true if any key is pressed
// and sets 'abortAll' to true.
bool isAbort() {
	if (!abortAll)
		if (getLcdKey() != LCD_KEY_NONE)
			abortAll = true;
	return abortAll;
}


// Waits for a certain time or abort (keypress).
void waitMs(int waitTime) {
	int time;
	int startTime = millis();
	do {
		time = millis() - startTime;
		if (isAbort())
			return;
	} while (time < waitTime);
}


// Called if an error occurs.
// Prints error and waits on a key press.
// Then aborts.
// Prints 'area' in 1rst line and 'error' in 2nd line.
// If one argument is nullptr it is not printed.
void Error(const __FlashStringHelper* area, const __FlashStringHelper* error) {
	//lcd.clear();
	// 1rst line
	if (area) {
		lcd.setCursor(0, 0);
		lcd.print(F("                "));  // Clear line
		lcd.setCursor(0, 0);
		lcd.print(area);
	}
	// 2nd line
	if (error) {
		lcd.setCursor(0, 1);
		lcd.print(F("                "));  // Clear line
		lcd.setCursor(0, 1);
		lcd.print(error);
	}
	while (getLcdKey() == LCD_KEY_NONE);
	abortAll = true;
}


// Converts a unsigned long into a time string.
// Time string 'abbreviates' a lot.
// The returned string is statically allocated, i.e. it is overwritten
// when the function is used the 2nd time.
// The string has a max size of 4 without the terminating 0.
// @param time The time to convert in secs.
// @return A string, e.g. "54s" or "14m" or "23h" or "6d"
char* secsToString(unsigned long time) {
	static char ts[4 + 1];
	float fTime = time;
	if (fTime < 60.0) {
		// < 60s
		snprintf(ts, sizeof(ts), "%ds", (int)fTime);
	}
	else if (fTime < 60.0 * 60.0) {
		// < 60min = 1h
		fTime /= 60.0;
		snprintf(ts, sizeof(ts), "%dm", (int)fTime);
	}
	else if (fTime < 24.0 * 60.0 * 60.0) {
		// < 1d
		fTime /= 60.0 * 60.0;
		snprintf(ts, sizeof(ts), "%dh", (int)fTime);
	}
	else if (fTime < 100.0 * 24.0 * 60.0 * 60.0) {
		// <= 99d
		fTime /= 24.0 * 60.0 * 60.0;
		snprintf(ts, sizeof(ts), "%dd", (int)fTime);
	}
	else {
		// > 99d
		strcpy(ts, ">99d");
	}
	return ts;
}


// Converts a unsigned long into a string.
// The string is 'abbreviated'.
// The returned string is statically allocated, i.e. it is overwritten
// when the function is used the 2nd time.
// The string has a max size of 4 without the terminating 0.
// @param value The time to convert in secs.
// @return A string, e.g. "12" or "999" or "1k" (instead of 1000) or "332k"
// or "45M" (instead of (45000000) or ">99M".
char* longToString(unsigned long value) {
	static char vs[4 + 1];
	if (value < 1000l) {
		// < 1000
		snprintf(vs, sizeof(vs), "%d", value);
	}
	else if (value < 1000000l) {
		// < 1M
		value /= 1000l;
		snprintf(vs, sizeof(vs), "%dk", value);
	}
	else if (value < 100000000l) {
		// < 100M
		value /= 1000000l;
		snprintf(vs, sizeof(vs), "%dM", value);
	}
	else {
		// > 99M
		strcpy(vs, ">99M");
	}
	return vs;
}