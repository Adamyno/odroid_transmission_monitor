#include "wifi_scan_gui.h"
#include "config_utils.h"
#include "display_utils.h"
#include <WiFi.h>

// External TFT reference
extern TFT_eSPI tft;

// State variables
static WifiScanState wifiScanState = WIFI_SCAN_IDLE;
static int networkCount = 0;
static int selectedNetwork = 0;
static int scrollOffset = 0;
static int detailButtonIndex = 0; // 0=Connect

// Max visible networks on screen
#define VISIBLE_NETWORKS 6

// Cached network data
static String scannedSSIDs[MAX_NETWORKS];
static int scannedRSSI[MAX_NETWORKS];
static bool scannedSecure[MAX_NETWORKS];
static String scannedBSSID[MAX_NETWORKS];
static wifi_auth_mode_t scannedAuth[MAX_NETWORKS];

// Password entry state
static String passwordBuffer = "";
static int kbRow = 0;
static int kbCol = 0;
static bool shiftActive = false;

// Virtual keyboard layout
// Row 0: 1 2 3 4 5 6 7 8 9 0 - _
// Row 1: q w e r t y u i o p
// Row 2: a s d f g h j k l @
// Row 3: z x c v b n m . ! #
// Row 4: [SHIFT] [SPACE] [OK]

static const char *kbRowsLower[] = {
    "1234567890-_", "qwertyuiop", "asdfghjkl@", "zxcvbnm.!#",
    "" // Special row: SHIFT, SPACE, OK
};

static const char *kbRowsUpper[] = {"1234567890-_", "QWERTYUIOP", "ASDFGHJKL@",
                                    "ZXCVBNM.!#", ""};

static const int kbRowLengths[] = {12, 10, 10, 10,
                                   3}; // Last row has 3 special keys
static const int KB_ROWS = 5;

// Connection status message
static String connectionStatus = "";
static unsigned long statusTimeout = 0;

void initWifiScanGui() {
  wifiScanState = WIFI_SCAN_IDLE;
  passwordBuffer = "";
  kbRow = 0;
  kbCol = 0;
  shiftActive = false;
  networkCount = 0;
  selectedNetwork = 0;
  scrollOffset = 0;
  detailButtonIndex = 0;
}

// Draw scanning screen (called before actual scan)
void drawScanningScreen() {
  tft.fillRect(0, 24, 320, 216, UI_BG);

  tft.setTextSize(2);
  tft.setTextColor(UI_CYAN, UI_BG);
  tft.setCursor(80, 100);
  tft.print("Scanning...");

  tft.setTextSize(1);
  tft.setTextColor(UI_GREY, UI_BG);
  tft.setCursor(70, 140);
  tft.print("Please wait");
}

bool isInWifiScanMode() { return wifiScanState != WIFI_SCAN_IDLE; }

WifiScanState getWifiScanState() { return wifiScanState; }

void enterWifiScanMode() {
  initWifiScanGui();
  wifiScanState = WIFI_SCAN_LIST;

  // Show scanning screen first
  drawScanningScreen();

  // Ensure WiFi mode allows scanning (needs STA component)
  wifi_mode_t currentMode = WiFi.getMode();
  Serial.printf("WiFi Scan: Current mode=%d (0=NULL,1=STA,2=AP,3=AP_STA)\n",
                currentMode);

  if (currentMode == WIFI_MODE_AP || currentMode == WIFI_MODE_NULL) {
    Serial.println("WiFi Scan: Switching to AP_STA mode for scanning");
    WiFi.mode(WIFI_AP_STA);
    delay(500); // Longer delay for mode switch
  }

  // Disconnect from any STA connection to allow clean scan
  WiFi.disconnect(false); // false = don't turn off STA mode
  delay(100);

  // Perform initial scan
  Serial.println("WiFi Scan: Starting scan...");
  int n = WiFi.scanNetworks(false, true); // false=async, true=show hidden
  Serial.printf("WiFi Scan: Result=%d networks\n", n);

  if (n < 0) {
    Serial.printf("WiFi Scan Error: %d\n", n);
    networkCount = 0;
  } else {
    networkCount = min(n, MAX_NETWORKS);
    for (int i = 0; i < networkCount; i++) {
      scannedSSIDs[i] = WiFi.SSID(i);
      scannedRSSI[i] = WiFi.RSSI(i);
      scannedSecure[i] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
      scannedBSSID[i] = WiFi.BSSIDstr(i);
      scannedAuth[i] = WiFi.encryptionType(i);
    }
  }
  WiFi.scanDelete();
}

