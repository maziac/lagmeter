#include "Utilities.h"
#include "Measure.h"
#include <Arduino.h>

///////////////////////////////////////////////////////////////////
// Pin configuration:

// Simulation of the joystick button.
const int OUT_PIN_BUTTON = 8;

// The analog input for the photo sensor.
const int IN_PIN_PHOTO_SENSOR = 2;

// The analog input for the SVGA connector (blue, or red or green)
const int IN_PIN_SVGA = 1;

// Count of cycles to measure the input lag.
const int COUNT_CYCLES = 100;

// Define this for comparison measurements with an oscilloscope or a camera.
//#define OUT_PIN_BUTTON_COMPARE_TIME 3

// Keys for the measurement during 'measureMinPressTime'
const int KEY_MEASURE_MIN_UP1 = LCD_KEY_UP;
const int KEY_MEASURE_MIN_DOWN1 = LCD_KEY_DOWN;
const int KEY_MEASURE_MIN_UP10 = LCD_KEY_RIGHT;
const int KEY_MEASURE_MIN_DOWN10 = LCD_KEY_LEFT;

///////////////////////////////////////////////////////////////////


// The minimum diff required between min/max ov the SVGA signal.
#define SVGA_MIN_DIFF  20

// The checked accuracy in ms:
#define CHECK_ACCURACY  1
#define CHECK_ACCURACY_ERROR_STR "Err:Accuracy>1ms"


// Initializes the pins.
void setupMeasurement() {
  // Setup GPIOs
  pinMode(OUT_PIN_BUTTON, OUTPUT);
  digitalWrite(OUT_PIN_BUTTON, LOW); 
  pinMode(IN_PIN_PHOTO_SENSOR, INPUT);
  pinMode(IN_PIN_SVGA, INPUT);

#ifdef OUT_PIN_BUTTON_COMPARE_TIME
  pinMode(OUT_PIN_BUTTON_COMPARE_TIME, OUTPUT);
  digitalWrite(OUT_PIN_BUTTON_COMPARE_TIME, LOW); 
#endif
}


// State: test photo sensor.
// The photo sensor input is read and printed to the LCD.
// Press "KEY_TEST" to leave.
void testPhotoSensor() {
  lcd.clear();
  lcd.print(F("Button:"));
  lcd.setCursor(0,1);
  lcd.print(F("Sensor:"));
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
      
    // Wait for potential lag time
    waitMs(150); if(isAbort()) break;
    
    // Evaluate for some time
    for(int i=0; i<3; i++) {
      // Read photo sensor 
      int value = analogRead(IN_PIN_PHOTO_SENSOR);
      lcd.setCursor(8, 1);
      lcd.print(value);
      lcd.print(F("   "));
      waitMs(500); if(isAbort()) break;
    }
  }
}


// Returns the min/max photo sensor values for a given time frame.
struct MinMax getMaxMinAnalogIn(int inputPin, int measTime) {
  struct MinMax value;
  value.min = value.max = analogRead(inputPin);
  int time;
  int startTime = millis();
  do {
	  // Read photo sensor
    int currValue = analogRead(inputPin);
    if(currValue > value.max)
      value.max = currValue;
    else if(currValue < value.min)
      value.min = currValue;
 
    // Get elapsed time
  	time = millis() - startTime;
    if(isAbort())
      break;
  } while(time < measTime);
  
  // Add small safety margin
  value.min--;
  value.max++;
  return value;
}


