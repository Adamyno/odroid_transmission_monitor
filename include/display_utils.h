#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>

// --- Shared State ---
// UI Colors (matching web interface)
#define UI_BG 0x1906      // Dark background (#1a1a2e)
#define UI_TAB_BG 0x1104  // Tab bar background (#16213e)
#define UI_CYAN 0x07FF    // Accent color (#00dbde)
#define UI_CARD_BG 0x2008 // Card background (#202040)
#define UI_WHITE TFT_WHITE
#define UI_GREY 0x7BEF // Inactive text

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
void drawTransmissionStats(int x, int y, long dl, long ul);
void drawTransmissionIcon(int x, int y, bool connected);
void drawSmallTransmissionIcon(int x, int y, bool connected);

// Backlight
void setupBacklight();
void setBrightness(int duty);

#endif