void exitWifiScanMode() {
  wifiScanState = WIFI_SCAN_IDLE;
  passwordBuffer = "";
}

// Helper: Draw RSSI icon (4-bar style)
void drawRSSIIcon(int x, int y, int rssi) {
  int bars = 0;
  if (rssi >= -60)
    bars = 4;
  else if (rssi >= -70)
    bars = 3;
  else if (rssi >= -80)
    bars = 2;
  else if (rssi >= -90)
    bars = 1;

  int barWidths[] = {2, 2, 2, 2};
  int barHeights[] = {4, 7, 10, 13};
  int spacing = 3;

  for (int i = 0; i < 4; i++) {
    int bx = x + i * spacing;
    int by = y + (13 - barHeights[i]);
    uint16_t color = (i < bars) ? TFT_GREEN : TFT_DARKGREY;
    tft.fillRect(bx, by, barWidths[i], barHeights[i], color);
  }
}

// Helper: Draw lock icon
void drawLockIcon(int x, int y, bool locked) {
  if (locked) {
    // Draw simple lock: body + shackle
    tft.fillRect(x + 2, y + 5, 8, 7, TFT_YELLOW);
    tft.drawRect(x + 3, y, 6, 6, TFT_YELLOW);
  }
}

// Get security type string
String getSecurityString(wifi_auth_mode_t auth) {
  switch (auth) {
  case WIFI_AUTH_OPEN:
    return "Open";
  case WIFI_AUTH_WEP:
    return "WEP";
  case WIFI_AUTH_WPA_PSK:
    return "WPA";
  case WIFI_AUTH_WPA2_PSK:
    return "WPA2";
  case WIFI_AUTH_WPA_WPA2_PSK:
    return "WPA/WPA2";
  case WIFI_AUTH_WPA2_ENTERPRISE:
    return "WPA2-Enterprise";
  case WIFI_AUTH_WPA3_PSK:
    return "WPA3";
  default:
    return "Unknown";
  }
}

void drawNetworkListScreen() {
  // Clear content area
  tft.fillRect(0, 24, 320, 216, UI_BG);

  // Title bar
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, UI_BG);
  tft.setCursor(10, 30);
  tft.printf("WiFi Networks (%d found)", networkCount);

  if (networkCount == 0) {
    tft.setTextColor(TFT_YELLOW, UI_BG);
    tft.setCursor(10, 80);
    tft.print("No networks found");
    tft.setCursor(10, 100);
    tft.print("Press START to rescan");
  } else {
    // Draw visible networks
    int startY = 50;
    int rowH = 22;

    for (int i = 0; i < VISIBLE_NETWORKS && (scrollOffset + i) < networkCount;
         i++) {
      int idx = scrollOffset + i;
      int y = startY + i * rowH;

      // Highlight selected
      if (idx == selectedNetwork) {
        tft.fillRect(0, y, 320, rowH, UI_CYAN);
        tft.setTextColor(TFT_BLACK, UI_CYAN);
      } else {
        tft.setTextColor(TFT_WHITE, UI_BG);
      }

      // Lock icon (left side, if secured)
      if (scannedSecure[idx]) {
        drawLockIcon(5, y + 3, true);
      }

      // SSID (after lock icon area)
      String displaySSID = scannedSSIDs[idx];
      if (displaySSID.length() > 24) {
        displaySSID = displaySSID.substring(0, 21) + "...";
      }
      tft.setCursor(22, y + 6);
      tft.print(displaySSID);

      // RSSI value (right side)
      tft.setCursor(260, y + 6);
      tft.printf("%d", scannedRSSI[idx]);

      // RSSI icon (far right, after dBm value)
      drawRSSIIcon(300, y + 4, scannedRSSI[idx]);
    }

    // Scroll indicators
    if (scrollOffset > 0) {
      tft.setTextColor(TFT_WHITE, UI_BG);
      tft.setCursor(305, 50);
      tft.print("^");
    }
    if (scrollOffset + VISIBLE_NETWORKS < networkCount) {
      tft.setTextColor(TFT_WHITE, UI_BG);
      tft.setCursor(305, 50 + (VISIBLE_NETWORKS - 1) * 22);
      tft.print("v");
    }
  }

  // Hint bar at bottom
  tft.fillRect(0, 220, 320, 20, UI_TAB_BG);
  tft.setTextSize(1);
  tft.setTextColor(UI_GREY, UI_TAB_BG);
  tft.setCursor(5, 225);
  tft.print("A:Select  START:Rescan  B:Back");
}

