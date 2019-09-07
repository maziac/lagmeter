
#if !defined(__HIDReportMask_H__)
#define __HIDReportMask_H__
#include <usbhid.h>

#define BUFFER_SIZE 64

class HIDReportMask:  public USBReadParser {
public:
    uint8_t interesting_bit_idx = 0;
    int16_t interesting_bits[BUFFER_SIZE*2];
    uint8_t current_report = 0;
    uint8_t parse_buffer[BUFFER_SIZE];
    bool do_mask = false;
    void apply_mask(uint8_t len, uint8_t *buf);
    void Parse(const uint16_t len, const uint8_t *buf, const uint16_t &offset);
};
#endif
