/*
Lag-Meter:
A device to measure the total lag (from USB controller to monitor output).
It also includes sources from
https://gitlab.com/loic.petit/usblag
by Loïc Petit (MIT License)
which itself contains code from USB Host Shield Library 2.0 (GPL2 License).


Licence
The drivers code are mainly built on extracted code from the USB Host Shield Library 2.0, licenced using GPL2.
The rest of the code is under the MIT Licence.


Other timings:
Reed Relais SIL 7271-D 5V:
From own measurements:
- Switch bouncing: 40us
- Delay: 5V Out to relais switching: < 250us


Measurement accuracy:
The code is not optimized, i.e. no assembler code is used for measurement.
But the C-code has an internal check for accuracy.
It aborts if accuracy gets too bad.
Current accuracy is < 1ms.
I also measured the lag directly with a LED connected to the relais. The
measured lag was 1ms in 100 trials.


Note: The measurement should also work with an 8MHz CPU. All the time adjustmeents are done
so that the correct time should be displayed.
However it was tested only with a 16MHz CPU.
*/


#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>


#include "src/Measurement/Utilities.h"
#include "src/Measurement/Common.h"
#include "src/Measurement/Measure.h"

// The SW version.
#define SW_VERSION "1.3"

// Enable this to get some additional output over serial port (especially for usblag).
#define SERIAL_IF_ENABLED


// Define used Keys.
// Lagmeter:
const int KEY_TEST_PHOTO_BUTTON = LCD_KEY_SELECT;
const int KEY_MEASURE_PHOTO = LCD_KEY_DOWN;
const int KEY_MEASURE_SVGA = LCD_KEY_UP;
const int KEY_MEASURE_SVGA_TO_PHOTO = LCD_KEY_LEFT;
const int KEY_MEASURE_MIN_TIME = LCD_KEY_RIGHT;
// usblag:
const int KEY_USBLAG_MEASURE     = LCD_KEY_DOWN;
const int KEY_USBLAG_MEASURE_1MS = LCD_KEY_UP;
const int KEY_USBLAG_TEST_BUTTON = LCD_KEY_SELECT;



// USB--------------------------------------
bool joystickButtonPressed = false;
bool joystickButtonChanged = false;
USB Usb;
USBHub Hub(&Usb);
byte button = 0;
unsigned long time;

// If USB device (game controller) is attached (via USB) then the mode is changed to
// true. This will also change the menu.
bool usbMode = false;
bool prevUsbMode = true;  // previous mode
bool xboxMode = false;

// The poll interval used to override the requested value. 0 = no override.
// USed for both: to show the requested value and to override the requested value.
int usedPollInterval = 0;
// -----------------------------------------



// Prints the main lag-meter menu.
void printLagMeterMenu() {
  lcd.clear();
  lcd.print(F("** Lag-Meter  **"));
  lcd.setCursor(0, 1);
  lcd.print(F("v" SW_VERSION "," __DATE__));
}

// Prints the main usblag menu.
void printUsblagMenu() {
  lcd.clear();
  lcd.print(F("** USB Lag  **"));
  lcd.setCursor(0, 1);
  if(xboxMode) {
    lcd.print(F("xbox"));
  }
  else {
    lcd.print(F("Req. poll="));
    lcd.print(usedPollInterval);
    lcd.print(F("ms"));
  }
}


// Prints a different pattern for button press or release.
void printJoystickButtonChanged() {
  static bool lastButtonPattern = false;
  if(joystickButtonChanged) {
    joystickButtonChanged = false;
    lcd.setCursor(0,1);
    lastButtonPattern = !lastButtonPattern;
    if(lastButtonPattern)
      lcd.print(F("####----####----"));
    else
      lcd.print(F("----####----####"));
  }
}



