#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>

struct GamePadEventData {
        uint8_t X, Y, Z1, Z2, Rz;
};

class JoystickEvents {
public:
    
    void JoystickEvents::OnGamePadChanged(const GamePadEventData *evt) {
            Serial.print("X1: ");
            PrintHex<uint8_t > (evt->X, 0x80);
            Serial.print("\tY1: ");
            PrintHex<uint8_t > (evt->Y, 0x80);
            Serial.print("\tX2: ");
            PrintHex<uint8_t > (evt->Z1, 0x80);
            Serial.print("\tY2: ");
            PrintHex<uint8_t > (evt->Z2, 0x80);
            Serial.print("\tRz: ");
            PrintHex<uint8_t > (evt->Rz, 0x80);
            Serial.println("");
    }
    
    void JoystickEvents::OnHatSwitch(uint8_t hat) {
            Serial.print("Hat Switch: ");
            PrintHex<uint8_t > (hat, 0x80);
            Serial.println("");
    }
    
    void JoystickEvents::OnButtonUp(uint8_t but_id) {
            Serial.print("Up: ");
            Serial.println(but_id, DEC);
    }
    
    void JoystickEvents::OnButtonDn(uint8_t but_id) {
            Serial.print("Dn: ");
            Serial.println(but_id, DEC);
    }
};

#define RPT_GEMEPAD_LEN    5

class JoystickReportParser : public HIDReportParser {
        JoystickEvents *joyEvents;

        uint8_t oldPad[RPT_GEMEPAD_LEN];
        uint8_t oldHat;
        uint16_t oldButtons;

public:

  JoystickReportParser::JoystickReportParser(JoystickEvents *evt) :
  joyEvents(evt),
  oldHat(0xDE),
  oldButtons(0) {
          for (uint8_t i = 0; i < RPT_GEMEPAD_LEN; i++)
                  oldPad[i] = 0xD;
  }
  
  void JoystickReportParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
          bool match = true;
  
          // Checking if there are changes in report since the method was last called
          for (uint8_t i = 0; i < RPT_GEMEPAD_LEN; i++)
                  if (buf[i] != oldPad[i]) {
                          match = false;
                          break;
                  }
  
          // Calling Game Pad event handler
          if (!match && joyEvents) {
                  joyEvents->OnGamePadChanged((const GamePadEventData*)buf);
  
                  for (uint8_t i = 0; i < RPT_GEMEPAD_LEN; i++) oldPad[i] = buf[i];
          }
  
          uint8_t hat = (buf[5] & 0xF);
  
          // Calling Hat Switch event handler
          if (hat != oldHat && joyEvents) {
                  joyEvents->OnHatSwitch(hat);
                  oldHat = hat;
          }
  
          uint16_t buttons = (0x0000 | buf[6]);
          buttons <<= 4;
          buttons |= (buf[5] >> 4);
          uint16_t changes = (buttons ^ oldButtons);
  
          // Calling Button Event Handler for every button changed
          if (changes) {
                  for (uint8_t i = 0; i < 0x0C; i++) {
                          uint16_t mask = (0x0001 << i);
  
                          if (((mask & changes) > 0) && joyEvents) {
                                  if ((buttons & mask) > 0)
                                          joyEvents->OnButtonDn(i + 1);
                                  else
                                          joyEvents->OnButtonUp(i + 1);
                          }
                  }
                  oldButtons = buttons;
          }
  }
};




class UsbHidJoystick : public HIDUniversal {
public:
  uint8_t pollIntervalCopy; // The real variable is private.
  
  UsbHidJoystick(USB* p) : HIDUniversal (p) {};
  
  virtual uint8_t Init (uint8_t parent, uint8_t port, bool lowspeed) {
    uint8_t res = HIDUniversal::Init(parent, port, lowspeed);
    Serial.println("UsbHidJoystick::Init done.");
    Serial.print("Poll interval: ");
    Serial.print(pollIntervalCopy);
    Serial.println(" ms.");
    return res;
  }
  
  virtual uint8_t Release () {
    uint8_t res = HIDUniversal::Release();
    Serial.println("UsbHidJoystick::Release done.");
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
    pollIntervalCopy = ep->bInterval;
    HIDUniversal::EndpointXtract(conf, iface, alt, proto, ep);
    Serial.println("UsbHidJoystick::EndpointXtract done.");
#endif
  }
};



// Create the HID instances.
UsbHidJoystick Hid(&Usb);
JoystickEvents JoyEvents;
JoystickReportParser Joy(&JoyEvents);



// Initialize. Called from 'setup'.
void USBHIDInit() {
  if (!Hid.SetReportParser(0, &Joy))
    ErrorMessage<uint8_t > (PSTR("SetReportParser"), 1);
}
