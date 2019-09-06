#include "Utilities.h"
#include "Measure.h"
#include "Arduino.h"

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

///////////////////////////////////////////////////////////////////


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
    waitMs(150); if(isAbort()) return;
    
    // Evaluate for some time
    for(int i=0; i<3; i++) {
      // Read photo sensor 
      int value = analogRead(IN_PIN_PHOTO_SENSOR);
      lcd.setCursor(8, 1);
      lcd.print(value);
      lcd.print(F("   "));
      waitMs(500); if(isAbort()) return;
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
// @param outpValue Use HIGH or LOW for the output pin.
// @param range The range to compare the inputPin value to.
// @param waitTime The time to wait for.
void waitMsInput(int inputPin, int outpValue, struct MinMax range, int waitTime) {
  // Simulate joystick button
  digitalWrite(OUT_PIN_BUTTON, outpValue); 
  // Wait
  int time;
  int startTime = millis();
  int watchdogTime = startTime;
  do {
    // Get input value 
    int value = analogRead(inputPin);
    // Check for normal range
    if(value < range.min || value > range.max) {
      // Out of range, restart timer
      startTime = millis();
    }
    // Get time
    int currTime = millis();
  	time = currTime - startTime;
    if(isAbort())
      return;
    // Check if waited too long (4 secs)
    if(currTime-watchdogTime > 4000) {
      // Error
      Error(F("Error:"), F("Signal wrong"));
      break;
    }
  } while(time < waitTime);
}

    
// Waits until the photo sensor (or SVGA value) value gets into range.
// @param inputPin Pin from which the analog input is read. Photo sensor or SVGA.
// @param range The range to compare the inputPin value to.
// @param inputPinWait (Optional) If given: wait for inputPinWait value to get in 'rangeWait' before starting the measurement.
// @param rangeWait (optional) The range to compare the inputPinWait value to.
int measureLag(int inputPin, struct MinMax range, int inputPinWait = -1, struct MinMax rangeWait = {}) {
  int key = 1023;
  unsigned int tcount1 = 0;
  bool accuracyOvrflw = false;
  bool counterOvrflw = false;
  bool keyPressed = false;

  // Add another small safety margin if range is inverted
  range.min--;
  range.max++;

  // Turnoff interrupts.
  noInterrupts();

  // Setup timer 1 (16 bit timer) to measure the time:
  // Prescaler: 1024 -> resolution 64us (at F_CPU=16MHz).
  // No timer compare register.
  TCCR1A = 0; // No PWM
  TCCR1B = (1 << CS10) | (1 << CS12);  // Prescaler = 1024
  TIFR1 = 1<<TOV1;  // Clear pending bits
  TIMSK1 = 1<<TOIE1;  // For polling only

  // Setup timer 2 (8 bit timer) to check measurement accuracy:
  // Prescaler: 1024 -> resolution 64us (at F_CPU=16MHz).
  // No timer compare register.
  TCCR2A = 0; // No PWM
  TCCR2B = (1 << CS10) | (1 << CS12);  // Prescaler = 1024
  TIFR2 = 1<<TOV2;  // Clear pending bits
  TIMSK2 = 1<<TOIE2;  // For polling only

  // Reset timer
  const int adjust = 16000000/F_CPU;  // 1 for 16MHz, 2 for 8MHz.
  const int tcnt2Value = 256-(0.5/0.004/adjust); //  131;  // (256-131)*4us = 0.5ms, would work up to (256-220)*4us = 0.144ms
  TCNT2 = tcnt2Value;
  TCNT1 = 0;
  // Simulate joystick button
  digitalWrite(OUT_PIN_BUTTON, HIGH); 

int t1 = -1;
int t2 = -1;
int p1 = -1;
int p2 = -1;

  // Check if we wait for a 2nd trigger (required to measure the delay between SVGA out and monitor)
  if(inputPinWait >= 0) {
    // Wait on trigger
    while(true) {
      // Check range of wait-input-pin 
      int value = analogRead(inputPinWait);
      // Check for range
      if(value >= rangeWait.min && value <= rangeWait.max) {
        // Restart measurement
        TCNT2 = tcnt2Value;
        t1 = TCNT1;
        p1 = analogRead(inputPin);
        TCNT1 = 0;
        break;  
      }

      // Assure that measurement accuracy is good enough
      TCNT2 = tcnt2Value;
      if(TIFR2 & (1<<TOV2)) {
        // Error
        accuracyOvrflw = true;
        goto L_ERROR;
      }

      // Check if key pressed
      key = analogRead(0);
      // Return immediately if something is pressed
      if (key < 800) {
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

#ifdef OUT_PIN_BUTTON_COMPARE_TIME
  if(outpValue)
    digitalWrite(OUT_PIN_BUTTON_COMPARE_TIME, HIGH); 
#endif 

  while(true) {
    // Check range of input pin
    int value = analogRead(inputPin);
    // Check for inverted range
    if(value < range.min || value > range.max) {
        tcount1 = TCNT1;
        t2 = TCNT1;
        p2 = value;
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
    if (key < 800) {
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
  else if(accuracyOvrflw) {    // Assure that measuremnt accuracy is good enough
    // Error
    Error(F("Error:"), F("Accuracy >= 1ms"));
  }
  else if(counterOvrflw) {    // Overflow?
    // Interrupt pending bit set -> Overflow happened.
    // This means 4.19 seconds elapsed with no signal.
    Error(F("Error:"), F("No signal"));
  }

  // Calculate time from counter value.
  long tcount1l = tcount1;
  tcount1l *= 64*adjust;
  tcount1l = (tcount1l+500) / 1000; // with rounding


if((int)tcount1l < 5) {
  Serial.print("tcount1l=");
  Serial.println(tcount1l);
  Serial.print("t2=");
  Serial.println(t2);
  Serial.print("t1=");
  Serial.println(t1);
  Serial.print("p2=");
  Serial.println(p2);
  Serial.print("p1=");
  Serial.println(p1);
  Serial.print("rmin=");
  Serial.println(range.min);
  Serial.print("rmax=");
  Serial.println(range.max);
}

  return (int)tcount1l;
}


// Waits until the SVGA value gets into range, then measures the time until the photo
// sensor gets into range.
// Used to measure the delay of the monitor.
// @param range The range for the photo sensor.
// @param rangeWait The range for the SVGA value.
// @return the time.
int measureLagDiff(struct MinMax range, struct MinMax rangeWait) {
  return measureLag(IN_PIN_PHOTO_SENSOR, range, IN_PIN_SVGA, rangeWait);
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
    int time = measureLag(IN_PIN_PHOTO_SENSOR, buttonOffLight);
    if(isAbort()) return;
    // Output result: 
    lcd.print(time);
    lcd.print(F("ms     "));
    
    // Wait a random time to make sure we really get different results.
    int waitRnd = random(70, 150);
    // Wait until input (photo sensor) changes
    waitMsInput(IN_PIN_PHOTO_SENSOR, LOW, buttonOffLight, waitRnd);
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
  if(buttonOnSVGA.max-buttonOffSVGA.max < 20) {
    // Error
    Error(F("Calibr. Error:"), F("Signal too weak"));
    return;
  }
  
  // Calculate measure ranges 
  buttonOnSVGA.min = (buttonOnSVGA.max+buttonOffSVGA.max)/2;
  buttonOnSVGA.max = 1023;
  buttonOffSVGA.min = 0;
  
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
    int time = measureLag(IN_PIN_SVGA, buttonOffSVGA);
    if(isAbort()) return;
    // Output result: 
    lcd.print(time);
    lcd.print(F("ms     "));
        
    // Wait a random time to make sure we really get different results.
    int waitRnd = random(70, 150);
    // Wait until input (svga) changes
    waitMsInput(IN_PIN_SVGA, LOW, buttonOffSVGA, waitRnd);
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



// Calibrates photo sensor and SVGA output and measure the
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
//   to time when phot sensor changes.
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
  if(buttonOnSVGA.max-buttonOffSVGA.max < 20) {
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
  
  // Calculate measure ranges 
  buttonOnSVGA.min = (buttonOnSVGA.max+buttonOffSVGA.max)/2;
  buttonOnSVGA.max = 1023;
  buttonOffSVGA.min = 0;
  
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
    int time = measureLagDiff(buttonOffLight, buttonOnSVGA);
    if(isAbort()) return;
    // Output result: 
    lcd.print(time);
    lcd.print(F("ms     "));
    
     // Wait a random time to make sure we really get different results.
    int waitRnd = random(70, 150);
    // Wait until input (photo sensor) changes
    waitMsInput(IN_PIN_PHOTO_SENSOR, LOW, buttonOffLight, waitRnd);
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

