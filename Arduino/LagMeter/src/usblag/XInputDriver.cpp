/*
 * @WydD - 2019-01
 * This file was extracted by XBOXUSB.cpp in the original USB Host library
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

#include "XInputDriver.h"
#include "desc.h"
// To enable serial debugging see "settings.h"
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report send by the Xbox 360 Controller

XInputDriver::XInputDriver(USB *p) :
pUsb(p), // pointer to USB class instance - mandatory
bAddress(0) {
        for(uint8_t i = 0; i < XBOX_MAX_ENDPOINTS; i++) {
                epInfo[i].epAddr = 0;
                epInfo[i].maxPktSize = (i) ? 0 : 8;
                epInfo[i].bmSndToggle = 0;
                epInfo[i].bmRcvToggle = 0;
                epInfo[i].bmNakPower = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
        }

        if(pUsb) // register in USB subsystem
                pUsb->RegisterDeviceClass(this); //set devConfig[] entry
}


#define LOBYTE(x) ((char*)(&(x)))[0]
#define HIBYTE(x) ((char*)(&(x)))[1]
#define BUFSIZE 256    //buffer size
uint8_t testConfigurationXinput(USB *p, uint8_t addr, uint8_t conf, bool *isXboxOne)
{
  uint8_t buf[ BUFSIZE ];
  uint8_t* buf_ptr = buf;
  uint8_t rcode;
  uint8_t descr_length;
  uint8_t descr_type;
  uint16_t total_length;
  rcode = p->getConfDescr( addr, 0, 4, conf, buf );  //get total length
  LOBYTE( total_length ) = buf[ 2 ];
  HIBYTE( total_length ) = buf[ 3 ];
  if ( total_length > 256 ) {   //check if total length is larger than buffer
    total_length = 256;
  }
  rcode = p->getConfDescr( addr, 0, total_length, conf, buf ); //get the whole descriptor
  bool found = false;
  while ( buf_ptr < buf + total_length ) { //parsing descriptors
    descr_length = *( buf_ptr );
    descr_type = *( buf_ptr + 1 );
    if (descr_type == USB_DESCRIPTOR_INTERFACE) {
      USB_INTERFACE_DESCRIPTOR* intf_ptr = ( USB_INTERFACE_DESCRIPTOR* )buf_ptr;
      Serial.print("Found descriptor ");
      print_hex(intf_ptr->bInterfaceClass, 8);
      Serial.print(" ");
      print_hex(intf_ptr->bInterfaceProtocol, 8);
      Serial.print("\n");
      if (intf_ptr->bInterfaceClass == 0xFF && (
            intf_ptr->bInterfaceSubClass == 0x5D && intf_ptr->bInterfaceProtocol == 0x01 ||
            intf_ptr->bInterfaceSubClass == 0x42 && intf_ptr->bInterfaceProtocol == 0x00 ||
            intf_ptr->bInterfaceSubClass == 0x47 && intf_ptr->bInterfaceProtocol == 0xD0
      )) {
        *isXboxOne = intf_ptr->bInterfaceSubClass == 0x47;
        found = true;
      }
    } else if (found && descr_type == USB_DESCRIPTOR_ENDPOINT) {
      USB_ENDPOINT_DESCRIPTOR* ep_ptr = ( USB_ENDPOINT_DESCRIPTOR* )buf_ptr;
      if (ep_ptr->bEndpointAddress == 0x81) {
        return ep_ptr->bInterval;
      }
    }
    buf_ptr = ( buf_ptr + descr_length );    //advance buffer pointer
  }
  return 0;
}
uint8_t xinputDescriptorsTest(USB *p, UsbDevice *pdev, USB_DEVICE_DESCRIPTOR* desc, bool *isXboxOne)
{
  uint8_t addr = pdev->address.devAddress;
  uint8_t num_conf = desc->bNumConfigurations;
  for (int i = 0; i < num_conf; i++)
  {
    uint8_t rcode = testConfigurationXinput(p, addr, i, isXboxOne);
    if (rcode > 0) return rcode;
  }
  return 0;
}

uint8_t XInputDriver::Init(uint8_t parent, uint8_t port, bool lowspeed) {
        uint8_t buf[sizeof (USB_DEVICE_DESCRIPTOR)];
        USB_DEVICE_DESCRIPTOR * udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR*>(buf);
        uint8_t rcode;
        UsbDevice *p = NULL;
        EpInfo *oldep_ptr = NULL;

        // get memory address of USB device address pool
        AddressPool &addrPool = pUsb->GetAddressPool();
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nXInputDriver Init"), 0x80);
#endif
        // check if address has already been assigned to an instance
        if(bAddress) {
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nAddress in use"), 0x80);
#endif
                return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
        }

        // Get pointer to pseudo device with address 0 assigned
        p = addrPool.GetUsbDevicePtr(0);

        if(!p) {
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nAddress not found"), 0x80);
#endif
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
        }

        if(!p->epinfo) {
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nepinfo is null"), 0x80);
#endif
                return USB_ERROR_EPINFO_IS_NULL;
        }

        // Save old pointer to EP_RECORD of address 0
        oldep_ptr = p->epinfo;

        // Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
        p->epinfo = epInfo;

        p->lowspeed = lowspeed;

        // Get device descriptor
        rcode = pUsb->getDevDescr(0, 0, sizeof (USB_DEVICE_DESCRIPTOR), (uint8_t*)buf); // Get device descriptor - addr, ep, nbytes, data
        // Restore p->epinfo
        p->epinfo = oldep_ptr;

        if(rcode)
                goto FailGetDevDescr;

        VID = udd->idVendor;
        PID = udd->idProduct;
        bInterval = xinputDescriptorsTest(pUsb, p, udd, &isXboxOne);
        if (!bInterval) {
            goto FailUnknownDevice;
        }

        // Allocate new address according to device class
        bAddress = addrPool.AllocAddress(parent, false, port);

        if(!bAddress)
                return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;

        // Extract Max Packet Size from device descriptor
        epInfo[0].maxPktSize = udd->bMaxPacketSize0;

        // Assign new address to the device
        rcode = pUsb->setAddr(0, 0, bAddress);
        if(rcode) {
                p->lowspeed = false;
                addrPool.FreeAddress(bAddress);
                bAddress = 0;
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nsetAddr: "), 0x80);
                D_PrintHex<uint8_t > (rcode, 0x80);
#endif
                return rcode;
        }
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nAddr: "), 0x80);
        D_PrintHex<uint8_t > (bAddress, 0x80);
#endif
        //delay(300); // Spec says you should wait at least 200ms

        p->lowspeed = false;

        //get pointer to assigned address record
        p = addrPool.GetUsbDevicePtr(bAddress);
        if(!p)
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

        p->lowspeed = lowspeed;

        // Assign epInfo to epinfo pointer - only EP0 is known
        rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);
        if(rcode)
                goto FailSetDevTblEntry;

        /* The application will work in reduced host mode, so we can save program and data
           memory space. After verifying the VID we will use known values for the
           configuration values for device, interface, endpoints and HID for the XBOX360 Controllers */

        /* Initialize data structures for endpoints of device */
        epInfo[ XBOX_INPUT_PIPE ].epAddr = 0x01; // XBOX 360 report endpoint
        epInfo[ XBOX_INPUT_PIPE ].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
        epInfo[ XBOX_INPUT_PIPE ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
        epInfo[ XBOX_INPUT_PIPE ].maxPktSize = EP_MAXPKTSIZE;
        epInfo[ XBOX_INPUT_PIPE ].bmSndToggle = 0;
        epInfo[ XBOX_INPUT_PIPE ].bmRcvToggle = 0;
        epInfo[ XBOX_OUTPUT_PIPE ].epAddr = 0x02; // XBOX 360 output endpoint
        epInfo[ XBOX_OUTPUT_PIPE ].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
        epInfo[ XBOX_OUTPUT_PIPE ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
        epInfo[ XBOX_OUTPUT_PIPE ].maxPktSize = EP_MAXPKTSIZE;
        epInfo[ XBOX_OUTPUT_PIPE ].bmSndToggle = 0;
        epInfo[ XBOX_OUTPUT_PIPE ].bmRcvToggle = 0;

        rcode = pUsb->setEpInfoEntry(bAddress, 3, epInfo);
        if(rcode)
                goto FailSetDevTblEntry;

        delay(200); // Give time for address change

        rcode = pUsb->setConf(bAddress, epInfo[ XBOX_CONTROL_PIPE ].epAddr, 1);
        if(rcode)
                goto FailSetConfDescr;

#ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nXbox 360 Controller Connected\r\n"), 0x80);
#endif

        getallstrdescr(pUsb, udd, bAddress);
        setLedOn(static_cast<LEDEnum>(LED1));
        Xbox360Connected = true;
        
        return 0; // Successful configuration

        /* Diagnostic messages */
FailGetDevDescr:
#ifdef DEBUG_USB_HOST
        NotifyFailGetDevDescr();
        goto Fail;
#endif

FailSetDevTblEntry:
#ifdef DEBUG_USB_HOST
        NotifyFailSetDevTblEntry();
        goto Fail;
#endif

FailSetConfDescr:
#ifdef DEBUG_USB_HOST
        NotifyFailSetConfDescr();
#endif
        goto Fail;

FailUnknownDevice:
#ifdef DEBUG_USB_HOST
        NotifyFailUnknownDevice(VID, PID);
#endif
        rcode = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;

Fail:
#ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nXbox 360 Init Failed, error code: "), 0x80);
        NotifyFail(rcode);
#endif
        Release();
        return rcode;
}

/* Performs a cleanup after failed Init() attempt */
uint8_t XInputDriver::Release() {
        Xbox360Connected = false;
        pUsb->GetAddressPool().FreeAddress(bAddress);
        bAddress = 0;
        return 0;
}

/* Xbox Controller commands */
void XInputDriver::XboxCommand(uint8_t* data, uint16_t nbytes) {
        if (isXboxOne) {
              data[2] = cmdCounter++; // Increment the output command counter
        }
        pUsb->outTransfer(bAddress, epInfo[ XBOX_OUTPUT_PIPE ].epAddr, nbytes, data);
}
void XInputDriver::startDevice() {
        writeBuf[0] = 0x05;
        writeBuf[1] = 0x20;
        writeBuf[2] = 0x00;
        writeBuf[3] = 0x01;
        writeBuf[4] = 0x00;

        XboxCommand(writeBuf, 5);
        
        // A short buzz to show the controller is active
        uint8_t writeBuf[13];

}
void XInputDriver::initRumble() {
        // Activate rumble
        writeBuf[0] = 0x09;
        writeBuf[1] = 0x00;
        // Byte 2 is set in "XboxCommand"

        // Single rumble effect
        writeBuf[3] = 0x09; // Substructure (what substructure rest of this packet has)
        writeBuf[4] = 0x00; // Mode
        writeBuf[5] = 0x0F; // Rumble mask (what motors are activated) (0000 lT rT L R)
        writeBuf[6] = 0x04; // lT force
        writeBuf[7] = 0x04; // rT force
        writeBuf[8] = 0x20; // L force
        writeBuf[9] = 0x20; // R force
        writeBuf[10] = 0x80; // Length of pulse
        writeBuf[11] = 0x00; // Off period
        writeBuf[12] = 0x00; // Repeat count
        XboxCommand(writeBuf, 13);
}

void XInputDriver::setLedRaw(uint8_t value) {
        writeBuf[0] = 0x01;
        writeBuf[1] = 0x03;
        writeBuf[2] = value;

        XboxCommand(writeBuf, 3);
}

void XInputDriver::setLedOn(LEDEnum led) {
        if(led == OFF)
                setLedRaw(0);
        else if(led != ALL) // All LEDs can't be on a the same time
                setLedRaw(pgm_read_byte(&XBOX_LEDS[(uint8_t)led]) + 4);
}

void XInputDriver::setLedBlink(LEDEnum led) {
        setLedRaw(pgm_read_byte(&XBOX_LEDS[(uint8_t)led]));
}

void XInputDriver::setLedMode(LEDModeEnum ledMode) { // This function is used to do some special LED stuff the controller supports
        setLedRaw((uint8_t)ledMode);
}

void XInputDriver::setRumbleOn(uint8_t lValue, uint8_t rValue) {
        writeBuf[0] = 0x00;
        writeBuf[1] = 0x08;
        writeBuf[2] = 0x00;
        writeBuf[3] = lValue; // big weight
        writeBuf[4] = rValue; // small weight
        writeBuf[5] = 0x00;
        writeBuf[6] = 0x00;
        writeBuf[7] = 0x00;

        XboxCommand(writeBuf, 8);
}
