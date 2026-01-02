#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>

// --- Shared State ---
enum State {
  STATE_AP_MODE,
  STATE_CONNECTING,
  STATE_DHCP,
  STATE_CONNECTED,
  STATE_OTA,
  STATE_MENU,
  STATE_ABOUT,
  STATE_SETTINGS
};

// --- External Globals ---
extern TFT_eSPI tft;
extern State currentState;
extern int otaProgress;

// --- Display Functions ---
void drawStatusBar();
void drawWifiIcon(int x, int y, long rssi);
void drawAPIcon(int x, int y);
void drawBatteryIcon(int x, int y, float voltage);

#endif
