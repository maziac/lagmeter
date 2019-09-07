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


#if 0

#include "src/Measurement/Utilities.h"
#include "src/Measurement/Measure.h"

// The SW version.
#define SW_VERSION "0.7"



// Define used Keys.
const int KEY_TEST_PHOTO_BUTTON = LCD_KEY_LEFT;
const int KEY_MEASURE_PHOTO = LCD_KEY_DOWN;
const int KEY_MEASURE_SVGA = LCD_KEY_UP;
const int KEY_MEASURE_SVGA_TO_PHOTO = LCD_KEY_RIGHT;



// Prints the main menu.
void printLagMeterMenu() {
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
    printLagMeterMenu();
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


#else 

#include "src/Measurement/Utilities.h"
#include "src/Measurement/Measure.h"

// The SW version.
#define SW_VERSION "0.8"


// Define used Keys.
// Lagmeter:
const int KEY_TEST_PHOTO_BUTTON = LCD_KEY_LEFT;
const int KEY_MEASURE_PHOTO = LCD_KEY_DOWN;
const int KEY_MEASURE_SVGA = LCD_KEY_UP;
const int KEY_MEASURE_SVGA_TO_PHOTO = LCD_KEY_RIGHT;
// usblag:
const int KEY_USBLAG_MEASURE = LCD_KEY_DOWN;


// Main lag testing mode.
enum {
  NONE = 0, // Just for starting.
  USBLAG_MODE,  // Menu changes to usblag testing, i.e. the lag of a single controller is mesaured.
  LAGMETER_MODE  // The lag measure mode, button press until phot sensor change.
};

// If USB device (game controller) is attached (via USB) then the mode is changed to
// true. This will also change the menu.
int mainMode = LAGMETER_MODE;
int prevMainMode = NONE;  // previous mode


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
  lcd.print(F("Game Controller"));
}


// The number of samples for usblag
const int USBLAG_CYCLES = 100;

// Output pin to use for joystick button
#define BUTTON_PIN 8


