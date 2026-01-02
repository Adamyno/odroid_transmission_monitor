#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <Arduino.h>

// ODROID-GO Button Pins
#define BTN_MENU 13
#define BTN_SELECT 27
#define BTN_START 39
#define BTN_A 32
#define BTN_B 33
#define BTN_JOY_Y 35
#define BTN_JOY_X 34

void setupInputs();
void readInputs();

extern bool btnMenuPressed;
extern bool btnSelectPressed;
extern bool btnUpPressed;
extern bool btnDownPressed;
extern bool btnAPressed;
extern bool btnBPressed;

#endif