void drawNetworkDetailsScreen() {
  int idx = selectedNetwork;

  // Clear content area
  tft.fillRect(0, 24, 320, 216, UI_BG);

  // Network SSID as title
  tft.setTextSize(2);
  tft.setTextColor(UI_CYAN, UI_BG);
  tft.setCursor(10, 32);
  String displaySSID = scannedSSIDs[idx];
  if (displaySSID.length() > 18) {
    displaySSID = displaySSID.substring(0, 15) + "...";
  }
  tft.print(displaySSID);

  // Details
  tft.setTextSize(1);
  int y = 60;
  int lineH = 16;

  tft.setTextColor(TFT_WHITE, UI_BG);

  tft.setCursor(10, y);
  tft.print("BSSID: ");
  tft.setTextColor(UI_GREY, UI_BG);
  tft.print(scannedBSSID[idx]);
  y += lineH;

  tft.setTextColor(TFT_WHITE, UI_BG);
  tft.setCursor(10, y);
  tft.print("Signal: ");
  tft.setTextColor(UI_GREY, UI_BG);
  tft.printf("%d dBm", scannedRSSI[idx]);

  // Signal strength description
  String strength;
  if (scannedRSSI[idx] >= -60)
    strength = " (Excellent)";
  else if (scannedRSSI[idx] >= -70)
    strength = " (Good)";
  else if (scannedRSSI[idx] >= -80)
    strength = " (Fair)";
  else
    strength = " (Weak)";
  tft.print(strength);

  // RSSI icon next to signal text
  drawRSSIIcon(250, y - 2, scannedRSSI[idx]);
  y += lineH;

  tft.setTextColor(TFT_WHITE, UI_BG);
  tft.setCursor(10, y);
  tft.print("Security: ");
  tft.setTextColor(UI_GREY, UI_BG);
  tft.print(getSecurityString(scannedAuth[idx]));
  y += lineH + 8;

  // Action button - Connect only
  int btnW = 90;
  int btnH = 24;
  int btnX = 20;

  // Connect button
  tft.fillRoundRect(btnX, y, btnW, btnH, 4, TFT_GREEN);
  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.setCursor(btnX + 20, y + 7);
  tft.print("Connect");

  // Status message
  if (connectionStatus.length() > 0 && millis() < statusTimeout) {
    tft.setTextColor(TFT_YELLOW, UI_BG);
    tft.setCursor(10, 180);
    tft.print(connectionStatus);
  }

  // Hint bar
  tft.fillRect(0, 220, 320, 20, UI_TAB_BG);
  tft.setTextSize(1);
  tft.setTextColor(UI_GREY, UI_TAB_BG);
  tft.setCursor(5, 225);
  tft.print("A:Connect  B:Back");
}

