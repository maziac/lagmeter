#include <SPI.h>
#include <LiquidCrystal.h>



// Simulation of the joystick button.
const int OUT_PIN =  12;

// The analog input from the photo sensor.
const int INPUT_PIN = 5;


LiquidCrystal lcd(8, 13, 9, 4, 5, 6, 7); // LCD-Pins für unser Shield


// The setup routine.
void setup() {
  // Setup GPIOs
  pinMode(OUT_PIN, OUTPUT);
  digitalWrite(OUT_PIN, LOW); 
  pinMode(11, INPUT);

  // Setup LCD
  lcd.begin(16, 2);
  lcd.clear();
  
  // Print "Welcome"
  lcd.print("** Lag-Meter  **");
  delay(1000); // wait 1 sec
  lcd.setCursor(0, 1);
  lcd.print("Press 'Start'...");
  
  Serial.begin(9600);
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

// The maximum allowed diff for the photo sensor durign stabilizing.
const int STABILIZE_MAX_DIFF = 10;

// Waits until the signal at the photo sensor stabilizes for some time.
void waitOnStablePhotoSensor() {
  int startTime, time;
  int prevValue = -STABILIZE_MAX_DIFF-1;  // Make sure the first value diff is bigger
  do {
    int value = analogRead(INPUT_PIN);
    Serial.println(value);
    int diff = abs(value-prevValue);
    if(diff > STABILIZE_MAX_DIFF)
      startTime = millis();
    time = millis() - startTime;
    //Serial.println(time);
    prevValue = value;
  } while(time < STABILIZE_TIME);
}


// The internal states.
enum {
  STATE_IDLE,
  STATE_MEASURING
};

int state = STATE_IDLE;
int measureCycle = 0;
const int MAX_CYCLES = 5;



// Main loop.
void loop() {

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
      waitOnStablePhotoSensor();
      // Get time
      int startTime = millis();
      // Simulate joystick button press
      digitalWrite(OUT_PIN, HIGH); 
      // Wait until input (photo sensor) stabilizes
     // waitOnStablePhotoSensor();
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