// SETUP
void setup() {

  // Debug communication
#ifdef SERIAL_IF_ENABLED
  Serial.begin(115200);
  Serial.println(F("Serial connection up!"));
#endif

  // Increase ADC clock to speedup the analogRead function
  SET_ADC_CLOCK(0b101);  // Use 38kHz (strange effects below 0b100)
  int adc = analogRead(0); // throw away first value

  // Random seed
  randomSeed(adc);

  // Lagmeter initialization
  //pinMode(OUT_PIN_BUTTON, OUTPUT); Already setup by setupMeasurement.
  // Setup pins
  setupMeasurement();
  // Setup LCD
  lcd.begin(16, 2);
  lcd.clear();


  // usblag initilization
  if (Usb.Init() == -1) {
    Serial.println("OSC did not start.");
    Error(F("Error:"), F("USB problem!!!"));
  }
  delay(200);

   // Start in Lagmeter mode
   printLagMeterMenu();
   usbMode = false;
   prevUsbMode = false;

#if 0
// Test
  Serial.println(secsToString(0l));
  Serial.println(secsToString(59l));
  Serial.println(secsToString(60l));
  Serial.println(secsToString(60l+45l));
  Serial.println(secsToString(2l*60l+30l));
  Serial.println(secsToString(7l*60l+10l));
  Serial.println(secsToString(10l*60l+25l));

  Serial.println(longToString(0l));
  Serial.println(longToString(1l));
  Serial.println(longToString(999l));
  Serial.println(longToString(1000l));
  Serial.println(longToString(999000l));
  Serial.println(longToString(999999l));
  Serial.println(longToString(1000000l));
  Serial.println(longToString(99000000l));
  Serial.println(longToString(99999999l));
  Serial.println(longToString(100000000l));
#endif
}


// Checks for keypresses for LagMeter mode.
void handleLagMeter() {
  // Check to print the menu
  if(abortAll) {
    printLagMeterMenu();
    abortAll = false;
  }

  // Handle user input
  int key = getLcdKey();
  switch(key) {
    case KEY_TEST_PHOTO_BUTTON:
	    testPhotoSensor();
      break;
    case KEY_MEASURE_PHOTO:
      measurePhotoSensor();
      break;
    case KEY_MEASURE_SVGA:
      measureAD2();
      break;
    case KEY_MEASURE_SVGA_TO_PHOTO:
      measureSvgaToMonitor();
      break;
    case KEY_MEASURE_MIN_TIME:
      measureMinPressTime();
      break;
  }
}


// isUsbAbort: returns true if any key is pressed or
// if usbMode is false and sets 'abortAll' to true.
bool isUsbAbort() {
  if(!usbMode)
    abortAll = true;
  return isAbort();
}


// ON/OFF of the button and showing the result.
// I.e. a quick test to see if the connection is OK.
void usblagTestButton() {
  lcd.clear();
  lcd.print(F("Button:"));
  int outpValue = LOW;
  while(!abortAll) {
    // Stimulate button
    outpValue = ~outpValue;
    digitalWrite(OUT_PIN_BUTTON, outpValue);

    // Print button state
    lcd.setCursor(8, 0);
    if(outpValue)
      lcd.print(F("ON "));
    else
      lcd.print(F("OFF"));

    // Wait for some time
    for(int i=0; i<1500; i++) {
      delay(1); // wait 1ms
      Usb.Task();
      printJoystickButtonChanged();
      if(isUsbAbort()) break;
    }

  }
  // Button off
  digitalWrite(OUT_PIN_BUTTON, LOW);
}


// Measures the usb lag. I.e. the time from button press to received USB reaction.
// Retruns the time in mili seconds.
int measureUsbLag() {
  // Handle USB a few times just in case
  Usb.Task();
  Usb.Task();
  Usb.Task();
  Usb.Task();

  // "Press" button
  digitalWrite(OUT_PIN_BUTTON, HIGH);

  // Wait until button press
  long startTime = micros();
  long diffTime = 0;
  while(!joystickButtonPressed) {
    Usb.Task();
    if(isUsbAbort()) return;
    // Stop measuring
    long stopTime = micros();
    diffTime = stopTime - startTime;
    // Check if too long
    if(diffTime > 1000000l) {
      // More than a second
      Error(F("Error:"), F("No response!"));
      return 0;
    }
  }

  // Round
  diffTime += 500;
  int time = diffTime/1000; // Miliseconds
  return time;
}


