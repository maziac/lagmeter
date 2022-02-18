#ifndef __Measure_H__
#define __Measure_H__

// Structure to return min/max values.
struct MinMax {
	int16_t min;
	int16_t max;
};

// Same but for floats.
struct MinMaxFloat {
	double min;
	double max;
};


void setupMeasurement();
void testPhotoSensor();
void measurePhotoSensor();
void measureAD2();
void measureSvgaToMonitor();
void measureMinPressTime();

#endif