// Waits until the input pin value stays in range for a given time.
// @param inputPin Pin from which the analog input is read. Photo sensor or SVGA.
// @param threshold The value to compare the inputPin value to.
// @param positiveThreshold If true check that inputPin value is bigger, if false check that inputPin value is smaller.
// @param waitTime The time to wait for.
// @return The pressed key or LCD_KEY_NONE.
int waitMsInput(int inputPin, int threshold, bool positiveThreshold, int waitTime) {
  // Simulate joystick button
  digitalWrite(OUT_PIN_BUTTON, LOW); 
  // Wait
  int time;
  int startTime = millis();
  int watchdogTime = startTime;
  do {
    // Get input value 
    int value = analogRead(inputPin);
    // Check for threshold (not fulfilled)
    if(!((positiveThreshold && value > threshold) // Check if value is bigger
       || (!positiveThreshold && value < threshold))) // Check if value is smaller
       {
      // Out of range, restart timer
      startTime = millis(); 
    }
    // Get time
    int currTime = millis();
  	time = currTime - startTime;

    // Check for key press 
    int key = getLcdKey();
    if(key != LCD_KEY_NONE) {
        abortAll = true;
        return key;
    }

    // Check if waited too long (4 secs)
    if(currTime-watchdogTime > 4000) {
      // Error
      Error(nullptr, F("Err:Signal wrong"));
      break;
    }
  } while(time < waitTime);

  return LCD_KEY_NONE;
}

    
// Waits until the photo sensor (or SVGA value) value gets into range.
// @param inputPin Pin from which the analog input is read. Photo sensor or SVGA.
// @param threshold The value to compare the inputPin value to.
// @param positiveThreshold If true check that inputPin value is bigger, if false check that inputPin value is smaller.
// @param inputPinWait (Optional) If given: wait for inputPinWait value to get in 'rangeWait' before starting the measurement.
// @param thresholdWait (Optional) The value to compare the inputPinWait value to. (Always positive threshold)
int measureLag(int inputPin, int threshold, bool positiveThreshold, int inputPinWait = -1, int thresholdWait = 0) {
  int key = 1023;
  unsigned int tcount1 = 0;
  bool accuracyOvrflw = false;
  bool counterOvrflw = false;
  bool keyPressed = false;

  // Turnoff interrupts.
  noInterrupts();

  const int adjust = 16000000/F_CPU;  // 1 for 16MHz, 2 for 8MHz.
  //const int tcnt2Value = 256-(0.5/0.004/adjust); //  131;  // (256-131)*4us = 0.5ms, would work up to (256-220)*4us = 0.144ms
  const int tcnt2Value = 256-(CHECK_ACCURACY/0.064/adjust); // 2ms

  // Setup timer 1 (16 bit timer) to measure the time:
  // Prescaler: 1024 -> resolution 64us (at F_CPU=16MHz).
  // No timer compare register.
  TCCR1A = 0; // No PWM
  TCCR1B = (1 << CS10) | (1 << CS12);  // Prescaler = 1024
  TIMSK1 = 1<<TOIE1;  // For polling only

  // Setup timer 2 (8 bit timer) to check measurement accuracy:
  // Prescaler: 1024 -> resolution 64us (at F_CPU=16MHz).
  // No timer compare register.
  TCCR2A = 0; // No PWM
  TCCR2B = (1 << CS10) | (1 << CS12);  // Prescaler = 1024
  TIMSK2 = 1<<TOIE2;  // For polling only

  // Reset timer
  TCNT2 = tcnt2Value;
  TCNT1 = 0;
  TIFR1 = 1<<TOV1;  // Clear pending bits
  TIFR2 = 1<<TOV2;  // Clear pending bits

  // Simulate joystick button
  digitalWrite(OUT_PIN_BUTTON, HIGH); 

  // Check if we wait for a 2nd trigger (required to measure the delay between SVGA out and monitor)
  if(inputPinWait >= 0) {
    // Wait on trigger
    int counter = 1;
    while(true) {
      // Check range of wait-input-pin 
      int value = analogRead(inputPinWait);
      // Check for thresholdWait
      if(value > thresholdWait) {
        // Restart measurement
        TCNT2 = tcnt2Value;
        TCNT1 = 0;
        break;  
      }

      /*
      // Assure that measurement accuracy is good enough
      TCNT2 = tcnt2Value;
      if(TIFR2 & (1<<TOV2)) {
        // Error
        accuracyOvrflw = true;
        goto L_ERROR;
      }
      */

      // Do this not in every loop
      counter--;
      if(counter == 0) {
        counter = 1000;
        // Check if key pressed
        key = analogRead(0);
        // Return immediately if something is pressed
        if (key < LCD_KEY_PRESS_THRESHOLD) {
          keyPressed = true;
          goto L_ERROR;
        } 

        // Check for time out
        if(TIFR1 & (1<<TOV1)) {
          // Interrupt pending bit set -> Overflow happened.
          // This means 4.19 seconds elapsed with no signal.
          counterOvrflw = true;
          goto L_ERROR;
        }
      }
    }
  }

#ifdef OUT_PIN_BUTTON_COMPARE_TIME
  if(outpValue)
    digitalWrite(OUT_PIN_BUTTON_COMPARE_TIME, HIGH); 
#endif 

  TIFR2 = 1<<TOV2;  // Clear pending bits
  while(true) {
    // Check range of input pin
    int value = analogRead(inputPin);
    // Check for threshold
    if((positiveThreshold && value > threshold) // Check if value is bigger
       || (!positiveThreshold && value < threshold)) // Check if value is smaller
    {
      tcount1 = TCNT1;
      break;  
    }

    // Assure that measurement accuracy is good enough
    TCNT2 = tcnt2Value;
    if(TIFR2 & (1<<TOV2)) {
      // Error
      accuracyOvrflw = true;
      break;
    }

    // Check if key pressed
    key = analogRead(0);
    // Return immediately if something is pressed
    if (key < LCD_KEY_PRESS_THRESHOLD) {
      keyPressed = true;
      break;
    }

    // Check for time out
    if(TIFR1 & (1<<TOV1)) {
      // Interrupt pending bit set -> Overflow happened.
      // This means 4.19 seconds elapsed with no signal.
      counterOvrflw = true;
      break;
    }
  }

L_ERROR:
#ifdef OUT_PIN_BUTTON_COMPARE_TIME
  if(outpValue)
    digitalWrite(OUT_PIN_BUTTON_COMPARE_TIME, LOW); 
#endif

  // Disable timer interrupts
  TIMSK1 = 0;
  TIMSK2 = 0;

  // Enable interrupts
  interrupts();

  // Key pressed ? -> Abort
  if(keyPressed) {
    waitLcdKeyRelease();
    abortAll = true;
  }
  else if(accuracyOvrflw) {    // Assure that measurement accuracy is good enough
    // Error
    Error(F("Error:"), F(CHECK_ACCURACY_ERROR_STR));
  }
  else if(counterOvrflw) {    // Overflow?
    // Interrupt pending bit set -> Overflow happened.
    // This means 4.19 seconds elapsed with no signal.
    Error(F("Error:"), F("No signal"));
  }

  // Calculate time from counter value.
  long tcount1l = tcount1;
  tcount1l *= 64*adjust;
  tcount1l = (tcount1l+500l) / 1000l; // with rounding

  return (int)tcount1l;
}