// Measures the usb HID lag for 100x.
void usblagMeasure() {
  // Show test title
  lcd.clear();
  lcd.print(F("Test: USB "));
  if(xboxMode) {
    lcd.print(F("xbox"));
  }
  else {
    lcd.print(usedPollInterval);
    lcd.print(F("ms"));
  }
  waitMs(TITLE_TIME); if(isUsbAbort()) return;

  // Initialize
  digitalWrite(OUT_PIN_BUTTON, LOW);

  // Calibrate, i.e. wait a small moment
  lcd.clear();
  lcd.print(F("Calibr. Don't"));
  lcd.setCursor(0, 1);
  lcd.print(F("touch joystick."));

  for(int i=0; i<1000; i++) {
    Usb.Task();
    waitMs(2); if(isUsbAbort()) return;
  }
  setHidIdleValues();

  lcd.clear();
  struct MinMax timeRange = {1023, 0};
  float avg = 0.0;
  for(int i=1; i<=COUNT_CYCLES; i++) {
    // Print
    lcd.setCursor(0,0);
    Usb.Task();
    lcd.print(i);
    Usb.Task();
    lcd.print(F("/"));
    Usb.Task();
    lcd.print(COUNT_CYCLES);
    Usb.Task();
    lcd.print(F(": "));
    Usb.Task();

    // Wait a random time to make sure we really get different results.
    uint8_t waitRnd = random(70, 150);
    for(uint8_t i=0; i<waitRnd; i++) {
      delay(1);
      if(isUsbAbort()) return;
      Usb.Task();
    }

    // Measure lag
    int time = measureUsbLag();
    if(isUsbAbort()) return;

    // Output result:
    lcd.print(time);
    Usb.Task();
    lcd.print(F("ms     "));
    Usb.Task();

    // Calculate max/min.
    if(time > timeRange.max)
      timeRange.max = time;
    if(time < timeRange.min)
      timeRange.min = time;

    // Print min/max result
    lcd.setCursor(5,1);
    if(timeRange.min != timeRange.max) {
      lcd.print(timeRange.min);
      lcd.print(F("-"));
    }
    lcd.print(timeRange.max);
    lcd.print(F("ms     "));

    // Calculate average
    avg += time;

    // "Release" button
    digitalWrite(OUT_PIN_BUTTON, LOW);

    // Wait until no button press
    long startTime = millis();
    while(joystickButtonPressed) {
      //waitMs(1);
      if(isUsbAbort()) return;
      // Check if too long
      long stopTime = millis();
      long diffTime = stopTime - startTime;
      if(diffTime > 1000) {
        // More than a second
        Error(F("Error:"), F("No response."));
        return;
      }
      Usb.Task();
    }

  }

  // Print average:
  avg /= COUNT_CYCLES;
  lcd.setCursor(0,0);
  lcd.print(F("Avg lag: "));
  lcd.print((int)avg);
  lcd.print(F("ms     "));

  // Wait until keypress
  while(!isUsbAbort()) {
    delay(1);
    Usb.Task();
  }
}


// Checks for keypresses for usblag mode.
void handleUsblag() {
  // Prints patterns if joystick button has changed
  printJoystickButtonChanged();

  // Handle user input
  int key = getLcdKey();
  switch(key) {
    case KEY_USBLAG_TEST_BUTTON:
      // ON/OFF of the button and showing the result. I.e. a quick test to see if the connection is OK.
      usblagTestButton();
      printUsblagMenu();
      joystickButtonChanged = false;
      abortAll = false;
      break;

    case KEY_USBLAG_MEASURE:
      // Start testing usb device measurement
      usblagMeasure();
      printUsblagMenu();
      joystickButtonChanged = false;
      abortAll = false;
      break;

    case KEY_USBLAG_MEASURE_1MS:
      if(xboxMode) {
        // Not available in xbox mode
          lcd.clear();
          lcd.print(F("Not available."));
          while(!isUsbAbort())
            delay(100);
          printUsblagMenu();
          joystickButtonChanged = false;
          abortAll = false;
      }
      else {
        // Start testing usb device measurement
        setPollInterval(1); // Use 1ms poll interval
        usblagMeasure();
        printUsblagMenu();
        joystickButtonChanged = false;
        abortAll = false;
      }
      break;
  }
}


// MAIN LOOP
void loop() {

  // Check if main mode changed.
  if(usbMode != prevUsbMode) {
    prevUsbMode = usbMode;
    if(usbMode) {
      // Usblag mode
      printUsblagMenu();
    }
    else {
      // Reset
      asm( "   jmp 0");
    }
  }

#if 01
  // Handle mode
  if(usbMode) {
    // Usblag mode
    handleUsblag();
   }
  else {
    // Lagmeter mode
    handleLagMeter();
  }
#endif

  // Handle USB
  Usb.Task();
}
