#ifndef GUI_HANDLER_H
#define GUI_HANDLER_H

#include "display_utils.h" // For shared State enum
#include <TFT_eSPI.h>      // Include TFT library

void drawMenu();
void drawAbout();
void drawSettings();
void drawStatus(); // For the detailed status page if distinct from dashboard

// Navigation
void menuUp();
void menuDown();
void menuSelect();
void menuBack();

extern TFT_eSPI tft; // Reuse the tft object
extern int menuIndex;

#endif