void drawPasswordScreen() {
  int idx = selectedNetwork;

  // Clear content area
  tft.fillRect(0, 24, 320, 216, UI_BG);

  // Title: SSID
  tft.setTextSize(1);
  tft.setTextColor(UI_CYAN, UI_BG);
  tft.setCursor(10, 30);
  tft.print("Connect to: ");
  tft.setTextColor(TFT_WHITE, UI_BG);
  String displaySSID = scannedSSIDs[idx];
  if (displaySSID.length() > 20) {
    displaySSID = displaySSID.substring(0, 17) + "...";
  }
  tft.print(displaySSID);

  // Password field
  tft.setTextColor(TFT_WHITE, UI_BG);
  tft.setCursor(10, 50);
  tft.print("Password:");

  // Password box
  tft.drawRect(10, 62, 300, 20, UI_WHITE);
  tft.fillRect(11, 63, 298, 18, TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(14, 67);

  // Show password (visible, truncate if too long)
  String displayPwd = passwordBuffer;
  if (displayPwd.length() > 38) {
    displayPwd = displayPwd.substring(displayPwd.length() - 38);
  }
  tft.print(displayPwd);
  tft.print("_"); // Cursor

  // Virtual keyboard
  int kbStartY = 90;
  int keyW = 22;
  int keyH = 22;
  int keySpacing = 2;

  const char **kbRows = shiftActive ? kbRowsUpper : kbRowsLower;

  for (int row = 0; row < KB_ROWS; row++) {
    int y = kbStartY + row * (keyH + keySpacing);

    if (row < 4) {
      // Regular character rows
      int len = strlen(kbRows[row]);
      int rowStartX = (320 - len * (keyW + keySpacing)) / 2;

      for (int col = 0; col < len; col++) {
        int x = rowStartX + col * (keyW + keySpacing);
        bool selected = (kbRow == row && kbCol == col);

        if (selected)
          tft.fillRoundRect(x, y, keyW, keyH, 2, UI_CYAN);
        else
          tft.drawRoundRect(x, y, keyW, keyH, 2, UI_GREY);

        tft.setTextColor(selected ? TFT_BLACK : TFT_WHITE,
                         selected ? UI_CYAN : UI_BG);
        tft.setCursor(x + 7, y + 6);
        tft.print(kbRows[row][col]);
      }
    } else {
      // Special row: SHIFT, SPACE, OK
      int specialW[] = {50, 100, 50};
      const char *specialLabels[] = {"SHIFT", "SPACE", "OK"};
      int totalW = 50 + 100 + 50 + 2 * keySpacing;
      int startX = (320 - totalW) / 2;

      int x = startX;
      for (int col = 0; col < 3; col++) {
        bool selected = (kbRow == 4 && kbCol == col);

        if (selected)
          tft.fillRoundRect(x, y, specialW[col], keyH, 2, UI_CYAN);
        else
          tft.drawRoundRect(x, y, specialW[col], keyH, 2, UI_GREY);

        tft.setTextColor(selected ? TFT_BLACK : TFT_WHITE,
                         selected ? UI_CYAN : UI_BG);
        int textX = x + (specialW[col] - strlen(specialLabels[col]) * 6) / 2;
        tft.setCursor(textX, y + 6);
        tft.print(specialLabels[col]);

        x += specialW[col] + keySpacing;
      }
    }
  }

  // Status message
  if (connectionStatus.length() > 0 && millis() < statusTimeout) {
    tft.setTextColor(TFT_RED, UI_BG);
    tft.setCursor(10, 205);
    tft.print(connectionStatus);
  }

  // Hint bar
  tft.fillRect(0, 220, 320, 20, UI_TAB_BG);
  tft.setTextSize(1);
  tft.setTextColor(UI_GREY, UI_TAB_BG);
  tft.setCursor(5, 225);
  tft.print("A:Type B:Delete START:Connect SEL:Cancel");
}

void drawConnectingScreen() {
  // Clear content area
  tft.fillRect(0, 24, 320, 216, UI_BG);

  tft.setTextSize(2);
  tft.setTextColor(UI_CYAN, UI_BG);
  tft.setCursor(100, 100);
  tft.print("Connecting...");

  tft.setTextSize(1);
  tft.setTextColor(UI_GREY, UI_BG);
  tft.setCursor(80, 140);
  tft.print("Please wait");
}

void drawWifiScanScreen() {
  switch (wifiScanState) {
  case WIFI_SCAN_LIST:
    drawNetworkListScreen();
    break;
  case WIFI_SCAN_DETAILS:
    drawNetworkDetailsScreen();
    break;
  case WIFI_SCAN_PASSWORD:
    drawPasswordScreen();
    break;
  case WIFI_SCAN_CONNECTING:
    drawConnectingScreen();
    break;
  default:
    break;
  }
}

// Try to connect to the selected network
bool tryConnect(const String &ssidToConnect, const String &pwd) {
  connectionStatus = "Connecting...";
  statusTimeout = millis() + 30000;

  wifiScanState = WIFI_SCAN_CONNECTING;
  drawWifiScanScreen();

  // Stop AP and try station mode
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidToConnect.c_str(), pwd.c_str());

  Serial.printf("Connecting to %s...\n", ssidToConnect.c_str());

  // Wait for connection (max 15 seconds)
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Connected! IP: %s\n", WiFi.localIP().toString().c_str());

    // Save credentials
    ssid = ssidToConnect;
    password = pwd;
    saveConfig();

    connectionStatus = "";
    exitWifiScanMode();
    return true;
  } else {
    Serial.println("Connection failed!");

    // Go back to AP mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str(),
                apPassword.length() > 0 ? apPassword.c_str() : NULL);

    connectionStatus = "Connection failed! Check password.";
    statusTimeout = millis() + 5000;
    wifiScanState = WIFI_SCAN_PASSWORD;
    return false;
  }
}

