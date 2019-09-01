#include <SPI.h>
#include <LiquidCrystal.h>



/*

Reed Relais SIL 7271-D 5V:
From own measurements:
- Switch bouncing: 40us
- Delay: 5V Out to relais switching: < 250us


LCD keypad shield, keys:
  x = analogRead (0);
  if (x < 60) {
    // Right
  }
  else if (x < 200) {
    // Up
  }
  else if (x < 400) {
    // Down
  }
  else if (x < 600) {
    // Left
  }
  else if (x < 800) {
  	// Select");
  }


Measurement accuracy:
The code is not optimized, i.e. no assembler code used for measurement.
But the C-code has an internal check for accuracy.
It aborts if accuracy gets too bad.
Current accuracy is < 3ms.
I also measured the lag directly with a LED connected to the relais. The 
measured lag was 0-2ms in 100 trials.


Note: The measurement should also work with an 8MHz CPU. All the time adjustmeents are done
so that the correct time should be displayed.
However it was tested only with a 16MHz CPU.
*/


// Structure to return min/max values.
struct MinMax {
  int min;
  int max;
};


// Simulation of the joystick button.
const int OUT_PIN =  8;

// The analog input from the photo sensor.
const int INPUT_PIN = 2;


// Count of cycles to measure the input lag.
const int COUNT_CYCLES = 100;


// Defines for the available LCD keys.
enum {
  LCD_KEY_NONE,
  LCD_KEY_SELECT,
  LCD_KEY_LEFT,
  LCD_KEY_RIGHT,
  LCD_KEY_UP,
  LCD_KEY_DOWN
};


// Define used Keys.
const int KEY_START = LCD_KEY_DOWN;
const int KEY_TEST = LCD_KEY_LEFT;



// Is set if a function is (forcefully) left.
// E.g. because of a keypress (aboort.
// The menu starts at the beginning afterwards.
bool abortAll = true;


//LiquidCrystal lcd(8, 13, 9, 4, 5, 6, 7); // LCD pin configuration.
//LiquidCrystal lcd(8, 3, 2, 4, 5, 6, 7); // LCD pin configuration.
LiquidCrystal lcd(19, 17, 18, 4, 5, 6, 7); // LCD pin configuration.



// The setup routine.
void setup() {
  // Setup GPIOs
  pinMode(OUT_PIN, OUTPUT);
  digitalWrite(OUT_PIN, LOW); 
  pinMode(2, INPUT_PULLUP);

  // Setup LCD
  lcd.begin(16, 2);
  lcd.clear();
  
  // Debug communication
  Serial.begin(115200);
  Serial.println("Serial connection up!");
}


// Prints the main menu.
void printMainMenu() {
  // Print "Welcome"
  lcd.clear();
  lcd.print("** Lag-Meter  **");
  lcd.setCursor(0, 1);
  lcd.print("Press a key...");
}


// Substitute for the interrupt based delay function.
// This function does an active wait.
void sleepMs() {

}


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
void Error(char* area, char* error) {
  lcd.clear();
  lcd.print(area);
  lcd.setCursor(0,1);
  lcd.print(error);
  while(getLcdKey() == LCD_KEY_NONE);
  abortAll = true;
}


// State: test photo sensor.
// The photo sensor input is read and printed to the LCD.
// Press "KEY_TEST" to leave.
void testPhotoSensor() {
  lcd.clear();
  lcd.print("Button:");
  lcd.setCursor(0,1);
  lcd.print("Sensor:");
  int outpValue = LOW;
  while(!abortAll) {
    // Stimulate button
    outpValue = ~outpValue;
    digitalWrite(OUT_PIN, outpValue);

    // Print button state
    lcd.setCursor(8, 0);
    if(outpValue)
      lcd.print("ON ");
    else
      lcd.print("OFF");
      
    // Wait for potential lag time
    waitMs(150); if(isAbort()) return;
    
    // Evaluate for some time
    for(int i=0; i<3; i++) {
      // Read photo sensor 
      int value = analogRead(INPUT_PIN);
      lcd.setCursor(8, 1);
      lcd.print(value);
      lcd.print("   ");
      waitMs(500); if(isAbort()) return;
    }

  }
  //abortAll = true;
}


// Returns the min/max photo sensor values for a given time frame.
struct MinMax getMaxMinPhotoValue(int measTime) {
  struct MinMax value;
  value.min = value.max = analogRead(INPUT_PIN);
  int time;
  int startTime = millis();
  do {
	  // Read photo sensor
    int currValue = analogRead(INPUT_PIN);
    if(currValue > value.max)
      value.max = currValue;
    else if(currValue < value.min)
      value.min = currValue;
 
    // Get elapsed time
  	time = millis() - startTime;
    if(isAbort())
      break;
  } while(time < measTime);
  
  // Allow 1 for uncertainty
  value.min--;
  value.max++;
  return value;
}


