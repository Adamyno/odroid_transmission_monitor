#include "display_utils.h"
#include "battery_utils.h"
#include "transmission_client.h" // For stats

// Tracking variables for differential updates
static State lastState = (State)-1;
static long lastRssi = -1000;
static IPAddress lastIp;
static bool lastBlinkState = false;
static float lastBattery = -1.0;
static bool lastAltSpeedEnabled = false;

#define STATUSBAR_BG 0x2104

// Backlight Control
#define BACKLIGHT_PIN 14
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

void setupBacklight() {
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(BACKLIGHT_PIN, PWM_CHANNEL);
  // Default to max brightness (will be overwritten by config)
  ledcWrite(PWM_CHANNEL, 255);
}

void setBrightness(int duty) {
  if (duty < 0)
    duty = 0;
  if (duty > 255)
    duty = 255;
  ledcWrite(PWM_CHANNEL, duty);
}

// Helper for text width
int getTextWidth(String text) { return tft.textWidth(text); }

String formatSpeedShort(long bytes) {
  if (bytes < 1024)
    return String(bytes) + "B";
  if (bytes < 1048576)
    return String(bytes / 1024.0, 1) + "K";
  return String(bytes / 1048576.0, 1) + "M";
}

void drawStatusBar() {
  bool currentBlink = (millis() / 500) % 2 == 0;
  long currentRssi =
      (currentState == STATE_CONNECTED || currentState == STATE_DHCP ||
       currentState == STATE_MENU || currentState == STATE_ABOUT ||
       currentState == STATE_SETTINGS || currentState == STATE_OTA)
          ? WiFi.RSSI()
          : 0;
  IPAddress currentIp;
  if (currentState == STATE_CONNECTED || currentState == STATE_MENU ||
      currentState == STATE_ABOUT || currentState == STATE_SETTINGS)
    currentIp = WiFi.localIP();
  else if (currentState == STATE_AP_MODE)
    currentIp = WiFi.softAPIP();

  bool stateChanged = (currentState != lastState);
  bool rssiChanged = (currentRssi != lastRssi);
  bool blinkChanged = (currentBlink != lastBlinkState);
  static int lastOtaProgress = -1;
  bool otaChanged =
      (currentState == STATE_OTA && otaProgress != lastOtaProgress);
  static bool lastTransConnected = false;
  bool transConnected = transmission.isConnected();
  bool transChanged = (transConnected != lastTransConnected);
  static long lastDl = -1;
  static long lastUl = -1;
  long currentDl = transmission.getDownloadSpeed();
  long currentUl = transmission.getUploadSpeed();
  bool currentAltSpeed = transmission.isAltSpeedEnabled();
  bool statsChanged = (currentDl != lastDl || currentUl != lastUl ||
                       currentAltSpeed != lastAltSpeedEnabled);

  // Redraw if anything significant changed to ensure layout consistency
  // Ideally we use a full redraw technique for the status bar because positions
  // are dynamic now. To avoid flicker, we can fillRect only if needed, but
  // since positions shift, we might need to clear chunks. For simplicity: If
  // any layout-affecting validation changes, redraw the whole bar.

  // Battery has its own 2-second update timer - allow redraw for it
  static unsigned long lastBatteryCheck = 0;
  bool batteryTimerExpired = (millis() - lastBatteryCheck > 2000);
  if (batteryTimerExpired)
    lastBatteryCheck = millis();

  bool needsRedraw = stateChanged || rssiChanged ||
                     (currentState == STATE_CONNECTING && blinkChanged) ||
                     otaChanged || transChanged || statsChanged ||
                     batteryTimerExpired;

  if (!needsRedraw)
    return;

  // Fixed positions for each status bar element (right to left)
  // This prevents smearing (positions are fixed) and flickering (only clear
  // what changes)
  int const Y = 4;
  int const BATT_ICON_X = 295; // Battery icon (20px wide)
  int const BATT_PCT_X = 265;  // Battery % (moved 5px right, closer to icon)
  int const WIFI_ICON_X = 240; // WiFi icon (16px wide)
  // Transmission area: 0 to WIFI_ICON_X - 5

  // Only clear entire bar on major state changes
  if (stateChanged) {
    tft.fillRect(0, 0, 320, 24, STATUSBAR_BG);
  }

  // Battery averaging: sample every call, update display every 2 seconds
  static float batterySum = 0;
  static int batterySamples = 0;
  static unsigned long lastBatteryUpdate = 0;
  static int lastDisplayedPct = -1;

  // 1. Battery Icon & Percentage
  float currentBattery = -1.0;
  if (currentState != STATE_CONNECTING && currentState != STATE_DHCP) {
    currentBattery = getBatteryVoltage();
    batterySum += currentBattery;
    batterySamples++;
  }

  // Update battery display every 2 seconds
  bool batteryNeedsUpdate =
      (millis() - lastBatteryUpdate > 2000) && batterySamples > 0;
  if (batteryNeedsUpdate || stateChanged) {
    float avgBattery =
        (batterySamples > 0) ? (batterySum / batterySamples) : currentBattery;
    batterySum = 0;
    batterySamples = 0;
    lastBatteryUpdate = millis();

    drawBatteryIcon(BATT_ICON_X, Y, avgBattery);

    // 2. Battery Percentage (fixed slot)
    float pct = (avgBattery - 3.4) / (4.2 - 3.4);
    if (pct < 0)
      pct = 0;
    if (pct > 1)
      pct = 1;
    int pctInt = (int)(pct * 100);

    // Only redraw if percentage changed
    if (pctInt != lastDisplayedPct || stateChanged) {
      lastDisplayedPct = pctInt;
      String batStr = String(pctInt) + "%";
      // Clear the battery percentage slot first
      tft.fillRect(BATT_PCT_X, 0, 30, 24, STATUSBAR_BG);
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE, STATUSBAR_BG);
      tft.setCursor(BATT_PCT_X, Y + 4);
      tft.print(batStr);
    }
  }

  // 3. WiFi Signal (fixed position)
  int cursorX = WIFI_ICON_X;

  // Logic from old drawWifiIcon
  bool isAP = (WiFi.getMode() == WIFI_AP) || ((WiFi.getMode() == WIFI_AP_STA) &&
                                              (WiFi.status() != WL_CONNECTED));
  if (isAP) {
    drawAPIcon(cursorX, Y);
  } else if (currentState == STATE_CONNECTING) {
    drawWifiIcon(cursorX, Y, -100);
  } else if (currentState == STATE_DHCP || currentState == STATE_CONNECTED ||
             currentState == STATE_MENU || currentState == STATE_ABOUT ||
             currentState == STATE_SETTINGS) {
    drawWifiIcon(cursorX, Y, currentRssi);
  } else if (currentState == STATE_OTA) {
    if (currentBlink)
      drawWifiIcon(cursorX, Y, currentRssi);
  } else {
    // X
    tft.drawLine(cursorX, Y, cursorX + 15, Y + 15, TFT_RED);
    tft.drawLine(cursorX + 15, Y, cursorX, Y + 15, TFT_RED);
  }

  // 4. Transmission Icon & Stats - ONLY when CONNECTED to server
  if (transConnected &&
      (currentState == STATE_CONNECTED || currentState == STATE_MENU ||
       currentState == STATE_ABOUT || currentState == STATE_SETTINGS)) {

    // Clear the transmission area if stats changed (to prevent ghosting)
    if (statsChanged || transChanged) {
      tft.fillRect(0, 0, cursorX, 24, STATUSBAR_BG);
    }

    // cursorX -= (16 + 5); // Icon is 16x16
    // drawSmallTransmissionIcon(cursorX, Y, transConnected);

    // 5. Speed Stats
    // Layout: [DL Text][DL Arrow] [UL Text][UL Arrow] [WiFi]

    // Add 3px padding from WiFi icon as requested
    cursorX -= 3;

    int slotWidth = 50; // Fixed width for each stat block to prevent jitter

    // Upload Block
    String ulStr = formatSpeedShort(currentUl);
    int ulTextW = tft.textWidth(ulStr);

    // Align right within slot
    cursorX -= slotWidth;
    int ulIconX = cursorX + slotWidth - 10; // Icon at far right of slot
    int ulTextX = ulIconX - 2 - ulTextW;    // Text to left of icon

    // Draw Upload Arrow (Red Up)
    tft.fillTriangle(ulIconX, Y + 8, ulIconX + 8, Y + 8, ulIconX + 4, Y + 2,
                     TFT_RED);
    tft.fillRect(ulIconX + 3, Y + 8, 2, 4, TFT_RED);

    tft.setCursor(ulTextX, Y + 4);
    tft.print(ulStr);

    // Download Block
    String dlStr = formatSpeedShort(currentDl);
    int dlTextW = tft.textWidth(dlStr);

    // Align right within slot
    cursorX -= slotWidth;
    int dlIconX = cursorX + slotWidth - 10;
    int dlTextX = dlIconX - 2 - dlTextW;

    // Draw Download Arrow (Green Down)
    tft.fillTriangle(dlIconX, Y + 6, dlIconX + 8, Y + 6, dlIconX + 4, Y + 12,
                     TFT_GREEN);
    tft.fillRect(dlIconX + 3, Y + 2, 2, 4, TFT_GREEN);

    tft.setCursor(dlTextX, Y + 4);
    tft.print(dlStr);

    // 5.1 Free Space Display (to the left of download stats)
    long long freeBytes = transmission.getFreeSpace();
    if (freeBytes > 0) {
      // Format as GB
      int freeGB = freeBytes / 1073741824LL; // 1024^3
      String freeStr = String(freeGB) + "G";
      int freeTextW = tft.textWidth(freeStr);

      cursorX -= (freeTextW + 16); // Text + icon + spacing
      int fx = cursorX;
      int fy = Y + 2;

      // HDD Icon: Gray rectangle with disk inside
      tft.fillRoundRect(fx, fy + 2, 12, 10, 1, TFT_DARKGREY);  // HDD body
      tft.drawRoundRect(fx, fy + 2, 12, 10, 1, TFT_LIGHTGREY); // Border
      tft.fillCircle(fx + 6, fy + 6, 3, TFT_LIGHTGREY);        // Disk platter
      tft.fillCircle(fx + 6, fy + 6, 1, TFT_DARKGREY);         // Center hole
      tft.fillRect(fx + 9, fy + 9, 2, 2, TFT_GREEN);           // Activity LED

      // Free space text
      tft.setTextColor(TFT_WHITE, STATUSBAR_BG);
      tft.setCursor(fx + 14, Y + 4);
      tft.print(freeStr);
    }

    // 5.2 Turtle Icon for Alt-Speed Mode (to the left of free space)
    if (transmission.isAltSpeedEnabled()) {
      cursorX -= 18; // Space for turtle icon
      int tx = cursorX;
      int ty = Y + 2;

// Detailed Turtle (v2)
// Color constants
#define TURTLE_BROWN 0xCC68 // Light brown-ish
      // Shell: Green
      // Head/Legs: Brown

      // Bigger Shell (12x9)
      tft.fillRoundRect(tx + 2, ty + 3, 12, 9, 4, TFT_GREEN);

      // Shell Pattern
      tft.drawPixel(tx + 5, ty + 5, STATUSBAR_BG);
      tft.drawPixel(tx + 9, ty + 5, STATUSBAR_BG);
      tft.drawPixel(tx + 7, ty + 8, STATUSBAR_BG);

      // Head (Brown)
      tft.fillCircle(tx + 15, ty + 5, 2, TURTLE_BROWN);
      // Eye (Black)
      tft.drawPixel(tx + 15, ty + 4, TFT_BLACK);

      // Back Legs Only (Brown)
      tft.fillRect(tx + 3, ty + 11, 2, 3, TURTLE_BROWN);  // Back Left
      tft.fillRect(tx + 11, ty + 11, 2, 3, TURTLE_BROWN); // Back Right
    }

  } else if (transChanged && !transConnected) {
    // Clear transmission area when disconnected
    // Fix: Clear up to cursorX to ensure no artifacts (like the 1px from UL
    // icon) are left. Previously cursorX - 5 missed the rightmost edge of the
    // icon.
    tft.fillRect(0, 0, cursorX, 24, STATUSBAR_BG);
  }

  // 6. Left side status text (Connecting etc/ OTA)
  if (currentState == STATE_OTA) {
    tft.setCursor(5, 4);
    tft.print("OTA...");

    // Restore OTA Progress Bar
    // Draw a frame or just a bar
    int barWidth = 100;
    int barHeight = 4;
    int x = 60; // To the right of "OTA..." text
    int y = 10;

    // Background
    tft.fillRect(x, y, barWidth, barHeight, UI_GREY);
    // Progress
    int fillW = (otaProgress * barWidth) / 100;
    tft.fillRect(x, y, fillW, barHeight, TFT_GREEN);

  } else if (currentState == STATE_CONNECTING) {
    tft.setCursor(5, 4);
    tft.print("Connecting...");
  }

  lastState = currentState;
  lastRssi = currentRssi;
  lastBlinkState = currentBlink;
  lastOtaProgress = otaProgress;
  lastTransConnected = transConnected;
  lastDl = currentDl;
  lastUl = currentUl;
  lastAltSpeedEnabled = currentAltSpeed;
  lastBattery = currentBattery;
}

