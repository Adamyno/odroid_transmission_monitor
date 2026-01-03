#include "gui_handler.h"
#include "battery_utils.h"
#include "config_utils.h"
#include "input_handler.h"
#include "transmission_client.h"
#include "web_pages.h" // For VERSION constant
#include <WiFi.h>

// Menu Items
const char *menuItems[] = {"Status", "Settings", "About"};
const int menuCount = 3;
int menuIndex = 0;

// Draw the tab bar at the top (below status bar)
void drawTabBar(int activeTab) {
  int tabY = 24;
  int tabH = 28;
  int tabW = 106; // 320 / 3

  // Tab bar background
  tft.fillRect(0, tabY, 320, tabH, UI_TAB_BG);

  for (int i = 0; i < menuCount; i++) {
    int tabX = i * tabW;

    if (i == activeTab) {
      // Active tab: cyan text, underline
      tft.setTextColor(UI_CYAN, UI_TAB_BG);
      tft.drawFastHLine(tabX + 10, tabY + tabH - 2, tabW - 20, UI_CYAN);
    } else {
      tft.setTextColor(UI_GREY, UI_TAB_BG);
    }

    tft.setTextSize(1);
    // Center text in tab
    int textW = strlen(menuItems[i]) * 6; // approx 6px per char at size 1
    int textX = tabX + (tabW - textW) / 2;
    tft.setCursor(textX, tabY + 10);
    tft.print(menuItems[i]);
  }
}

// Helper to draw signal strength icon
void drawSignalIcon(int x, int y, int rssi) {
  int bars = 0;
  if (rssi >= -60)
    bars = 4;
  else if (rssi >= -70)
    bars = 3;
  else if (rssi >= -80)
    bars = 2;
  else if (rssi >= -90)
    bars = 1;

  for (int i = 0; i < 4; i++) {
    int barH = (i + 1) * 4;
    uint16_t color = (i < bars) ? UI_CYAN : UI_GREY;
    tft.fillRect(x + (i * 6), y + (16 - barH), 4, barH, color);
  }
}

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
  // Fill content area with dark background
  tft.fillRect(0, 24, 320, 216, UI_BG);

  // Draw tab bar with About (index 2) active
  drawTabBar(2);

  int contentY = 54; // Start below tab bar

  // Card background
  int cardX = 20;
  int cardY = contentY + 10;
  int cardW = 280;
  int cardH = 150;
  tft.fillRoundRect(cardX, cardY, cardW, cardH, 8, UI_CARD_BG);

  // Title
  tft.setTextSize(2);
  tft.setTextColor(UI_CYAN, UI_CARD_BG);
  tft.setCursor(cardX + 20, cardY + 15);
  tft.print("About");

  // Divider
  tft.drawFastHLine(cardX + 15, cardY + 40, cardW - 30, UI_GREY);

  // Content
  tft.setTextSize(1);
  int lineH = 18;
  int startTextY = cardY + 55;

  // App Name
  tft.setTextColor(UI_WHITE, UI_CARD_BG);
  tft.setCursor(cardX + 20, startTextY);
  tft.print("Odroid Transmission Remote Monitor");

  // Version
  tft.setTextColor(UI_GREY, UI_CARD_BG);
  tft.setCursor(cardX + 20, startTextY + lineH);
  tft.print("Version: ");
  tft.setTextColor(UI_CYAN, UI_CARD_BG);
  tft.print(VERSION);

  // Developer
  tft.setTextColor(UI_GREY, UI_CARD_BG);
  tft.setCursor(cardX + 20, startTextY + lineH * 2);
  tft.print("Developer: ");
  tft.setTextColor(UI_WHITE, UI_CARD_BG);
  tft.print("Adamyno");

  // Platform
  tft.setTextColor(UI_GREY, UI_CARD_BG);
  tft.setCursor(cardX + 20, startTextY + lineH * 3);
  tft.print("Platform: ");
  tft.setTextColor(UI_WHITE, UI_CARD_BG);
  tft.print("ODROID-GO (ESP32)");

  // Build Date
  tft.setTextColor(UI_GREY, UI_CARD_BG);
  tft.setCursor(cardX + 20, startTextY + lineH * 4);
  tft.print("Build Date: ");
  tft.setTextColor(UI_WHITE, UI_CARD_BG);
  tft.print(BUILD_DATE);
}