// Prints the measured time for usblag.
// @param measuredTime The measured time in ms.
extern int total;
float usblagMin;
float usblagMax;
float usblagAvg;
bool usbEventToggle = false;
void printUsbLag(float measuredTime) {
    if(total < 0) {
      // Print something in 2nd line. Can be used as a test if game controller reacts.
      lcd.setCursor(0,1);
      usbEventToggle = !usbEventToggle;
      if(usbEventToggle)
        lcd.print(F("****    ****    "));
      else
        lcd.print(F("    ****    ****"));
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


#include "src/usblag/HIDDriver.h"
#include "src/usblag/XInputDriver.h"
#include <BTHID.h>
#include "src/usblag/HIDReportMask.h"


//#define BUTTON_PIN 8

#define ENABLE_BT 0
#define NAME_RETRIEVAL 1

#include "src/usblag/desc.h"

unsigned long nextchange;
unsigned long time;
int total = 5000;
byte state = 0;
byte button = 0;
USB Usb;
HIDReportMask report_mask;

#define BUFFER_SIZE 64

#define PS4_VID         0x054C // Sony Corporation
#define PS4_PID         0x05C4 // PS4 Controller
#define PS4_PID_SLIM 0x09CC // PS4 Slim Controller

uint8_t intervalPow2(uint8_t interval) {
  if (interval <= 2) {
    return interval;
  }
  if (interval < 4) {
    return 2;
  }
  if (interval < 8) {
    return 4;
  }
  if (interval < 16) {
    return 8;
  }
  if (interval < 32) {
    return 16;
  }
  return 32;
}

uint8_t pollBuffer[BUFFER_SIZE];
uint8_t prevBuffer[BUFFER_SIZE];

class TimingManager {
  public:
    EpInfo* pollingEP = 0;
    uint8_t address = 0;
    USB* usbRef;
    uint8_t pollingInterval = 0;
    uint32_t nextPollingTime = 0;
    bool doPoll = false;
    uint8_t overrideInterval = 0;
    TimingManager(USB* usb): usbRef(usb) {}
    bool doInit(uint8_t interval, uint8_t bAddress, EpInfo* ep) {
      pollingInterval = interval;
      pollingEP = ep;
      doPoll = true;
      overrideInterval = 0;
      address = bAddress;
      nextPollingTime = 0;
      memset(prevBuffer, 0, BUFFER_SIZE);
      Serial.print("Interval:\t\t");
      Serial.print(interval);
      Serial.println("ms");
    }
    bool canPoll() {
      if (!doPoll || !pollingEP) return false;
      // do burn
      if (overrideInterval == 255) return true;
      if((int32_t)((uint32_t)millis() - nextPollingTime) >= 0L) {
        nextPollingTime = (uint32_t)millis() + (overrideInterval > 0 ? overrideInterval : pollingInterval);
        return true;
      }
      return false;
    }
    uint8_t pollDevice(uint16_t VID, uint16_t PID) {
      if (!canPoll()) return 0;
      memset(pollBuffer, 0, BUFFER_SIZE);
      uint16_t buffer_size = pollingEP->maxPktSize;
      uint8_t res = usbRef->inTransfer(address, pollingEP->epAddr, &buffer_size, pollBuffer);
      if (res != 0 && res != hrNAK) {
        Serial.print("Cant poll device ");
        Serial.print(res);
        Serial.print("\n");
        Error(F("USB Device:"), F("Can't poll!"));
      }
      if (res == hrNAK) { return 0; }
      return doMeasure(VID, PID, pollingEP->maxPktSize, pollBuffer);
    }
    uint8_t doMeasure(uint16_t VID, uint16_t PID, uint8_t len, uint8_t *buf) {
      report_mask.apply_mask(len, buf);
      if (memcmp(buf, prevBuffer, len) != 0) {
        float measuredTime = (micros()-time)/1000.0;  // in ms
        printUsbLag(measuredTime);
        Serial.println(measuredTime);
        /*for (uint8_t i = 0 ; i < len; ++i) {
          print_hex(buf[i], 8);
        }
        Serial.println();
        */
        memcpy(prevBuffer, buf, len);
        time = 0;
      }
      return 0;
    }
};
class BTController : public BTHID, public TimingManager {
  public:
    BTController(BTD *p, bool pair = false, const char *pin = "0000"): BTHID(p, pair, pin), TimingManager(NULL) {
    }
    virtual void OnInitBTHID() {
      TimingManager::doInit(0, 0, NULL);
      enable_sixaxis();
      Serial.print("BTHID Controller initialized\n");
      report_mask.do_mask = false;
    }
    virtual void ParseBTHIDData(uint8_t len, uint8_t *buf) {
      doMeasure(PS4_VID, PS4_PID, 10, buf+2);
    }
    void enable_sixaxis() { // Command used to make the PS4 controller send out the entire output report
            uint8_t buf[2];
            buf[0] = 0x43; // HID BT Get_report (0x40) | Report Type (Feature 0x03)
            buf[1] = 0x02; // Report ID

            HID_Command(buf, 2);
    };

    void HID_Command(uint8_t *data, uint8_t nbytes) {
            pBtd->L2CAP_Command(hci_handle, data, nbytes, control_scid[0], control_scid[1]);
    };
};

class XBoxController : public XInputDriver, public TimingManager {
  public:
    uint8_t readBuf[EP_MAXPKTSIZE]; // General purpose buffer for input data
    XBoxController(USB* usb): XInputDriver(usb), TimingManager(usb) {}
    uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed) {
      uint8_t res = XInputDriver::Init(parent, port, lowspeed);
      if (res) return res;
      TimingManager::doInit(bInterval, bAddress, &epInfo[ XBOX_INPUT_PIPE ]);
      if (isXboxOne) {
        Serial.print("XBOX One Controller initialized\n");
        
        startDevice();
        
        //initRumble();
      } else {
        Serial.print("XBOX Controller initialized\n");
      }
      report_mask.do_mask = false;
      return 0;
    }
    uint8_t Release() {
      XInputDriver::Release();
      TimingManager::doPoll = false;
      return 0;
    }
    uint8_t Poll() { return pollDevice(VID, PID); }
};

class HIDController : public HIDDriver, public TimingManager {
  public:
    uint8_t readBuf[EP_MAXPKTSIZE]; // General purpose buffer for input data
    HIDController(USB* usb): HIDDriver(usb), TimingManager(usb) {}
    uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed) {
      uint8_t res = HIDDriver::Init(parent, port, lowspeed);
      if (res) return res;
      TimingManager::doInit(pollInterval, bAddress, &epInfo[hidInterfaces[0].epIndex[epInterruptInIndex]]);
      
      if (GetReportDescr(0, &report_mask)) {
        Serial.println("Cannot read HID report");
      }
      if (bNumIface > 1) {
        Serial.println("Warning! Multi-channel HID device not supported! Selecting only the first one.");
      }
      Serial.print("HID Controller initialized\n");
      mainMode = USBLAG_MODE;
      return 0;
    }
    uint8_t Release() {
      HIDDriver::Release();
      TimingManager::doPoll = false;
      return 0;
    }
    uint8_t Poll() { return pollDevice(VID, PID); }
};

HIDController Hid(&Usb);
XBoxController xbox(&Usb);

#if ENABLE_BT

BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so

/* You can create the instance of the PS4BT class in two ways */
// This will start an inquiry and then pair with the PS4 controller - you only have to do this once
// You will need to hold down the PS and Share button at the same time, the PS4 controller will then start to blink rapidly indicating that it is in pairing mode
BTController btHid(&Btd);

