#ifndef __COMMON_H__
#define __COMMON_H__


///////////////////////////////////////////////////////////////////
// Pin configuration:

// Simulation of the joystick button.
const int OUT_PIN_BUTTON = 8;
//const int OUT_PIN_BUTTON = 3;

// The analog input for the photo sensor.
const int IN_PIN_PHOTO_SENSOR = 2;

// The analog input for the SVGA connector (blue, or red or green)
const int IN_PIN_SVGA = 1;
///////////////////////////////////////////////////////////////////

// Count of cycles to measure the input lag.
const int COUNT_CYCLES = 100;

// The checked accuracy in ms:
#define CHECK_ACCURACY  1
#define CHECK_ACCURACY_ERROR_STR "Err:Accuracy>1ms"

// Time to show the title of each test.
#define TITLE_TIME  1500    // in ms

#endif
