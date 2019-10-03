#ifndef __Measure_H__
#define __Measure_H__

// Structure to return min/max values.
struct MinMax {
  int min;
  int max;
};


void setupMeasurement();
void testPhotoSensor();
void measurePhotoSensor();
void measureAD2();
void measureSvgaToMonitor();
void measureMinPressTime();

#endif