#endif


// SETUP
void setup() {

  // Debug communication
  Serial.begin(115200);
  Serial.println(F("Serial connection up!"));

  // Random seed
  randomSeed(1234);

  // Lagmeter initialization
  pinMode(BUTTON_PIN, OUTPUT);
  // Setup pins 
  setupMeasurement();
  // Setup LCD
  lcd.begin(16, 2);
  lcd.clear();
  

  // usblag initilization
  if (Usb.Init() == -1)
    Serial.println("OSC did not start.");
  delay(200);
  nextchange = micros() + 5000*1000;
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
      measureSVGA();
      break;
    case KEY_MEASURE_SVGA_TO_PHOTO:
      measureSvgaToMonitor();
      break;   
  }
}


// Checks for keypresses for usblag mode.
void handleUsblag() {
  // Check to print the menu
  if(abortAll) {
    digitalWrite(BUTTON_PIN, false);
    abortAll = false;
    total = -1; // Show game controller action
    printUsblagMenu();
  }

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
    case KEY_USBLAG_MEASURE:
      // Start testing usb device measurement
      button = false;
      total = 0;
      digitalWrite(BUTTON_PIN, button);
      time = 0;
      Serial.println("Launching test");
      nextchange = micros() + random(50, 70)*1000 + random(0, 250)*4;
      // Use USB polling interval of 1ms.
      Hid.overrideInterval = 1;
      xbox.overrideInterval = 1;
      // Infrom user
      lcd.clear();
      lcd.println(F("Start testing..."));
      // Wait for potential lag time
      waitMs(1000); 
      lcd.clear();
      if(isAbort()) return;
      break;
  }
  
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
}


// MAIN LOOP
int looptime = 0;
void loop() {

#if 01

#if 01
  // Check if main mode changed.
  if(mainMode != prevMainMode) {
    prevMainMode = mainMode;
    switch(mainMode) {
      case LAGMETER_MODE:
        printLagMeterMenu();
        break;
      case USBLAG_MODE:
        printUsblagMenu();
        total = -1; // Show game controller action
        break;  
    }
  }

  switch(mainMode) {
      case LAGMETER_MODE:
        handleLagMeter();
        break;
      case USBLAG_MODE:
        handleUsblag();
        break;  
  }
  
 // Usb.Task();
 #endif 
 
//#else

#if 01
  Usb.Task();
#else
  unsigned long current_time = micros();
  if (current_time >= nextchange && total < 1000) {
    if (time != 0) {
      Serial.println("Input was dropped!");
    }
    button = !button;
    digitalWrite(BUTTON_PIN, button);
    time = micros();
    total++;
    nextchange = micros() + random(50, 70)*1000 + random(0, 250)*4;
  }
  
  if (Serial.available()) {
    byte inByte = Serial.read();
    uint8_t reading_interval;
    switch (inByte) {
      case '1':
        button = true;
        digitalWrite(BUTTON_PIN, button);
        time = micros();
        Serial.println("High");
        break;
      case '0':
        button = false;
        digitalWrite(BUTTON_PIN, button);
        time = micros();
        Serial.println("Low");
        break;
      case 'b':
        Hid.overrideInterval = 255;
        xbox.overrideInterval = 255;
        Serial.println("Switch to burn mode: disabling polling interval");
        break;
      case 'o':
        Hid.overrideInterval = 1;
        xbox.overrideInterval = 1;
        Serial.println("Override polling interval to 1ms!");
        break;
      case 'l':
        Serial.print("Last loop timing... ");
        Serial.print(looptime);
        Serial.println("µs");
        break;
      case 'n':
        Hid.overrideInterval = 0;
        xbox.overrideInterval = 0;
        Serial.println("Back to normal polling interval!");
        break;
      case 'p':
        reading_interval = intervalPow2(Hid.pollingInterval ? Hid.pollingInterval : xbox.pollingInterval);
        Hid.overrideInterval = reading_interval;
        xbox.overrideInterval = reading_interval;
        Serial.print("Override polling interval to ");
        Serial.print(reading_interval);
        Serial.print("ms!");
        break;
      case 't':
        button = false;
        total = 0;
        digitalWrite(BUTTON_PIN, button);
        time = 0;
        Serial.println("Launching test");
        randomSeed(analogRead(0));
        nextchange = micros() + random(50, 70)*1000 + random(0, 250)*4;
        break;
      case 's':
        reading_interval = Serial.parseInt();
        Hid.overrideInterval = reading_interval;
        xbox.overrideInterval = reading_interval;
        Serial.print("Override polling to ");
        Serial.print(reading_interval);
        Serial.println("ms!");
        break;
    }
  }
  
  Usb.Task();
  looptime = micros() - current_time;
#endif
  
#endif

}


#endif
