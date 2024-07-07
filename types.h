#ifndef __TYPES_H__
#define __TYPES_H__

#include <Arduino.h>

#define MAX_READINGS_DISPLAY (60 * 3 / 5)  // 3 hours, 5 min intervals
#define MAX_DEVICES 10

typedef struct device {
  char device[50];
  int battery;
  // char type[50];
  long long mills;
  bool set;
} device_t;

typedef struct reading {
  int sgv;
  long long ts_ms;
} reading;

typedef struct display_data {
  int sgv;
  int sgv_delta;
  long long sgv_ts_ms;
  struct tm tm;
  device devices[MAX_DEVICES];
  reading readings[MAX_READINGS_DISPLAY];
  int8_t wifi_connected;
  char lan_ip[20];
  int8_t ota_in_progress;
  int8_t ota_progress;
  char status_override[100];
} display_data_t;

#endif  // __TYPES_H__