void drawBatteryIcon(int x, int y, float voltage) {
  // Outline
  tft.drawRect(x, y + 2, 18, 12, TFT_WHITE);
  tft.fillRect(x + 18, y + 5, 2, 6, TFT_WHITE);

  // Fill based on voltage (approx 3.3V - 4.2V range)
  float percentage = (voltage - 3.4) / (4.2 - 3.4);
  if (percentage < 0)
    percentage = 0;
  if (percentage > 1.0)
    percentage = 1.0;

  int w = percentage * 14;
  uint16_t color = TFT_GREEN;
  if (percentage < 0.2)
    color = TFT_RED;
  else if (percentage < 0.5)
    color = TFT_ORANGE;

  if (w > 0) {
    tft.fillRect(x + 2, y + 4, w, 8, color);
  }
}

void drawAPIcon(int x, int y) {
  tft.fillRoundRect(x, y, 24, 16, 3, TFT_ORANGE);
  tft.setTextColor(TFT_BLACK, TFT_ORANGE);
  tft.setTextSize(1);
  tft.setCursor(x + 6, y + 4);
  tft.print("AP");
}

void drawWifiIcon(int x, int y, long rssi) {
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
    int h = (i + 1) * 4;
    uint16_t color = (i < bars) ? TFT_GREEN : TFT_BLACK;
    if (rssi == -100)
      color = 0x7BEF; // Custom Grey for connecting state
    tft.fillRect(x + (i * 4), y + (16 - h), 3, h, color);
  }
}

