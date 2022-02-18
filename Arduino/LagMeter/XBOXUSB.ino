#include "src/usb/modifiedXBOXUSB.h"



class XBOXUSBjoystick : public ModifiedXBOXUSB {
public:
	XBOXUSBjoystick(USB* p) : ModifiedXBOXUSB(p) {};

	virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed) override {
		uint8_t res = ModifiedXBOXUSB::Init(parent, port, lowspeed);
#if 0
		Serial.println("XBOXUSBjoystick::Init.");
#endif
		usbMode = true;
		xboxMode = true;
		return res;
	}

	virtual uint8_t Release() override {
		uint8_t res = ModifiedXBOXUSB::Release();
#if 0
		Serial.println("XBOXUSBjoystick::Release done.");
#endif
		usbMode = false;
		xboxMode = false;
		return res;
	}

	virtual void readReport() override {
		uint32_t OldBtnState = OldButtonState;
		ModifiedXBOXUSB::readReport();
		if (ButtonState != OldBtnState) {
			// Button changed
			joystickButtonChanged = true;
#if 0
			Serial.println("XBOXUSBjoystick::readReport(): Button changed");
			Serial.print("ButtonState=");
			Serial.println(ButtonState);
			Serial.print("OldButtonState=");
			Serial.println(OldButtonState);
#endif
		}
		joystickButtonPressed = (ButtonState != 0);
	}
};


XBOXUSBjoystick Xbox(&Usb);
