#ifndef __Measure_H__
#define __Measure_H__

// Structure to return min/max values.
struct MinMax {
  int min;
  int max;
};


void setupMeasurement();
void testPhotoSensor();
struct MinMax getMaxMinAnalogIn(int inputPin, int measTime);
void waitMsInput(int inputPin, int outpValue, struct MinMax range, int waitTime);
int measureLag(int inputPin, int outpValue, struct MinMax range, bool invertRange = false, int inputPinWait = -1, struct MinMax rangeWait = {});
int measureLagDiff(struct MinMax range, struct MinMax rangeWait);
void measurePhotoSensor();
void measureSVGA();
void measureSvgaToMonitor();

#endif
