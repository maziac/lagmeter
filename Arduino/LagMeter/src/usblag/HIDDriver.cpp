/*
 * @WydD - 2019-01
 * This file was extracted by hiduniversal.cpp in the original USB Host library
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

#include "HIDDriver.h"
#include "desc.h"

HIDDriver::HIDDriver(USB *p) :
USBHID(p),
pollInterval(0) {
        Initialize();

        if(pUsb)
                pUsb->RegisterDeviceClass(this);
}

void HIDDriver::Initialize() {
        for(uint8_t i = 0; i < maxHidInterfaces; i++) {
                hidInterfaces[i].bmInterface = 0;
                hidInterfaces[i].bmProtocol = 0;

                for(uint8_t j = 0; j < maxEpPerInterface; j++)
                        hidInterfaces[i].epIndex[j] = 0;
        }
        for(uint8_t i = 0; i < totalEndpoints; i++) {
                epInfo[i].epAddr = 0;
                epInfo[i].maxPktSize = (i) ? 0 : 8;
                epInfo[i].bmSndToggle = 0;
                epInfo[i].bmRcvToggle = 0;
                epInfo[i].bmNakPower = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
        }
        bNumEP = 1;
        bNumIface = 0;
        bConfNum = 0;
        pollInterval = 0;
}

uint8_t HIDDriver::Init(uint8_t parent, uint8_t port, bool lowspeed) {
        const uint8_t constBufSize = sizeof (USB_DEVICE_DESCRIPTOR);

        uint8_t buf[constBufSize];
        USB_DEVICE_DESCRIPTOR * udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR*>(buf);
        uint8_t rcode;
        UsbDevice *p = NULL;
        EpInfo *oldep_ptr = NULL;
        uint8_t len = 0;

        uint8_t num_of_conf; // number of configurations
        //uint8_t num_of_intf; // number of interfaces

        AddressPool &addrPool = pUsb->GetAddressPool();

        USBTRACE("HU Init\r\n");

        if(bAddress)
                return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;

        // Get pointer to pseudo device with address 0 assigned
        p = addrPool.GetUsbDevicePtr(0);

        if(!p)
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

        if(!p->epinfo) {
                USBTRACE("epinfo\r\n");
                return USB_ERROR_EPINFO_IS_NULL;
        }

        // Save old pointer to EP_RECORD of address 0
        oldep_ptr = p->epinfo;

        // Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
        p->epinfo = epInfo;

        p->lowspeed = lowspeed;

        // Get device descriptor
        rcode = pUsb->getDevDescr(0, 0, 8, (uint8_t*)buf);
        if(!rcode)
                len = (buf[0] > constBufSize) ? constBufSize : buf[0];

        if(rcode) {
                // Restore p->epinfo
                p->epinfo = oldep_ptr;

                goto FailGetDevDescr;
        }

        // Restore p->epinfo
        p->epinfo = oldep_ptr;

        // Allocate new address according to device class
        bAddress = addrPool.AllocAddress(parent, false, port);

        if(!bAddress)
                return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;

        // Extract Max Packet Size from the device descriptor
        epInfo[0].maxPktSize = udd->bMaxPacketSize0;

        // Assign new address to the device
        rcode = pUsb->setAddr(0, 0, bAddress);

        if(rcode) {
                p->lowspeed = false;
                addrPool.FreeAddress(bAddress);
                bAddress = 0;
                USBTRACE2("setAddr:", rcode);
                return rcode;
        }

        //delay(2); //per USB 2.0 sect.9.2.6.3

        USBTRACE2("Addr:", bAddress);

        p->lowspeed = false;

        p = addrPool.GetUsbDevicePtr(bAddress);

        if(!p)
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

        p->lowspeed = lowspeed;

        if(len)
                rcode = pUsb->getDevDescr(bAddress, 0, len, (uint8_t*)buf);

        if(rcode)
                goto FailGetDevDescr;

        VID = udd->idVendor; // Can be used by classes that inherits this class to check the VID and PID of the connected device
        PID = udd->idProduct;

        num_of_conf = udd->bNumConfigurations;

        // Assign epInfo to epinfo pointer
        rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);

        if(rcode)
                goto FailSetDevTblEntry;

        USBTRACE2("NC:", num_of_conf);

        for(uint8_t i = 0; i < num_of_conf; i++) {
                //HexDumper<USBReadParser, uint16_t, uint16_t> HexDump;
                ConfigDescParser<USB_CLASS_HID, 0, 0,
                        CP_MASK_COMPARE_CLASS> confDescrParser(this);

                //rcode = pUsb->getConfDescr(bAddress, 0, i, &HexDump);
                rcode = pUsb->getConfDescr(bAddress, 0, i, &confDescrParser);

                if(rcode)
                        goto FailGetConfDescr;

                if(bNumEP > 1)
                        break;
        } // for

        if(bNumEP < 2)
                return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;

        // Assign epInfo to epinfo pointer
        rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);

        USBTRACE2("Cnf:", bConfNum);

        // Set Configuration Value
        rcode = pUsb->setConf(bAddress, 0, bConfNum);

        if(rcode)
                goto FailSetConfDescr;

        for(uint8_t i = 0; i < bNumIface; i++) {
                if(hidInterfaces[i].epIndex[epInterruptInIndex] == 0)
                        continue;

                rcode = SetIdle(hidInterfaces[i].bmInterface, 0, 0);

                if(rcode && rcode != hrSTALL)
                        goto FailSetIdle;
        }

        USBTRACE("HU configured\r\n");

        getallstrdescr(pUsb, udd, bAddress);
        return 0;

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

FailGetConfDescr:
#ifdef DEBUG_USB_HOST
        NotifyFailGetConfDescr();
        goto Fail;
#endif

FailSetConfDescr:
#ifdef DEBUG_USB_HOST
        NotifyFailSetConfDescr();
        goto Fail;
#endif


FailSetIdle:
#ifdef DEBUG_USB_HOST
        USBTRACE("SetIdle:");
#endif

#ifdef DEBUG_USB_HOST
Fail:
        NotifyFail(rcode);
#endif
        Release();
        return rcode;
}

HIDDriver::HIDInterface* HIDDriver::FindInterface(uint8_t iface, uint8_t alt, uint8_t proto) {
        for(uint8_t i = 0; i < bNumIface && i < maxHidInterfaces; i++)
                if(hidInterfaces[i].bmInterface == iface && hidInterfaces[i].bmAltSet == alt
                        && hidInterfaces[i].bmProtocol == proto)
                        return hidInterfaces + i;
        return NULL;
}

void HIDDriver::EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *pep) {
        // If the first configuration satisfies, the others are not concidered.
        if(bNumEP > 1 && conf != bConfNum)
                return;

        //ErrorMessage<uint8_t>(PSTR("\r\nConf.Val"), conf);
        //ErrorMessage<uint8_t>(PSTR("Iface Num"), iface);
        //ErrorMessage<uint8_t>(PSTR("Alt.Set"), alt);

        bConfNum = conf;

        uint8_t index = 0;
        HIDInterface *piface = FindInterface(iface, alt, proto);

        // Fill in interface structure in case of new interface
        if(!piface) {
                piface = hidInterfaces + bNumIface;
                piface->bmInterface = iface;
                piface->bmAltSet = alt;
                piface->bmProtocol = proto;
                bNumIface++;
        }

        if((pep->bmAttributes & bmUSB_TRANSFER_TYPE) == USB_TRANSFER_TYPE_INTERRUPT)
                index = (pep->bEndpointAddress & 0x80) == 0x80 ? epInterruptInIndex : epInterruptOutIndex;

        if(index) {
                // Fill in the endpoint info structure
                epInfo[bNumEP].epAddr = (pep->bEndpointAddress & 0x0F);
                epInfo[bNumEP].maxPktSize = (uint8_t)pep->wMaxPacketSize;
                epInfo[bNumEP].bmSndToggle = 0;
                epInfo[bNumEP].bmRcvToggle = 0;
                epInfo[bNumEP].bmNakPower = USB_NAK_NOWAIT;

                // Fill in the endpoint index list
                piface->epIndex[index] = bNumEP; //(pep->bEndpointAddress & 0x0F);
                if(pollInterval < pep->bInterval) // Set the polling interval as the smallest polling interval obtained from endpoints
                        pollInterval = pep->bInterval;

                bNumEP++;
        }
        //PrintEndpointDescriptor(pep);
}

uint8_t HIDDriver::Release() {
        pUsb->GetAddressPool().FreeAddress(bAddress);
        return 0;
}