// Waits until the SVGA value gets into range, then measures the time until the photo
// sensor gets into range.
// Used to measure the delay of the monitor.
// @param range The range for the photo sensor.
// @param rangeWait The range for the SVGA value.
// @return the time.
int measureLagDiff(int threshold, int thresholdWait) {
  return measureLag(IN_PIN_PHOTO_SENSOR, threshold, false, IN_PIN_SVGA, thresholdWait);
}


// Calibrates the photo sensor and afterwards measure the
// input lag for a few cycles.
// Calibration: 
//   Simulate joystick button press -> measure min/max photo
//   sensor value.
//   Simulate joystick button unpress -> measure min/max photo
//   sensor value.
// Measurement:
//   Simulate joystick button press -> measure time until photo
//   sensor value changes.
void measurePhotoSensor() {
  // Calibrate
  lcd.clear();
  lcd.print(F("Calib. Photo S."));
  // Simulate joystick button press
  digitalWrite(OUT_PIN_BUTTON, HIGH); 
  waitMs(500); if(isAbort()) return;
  // Get max/min light value
  struct MinMax buttonOnLight = getMaxMinAnalogIn(IN_PIN_PHOTO_SENSOR, 1500);
  if(isAbort()) return;
  // Print
  lcd.setCursor(0,1);
  lcd.print(buttonOnLight.min);
  lcd.print(F("-"));
  lcd.print(buttonOnLight.max);
  // Simulate joystick button unpress
  digitalWrite(OUT_PIN_BUTTON, LOW); 
  waitMs(500); if(isAbort()) return;
  // Get max/min light value
  struct MinMax buttonOffLight = getMaxMinAnalogIn(IN_PIN_PHOTO_SENSOR, 1500);
  if(isAbort()) return;
  // Print
  lcd.setCursor(0,1);
  lcd.print(buttonOffLight.min);
  lcd.print(F("-"));
  lcd.print(buttonOffLight.max);
  lcd.print(F("         "));
  waitMs(1000); if(isAbort()) return;

  // Check values. They should not overlap.
  bool overlap = (buttonOnLight.max >= buttonOffLight.min && buttonOnLight.min <= buttonOffLight.max);
  if(overlap) {
    // Error
    Error(F("Calibr. Error:"), F("Ranges overlap"));
    return;
  }
  
  // Calculate threshold in the middle (buttonOffLight value is bigger than buttonOnLight value)
  int threshold = (buttonOnLight.max + buttonOffLight.min)/2;

  // Print
  lcd.clear();
  lcd.print(F("Start testing..."));
  waitMs(1000); if(isAbort()) return;
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(F("Lag: "));

  // Measure a few cycles
  struct MinMax timeRange = {1023, 0};
  float avg = 0.0;
  for(int i=1; i<=COUNT_CYCLES; i++) {
    // Print
    lcd.setCursor(0,0);
    lcd.print(i);
    lcd.print(F("/"));
    lcd.print(COUNT_CYCLES);
    lcd.print(F(": "));
    
    // Wait until input (photo sensor) changes
    int time = measureLag(IN_PIN_PHOTO_SENSOR, threshold, false);
    if(isAbort()) return;
    // Output result: 
    lcd.print(time);
    lcd.print(F("ms     "));
    
    // Wait a random time to make sure we really get different results.
    int waitRnd = random(70, 150);
    // Wait until input (photo sensor) changes
    waitMsInput(IN_PIN_PHOTO_SENSOR, threshold, true, waitRnd);
    if(isAbort()) return;

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
  }

  // Print average:
  avg /= COUNT_CYCLES;
  lcd.setCursor(0,0);
  lcd.print(F("Avg Phot: "));
  lcd.print((int)avg);
  lcd.print(F("ms     "));
 
  // Wait on key press.
  while(getLcdKey() != LCD_KEY_NONE);
}