// Waits until the photo sensor value gets into range.
// Returns the time.
int measureLag(int outpValue, struct MinMax range) {
  int key = 1023;
  unsigned int tcount1 = 0;
  bool accuracyOvrflw = false;
  bool counterOvrflw = false;
  bool keyPressed = false;

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
  digitalWrite(OUT_PIN, outpValue); 

  while(true) {
    // Check range of photo sensor 
    int value = analogRead(INPUT_PIN);
    if(value >= range.min && value <= range.max) {
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
  };

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
    Error("Error:", "Accuracy >= XXms");
  }
  else if(counterOvrflw) {    // Overflow?
    // Interrupt pending bit set -> Overflow happened.
    // This means 4.19 seconds elapsed with no signal.
    Error("Error:", "No signal");
  }

  // Calculate time from counter value.
  long tcount1l = tcount1;
  tcount1l *= 64*adjust;
  tcount1l = (tcount1l+500) / 1000; // with rounding

  return (int)tcount1l;
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
void calibrateAndMeasureLag() {
  // Wait until input (photo sensor) stabilizes
 // waitOnStablePhotoSensor(1000);
  if(isAbort())
    return;

  // Calibrate
  lcd.clear();
  lcd.print("Calibrating...");
  // Simulate joystick button press
  digitalWrite(OUT_PIN, HIGH); 
  waitMs(500); if(isAbort()) return;
  // Get max/min light value
  struct MinMax buttonOnLight = getMaxMinPhotoValue(1500);
  if(isAbort()) return;
  // Print
  lcd.setCursor(0,1);
  lcd.print(buttonOnLight.min);
  lcd.print("-");
  lcd.print(buttonOnLight.max);
  // Simulate joystick button unpress
  digitalWrite(OUT_PIN, LOW); 
  waitMs(500); if(isAbort()) return;
  // Get max/min light value
  struct MinMax buttonOffLight = getMaxMinPhotoValue(1500);
  if(isAbort()) return;
  // Print
  lcd.setCursor(0,1);
  lcd.print(buttonOffLight.min);
  lcd.print("-");
  lcd.print(buttonOffLight.max);
  lcd.print("         ");
  waitMs(1000); if(isAbort()) return;

  // Check values. They should not overlap.
  bool overlap = (buttonOnLight.max >= buttonOffLight.min && buttonOnLight.min <= buttonOffLight.max);
  if(overlap) {
    // Error
    Error("Calibr. Error:", "Ranges overlap");
    return;
  }
  
  // Print
  lcd.clear();
  lcd.print("Start testing...");
  waitMs(1000); if(isAbort()) return;
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Lag: ");

  // Measure a few cycles
  struct MinMax timeRange = {1023, 0};
  float avg = 0.0;
  for(int i=1; i<=COUNT_CYCLES; i++) {
    // Print
    lcd.setCursor(0,0);
    lcd.print(i);
    lcd.print("/");
    lcd.print(COUNT_CYCLES);
    lcd.print(": ");
    
    // Wait until input (photo sensor) changes
    int time = measureLag(HIGH, buttonOnLight);
    if(isAbort()) return;
    // Output result: 
    lcd.print(time);
    lcd.print("ms     ");
    
    // Wait until input (photo sensor) changes
    measureLag(LOW, buttonOffLight);
    if(isAbort()) return;

    // Wait a random time to make sure we really get different results.
    int waitRnd = random(100, 150);
    //lcd.setCursor(0,0);
    //lcd.print(waitRnd);
    waitMs(waitRnd); if(isAbort()) return;

    // Calculate max/min.
    if(time > timeRange.max)
      timeRange.max = time;
    if(time < timeRange.min)
      timeRange.min = time;

    // Print min/max result
    lcd.setCursor(5,1);
    if(timeRange.min != timeRange.max) {
      lcd.print(timeRange.min);
      lcd.print("-");
    }
    lcd.print(timeRange.max);
    lcd.print("ms     ");

    // Calculate average
    avg += time;
  }

  // Print average:
  avg /= COUNT_CYCLES;
  lcd.setCursor(0,0);
  lcd.print("Average: ");
  lcd.print((int)avg);
  lcd.print("ms     ");
  
  // Wait on key press.
  while(getLcdKey() != LCD_KEY_NONE);
}



const int BUTTON_PIN =  12;
//int out = LOW;

// Main loop.
void loop() {
/*
  pinMode(BUTTON_PIN, OUTPUT);
digitalWrite(BUTTON_PIN, HIGH);
delay(100);
digitalWrite(BUTTON_PIN, LOW);
delay(100);
return;
*/

  /*
  //Serial.println(millis());
  //if(TCNT0 < 1000)
    Serial.println(TCNT0);
  return;
  */

  /*
  Serial.println(out);
  digitalWrite(OUT_PIN, out);
  delay(10);
  out = ~out;
  return;
  */
  
  
  if(abortAll) {
    printMainMenu();
    abortAll = false;
  }
  
  int key = getLcdKey();
  
  switch(key) {
    case KEY_START:
      calibrateAndMeasureLag();
      break;
    case KEY_TEST:
	  testPhotoSensor();
      break;
  }
}



