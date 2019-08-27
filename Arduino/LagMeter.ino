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

*/


// Structure to return min/max values.
struct MinMax {
  int min;
  int max;
};


// Simulation of the joystick button.
const int OUT_PIN =  11;

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


LiquidCrystal lcd(8, 13, 9, 4, 5, 6, 7); // LCD pin configuration.



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
  lcd.print("Press 'Start'...");
}


// Returns the pressed key (or LCD_KEY_NONE).
int getLcdKey() {
  int x = analogRead(0);
  // Return immediately if nothing is pressed
  if (x >= 800) 
    return LCD_KEY_NONE;
  
  // Some key has been pressed
  delay(100);	// poor man's debounce

  // Wait until released
  while(analogRead(0) < 800);

  delay(100); // debounce
 
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


// Waits until the photo sensor value gets into range.
void waitOnPhotoSensorRange(struct MinMax range) {
  int prevTime, startTime;
  startTime = prevTime = millis();
  while(true) {
    // Check accuracy
    int nextTime = millis();
    if(nextTime-prevTime >= 3) {
      // Error
      Error("Error:", "Accuracy >= 3ms");
      return;
    }
    prevTime = nextTime;

    // Get photo sensor
    int value = analogRead(INPUT_PIN);
    // Check range
    if(value >= range.min && value <= range.max)
  		break;     
    // Wait a little
    //delayMicroseconds(100);
    if(isAbort())
      break;

    // Abort if taken too long
    if(nextTime-startTime > 5000)  {
      // Error
      Error("Error:", "No signal");
      return;
    }
  }
}


// State: test phot sensor.
// The photo sensor input is read and printed to the LCD.
// Press "KEY_TEST" to leave.
void testPhotoSensor() {
  lcd.clear();
  lcd.print("Test sensor...");
  lcd.setCursor(0,1);
  lcd.print("(Max=1023)");
  while(getLcdKey() == LCD_KEY_NONE) {
  	// Read photo sensor 
	int value = analogRead(INPUT_PIN);
    lcd.setCursor(11, 1);
    lcd.print(value);
    lcd.print("   ");
    delay(500);
  }
  abortAll = true;
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
  
  return value;
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
    
    // Get time
    int startTime = millis();
    // Simulate joystick button press
    digitalWrite(OUT_PIN, HIGH); 
    // Wait until input (photo sensor) changes
    waitOnPhotoSensorRange(buttonOnLight);
    // Get time
    int endTime = millis();
    int time = endTime - startTime;
    if(isAbort()) return;
    // Output result: 
    lcd.print(time);
    lcd.print("ms     ");
    
    // Simulate joystick button unpress
    digitalWrite(OUT_PIN, LOW); 
    // Wait until input (photo sensor) changes
    waitOnPhotoSensorRange(buttonOffLight);
    if(isAbort()) return;

    // Wait a random time to make sure we really get different results.
    int waitRnd = random(100, 150);
    //lcd.setCursor(0,0);
    //lcd.print(waitRnd);
    waitMs(waitRnd); if(isAbort()) return;

    // Calculate max/min.
    if(time > timeRange.max)
      timeRange.max = time;
    else if(time < timeRange.min)
      timeRange.min = time;

    // Print min/max result
    lcd.setCursor(5,1);
    lcd.print(timeRange.min);
    lcd.print("-");
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




//int out = LOW;

// Main loop.
void loop() {

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


