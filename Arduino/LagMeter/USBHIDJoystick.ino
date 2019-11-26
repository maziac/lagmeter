#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>
#include <MemoryUsage.h>


// Max values to observe.
#define MAX_VALUES  64


class JoystickReportParser : public HIDReportParser {
protected:
  uint8_t lastReportedValues[MAX_VALUES] = {};
  uint8_t idleValues[MAX_VALUES] = {};
  unsigned long startTime;

public:
  JoystickReportParser::JoystickReportParser() {
    reset();
  }

  void reset() {
    startTime = 0;
    joystickButtonPressed = false;
    joystickButtonChanged = false;
    Serial.println("joystickButtonPressed = false");
  }

  // Store the last reported values as Idle values.
  // Any change to these values is recognized as "button press".
  void setIdleValues() {
    for(uint8_t i=0; i<MAX_VALUES; i++) {
      idleValues[i] = lastReportedValues[i];
    }
  }

  void JoystickReportParser::Parse(USBHID* hid, bool is_rpt_id, uint8_t len, uint8_t* buf) {
    // Print
    for(uint8_t i=0; i<len; i++) {
      SerialPrintHex<uint8_t>(buf[i]); 
      Serial.print(F(", "));
    }
    Serial.println();
    
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

    // Return if no change
    if(match)
      return;

    // Suppress for some time (1s)
    unsigned long time = millis();
    Serial.print(time);
    Serial.print(F(", "));
    Serial.print(startTime);
    if(startTime == 0)
      startTime = time;
    if(time > startTime+1000)
      joystickButtonChanged = true;
    Serial.print(F(", "));
    Serial.println(joystickButtonChanged);
    
    // Values changed. Check if "button press"
    for(uint8_t i=0; i<count; i++) {
      if(idleValues[i] != lastReportedValues[i]) {
        // Some button was pressed.
        joystickButtonPressed = true;
        Serial.println("joystickButtonPressed = true");
        return;
      }
    }

    // Idle
    joystickButtonPressed = false;
    Serial.println("joystickButtonPressed = false");
   }
};

JoystickReportParser HidJoyParser;



class UsbHidJoystick : public HIDUniversal {
public:
  uint8_t pollIntervalCopy; // The real variable is private.
  
  UsbHidJoystick(USB* p) : HIDUniversal (p), pollIntervalCopy(0) {};
  
  virtual uint8_t Init (uint8_t parent, uint8_t port, bool lowspeed) {
    uint8_t res = HIDUniversal::Init(parent, port, lowspeed);
    Serial.println("UsbHidJoystick::Init done.");
    Serial.print("Poll interval: ");
    Serial.print(pollIntervalCopy);
    Serial.println(" ms.");
    MEMORY_PRINT_START
    MEMORY_PRINT_HEAPSTART
    MEMORY_PRINT_HEAPEND
    MEMORY_PRINT_STACKSTART
    MEMORY_PRINT_END
    MEMORY_PRINT_HEAPSIZE
    FREERAM_PRINT
    requestedPollInterval = pollIntervalCopy;
    usbMode = true;
    HidJoyParser.reset();
    return res;
  }
  
  virtual uint8_t Release () {
    uint8_t res = HIDUniversal::Release();
    Serial.println("UsbHidJoystick::Release done.");
    usbMode = false;
    HidJoyParser.reset();
    pollIntervalCopy = 0;
    requestedPollInterval = 0;
    return res;
  }


  virtual void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR* ep) {
#if 0
    Serial.print("UsbHidJoystick::EndpointXtract: ");
    Serial.print(iface);
    Serial.print(", ");
    Serial.print(ep->bInterval);
    ((USB_ENDPOINT_DESCRIPTOR*)ep)->bInterval = 4;  // Funktioniert nicht
    HIDUniversal::EndpointXtract(conf, iface, alt, proto, ep);
    Serial.print(", ");
    Serial.println(ep->bInterval);
#else
    HIDUniversal::EndpointXtract(conf, iface, alt, proto, ep);
    if(pollIntervalCopy == 0)
      pollIntervalCopy = ep->bInterval; // Use the first found interval
    Serial.print("UsbHidJoystick::EndpointXtract done. Interface ");
    Serial.print(iface);
    Serial.print(", ");
    Serial.print(ep->bInterval);
    Serial.println(" ms.");
#endif
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
