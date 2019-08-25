#include <SPI.h>
#include <LiquidCrystal.h>



/*

Own measurements:

Reed Relais SIL 7271-D 5V:
- Switch bouncing: 40us
- Delay: 5V Out to relais switching: < 250us

*/



// Simulation of the joystick button.
const int OUT_PIN =  11;

// The analog input from the photo sensor.
const int INPUT_PIN = 2;


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
  
  // Print "Welcome"
  lcd.print("** Lag-Meter  **");
  delay(1000); // wait 1 sec
  lcd.setCursor(0, 1);
  lcd.print("Press 'Start'...");

  Serial.begin(115200);
  Serial.println("Serial connection up!");
}



// Check if the start key was pressed.
bool getStartKey() {
  
  int x; //  variable
  x = analogRead (0); // assign 'x' AnalogueInput (Shield's buttons)
  if (x > 600 && x < 800 ) // SELECT Button
    return true;
  return false;
  
  int value = digitalRead(2);
  //Serial.println(value);
  
  return (value == LOW);
}


// Stabilize time for photo sensor in ms.
const int STABILIZE_TIME = 1000;

// The maximum allowed diff for the photo sensor during stabilizing.
const int STABILIZE_MAX_DIFF = 10;

// Waits until the signal at the photo sensor stabilizes for some time.
int waitOnStablePhotoSensor() {
  int startTime, time, count;
  float avg;
  int prevValue = -STABILIZE_MAX_DIFF-1;  // Make sure the first value diff is bigger
  do {
    int value = analogRead(INPUT_PIN);
    //Serial.println(value);
    int diff = abs(value-prevValue);
    if(diff > STABILIZE_MAX_DIFF) {
      startTime = millis();
      avg = 0.0;
      count = 0;
    }
    time = millis() - startTime;
    //Serial.println(time);
    prevValue = value;
    avg += prevValue;
    count++;
    // Wait a little
    delayMicroseconds(100);
  } while(time < STABILIZE_TIME);
  
  // Calculate average
  avg /= count;
  return (int) avg;
}


// Waits until the light value changes.
void waitOnPhotoSensorChange(int lightValue) {
   while(true) {
    int value = analogRead(INPUT_PIN);
    //Serial.println(value);
    int diff = abs(value-lightValue);
    if(diff > STABILIZE_MAX_DIFF) {
		break;
    }
    // Wait a little
    delayMicroseconds(100);
  }
}



// The internal states.
enum {
  STATE_IDLE,
  STATE_MEASURING
};

int state = STATE_IDLE;
int measureCycle = 0;
const int MAX_CYCLES = 5;


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

  // Statemachine 
  switch(state) {
    case STATE_IDLE:
      //delay(200);
      if(getStartKey()) {
        // Start the measurement
        state = STATE_MEASURING;
        measureCycle = 1;
        lcd.clear();
        // Print cycle
        lcd.print("Testing...");
      }
      break;

    case STATE_MEASURING:
      // Wait until input (photo sensor) stabilizes
      int lightValue = waitOnStablePhotoSensor();
    	Serial.println(lightValue);
      // Get time
      int startTime = millis();
      // Simulate joystick button press
      digitalWrite(OUT_PIN, HIGH); 
      // Wait until input (photo sensor) changes
      waitOnPhotoSensorChange(lightValue);
      // Get time
      int endTime = millis();
      int time = endTime - startTime;
      // Output result: 
      lcd.clear();
      lcd.print("Test: ");
      lcd.print(measureCycle);
      lcd.print("/");
      lcd.print(MAX_CYCLES);
      lcd.setCursor(0,1);
      lcd.print(time);
      lcd.print(" ms     ");
      // Next
      measureCycle ++;
      if(measureCycle > MAX_CYCLES) {
        // Measurement ends
         state = STATE_IDLE;
      }
      break;
  }


}


/*

  int x;
  x = analogRead (0);
  lcd.setCursor(10,1);
  if (x < 60) {
    lcd.print ("Right ");
  }
  else if (x < 200) {
    lcd.print ("Up    ");
  }
  else if (x < 400){
    lcd.print ("Down  ");
  }
  else if (x < 600){
    lcd.print ("Left  ");
  }
  else if (x < 800){
    lcd.print ("Select");
  }
  */

