#ifndef WIFI_SCAN_GUI_H
#define WIFI_SCAN_GUI_H

#include <Arduino.h>
#include <WiFi.h>

// WiFi Scan GUI States
enum WifiScanState {
  WIFI_SCAN_IDLE,      // Not in WiFi scan mode
  WIFI_SCAN_LIST,      // Showing network list
  WIFI_SCAN_DETAILS,   // Showing selected network details
  WIFI_SCAN_PASSWORD,  // Password entry screen
  WIFI_SCAN_CONNECTING // Connection in progress
};

// Max networks to cache
#define MAX_NETWORKS 20

// Functions
void initWifiScanGui();
void drawWifiScanScreen();
bool handleWifiScanInput(bool up, bool down, bool left, bool right, bool a,
                         bool b, bool start, bool select);
void enterWifiScanMode();
void exitWifiScanMode();
bool isInWifiScanMode();

// Get current state for external reference
WifiScanState getWifiScanState();

#endif