// Globals for Settings State
int settingsIndex =
    -1; // -1: Inactive (Nav Mode), 0: Brightness, 1: Test Button
const int settingsCount = 2; // Brightness, Test Button

void resetSettingsMenu() { settingsIndex = -1; }

void drawSettings() {
  // Fill content area with dark background
  tft.fillRect(0, 24, 320, 216, UI_BG);

  // Draw tab bar with Settings (index 1) active
  drawTabBar(1);

  int contentY = 54;
  int startX = 20;
  int startY = contentY + 10;
  int lineH = 30;

  // 1. Brightness
  tft.setTextSize(1);
  if (settingsIndex == 0)
    tft.setTextColor(TFT_GREEN, UI_BG); // Selected
  else
    tft.setTextColor(UI_CYAN, UI_BG); // Inactive but visible

  tft.setCursor(startX, startY);
  tft.print("Brightness");

  // Draw Brightness Bar
  int barX = startX + 120;
  int barW = 140;
  int barH = 10;

  // Draw arrows if active
  if (settingsIndex == 0) {
    tft.setTextColor(TFT_GREEN, UI_BG);
    tft.setCursor(barX - 15, startY);
    tft.print("<");
    tft.setCursor(barX + barW + 5, startY);
    tft.print(">");
  } else {
    // Clear arrows if inactive (overwrite with BG color)
    tft.setTextColor(UI_BG, UI_BG);
    tft.setCursor(barX - 15, startY);
    tft.print("<");
    tft.setCursor(barX + barW + 5, startY);
    tft.print(">");
  }

  tft.drawRect(barX, startY, barW, barH, UI_WHITE);

  int filledW = map(brightness, 10, 255, 0, barW - 2);
  tft.fillRect(barX + 1, startY + 1, filledW, barH - 2, UI_CYAN);

  // 2. Transmission Info (Read Only)
  startY += lineH + 10;
  tft.setTextColor(UI_GREY, UI_BG);
  tft.setCursor(startX, startY);
  tft.print("Trans Host:");
  tft.setTextColor(UI_WHITE, UI_BG);
  tft.setCursor(startX + 100, startY);
  tft.print(transHost);

  startY += 20;
  tft.setTextColor(UI_GREY, UI_BG);
  tft.setCursor(startX, startY);
  tft.print("Trans Port:");
  tft.setTextColor(UI_WHITE, UI_BG);
  tft.setCursor(startX + 100, startY);
  tft.print(String(transPort));

  // 3. Test Button
  startY += 30;
  if (settingsIndex == 1)
    tft.fillRoundRect(startX, startY, 120, 25, 5, UI_CYAN);
  else
    tft.drawRoundRect(startX, startY, 120, 25, 5, UI_CYAN);

  tft.setTextColor(settingsIndex == 1 ? TFT_BLACK : UI_CYAN,
                   settingsIndex == 1 ? UI_CYAN : UI_BG);
  tft.setTextSize(1);
  // Center text
  tft.setCursor(startX + 15, startY + 5);
  tft.print("Test Connection");

  // 4. Info Label (Bottom)
  tft.setTextColor(UI_GREY, UI_BG);
  tft.setCursor(startX, 225); // Bottom alignment
  tft.print("Edit details via Web UI");
}

#include "web_server.h" // For testTransmission

