/*
 * @WydD - 2019-01
 * This file was extracted by XBOXUSB.h in the original USB Host library
 * Most of the configuration code was kept and irrelevant parts were removed for clarity.
 * Some alterations were done to expose more internal details to subclasses.
 */

/* Copyright (C) 2012 Kristian Lauszus, TKJ Electronics. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com
 */

#ifndef _XInputDriver_h_
#define _XInputDriver_h_

#include <Usb.h>
#include <usbhid.h>
#include <xboxEnums.h>

/* Data Xbox 360 taken from descriptors */
#define EP_MAXPKTSIZE       32 // max size for data via USB

/* Names we give to the 3 Xbox360 pipes */
#define XBOX_CONTROL_PIPE    0
#define XBOX_INPUT_PIPE      1
#define XBOX_OUTPUT_PIPE     2

#define XBOX_REPORT_BUFFER_SIZE 14 // Size of the input report buffer

#define XBOX_MAX_ENDPOINTS   3

/** This class implements support for a Xbox wired controller via USB. */
class XInputDriver : public USBDeviceConfig {
public:
        /**
         * Constructor for the XInputDriver class.
         * @param  pUsb   Pointer to USB class instance.
         */
        XInputDriver(USB *pUsb);

        /** @name USBDeviceConfig implementation */
        /**
         * Initialize the Xbox Controller.
         * @param  parent   Hub number.
         * @param  port     Port number on the hub.
         * @param  lowspeed Speed of the device.
         * @return          0 on success.
         */
        uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
        /**
         * Release the USB device.
         * @return 0 on success.
         */
        uint8_t Release();

        /**
         * Get the device address.
         * @return The device address.
         */
        virtual uint8_t GetAddress() {
                return bAddress;
        };
        
        /**
         * Turn rumble on.
         * @param lValue     Left motor (big weight) inside the controller.
         * @param rValue     Right motor (small weight) inside the controller.
         */
        void setRumbleOn(uint8_t lValue, uint8_t rValue);
        /**
         * Set LED value. Without using the ::LEDEnum or ::LEDModeEnum.
         * @param value      See:
         * setLedOff(), setLedOn(LEDEnum l),
         * setLedBlink(LEDEnum l), and setLedMode(LEDModeEnum lm).
         */
        void setLedRaw(uint8_t value);

        /** Turn all LEDs off the controller. */
        void setLedOff() {
                setLedRaw(0);
        };
        /**
         * Turn on a LED by using ::LEDEnum.
         * @param l          ::OFF, ::LED1, ::LED2, ::LED3 and ::LED4 is supported by the Xbox controller.
         */
        void setLedOn(LEDEnum l);
        /**
         * Turn on a LED by using ::LEDEnum.
         * @param l          ::ALL, ::LED1, ::LED2, ::LED3 and ::LED4 is supported by the Xbox controller.
         */
        void setLedBlink(LEDEnum l);
        /**
         * Used to set special LED modes supported by the Xbox controller.
         * @param lm         See ::LEDModeEnum.
         */
        void setLedMode(LEDModeEnum lm);
        void startDevice();
        void initRumble();

        /** True if a Xbox 360 controller is connected. */
        bool Xbox360Connected;

protected:
        /** Pointer to USB class instance. */
        USB *pUsb;
        /** Device address. */
        uint8_t bAddress;
        /** Endpoint poll interval. */
        uint8_t bInterval;
        bool isXboxOne;
        uint8_t cmdCounter = 0;
        /** Endpoint info structure. */
        EpInfo epInfo[XBOX_MAX_ENDPOINTS];
        uint16_t PID, VID; // PID and VID of connected device

private:
        uint8_t readBuf[EP_MAXPKTSIZE]; // General purpose buffer for input data
        uint8_t writeBuf[8]; // General purpose buffer for output data

        /* Private commands */
        void XboxCommand(uint8_t* data, uint16_t nbytes);
};
#endif