// Calibrates on the SVGA output and measure the
// input lag for a few cycles.
// Uses the blue-output of SVGA (but you could also use red or green).
// Calibration: 
//   Simulate joystick button press -> measure svga brightness, i.e. max signal.
//   Simulate joystick button unpress -> measure svga darkness, i.e. max signal.
// Measurement:
//   Simulate joystick button press -> measure time until svga value changes
void measureSVGA() {
  // Calibrate
  lcd.clear();
  lcd.print(F("Calibrate SVGA"));
  // Simulate joystick button press
  digitalWrite(OUT_PIN_BUTTON, HIGH); 
  waitMs(500); if(isAbort()) return;
  // Get max svga value
  struct MinMax buttonOnSVGA = getMaxMinAnalogIn(IN_PIN_SVGA, 1500);
  if(isAbort()) return;
  // Print
  lcd.setCursor(0,1);
  lcd.print(buttonOnSVGA.max);
  // Simulate joystick button unpress
  digitalWrite(OUT_PIN_BUTTON, LOW); 
  waitMs(500); if(isAbort()) return;
  // Get max/min light value
  struct MinMax buttonOffSVGA = getMaxMinAnalogIn(IN_PIN_SVGA, 1500);
  if(isAbort()) return;
  // Print
  lcd.setCursor(0,1);
  lcd.print(buttonOffSVGA.max);
  lcd.print(F("         "));
  waitMs(1000); if(isAbort()) return;

  // Print diff
  lcd.setCursor(0,1);
  lcd.print(F("Diff="));
  lcd.print(buttonOnSVGA.max-buttonOffSVGA.max);
  lcd.print(F("         "));
  waitMs(1000); if(isAbort()) return;

  // Check values. They should differ clearly. (Should be around 100.)
  if(buttonOnSVGA.max-buttonOffSVGA.max < SVGA_MIN_DIFF) {
    // Error
    Error(F("Calibr. Error:"), F("Signal too weak"));
    return;
  }
  
  // Calculate threshold in the middle 
  int threshold = (buttonOnSVGA.max+buttonOffSVGA.max)/2;
  
  // Print
  lcd.clear();
  lcd.print(F("Start testing..."));
  waitMs(1000); if(isAbort()) return;
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(F("Lag: "));

  // Measure a few cycles
  struct MinMax timeRange = {1023, 0};
  float avg = 0.0;
  for(int i=1; i<=COUNT_CYCLES; i++) {
    // Print
    lcd.setCursor(0,0);
    lcd.print(i);
    lcd.print(F("/"));
    lcd.print(COUNT_CYCLES);
    lcd.print(F(": "));
    
    // Wait until input (svga) changes
    int time = measureLag(IN_PIN_SVGA, threshold, true);
    if(isAbort()) return;
    // Output result: 
    lcd.print(time);
    lcd.print(F("ms     "));
        
    // Wait a random time to make sure we really get different results.
    int waitRnd = random(70, 150);
    // Wait until input (svga) changes
    waitMsInput(IN_PIN_SVGA, threshold, false, waitRnd);
    if(isAbort()) return;

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
  }

  // Print average:
  avg /= COUNT_CYCLES;
  lcd.setCursor(0,0);
  lcd.print(F("Avg SVGA: "));
  lcd.print((int)avg);
  lcd.print(F("ms     "));
  
  // Wait on key press.
  while(getLcdKey() != LCD_KEY_NONE);
}



