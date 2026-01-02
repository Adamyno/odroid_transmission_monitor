#ifndef BATTERY_UTILS_H
#define BATTERY_UTILS_H

#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

void setupBattery();
double getBatteryVoltage();

#endif
