#include "gui_handler.h"
#include "input_handler.h"
#include "web_pages.h" // For VERSION constant

// Menu Items
const char *menuItems[] = {"Status", "Settings", "About"};
const int menuCount = 3;
int menuIndex = 0;

void drawMenu() {
  tft.fillRect(0, 24, 320, 216, TFT_BLACK); // Clear main area (keep status bar)

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 40);
  tft.println("MAIN MENU");

  tft.drawFastHLine(0, 70, 320, TFT_WHITE);

  int startY = 90;
  int stepY = 30;

  for (int i = 0; i < menuCount; i++) {
    if (i == menuIndex) {
      tft.setTextColor(TFT_GREEN, TFT_BLACK); // Selected
      tft.setCursor(20, startY + (i * stepY));
      tft.print("> ");
    } else {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setCursor(40, startY + (i * stepY));
    }
    tft.print(menuItems[i]);
  }
}

void drawAbout() {
  tft.fillRect(0, 24, 320, 216, TFT_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.setCursor(10, 40);
  tft.println("ABOUT");
  tft.drawFastHLine(0, 70, 320, TFT_WHITE);

  tft.setTextSize(1);
  tft.setCursor(10, 90);
  tft.print("Transmission Monitor");

  tft.setCursor(10, 110);
  tft.print("Version: ");
  tft.print(VERSION);

  tft.setCursor(10, 130);
  tft.print("Developer: Adamyno");

  tft.setCursor(10, 150);
  tft.print("Platform: ODROID-GO (ESP32)");

  tft.setCursor(10, 190);
  tft.print("Press MENU or B to back");
}

void drawSettings() {
  tft.fillRect(0, 24, 320, 216, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 40);
  tft.println("SETTINGS");
  tft.drawFastHLine(0, 70, 320, TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 90);
  tft.println("To be implemented.");
}

void drawStatus() {
  // This could be the detailed status page, similar to web dashboard?
  tft.fillRect(0, 24, 320, 216, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 40);
  tft.println("STATUS");
  tft.drawFastHLine(0, 70, 320, TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 90);
  tft.println("Use Web Interface for detailed info.");
}

// Navigation Logic
void menuUp() {
  if (menuIndex > 0) {
    menuIndex--;
    drawMenu();
  }
}

void menuDown() {
  if (menuIndex < menuCount - 1) {
    menuIndex++;
    drawMenu();
  }
}

void menuSelect() {
  // Current state handling logic will be in main loop to verify what to do
}