// Calibrates photo sensor and SVGA output and measures the
// latency between SVGA out and photo sensor signal on monitor.
// Uses the blue-output of SVGA (but you could also use red or green).
// Calibration: 
//   Simulate joystick button press -> measure svga brightness, i.e. max signal.
//   Simulate joystick button unpress -> measure svga darkness, i.e. max signal.
//   Simulate joystick button press -> measure min/max photo
//   sensor value.
//   Simulate joystick button unpress -> measure min/max photo
//   sensor value.
// Measurement:
//   Simulate joystick button press -> measure time when svga value changes
//   to time when photo sensor changes.
void measureSvgaToMonitor() {
  // Calibrate
  lcd.clear();
  lcd.print(F("Calibrate SVGA"));
  // Simulate joystick button press
  digitalWrite(OUT_PIN_BUTTON, HIGH); 
  waitMs(500); if(isAbort()) return;
  // Get max svga value
  struct MinMax buttonOnSVGA = getMaxMinAnalogIn(IN_PIN_SVGA, 1500);
  if(isAbort()) return;
  // Print
  lcd.setCursor(0,1);
  lcd.print(buttonOnSVGA.max);
  // Simulate joystick button unpress
  digitalWrite(OUT_PIN_BUTTON, LOW); 
  waitMs(500); if(isAbort()) return;
  // Get max/min light value
  struct MinMax buttonOffSVGA = getMaxMinAnalogIn(IN_PIN_SVGA, 1500);
  if(isAbort()) return;
  // Print
  lcd.setCursor(0,1);
  lcd.print(buttonOffSVGA.max);
  lcd.print(F("         "));
  waitMs(1000); if(isAbort()) return;

  // Print diff
  lcd.setCursor(0,1);
  lcd.print(F("Diff="));
  lcd.print(buttonOnSVGA.max-buttonOffSVGA.max);
  lcd.print(F("         "));
  waitMs(1000); if(isAbort()) return;

 // Check values. They should differ clearly. (Should be around 100.)
  if(buttonOnSVGA.max-buttonOffSVGA.max < SVGA_MIN_DIFF) {
    // Error
    Error(F("Calibr. Error:"), F("Signal too weak"));
    return;
  }

  // Calibrate
  lcd.clear();
  lcd.print(F("Calib. Photo S."));
  // Simulate joystick button press
  digitalWrite(OUT_PIN_BUTTON, HIGH); 
  waitMs(500); if(isAbort()) return;
  // Get max/min light value
  struct MinMax buttonOnLight = getMaxMinAnalogIn(IN_PIN_PHOTO_SENSOR, 1500);
  if(isAbort()) return;
  // Print
  lcd.setCursor(0,1);
  lcd.print(buttonOnLight.min);
  lcd.print(F("-"));
  lcd.print(buttonOnLight.max);
  // Simulate joystick button unpress
  digitalWrite(OUT_PIN_BUTTON, LOW); 
  waitMs(500); if(isAbort()) return;
  // Get max/min light value
  struct MinMax buttonOffLight = getMaxMinAnalogIn(IN_PIN_PHOTO_SENSOR, 1500);
  if(isAbort()) return;
  // Print
  lcd.setCursor(0,1);
  lcd.print(buttonOffLight.min);
  lcd.print(F("-"));
  lcd.print(buttonOffLight.max);
  lcd.print(F("         "));
  waitMs(1000); if(isAbort()) return;

  // Check values. They should not overlap.
  bool overlap = (buttonOnLight.max >= buttonOffLight.min && buttonOnLight.min <= buttonOffLight.max);
  if(overlap) {
    // Error
    Error(F("Calibr. Error:"), F("Ranges overlap"));
    return;
  }
  
  // Calculate threshold in the middle (buttonOffLight value is bigger than buttonOnLight value)
  int threshold = (buttonOnLight.max + buttonOffLight.min)/2;

  // Calculate threshold in the middle 
  int thresholdWait = (buttonOnSVGA.max+buttonOffSVGA.max)/2;
  
  // Print
  lcd.clear();
  lcd.print(F("Start testing..."));
  waitMs(1000); if(isAbort()) return;
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(F("Lag: "));

  // Measure a few cycles
  struct MinMax timeRange = {1023, 0};
  float avg = 0.0;
  for(int i=1; i<=COUNT_CYCLES; i++) {
    // Print
    lcd.setCursor(0,0);
    lcd.print(i);
    lcd.print(F("/"));
    lcd.print(COUNT_CYCLES);
    lcd.print(F(": "));
    
    // Wait until input (photo sensor) changes
    int time = measureLagDiff(threshold, thresholdWait);
    if(isAbort()) return;
    // Output result: 
    lcd.print(time);
    lcd.print(F("ms     "));
    
     // Wait a random time to make sure we really get different results.
    int waitRnd = random(70, 150);
    // Wait until input (photo sensor) changes
    waitMsInput(IN_PIN_PHOTO_SENSOR, threshold, true, waitRnd);
    if(isAbort()) return;

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
  }

  // Print average:
  avg /= COUNT_CYCLES;
  lcd.setCursor(0,0);
  lcd.print(F("Avg Mon: "));
  lcd.print((int)avg);
  lcd.print(F("ms     "));
 
  // Wait on key press.
  while(getLcdKey() != LCD_KEY_NONE);
}





