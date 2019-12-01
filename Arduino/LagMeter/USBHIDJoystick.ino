#include <usbhid.h>
//#include <hiduniversal.h>
#include "src/usb/modifiedhiduniversal.h"
#include <usbhub.h>
//#include <MemoryUsage.h>


// Max values to observe.
#define MAX_VALUES  64


class JoystickReportParser : public HIDReportParser {
protected:
  uint8_t lastReportedValues[MAX_VALUES] = {};
  uint8_t idleValues[MAX_VALUES] = {};
  unsigned long startTime = 0;

public:
  JoystickReportParser::JoystickReportParser() {
  }

  // Store the last reported values as Idle values.
  // Any change to these values is recognized as "button press".
  void setIdleValues() {
    for(uint8_t i=0; i<MAX_VALUES; i++) {
      idleValues[i] = lastReportedValues[i];
    }
    joystickButtonPressed = false;
  }

  void JoystickReportParser::Parse(USBHID* hid, bool is_rpt_id, uint8_t len, uint8_t* buf) {
    /*
    // Print
    for(uint8_t i=0; i<len; i++) {
      SerialPrintHex<uint8_t>(buf[i]); 
      Serial.print(F(", "));
    }
    Serial.println();
    */
    
    // Check if changed
    uint8_t count = min(MAX_VALUES, len);
    bool match = true;
    // Check and copy
    for(uint8_t i=0; i<count; i++) {
      if(buf[i] != lastReportedValues[i]) {
        match = false;
        lastReportedValues[i] = buf[i];
      }
    }

    // Suppress for some time (1s)
    unsigned long time = millis();
    //Serial.print(time);
    //Serial.print(F(", "));
    //Serial.print(startTime);
    if(startTime == 0)
      startTime = time;
    if(time > startTime+1000)
      joystickButtonChanged = true;
    //Serial.print(F(", "));
    //Serial.println(joystickButtonChanged);
    
    // Values changed. Check if "button press"
    for(uint8_t i=0; i<count; i++) {
      if(idleValues[i] != lastReportedValues[i]) {
        // Some button was pressed.
        joystickButtonPressed = true;
        //Serial.println("joystickButtonPressed = true");
        return;
      }
    }

    // Idle
    joystickButtonPressed = false;
    //Serial.println("joystickButtonPressed = false");
  }
};

JoystickReportParser HidJoyParser;



class UsbHidJoystick : public ModifiedHIDUniversal {
public:
  uint8_t pollIntervalCopy; // The real variable is private.
  
  UsbHidJoystick(USB* p) : ModifiedHIDUniversal (p), pollIntervalCopy(0) {};
  
  virtual uint8_t Init (uint8_t parent, uint8_t port, bool lowspeed) {
    uint8_t res = ModifiedHIDUniversal::Init(parent, port, lowspeed);
    Serial.println("UsbHidJoystick::Init done.");
    Serial.print("Poll interval: ");
    Serial.print(pollIntervalCopy);
    Serial.println(" ms.");
    requestedPollInterval = pollIntervalCopy;
    usbMode = true;
    return res;
  }
  
  virtual uint8_t Release () {
    uint8_t res = ModifiedHIDUniversal::Release();
    Serial.println("UsbHidJoystick::Release done.");
    usbMode = false;
    return res;
  }


  virtual void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR* ep) {
    ModifiedHIDUniversal::EndpointXtract(conf, iface, alt, proto, ep);
    if(pollIntervalCopy == 0)
      pollIntervalCopy = ep->bInterval; // Use the first found interval
    Serial.print("UsbHidJoystick::EndpointXtract done. Interface ");
    Serial.print(iface);
    Serial.print(", ");
    Serial.print(ep->bInterval);
    Serial.println(" ms.");
  }
};



// Create the HID instances.
UsbHidJoystick Hid(&Usb);



// Initialize. Called from 'setup'.
void USBHIDInit() {
  if (!Hid.SetReportParser(0, &HidJoyParser)) { 
    Serial.println("SetReportParser problem.");
    Error(F("Error:"), F("SetReportParser!!!"));
  }
}



// Sets the idle values for the joystick. I.e. no button pressed.
void setHidIdleValues() {
  HidJoyParser.setIdleValues();
}
