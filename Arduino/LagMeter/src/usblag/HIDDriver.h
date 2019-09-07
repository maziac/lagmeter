/*
 * @WydD - 2019-01
 * This file was extracted by hiduniversal.h in the original USB Host library
 * Most of the configuration code was kept and irrelevant parts were removed for clarity.
 * Some alterations were done to expose more internal details to subclasses.
 */

/* Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
 */

#if !defined(__HIDDriver_H__)
#define __HIDDriver_H__

#include "usbhid.h"
//#include "hidescriptorparser.h"

#define HID_BUFFER_LENGTH 64

class HIDDriver : public USBHID {

        struct HIDInterface {
                struct {
                        uint8_t bmInterface : 3;
                        uint8_t bmAltSet : 3;
                        uint8_t bmProtocol : 2;
                };
                uint8_t epIndex[maxEpPerInterface];
        };

        void Initialize();
        HIDInterface* FindInterface(uint8_t iface, uint8_t alt, uint8_t proto);
        uint8_t bNumEP; // total number of EP in the configuration
        uint8_t bConfNum; // configuration number
protected:
        EpInfo epInfo[totalEndpoints];
        HIDInterface hidInterfaces[maxHidInterfaces];

        uint16_t PID, VID; // PID and VID of connected device

public:
        HIDDriver(USB *p);
        uint8_t bNumIface; // number of interfaces in the configuration

        uint8_t pollInterval;
        // USBDeviceConfig implementation
        uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
        uint8_t Release();

        virtual uint8_t GetAddress() {
                return bAddress;
        };

        // UsbConfigXtracter implementation
        void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *ep);
};

#endif // __HIDDriver_H__
