#ifndef GUI_HANDLER_H
#define GUI_HANDLER_H

#include "display_utils.h" // For shared State enum
#include <TFT_eSPI.h>      // Include TFT library

void drawMenu();
void drawAbout();
void drawSettings();
void drawStatus();
void updateStatusValues();
void drawTabBar(int activeTab);
void drawDashboard();
bool handleSettingsInput(bool up, bool down, bool left, bool right, bool a,
                         bool b);
void resetSettingsMenu();

// Navigation
void menuUp();
void menuDown();
void menuSelect();
void menuBack();

extern TFT_eSPI tft; // Reuse the tft object
extern int menuIndex;
extern const char *const BUILD_DATE;

#endif