bool handleSettingsInput(bool up, bool down, bool left, bool right, bool a,
                         bool b) {
  bool update = false;

  // Navigation logic
  if (up) {
    if (settingsIndex > -1) {
      settingsIndex--;
      update = true;
    }
  } else if (down) {
    if (settingsIndex < settingsCount - 1) { // -1 -> 0, 0 -> 1
      settingsIndex++;
      update = true;
    }
  }

  // Action logic
  if (settingsIndex == -1) {
    // Inactive mode: Let Main handle Left/Right for Tab switch
    if (left || right)
      return false;
  } else if (settingsIndex == 0) { // Brightness
    if (left) {
      brightness -= 15;
      if (brightness < 10)
        brightness = 10;
      setBrightness(brightness);
      saveConfig();
      update = true;
    } else if (right) {
      brightness += 15;
      if (brightness > 255)
        brightness = 255;
      setBrightness(brightness);
      saveConfig();
      update = true;
    }
  } else if (settingsIndex == 1) { // Test
    if (a) {
      tft.fillRect(20, 200, 280, 20, UI_BG); // Clear msg area
      tft.setCursor(20, 200);
      tft.setTextColor(TFT_YELLOW, UI_BG);
      tft.print("Testing...");

      String res = testTransmission(transHost, transPort, transPath, transUser,
                                    transPass);

      tft.fillRect(20, 200, 280, 20, UI_BG);
      tft.setCursor(20, 200);
      if (res.indexOf("Success") >= 0)
        tft.setTextColor(TFT_GREEN, UI_BG);
      else
        tft.setTextColor(TFT_RED, UI_BG);
      tft.print(res.substring(0, 40));
    }
  }

  if (update) {
    drawSettings();
  }

  return true; // We handled (or swallowed) input. Except L/R in -1 state.
}

// Globals for status caching to prevent flickering
String lastSSID = "";
String lastIP = "";
int lastRSSI = -999;
float lastBatt = 0.0;
bool firstRunStatus = true;

void drawStatusLayout() {
  // Fill content area with dark background
  tft.fillRect(0, 24, 320, 216, UI_BG);

  // Draw tab bar with Status (index 0) active
  drawTabBar(0);

  int contentY = 54;

  // Card background
  int cardX = 20;
  int cardY = contentY + 10;
  int cardW = 280;
  int cardH = 160;
  tft.fillRoundRect(cardX, cardY, cardW, cardH, 8, UI_CARD_BG);

  // Title
  tft.setTextSize(2);
  tft.setTextColor(UI_CYAN, UI_CARD_BG);
  tft.setCursor(cardX + 20, cardY + 15);
  tft.print("Status");

  // Divider
  tft.drawFastHLine(cardX + 15, cardY + 40, cardW - 30, UI_GREY);

  // Labels (Static)
  tft.setTextSize(1);
  int lineH = 20;
  int startTextY = cardY + 55;

  tft.setTextColor(UI_GREY, UI_CARD_BG);

  tft.setCursor(cardX + 20, startTextY);
  tft.print("Network: ");

  tft.setCursor(cardX + 20, startTextY + lineH);
  tft.print("IP Address: ");

  tft.setCursor(cardX + 20, startTextY + lineH * 2);
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    tft.print("Password: ");
  } else {
    tft.print("Signal: ");
  }

  tft.setCursor(cardX + 20, startTextY + lineH * 3);
  tft.print("MAC: ");
  tft.setTextColor(UI_WHITE, UI_CARD_BG);
  tft.setCursor(cardX + 100, startTextY + lineH * 3);
  tft.print(WiFi.macAddress()); // MAC is static usually

  tft.setTextColor(UI_GREY, UI_CARD_BG);
  tft.setCursor(cardX + 20, startTextY + lineH * 4);
  tft.print("Battery: ");
}

