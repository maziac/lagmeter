#include "HIDReportMask.h"

void HIDReportMask::apply_mask(uint8_t len, uint8_t *buf) {
  if (!do_mask) {
    return;
  }
  memset(parse_buffer, 0, len);
  for (uint16_t ib = 0 ; ib < interesting_bit_idx ; ib++) {
    uint8_t the_bit = interesting_bits[ib];
    if (the_bit/8 >= len) {
      break;
    }
    parse_buffer[the_bit/8] |= (buf[the_bit/8] & (1 << (the_bit % 8)));
  }
  memcpy(buf, parse_buffer, len);
}

void HIDReportMask::Parse(const uint16_t len, const uint8_t *buf, const uint16_t &offset) {
  if (offset > 0) {
    return;
  }
  uint8_t* inVals = buf + offset;
  int possible_errors = 0;
  uint8_t button_report = 0;
  uint16_t current_offset = 0;
  uint8_t current_report_size = 0;
  uint8_t current_report_count = 0;
  interesting_bit_idx = 0;
  current_report = 0;
  for (uint16_t i = 0 ; i < 128 ; i++) {
    interesting_bits[i] = -1;
  }
  bool interesting = false;
  
  for (uint16_t i = 0; i < len; ) {
    uint8_t b0 = inVals[i++];
    uint8_t bSize = b0 & 0x03;
    bSize = bSize == 3 ? 4 : bSize; // size is 4 when bSize is 3
    uint8_t bType = (b0 >> 2) & 0x03;
    uint8_t bTag = (b0 >> 4) & 0x0F;
  
    if (bType == 0x03 && bTag == 0x0F && bSize == 2 && i + 2 < len) {
      // ignore long data
      uint8_t bDataSize = inVals[i++];
      uint8_t bLongItemTag = inVals[i++];
      for (int j = 0; j < bDataSize && i < len; j++) {
        i++;
      }
    } else {
      uint8_t bSizeActual = 0;
      uint8_t itemVal = 0;
      for (uint8_t j = 0; j < bSize; j++) {
        if (i + j < len) {
          itemVal += inVals[i + j] << (8 * j);
          bSizeActual++;
        }
      } 
  
      if (bType == 0x00) {
        if (bTag == 0x08) {
          if (interesting) {
            for (uint16_t b = 0 ; b < current_report_count*current_report_size ; b++) {
              interesting_bits[interesting_bit_idx++] = current_offset + b;
            }
            interesting = false;
          }
          current_offset += current_report_count*current_report_size;
        }
      } else if (bType == 0x01) {
        if (bTag == 0x00) {
          if (itemVal == 0x09) {
            // button
            interesting = true;
          }
        } else if (bTag == 0x07) {
          current_report_size = itemVal;
        } else if (bTag == 0x08) {
          if (current_report > 0) {
            // ignore other reports
            break;
          }
          current_report = itemVal;
          current_offset = 0;
        } else if (bTag == 0x09) {
          current_report_count = itemVal;
        }
      } else if (bType == 0x02) {
        if (bTag == 0x00) {
          if (itemVal == 0x39) {
            // hat switch
            interesting = true;
          }
        }
      }
      i += bSize;
    }
  }
  if (current_report > 0) {
    // there is a report now
    for (uint16_t ib = 0 ; ib < interesting_bit_idx ; ib++) {
      interesting_bits[ib] += 8;
    }
  }
  do_mask = true;
}
