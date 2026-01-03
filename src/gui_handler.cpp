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
int settingsIndex = -1; // -1=Nav, 0=Brightness, 1=Host, 2=Port, 3=Test/Save row
const int settingsCount = 4; // Brightness, Host, Port, Test/Save (same row)

// Edit mode state
int editMode = 0;             // 0=none, 1=editing IP, 2=editing Port
int editOctet = 0;            // 0-3 for IP octets
int editDigit = 0;            // 0-4 for port digits
uint8_t tempIP[4];            // Temporary IP during edit
char tempPort[6];             // Temporary port string during edit
bool tempInitialized = false; // True after user entered edit mode at least once
bool testSaveSelected =
    false; // false=Test, true=Save (for LEFT/RIGHT navigation)

void resetSettingsMenu() {
  settingsIndex = -1;
  editMode = 0;
  editOctet = 0;
  editDigit = 0;
  tempInitialized = false;
  testSaveSelected = false;
}

void drawSettings() {
  // Fill content area with dark background
  tft.fillRect(0, 24, 320, 216, UI_BG);

  // Draw tab bar with Settings (index 1) active
  drawTabBar(1);

  int contentY = 54;
  int startX = 20;
  int startY = contentY + 10;
  int lineH = 22;

  // 1. Brightness (index 0)
  tft.setTextSize(1);
  if (settingsIndex == 0)
    tft.setTextColor(TFT_GREEN, UI_BG);
  else
    tft.setTextColor(UI_CYAN, UI_BG);

  tft.setCursor(startX, startY);
  tft.print("Brightness");

  int barX = startX + 100;
  int barW = 120;
  int barH = 10;

  if (settingsIndex == 0) {
    tft.setTextColor(TFT_GREEN, UI_BG);
    tft.setCursor(barX - 12, startY);
    tft.print("<");
    tft.setCursor(barX + barW + 3, startY);
    tft.print(">");
  }

  tft.drawRect(barX, startY, barW, barH, UI_WHITE);
  int filledW = map(brightness, 10, 255, 0, barW - 2);
  tft.fillRect(barX + 1, startY + 1, filledW, barH - 2, UI_CYAN);

  // 2. Trans Host (index 1)
  startY += lineH;
  if (settingsIndex == 1)
    tft.setTextColor(TFT_GREEN, UI_BG);
  else
    tft.setTextColor(UI_CYAN, UI_BG);

  tft.setCursor(startX, startY);
  tft.print("Trans Host:");

  // Display IP value
  int ipX = startX + 90;
  if (editMode == 1) {
    // Editing IP - show octets with current one highlighted
    for (int i = 0; i < 4; i++) {
      if (i == editOctet)
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
      else
        tft.setTextColor(UI_WHITE, UI_BG);

      char buf[4];
      sprintf(buf, "%3d", tempIP[i]);
      tft.setCursor(ipX + i * 36, startY);
      tft.print(buf);
      if (i < 3) {
        tft.setTextColor(UI_WHITE, UI_BG);
        tft.print(".");
      }
    }
  } else {
    tft.setTextColor(UI_WHITE, UI_BG);
    tft.setCursor(ipX, startY);
    // Show temp values if initialized, otherwise show saved values
    if (tempInitialized) {
      char ipBuf[16];
      sprintf(ipBuf, "%d.%d.%d.%d", tempIP[0], tempIP[1], tempIP[2], tempIP[3]);
      tft.print(ipBuf);
    } else {
      tft.print(transHost);
    }
    // Show "Press A" hint if selected
    if (settingsIndex == 1) {
      tft.setTextColor(TFT_YELLOW, UI_BG);
      tft.setCursor(250, startY);
      tft.print("Press A");
    }
  }

  // 3. Trans Port (index 2)
  startY += lineH;
  if (settingsIndex == 2)
    tft.setTextColor(TFT_GREEN, UI_BG);
  else
    tft.setTextColor(UI_CYAN, UI_BG);

  tft.setCursor(startX, startY);
  tft.print("Trans Port:");

  int portX = startX + 90;
  if (editMode == 2) {
    // Editing Port - show digits with current one highlighted
    for (int i = 0; i < 5; i++) {
      if (i == editDigit)
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
      else
        tft.setTextColor(UI_WHITE, UI_BG);

      tft.setCursor(portX + i * 12, startY);
      tft.print(tempPort[i]);
    }
  } else {
    tft.setTextColor(UI_WHITE, UI_BG);
    tft.setCursor(portX, startY);
    // Show temp values if initialized, otherwise show saved values
    if (tempInitialized) {
      tft.print(tempPort);
    } else {
      char portBuf[6];
      sprintf(portBuf, "%05d", transPort);
      tft.print(portBuf);
    }
    // Show "Press A" hint if selected
    if (settingsIndex == 2) {
      tft.setTextColor(TFT_YELLOW, UI_BG);
      tft.setCursor(250, startY);
      tft.print("Press A");
    }
  }

  // 4. Test/Save Buttons (index 3, same row, LEFT/RIGHT to switch)
  startY += lineH + 10;
  int btnW = 60;
  int btnH = 22;

  // Test Button (left)
  bool testSelected = (settingsIndex == 3 && !testSaveSelected);
  if (testSelected)
    tft.fillRoundRect(startX, startY, btnW, btnH, 4, UI_CYAN);
  else
    tft.drawRoundRect(startX, startY, btnW, btnH, 4, UI_CYAN);

  tft.setTextColor(testSelected ? TFT_BLACK : UI_CYAN,
                   testSelected ? UI_CYAN : UI_BG);
  tft.setCursor(startX + 15, startY + 6);
  tft.print("Test");

  // Save Button (right)
  int saveX = startX + btnW + 20;
  bool saveSelected = (settingsIndex == 3 && testSaveSelected);
  if (saveSelected)
    tft.fillRoundRect(saveX, startY, btnW, btnH, 4, TFT_GREEN);
  else
    tft.drawRoundRect(saveX, startY, btnW, btnH, 4, TFT_GREEN);

  tft.setTextColor(saveSelected ? TFT_BLACK : TFT_GREEN,
                   saveSelected ? TFT_GREEN : UI_BG);
  tft.setCursor(saveX + 15, startY + 6);
  tft.print("Save");

  // Info at bottom
  tft.setTextColor(UI_GREY, UI_BG);
  tft.setCursor(startX, 225);
  if (editMode > 0)
    tft.print("UP/DOWN: value  L/R: position  A: done");
  else
    tft.print("A: edit/action  B: back to menu");
}