// Waits until the photo sensor (or SVGA value) value gets into range.
// @param inputPin Pin from which the analog input is read. Photo sensor or SVGA.
// @param pressTime The time he button press is simulated.
// @param threshold The value to compare the inputPin value to.
// @param positiveThreshold If true check that inputPin value is bigger, if false check that inputPin value is smaller.
// @param maxMeasureTime In ms. The function is left after this time (if no signal is found). 
// @return -1 if no reaction was found. Otherwise the used time.
int checkReactionWithPressTime(int inputPin, int pressTime, int threshold, bool positiveThreshold, int maxMeasureTime) {
  int key = 1023;
  unsigned int tcount1 = 0;
  bool accuracyOvrflw = false;
  bool counterOvrflw = false;
  bool keyPressed = false;

  // Turnoff interrupts.
  noInterrupts();

  const int adjust = 16000000/F_CPU;  // 1 for 16MHz, 2 for 8MHz.
  const int tcnt2Value = 256-(CHECK_ACCURACY/0.064/adjust); // 2ms
  const int tcnt1ValueOff = -(int)(((float)pressTime)/0.064/adjust);  
  const int tcnt1ValueTooLong = -(int)(((float)maxMeasureTime)/0.064/adjust);
  bool switchOff = true;

/*
  Serial.print(F("tcnt1ValueOff="));
  Serial.println(tcnt1ValueOff);
  Serial.print(F("tcnt1ValueTooLong="));
  Serial.println(tcnt1ValueTooLong);
*/

  // 16Mhz, prescaler = 1024
  // 1024/16000000 = 0.000064s = 64us
  // 65536*0.000064s = 4.2s max

  // Setup timer 1 (16 bit timer) to measure the time:
  // Prescaler: 1024 -> resolution 64us (at F_CPU=16MHz).
  // No timer compare register.
  TCCR1A = 0; // No PWM
  TCCR1B = (1 << CS10) | (1 << CS12);  // Prescaler = 1024
  TIMSK1 = 1<<TOIE1;  // For polling only

  // Setup timer 2 (8 bit timer) to check measurement accuracy:
  // Prescaler: 1024 -> resolution 64us (at F_CPU=16MHz).
  // No timer compare register.
  TCCR2A = 0; // No PWM
  TCCR2B = (1 << CS10) | (1 << CS12);  // Prescaler = 1024
  TIFR2 = 1<<TOV2;  // Clear pending bits
  TIMSK2 = 1<<TOIE2;  // For polling only

  // Reset timer
  TCNT2 = tcnt2Value;
  TCNT1 = tcnt1ValueOff;
  TIFR1 = 1<<TOV1;  // Clear pending bits
  TIFR2 = 1<<TOV2;  // Clear pending bits
  // Simulate joystick button
  digitalWrite(OUT_PIN_BUTTON, HIGH); 

  while(true) {
    // Check range of input pin
    int value = analogRead(inputPin);
    // Check for threshold
    if((positiveThreshold && value > threshold) // Check if value is bigger
       || (!positiveThreshold && value < threshold)) // Check if value is smaller
    {
      tcount1 = TCNT1;
      break;  
    }

    // Assure that measurement accuracy is good enough
    TCNT2 = tcnt2Value;
    if(TIFR2 & (1<<TOV2)) {
      // Error
      accuracyOvrflw = true;
      break;
    }

    // Check if key pressed
    key = analogRead(0);
    //Check if something is pressed
    if (key < LCD_KEY_PRESS_THRESHOLD) {
      // key pressed -> abort
      keyPressed = true;
    }

    // Check for time out
    if(TIFR1 & (1<<TOV1)) {
      // Interrupt pending bit set -> Overflow happened.
      // Check mode:
      if(switchOff) {
        // Switch button off
        digitalWrite(OUT_PIN_BUTTON, LOW); 
        switchOff = false;
        TCNT1 = tcnt1ValueTooLong;
        TIFR1 = 1<<TOV1;  // Clear pending bits
      }
      else {
        // This means maxMeasureTime elapsed with no signal.
        counterOvrflw = true;
        break;
      }
    }
  }

  // Disable timer interrupts
  TIMSK1 = 0;
  TIMSK2 = 0;

  // Enable interrupts
  interrupts();

  // Key pressed ? -> Abort
  if(keyPressed) {
    //waitLcdKeyRelease();
    //abortAll = true;
    return -1;
  }
  else if(accuracyOvrflw) {    // Assure that measurement accuracy is good enough
    // Error
    Error(nullptr, F(CHECK_ACCURACY_ERROR_STR));
  }
  else if(counterOvrflw) {
    return -1;
  }

  // Calculate time from counter value.
  long tcount1l = tcount1 - tcnt1ValueTooLong;
  tcount1l *= 64*adjust;
  tcount1l = (tcount1l+500l) / 1000l; // with rounding

  return (int)tcount1l;
}