bool handleWifiScanInput(bool up, bool down, bool left, bool right, bool a,
                         bool b, bool start, bool select) {
  bool update = false;

  switch (wifiScanState) {
  case WIFI_SCAN_LIST:
    if (up && selectedNetwork > 0) {
      selectedNetwork--;
      if (selectedNetwork < scrollOffset) {
        scrollOffset = selectedNetwork;
      }
      update = true;
    } else if (down && selectedNetwork < networkCount - 1) {
      selectedNetwork++;
      if (selectedNetwork >= scrollOffset + VISIBLE_NETWORKS) {
        scrollOffset = selectedNetwork - VISIBLE_NETWORKS + 1;
      }
      update = true;
    } else if (a && networkCount > 0) {
      wifiScanState = WIFI_SCAN_DETAILS;
      detailButtonIndex = 0;
      update = true;
    } else if (start) {
      // Rescan
      enterWifiScanMode();
      update = true;
    } else if (b) {
      exitWifiScanMode();
      update = true;
    }
    break;

  case WIFI_SCAN_DETAILS: {
    if (a) {
      // Connect
      if (!scannedSecure[selectedNetwork]) {
        // Open network - connect directly
        tryConnect(scannedSSIDs[selectedNetwork], "");
      } else {
        // Password required
        passwordBuffer = "";
        kbRow = 0;
        kbCol = 0;
        shiftActive = false;
        wifiScanState = WIFI_SCAN_PASSWORD;
      }
      update = true;
    } else if (b) {
      wifiScanState = WIFI_SCAN_LIST;
      update = true;
    }
  } break;

  case WIFI_SCAN_PASSWORD:
    if (up && kbRow > 0) {
      kbRow--;
      // Adjust column if new row is shorter
      int maxCol = (kbRow < 4) ? strlen(kbRowsLower[kbRow]) - 1 : 2;
      if (kbCol > maxCol)
        kbCol = maxCol;
      update = true;
    } else if (down && kbRow < KB_ROWS - 1) {
      kbRow++;
      int maxCol = (kbRow < 4) ? strlen(kbRowsLower[kbRow]) - 1 : 2;
      if (kbCol > maxCol)
        kbCol = maxCol;
      update = true;
    } else if (left) {
      int maxCol = (kbRow < 4) ? strlen(kbRowsLower[kbRow]) - 1 : 2;
      if (kbCol > 0)
        kbCol--;
      else
        kbCol = maxCol; // Wrap
      update = true;
    } else if (right) {
      int maxCol = (kbRow < 4) ? strlen(kbRowsLower[kbRow]) - 1 : 2;
      if (kbCol < maxCol)
        kbCol++;
      else
        kbCol = 0; // Wrap
      update = true;
    } else if (a) {
      // Type character or special action
      if (kbRow < 4) {
        const char *row = shiftActive ? kbRowsUpper[kbRow] : kbRowsLower[kbRow];
        if (kbCol < (int)strlen(row) && passwordBuffer.length() < 63) {
          passwordBuffer += row[kbCol];
        }
      } else {
        // Special row
        if (kbCol == 0) {
          // SHIFT
          shiftActive = !shiftActive;
        } else if (kbCol == 1) {
          // SPACE
          if (passwordBuffer.length() < 63) {
            passwordBuffer += ' ';
          }
        } else if (kbCol == 2) {
          // OK - validate password length first
          if (passwordBuffer.length() < 8) {
            connectionStatus = "Password too short (min 8 chars)";
            statusTimeout = millis() + 3000;
          } else if (passwordBuffer.length() > 32) {
            connectionStatus = "Password too long (max 32 chars)";
            statusTimeout = millis() + 3000;
          } else {
            tryConnect(scannedSSIDs[selectedNetwork], passwordBuffer);
          }
        }
      }
      update = true;
    } else if (b) {
      // Delete last character (B only deletes, does NOT exit)
      if (passwordBuffer.length() > 0) {
        passwordBuffer =
            passwordBuffer.substring(0, passwordBuffer.length() - 1);
      }
      update = true;
    } else if (start) {
      // Connect with current password - validate length first
      if (passwordBuffer.length() < 8) {
        connectionStatus = "Password too short (min 8 chars)";
        statusTimeout = millis() + 3000;
        update = true;
      } else if (passwordBuffer.length() > 32) {
        connectionStatus = "Password too long (max 32 chars)";
        statusTimeout = millis() + 3000;
        update = true;
      } else {
        tryConnect(scannedSSIDs[selectedNetwork], passwordBuffer);
        update = true;
      }
    } else if (select) {
      // Cancel - back to details
      wifiScanState = WIFI_SCAN_DETAILS;
      update = true;
    }
    break;

  case WIFI_SCAN_CONNECTING:
    // No input during connection
    break;

  default:
    break;
  }

  if (update) {
    drawWifiScanScreen();
  }

  return true; // Input consumed
}
