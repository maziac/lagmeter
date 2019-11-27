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

//#include "hidjoystickrptparser.h"


#include "src/Measurement/Utilities.h"
#include "src/Measurement/Measure.h"

// The SW version.
#define SW_VERSION "0.20"

// Enable this to get some additional output over serial port (especially for usblag).
#define SERIAL_IF_ENABLED


// Define used Keys.
// Lagmeter:
const int KEY_TEST_PHOTO_BUTTON = LCD_KEY_SELECT;
const int KEY_MEASURE_PHOTO = LCD_KEY_DOWN;
const int KEY_MEASURE_SVGA = LCD_KEY_UP;
const int KEY_MEASURE_SVGA_TO_PHOTO = LCD_KEY_RIGHT;
const int KEY_MEASURE_MIN_TIME = LCD_KEY_LEFT;
// usblag:
const int KEY_USBLAG_MEASURE_1MS = LCD_KEY_DOWN;
const int KEY_USBLAG_MEASURE_8MS = LCD_KEY_UP;
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

// The poll interval requested by the attached USB device.
int requestedPollInterval = 0;
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
  lcd.print(F("Req. poll="));
  lcd.print(requestedPollInterval);
  lcd.print(F("ms"));
}


// The number of samples for usblag
const int USBLAG_CYCLES = 100;

// Output pin to use for joystick button
#define BUTTON_PIN 8


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

        
// Prints the measured time for usblag.
// @param measuredTime The measured time in ms.
int total;
float usblagMin;
float usblagMax;
float usblagAvg;
//bool usbEventToggle = false;
void printUsbLag(float measuredTime) {
    if(total < 0) {
      lcd.setCursor(0,1);
      // Print req. poll interval
      lcd.print(requestedPollInterval);
      lcd.print(F("ms "));
      // Print something in 2nd line. Can be used as a test that game controller reacts.
      //usbEventToggle = !usbEventToggle;
      if(joystickButtonPressed)
        lcd.print(F("####----####"));
      else
        lcd.print(F("----####----"));
      return;
    }
    if(total > USBLAG_CYCLES )
      return;

    // Init if first measurement 
    if(total <= 1) {
      usblagMin = 1000000.0;
      usblagMax = 0.0;
      usblagAvg = 0.0;
    }
    // Print cycle
    lcd.setCursor(0,0);
    lcd.print(total);
    lcd.print(F("/"));
    lcd.print(USBLAG_CYCLES);
    lcd.print(F(": "));
    // Print time
    lcd.print(measuredTime);
    lcd.print(F("ms     "));
    
    // Calculate max/min.
    if(measuredTime > usblagMax)
      usblagMax = measuredTime;
    if(measuredTime < usblagMin)
      usblagMin = measuredTime;

    // Print min/max result
    lcd.setCursor(0,1);
    if(usblagMin != usblagMax) {
      lcd.print(usblagMin);
      lcd.print(F("-"));
    }
    lcd.print(usblagMax);
    lcd.print(F("ms           "));

    // Calculate average
    usblagAvg += measuredTime;

    // Print average if last value
    if(total == USBLAG_CYCLES) {
      // Print average:
      usblagAvg /= USBLAG_CYCLES;
      lcd.setCursor(0,0);
      lcd.print(F("Avg: "));
      lcd.print(usblagAvg);
      lcd.print(F("ms         "));
      // Make sure button is off
      digitalWrite(BUTTON_PIN, false);
      // End measurement
      total++;
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
  //pinMode(BUTTON_PIN, OUTPUT); Already setup by setupMeasurement.
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

  USBHIDInit();

  //nextchange = micros() + 5000*1000;

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


// ON/OFF of the button and showing the result.
// I.e. a quick test to see if the connection is OK.
void usblagTestButton() {
  lcd.clear();
  lcd.print(F("Button:"));
  int outpValue = LOW;
  while(!abortAll) {
    // Stimulate button
    outpValue = ~outpValue;
    digitalWrite(BUTTON_PIN, outpValue);

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
      if(isAbort()) break;
    }

  }
  // Button off
  digitalWrite(BUTTON_PIN, LOW);
}


// Checks for keypresses for usblag mode.
void handleUsblag() {
  static bool prevJoystickButtonPressed = false;
  
  // Check to print the menu
  if(abortAll) {
    digitalWrite(BUTTON_PIN, false);
    abortAll = false;
    total = -1; // Show game controller action
    printUsblagMenu();
  }

  // Prints patterns if joystick button has changed
  printJoystickButtonChanged();

  // Handle user input
  int key = getLcdKey();
  if(key != LCD_KEY_NONE) {
    // Check if we should abort the current measurement
    if(total > 0) {
      abortAll = true;
      return;
    }
  }
  switch(key) {
    case KEY_USBLAG_TEST_BUTTON:
      // ON/OFF of the button and showing the result. I.e. a quick test to see if the connection is OK.
      usblagTestButton();
      //return;
      break;
    case KEY_USBLAG_MEASURE_1MS:
      // Start testing usb device measurement
      button = false;
      total = 0;
      digitalWrite(BUTTON_PIN, button);
      time = 0;
      Serial.print(F("Launching test"));
//      nextchange = micros() + random(50, 70)*1000 + random(0, 250)*4;
      // Use USB polling interval of 1ms.
     // Hid.overrideInterval = 1;
     // xbox.overrideInterval = 1;
      // Inform user
      lcd.clear();
      lcd.print(F("1ms poll..."));

      // Wait for potential lag time
      waitMs(1000); 
      lcd.clear();
      if(isAbort()) return;
      break;
      
    case KEY_USBLAG_MEASURE_8MS:
      // Start testing usb device measurement
      button = false;
      total = 0;
      digitalWrite(BUTTON_PIN, button);
      time = 0;
      Serial.print(F("Launching test"));
//      nextchange = micros() + random(50, 70)*1000 + random(0, 250)*4;
      // Use USB polling interval of 8ms.
     // Hid.overrideInterval = 8;
     // xbox.overrideInterval = 8;
      // Inform user
      lcd.clear();
      lcd.print(F("8ms poll..."));
      // Wait for potential lag time
      waitMs(1000); 
      lcd.clear();
      if(isAbort()) return;
      break;
  }

#if 0
  // Handle time
  unsigned long current_time = micros();
  if (current_time >= nextchange && total >= 0 && total < USBLAG_CYCLES) {
    if (time != 0) {
      Error(F("USB device:"), F("Input dropped!"));
      Serial.println("Input was dropped!");
      return;
    }
    button = !button;
    digitalWrite(BUTTON_PIN, button);
    time = micros();
    total++;
    nextchange = micros() + random(50, 70)*1000 + random(0, 250)*4;
  }
#endif
}


// MAIN LOOP 
void loop() {

  // Check if main mode changed.
  if(usbMode != prevUsbMode) {
    prevUsbMode = usbMode;
    if(usbMode) {
      // Usblag mode
      printUsblagMenu();
      total = -1; // Show game controller action
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
