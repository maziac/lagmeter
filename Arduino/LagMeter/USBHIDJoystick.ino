#include "src/usb/modifiedhiduniversal.h"

// Max values to observe.
#define MAX_VALUES  64


class JoystickReportParser : public HIDReportParser {
protected:
  uint8_t lastReportedValues[MAX_VALUES] = {0};
  uint16_t counts[MAX_VALUES];
  uint8_t measureIndex = 0;
  uint8_t idleValues[MAX_VALUES] = {}; // TODO: REMOVE
  unsigned long startTime = 0;
  const uint8_t startIndex = 0; //5;
  bool calibrationMode = false;

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


  /**
   * Sets the parse mode.
   * @param calib true = calibration mode: counts the number of changes per byte
   * false = sets joystickButtonPressed if the byte with the right index changed.
   */
  void JoystickReportParser::SetModeCalib(bool calib) {
    calibrationMode = calib;
    if(calibrationMode) {
      // Calibration mode
      memset(counts, 0, sizeof(uint16_t)*MAX_VALUES);
      measureIndex = 0;
    }
    else {
      // Normal measurement mode
      // Calculate max count index
      uint16_t maxCount = 0;
      measureIndex = 0;
      for (uint8_t i = 0; i < MAX_VALUES; i++)
      {
        if (counts[i] > maxCount)
        {
          maxCount = counts[i];
          measureIndex = i;
        }
      }
    }
  }


  /**
   * Called whenever a USB packet has been received.
   */
  void JoystickReportParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t* buf)
  {
    if(calibrationMode)
      ParseCalib(len, buf);
    else
      ParseMeasure(len, buf);
  }


  /**
   * Parses for calibration.
   * For each byte (index) it counts the number of changes.
   * At the end the index with the highest number is used for ParseMeasurement().
   */
  void JoystickReportParser::ParseCalib(uint8_t len, uint8_t* buf)
  {
#if 0
    // Print
    for(uint8_t i=0; i<len; i++) {
      SerialPrintHex<uint8_t>(buf[i]);
      Serial.print(F(", "));
    }
    Serial.println();
#endif

    // Safety check
    uint8_t count = min(MAX_VALUES, len);

    // Check and copy
    for (uint8_t i = 0; i < count; i++)
    {
      if (buf[i] != lastReportedValues[i])
      {
        lastReportedValues[i] = buf[i];
        counts[i]++;  // Increase count
      }
      //Serial.print(buf[i]);
      //Serial.print(F(", "));
    }
    //Serial.println();

#if 0
    Serial.println("Idle values");
    for (uint8_t i = 0; i < count; i++)
    {
      Serial.print(idleValues[i]);
      Serial.print(F(", "));
    }
    Serial.println();
#endif
  }


  /**
   * Parses for mesurement.
   * The byte with index, calibIndex, is checked for changes.
   */
  void JoystickReportParser::ParseMeasure(uint8_t len, uint8_t* buf)
  {
#if 0
    // Print
    for(uint8_t i=0; i<len; i++) {
      SerialPrintHex<uint8_t>(buf[i]);
      Serial.print(F(", "));
    }
    Serial.println();
#endif

    // Suppress for some time (1s)
    unsigned long time = millis();
    // Serial.print(time);
    // Serial.print(F(", "));
    // Serial.print(startTime);
    if (startTime == 0)
      startTime = time;
    if (time > startTime + 1000)
      joystickButtonChanged = true;
    // Serial.print(F(", "));
    // Serial.println(joystickButtonChanged);

    // Safety check
    if (measureIndex >= len) {
      //joystickButtonPressed = false;
      return;
    }

    // Check if "button press"
    joystickButtonPressed = (buf[measureIndex] != lastReportedValues[measureIndex]);
    lastReportedValues[measureIndex] = buf[measureIndex];

    Serial.print("joystickButtonPressed = ");
    Serial.println(joystickButtonPressed);
  }
};

JoystickReportParser HidJoyParser;



class UsbHidJoystick : public ModifiedHIDUniversal {
protected:
  String epPollIntervalsString;

public:
  UsbHidJoystick(USB* p) : ModifiedHIDUniversal (p) {};

  virtual uint8_t Init (uint8_t parent, uint8_t port, bool lowspeed) {
    uint8_t res = ModifiedHIDUniversal::Init(parent, port, lowspeed);
#if 0
    Serial.println("UsbHidJoystick::Init done.");
    Serial.print("Poll interval: ");
    Serial.print(epPollIntervalsString);
    Serial.print(" ms -> ");
    Serial.print(pollInterval);
    Serial.println(" ms");
#endif
    usedPollInterval = pollInterval;
    usbMode = true;
    if (!SetReportParser(0, &HidJoyParser)) {
      Serial.println("SetReportParser problem.");
      Error(F("Error:"), F("SetReportParser!!!"));
    }
    return res;
  }

  virtual uint8_t Release () {
    uint8_t res = ModifiedHIDUniversal::Release();
    Serial.println("UsbHidJoystick::Release done.");
    usbMode = false;
    return res;
  }

  // Sets (Overrides) the poll interval.
  void setPollInterval(int interval) {
    pollInterval = interval;
    usedPollInterval = pollInterval;
  }

  virtual void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR* ep) {
    ModifiedHIDUniversal::EndpointXtract(conf, iface, alt, proto, ep);
#if 0
    Serial.print("UsbHidJoystick::EndpointXtract done. Interface ");
    Serial.print(iface);
    Serial.print(", ");
    Serial.print(ep->bInterval);
    Serial.println(" ms.");
#endif
    if(epPollIntervalsString.length() != 0)
      epPollIntervalsString += ", ";
    epPollIntervalsString += ep->bInterval;
  }
};



// Create the HID instances.
UsbHidJoystick Hid(&Usb);



// Sets the idle values for the joystick. I.e. no button pressed.
void setHidIdleValues() {
  HidJoyParser.setIdleValues();
}


// Sets (Overrides) the poll interval.
void setPollInterval(int pollInterval) {
  Hid.setPollInterval(pollInterval);
  Usb.Task();
  Usb.Task();
}
