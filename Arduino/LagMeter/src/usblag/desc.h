#include <Usb.h>

uint8_t getallstrdescr(USB* usb, USB_DEVICE_DESCRIPTOR * udd, uint8_t addr);
void print_hex(int v, int num_places);