void drawSmallTransmissionIcon(int x, int y, bool connected) {
  uint16_t color = connected ? TFT_ORANGE : UI_GREY;
  // Small T-Handle (Gearshift)
  // |---|  (8px wide)
  //   |
  //   |
  tft.fillRect(x + 4, y + 2, 8, 3, color);   // Handle
  tft.drawFastVLine(x + 7, y + 2, 8, color); // Shaft
  // Base
  tft.fillRect(x + 5, y + 10, 5, 2, color);
}

void drawTransmissionStats(int x, int y, long dl, long ul) {
  // DL: Green Down Arrow
  tft.fillTriangle(x, y, x + 8, y, x + 4, y + 6, TFT_GREEN);
  tft.fillRect(x + 3, y - 4, 2, 4, TFT_GREEN);

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, STATUSBAR_BG);
  tft.setCursor(x + 12, y - 4);
  tft.print(formatSpeedShort(dl));

  int dlWidth = 50; // Reserved width

  // UL: Red Up Arrow
  int ulX = x + dlWidth + 10;
  tft.fillTriangle(ulX, y + 2, ulX + 8, y + 2, ulX + 4, y - 4, TFT_RED);
  tft.fillRect(ulX + 3, y + 2, 2, 4, TFT_RED);

  tft.setCursor(ulX + 12, y - 4);
  tft.print(formatSpeedShort(ul));
}

void drawTransmissionIcon(int x, int y, bool connected) {
  uint16_t color = connected ? 0xFD20 : 0x7BEF; // Orange-ish or Grey
  uint16_t bg = UI_BG;

  // Clear area (assuming 64x64)
  tft.fillRect(x, y, 64, 64, bg);

  // Draw T-Shaped Automatic Gearshift
  int centerX = x + 32;
  int centerY = y + 32;

  // Handle (Top)
  // Rounded Box form
  tft.fillRoundRect(centerX - 20, centerY - 20, 40, 12, 4, color);

  // Shaft
  tft.fillRect(centerX - 4, centerY - 10, 8, 30, color);

  // Base / Boot
  tft.fillCircle(centerX, centerY + 20, 8, color);

  // Highlight/Button on handle
  if (connected) {
    tft.fillRect(centerX - 18, centerY - 18, 5, 8, TFT_WHITE);
  }
}
