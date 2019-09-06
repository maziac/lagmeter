/*
Lag-Meter:
A device to measure the total lag (from USB controller to monitor output).


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

#include "src/Measurement/Utilities.h"
#include "src/Measurement/Measure.h"

// The SW version.
#define SW_VERSION "0.6"



// Define used Keys.
const int KEY_TEST_PHOTO_BUTTON = LCD_KEY_LEFT;
const int KEY_MEASURE_PHOTO = LCD_KEY_DOWN;
const int KEY_MEASURE_SVGA = LCD_KEY_UP;
const int KEY_MEASURE_SVGA_TO_PHOTO = LCD_KEY_RIGHT;



// Prints the main menu.
void printMainMenu() {
  // Print "Welcome"
  lcd.clear();
  lcd.print(F("** Lag-Meter  **"));
  lcd.setCursor(0, 1);
  lcd.print(F("v" SW_VERSION "," __DATE__));
}



// The setup routine.
void setup() {
  // Setup pins 
  setupMeasurement();

  // Random seed
  randomSeed(1234);


  // Setup LCD
  lcd.begin(16, 2);
  lcd.clear();
  
  // Debug communication
  Serial.begin(115200);
  Serial.println(F("Serial connection up!"));
}


// Main loop.
void loop() {  
  if(abortAll) {
    printMainMenu();
    abortAll = false;
  }
  
  int key = getLcdKey();
  
  switch(key) {
    case KEY_TEST_PHOTO_BUTTON:
	    testPhotoSensor();
      break;
    case KEY_MEASURE_PHOTO:
      measurePhotoSensor();
      break;
    case KEY_MEASURE_SVGA:
      measureSVGA();
      break;
    case KEY_MEASURE_SVGA_TO_PHOTO:
      measureSvgaToMonitor();
      break;   
  }
}
