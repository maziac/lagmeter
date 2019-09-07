#include "desc.h"


void print_hex(int v, int num_places)
{
  int mask = 0, n, num_nibbles, digit;

  for (n = 1; n <= num_places; n++) {
    mask = (mask << 1) | 0x0001;
  }
  v = v & mask; // truncate v to specified number of places

  num_nibbles = num_places / 4;
  if ((num_places % 4) != 0) {
    ++num_nibbles;
  }
  do {
    digit = ((v >> (num_nibbles - 1) * 4)) & 0x0f;
    Serial.print(digit, HEX);
  }
  while (--num_nibbles);
}
//  function to get single string description
uint8_t getstrdescr(USB* usb, uint8_t addr, uint8_t idx )
{
  uint8_t rcode = 0;
#if NAME_RETRIEVAL
  uint8_t buf[ 256 ];
  uint8_t length;
  uint8_t i;
  uint16_t langid;
  rcode = usb->getStrDescr( addr, 0, 1, 0, 0, buf );  //get language table length
  if ( rcode ) {
    Serial.println("Error retrieving LangID table length");
    return ( rcode );
  }
  length = buf[ 0 ];      //length is the first byte
  rcode = usb->getStrDescr( addr, 0, length, 0, 0, buf );  //get language table
  if ( rcode ) {
    Serial.print("Error retrieving LangID table ");
    return ( rcode );
  }
  langid = (buf[3] << 8) | buf[2];
  rcode = usb->getStrDescr( addr, 0, 1, idx, langid, buf );
  if ( rcode ) {
    Serial.print("Error retrieving string length ");
    return ( rcode );
  }
  length = buf[ 0 ];
  rcode = usb->getStrDescr( addr, 0, length, idx, langid, buf );
  if ( rcode ) {
    Serial.print("Error retrieving string ");
    return ( rcode );
  }
  for ( i = 2; i < length; i += 2 ) {   //string is UTF-16LE encoded
    Serial.print((char) buf[i]);
  }
#endif
  return ( rcode );
}


// function to get all string descriptors
uint8_t getallstrdescr(USB* usb, USB_DEVICE_DESCRIPTOR * buf, uint8_t addr)
{

  uint8_t rcode = 0;
#if NAME_RETRIEVAL
  if ( buf->iManufacturer > 0 ) {
    Serial.print("Manufacturer:\t\t");
    rcode = getstrdescr(usb, addr, buf->iManufacturer );   // get manufacturer string
    if ( rcode ) {
      Serial.println( rcode, HEX );
    }
    Serial.print("\r\n");
  }
  if ( buf->iProduct > 0 ) {
    Serial.print("Product:\t\t");
    rcode = getstrdescr(usb, addr, buf->iProduct );        // get product string
    if ( rcode ) {
      Serial.println( rcode, HEX );
    }
    Serial.print("\r\n");
  }
  if ( buf->iSerialNumber > 0 ) {
    Serial.print("Serial:\t\t\t");
    rcode = getstrdescr(usb, addr, buf->iSerialNumber );   // get serial string
    if ( rcode ) {
      Serial.println( rcode, HEX );
    }
    Serial.print("\r\n");
  }
#endif
  return rcode;
}