#include "web_server.h" // For testTransmission

bool handleSettingsInput(bool up, bool down, bool left, bool right, bool a,
                         bool b) {
  bool update = false;

  // If in edit mode, handle differently
  if (editMode == 1) {
    // Editing IP octets
    if (up) {
      tempIP[editOctet]++;
      if (tempIP[editOctet] > 255)
        tempIP[editOctet] = 0;
      update = true;
    } else if (down) {
      if (tempIP[editOctet] == 0)
        tempIP[editOctet] = 255;
      else
        tempIP[editOctet]--;
      update = true;
    } else if (right) {
      editOctet = (editOctet + 1) % 4;
      update = true;
    } else if (left) {
      editOctet = (editOctet + 3) % 4; // -1 with wrap
      update = true;
    } else if (a) {
      // Exit edit mode
      editMode = 0;
      update = true;
    }
    if (update)
      drawSettings();
    return true;
  } else if (editMode == 2) {
    // Editing Port digits
    if (up) {
      tempPort[editDigit]++;
      if (tempPort[editDigit] > '9')
        tempPort[editDigit] = '0';
      update = true;
    } else if (down) {
      if (tempPort[editDigit] == '0')
        tempPort[editDigit] = '9';
      else
        tempPort[editDigit]--;
      update = true;
    } else if (right) {
      editDigit = (editDigit + 1) % 5;
      update = true;
    } else if (left) {
      editDigit = (editDigit + 4) % 5; // -1 with wrap
      update = true;
    } else if (a) {
      // Exit edit mode
      editMode = 0;
      update = true;
    }
    if (update)
      drawSettings();
    return true;
  }

  // Normal navigation (not in edit mode)
  if (up) {
    if (settingsIndex > -1) {
      settingsIndex--;
      update = true;
    }
  } else if (down) {
    if (settingsIndex < settingsCount - 1) {
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
  } else if (settingsIndex == 1) { // Host
    if (a) {
      // Enter edit mode for IP - parse current transHost to octets
      editMode = 1;
      editOctet = 0;
      // Parse IP (from temp if already initialized, else from saved)
      if (!tempInitialized) {
        int o1, o2, o3, o4;
        if (sscanf(transHost.c_str(), "%d.%d.%d.%d", &o1, &o2, &o3, &o4) == 4) {
          tempIP[0] = o1;
          tempIP[1] = o2;
          tempIP[2] = o3;
          tempIP[3] = o4;
        } else {
          tempIP[0] = 192;
          tempIP[1] = 168;
          tempIP[2] = 1;
          tempIP[3] = 1;
        }
        sprintf(tempPort, "%05d", transPort); // Also init port
        tempInitialized = true;
      }
      update = true;
    }
  } else if (settingsIndex == 2) { // Port
    if (a) {
      // Enter edit mode for Port
      editMode = 2;
      editDigit = 0;
      if (!tempInitialized) {
        // Init port from saved
        sprintf(tempPort, "%05d", transPort);
        // Also init IP
        int o1, o2, o3, o4;
        if (sscanf(transHost.c_str(), "%d.%d.%d.%d", &o1, &o2, &o3, &o4) == 4) {
          tempIP[0] = o1;
          tempIP[1] = o2;
          tempIP[2] = o3;
          tempIP[3] = o4;
        } else {
          tempIP[0] = 192;
          tempIP[1] = 168;
          tempIP[2] = 1;
          tempIP[3] = 1;
        }
        tempInitialized = true;
      }
      update = true;
    }
  } else if (settingsIndex == 3) { // Test/Save row
    // LEFT/RIGHT to switch between Test and Save
    if (left && testSaveSelected) {
      testSaveSelected = false;
      update = true;
    } else if (right && !testSaveSelected) {
      testSaveSelected = true;
      update = true;
    } else if (a) {
      if (!testSaveSelected) {
        // Test button
        char tempIPStr[16];
        sprintf(tempIPStr, "%d.%d.%d.%d", tempIP[0], tempIP[1], tempIP[2],
                tempIP[3]);
        int tempPortVal = atoi(tempPort);

        tft.fillRect(20, 200, 280, 20, UI_BG);
        tft.setCursor(20, 200);
        tft.setTextColor(TFT_YELLOW, UI_BG);
        tft.print("Testing...");

        String res = testTransmission(String(tempIPStr), tempPortVal, transPath,
                                      transUser, transPass);

        tft.fillRect(20, 200, 280, 20, UI_BG);
        tft.setCursor(20, 200);
        if (res.indexOf("Success") >= 0)
          tft.setTextColor(TFT_GREEN, UI_BG);
        else
          tft.setTextColor(TFT_RED, UI_BG);
        tft.print(res.substring(0, 40));
      } else {
        // Save button
        char tempIPStr[16];
        sprintf(tempIPStr, "%d.%d.%d.%d", tempIP[0], tempIP[1], tempIP[2],
                tempIP[3]);
        transHost = String(tempIPStr);
        transPort = atoi(tempPort);
        saveConfig();

        tft.fillRect(20, 200, 280, 20, UI_BG);
        tft.setCursor(20, 200);
        tft.setTextColor(TFT_GREEN, UI_BG);
        tft.print("Settings Saved!");
        update = true;
      }
    }
  }

  if (update) {
    drawSettings();
  }

  return true;
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