void updateStatusValues() {
  int cardX = 20;
  int contentY = 54;
  int cardY = contentY + 10;
  int startTextY = cardY + 55;
  int lineH = 20;
  int valueX = cardX + 100; // Offset for values

  tft.setTextSize(1);
  tft.setTextColor(UI_WHITE, UI_CARD_BG);

  static String lastApPassword = "";

  // 1. Network (SSID or AP Name)
  // Treat as AP only if explicit AP mode OR (AP_STA and NOT connected to
  // Station)
  bool isAP = (WiFi.getMode() == WIFI_AP) || ((WiFi.getMode() == WIFI_AP_STA) &&
                                              (WiFi.status() != WL_CONNECTED));

  String currentSSID = isAP ? apSSID : WiFi.SSID();

  if (currentSSID != lastSSID || firstRunStatus) {
    tft.fillRect(valueX, startTextY, 180, 10, UI_CARD_BG);
    tft.setCursor(valueX, startTextY);
    tft.print(currentSSID);
    lastSSID = currentSSID;
  }

  // 2. IP Address
  String currentIP =
      isAP ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  if (currentIP != lastIP || firstRunStatus) {
    tft.fillRect(valueX, startTextY + lineH, 180, 10, UI_CARD_BG);
    tft.setCursor(valueX, startTextY + lineH);
    tft.print(currentIP);
    lastIP = currentIP;
  }

  // 3. Signal / Password
  if (isAP) {
    bool passChanged = (apPassword != lastApPassword);
    if (firstRunStatus || passChanged) {
      // Clear area
      tft.fillRect(valueX, startTextY + lineH * 2, 180, 20, UI_CARD_BG);
      tft.setCursor(valueX, startTextY + lineH * 2);
      // Show password or empty if open
      tft.print(apPassword);
      lastApPassword = apPassword;
    }
  } else {
    // Normal Signal Logic
    int currentRSSI = WiFi.RSSI();
    if (abs(currentRSSI - lastRSSI) > 2 || firstRunStatus) {
      // Clear area (text + icon space)
      tft.fillRect(valueX, startTextY + lineH * 2, 100, 20, UI_CARD_BG);
      tft.fillRect(cardX + 160, startTextY + lineH * 2 - 12, 30, 20,
                   UI_CARD_BG);

      tft.setCursor(valueX, startTextY + lineH * 2);
      tft.print(String(currentRSSI) + " dBm");

      drawSignalIcon(cardX + 160, startTextY + lineH * 2 - 12, currentRSSI);
      lastRSSI = currentRSSI;
    }
  }

  // 4. Battery
  float currentBatt = getBatteryVoltage();
  if (abs(currentBatt - lastBatt) > 0.05 || firstRunStatus) {
    tft.fillRect(valueX, startTextY + lineH * 4, 100, 10, UI_CARD_BG);
    tft.setCursor(valueX, startTextY + lineH * 4);
    tft.print(String(currentBatt, 2) + " V");
    lastBatt = currentBatt;
  }

  firstRunStatus = false;
}

void drawStatus() {
  firstRunStatus = true; // Force full update
  lastSSID = "";         // Reset caches
  drawStatusLayout();
  updateStatusValues();
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

void drawDashboard() {
  // Fill content area with dark background
  tft.fillRect(0, 24, 320, 216, UI_BG);

  int cardX = 20;
  int cardY = 44;
  int cardW = 280;
  int cardH = 160;
  tft.fillRoundRect(cardX, cardY, cardW, cardH, 8, UI_CARD_BG);

  // Title
  tft.setTextSize(2);
  tft.setTextColor(UI_CYAN, UI_CARD_BG);
  tft.setCursor(cardX + 20, cardY + 15);
  tft.print("Dashboard");

  // Divider
  tft.drawFastHLine(cardX + 15, cardY + 40, cardW - 30, UI_GREY);

  // Content
  tft.setTextSize(1);
  int lineH = 20;
  int startTextY = cardY + 55;

  tft.setTextColor(UI_WHITE, UI_CARD_BG);
  tft.setCursor(cardX + 20, startTextY);
  tft.print("Status: ");

  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(TFT_GREEN, UI_CARD_BG);
    tft.print("Connected");

    tft.setTextColor(UI_GREY, UI_CARD_BG);
    tft.setCursor(cardX + 20, startTextY + lineH);
    tft.print("IP: ");
    tft.setTextColor(UI_WHITE, UI_CARD_BG);
    tft.print(WiFi.localIP().toString());
  } else {
    tft.setTextColor(TFT_YELLOW, UI_CARD_BG);
    // Logo / Icon
    // Using drawTransmissionIcon
    // Center: 160, 132 (Center of 216 area is 24 + 108 = 132)
    // Icon is 64x64, so subtract 32
    int iconX = 160 - 32;
    int iconY = 120 - 32;

#include "transmission_client.h" // Need access to check connection
    drawTransmissionIcon(iconX, iconY, transmission.isConnected());

    // Connection Status Text
    tft.setTextSize(1);
    tft.setTextColor(UI_GREY, UI_BG);
    String statusMsg =
        (WiFi.status() == WL_CONNECTED)
            ? (transmission.isConnected() ? "Connected to Server"
                                          : "Searching for Server...")
            : "WiFi Disconnected";

    int txtW = tft.textWidth(statusMsg);
    tft.setCursor(160 - (txtW / 2), 170);
    tft.print(statusMsg);

    tft.setTextColor(UI_GREY, UI_BG);
    tft.setCursor(10, 220);
    tft.print("Press MENU for options");
  }
}