// Checks the pressed key and returns a corresponding difference time for
// the press time.
// 
int checkKeyToChangePressTime(int key = LCD_KEY_NONE) {
  // Check if we need to read the key press
  if(key == LCD_KEY_NONE) {
    // Get key
    key = getLcdKey();
  }

  // Check which key is pressed
  if(key == KEY_MEASURE_MIN_UP1)
    return 1;
  if(key == KEY_MEASURE_MIN_DOWN1)
    return -1;
  if(key == KEY_MEASURE_MIN_UP10)
    return 10;
  if(key == KEY_MEASURE_MIN_DOWN10)
    return -10;

  return 0;
}


// Calibrates photo sensor and SVGA output and measures the
// minimal time that the button needs to be pressed to be recognized
// by the system (i.e. the emulator).
// Since the normal polling time of an emulator should be 20ms (or 17ms for US) this should be the result.
// This method verifies this assumption.
// 
// Pseudo code:
// 1. Start with button-press (relais close) time of 1ms
// 2. Do 100x test 
// 3. If one of the tests fail then increase close-time by 1ms and start at 2
// 4. If all 100 tests pass the minimum time has been found.
//
// A test lasts until either an SVGA signal or the photo sensor signal is
// found. For convenience the user needs to conenct only one of both.
// If no signal is found for 300ms the test has failed.
void measureMinPressTime() {
  // Calibrate
  lcd.clear();
  lcd.print(F("Calibrate SVGA"));
  // Simulate joystick button press
  digitalWrite(OUT_PIN_BUTTON, HIGH); 
  waitMs(500); if(isAbort()) return;
  // Get max svga value
  struct MinMax buttonOnSVGA = getMaxMinAnalogIn(IN_PIN_SVGA, 1500);
  if(isAbort()) return;
  // Print
  lcd.setCursor(0,1);
  lcd.print(buttonOnSVGA.max);
  // Simulate joystick button unpress
  digitalWrite(OUT_PIN_BUTTON, LOW); 
  waitMs(500); if(isAbort()) return;
  // Get max/min light value
  struct MinMax buttonOffSVGA = getMaxMinAnalogIn(IN_PIN_SVGA, 1500);
  if(isAbort()) return;
  // Print
  lcd.setCursor(0,1);
  lcd.print(buttonOffSVGA.max);
  lcd.print(F("         "));
  waitMs(1000); if(isAbort()) return;

  // Print diff
  lcd.setCursor(0,1);
  lcd.print(F("Diff="));
  lcd.print(buttonOnSVGA.max-buttonOffSVGA.max);
  lcd.print(F("         "));
  waitMs(1000); if(isAbort()) return;
  
  // Check if SVGA signal can be used
  bool useSVGA = (buttonOnSVGA.max-buttonOffSVGA.max >= SVGA_MIN_DIFF);
  //Serial.println(F("SVGA"));
  //Serial.println(buttonOnSVGA.max);
  //Serial.println(buttonOffSVGA.max);
  int threshold;
  int pin;

  if(useSVGA) {
    // Use SVGA threshold
    threshold = (buttonOnSVGA.max+buttonOffSVGA.max)/2;
    pin = IN_PIN_SVGA;
  }
  else {
    // Calibrate photo sensor 
    lcd.clear();
    lcd.print(F("Calib. Photo S."));
    // Simulate joystick button press
    digitalWrite(OUT_PIN_BUTTON, HIGH); 
    waitMs(500); if(isAbort()) return;
    // Get max/min light value
    struct MinMax buttonOnLight = getMaxMinAnalogIn(IN_PIN_PHOTO_SENSOR, 1500);
    if(isAbort()) return;
    // Print
    lcd.setCursor(0,1);
    lcd.print(buttonOnLight.min);
    lcd.print(F("-"));
    lcd.print(buttonOnLight.max);
    // Simulate joystick button unpress
    digitalWrite(OUT_PIN_BUTTON, LOW); 
    waitMs(500); if(isAbort()) return;
    // Get max/min light value
    struct MinMax buttonOffLight = getMaxMinAnalogIn(IN_PIN_PHOTO_SENSOR, 1500);
    if(isAbort()) return;
    // Print
    lcd.setCursor(0,1);
    lcd.print(buttonOffLight.min);
    lcd.print(F("-"));
    lcd.print(buttonOffLight.max);
    lcd.print(F("         "));
    waitMs(1000); if(isAbort()) return;

    // Check values. They should not overlap.
    bool overlap = (buttonOnLight.max >= buttonOffLight.min && buttonOnLight.min <= buttonOffLight.max);
    if(overlap) {
      // Error
      Error(F("Calibr. Error:"), F("Ranges overlap"));
      return;
    }

    // Calculate threshold in the middle (buttonOffLight value is bigger than buttonOnLight value)
    threshold = (buttonOnLight.max + buttonOffLight.min)/2;
    pin = IN_PIN_PHOTO_SENSOR;
  }

  // Print
  lcd.clear();
  lcd.print(F("Start testing..."));
  waitMs(1000); if(isAbort()) return;
  lcd.clear();

  // Measure: Start with 15ms
  int pressTime = 15;
  unsigned long startTime;
  unsigned long offsetTime;
  char s[9+1];
  unsigned long maxTime = 0;
  unsigned long totalTime = 0;
  while(true) {
    int pressDiff = 1;
    // Print
    lcd.setCursor(0,0);
    lcd.print(pressTime);
    lcd.print(F("ms:"));    
  
    // Start measurement
    offsetTime = 0;
    startTime = millis();
    unsigned long cycle = 0;
    while(true) {
      cycle ++;
      if(cycle == 0) {
        // Will take a long time 
        while(getLcdKey() == LCD_KEY_NONE);  // Wait on key press 
        return;
      }

      // Wait until input changes
      int time = checkReactionWithPressTime(pin, pressTime, threshold, useSVGA, 300);
      // Check key
      if(time < 0) {
        int key = analogRead(0);
        if (key < LCD_KEY_PRESS_THRESHOLD) {
          pressDiff = checkKeyToChangePressTime();
          if(pressDiff == 0) {
            // Other key pressed
            abortAll = true;
            return;
          }
        }
        break;
      }

      // Calculate time 
      offsetTime += time;
      totalTime = millis() - startTime + offsetTime;
      totalTime /= 1000l;  // in secs
      
      // Print count and time, e.g. "4k 1h"
      snprintf(s, sizeof(s), "%s, %s", longToString(cycle), secsToString(totalTime));
      lcd.setCursor(6,0);
      lcd.print(s);  
      lcd.print(F("    "));

      // Wait a random time to make sure we really get different results.
      int waitRnd = random(30, 80);
      // Wait until input changes
      int key = waitMsInput(IN_PIN_SVGA, threshold, !useSVGA, waitRnd);
      // Check for keypress
      if(key != LCD_KEY_NONE) {
        pressDiff = checkKeyToChangePressTime(key);
        if(pressDiff == 0) {
          // Other key pressed
          abortAll = true;
          return;
        }
        abortAll = false;
      }
      if(abortAll)
        return;
    }

    // Print to serial
    Serial.print(pressTime);
    Serial.print(F("\t"));
    Serial.print(cycle);
    Serial.print(F("\t"));
    Serial.println(totalTime);

    // Check for new max. time
    if(totalTime > maxTime) {
      // Print in 2nd line
      lcd.setCursor(0,1);
      lcd.print(pressTime);
      lcd.print(F("ms:"));    
      lcd.setCursor(6,1);
      lcd.print(s);  
      lcd.print(F("    "));
      maxTime = totalTime;
    }

    // Change press time (in ms)
    pressTime += pressDiff;
    if(pressTime < 1)
      pressTime = 1;
  }
}


 
