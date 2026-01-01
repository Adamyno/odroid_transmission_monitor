#include "display_utils.h"

// Tracking variables for differential updates
static State lastState = (State)-1;
static long lastRssi = -1000;
static IPAddress lastIp;
static bool lastBlinkState = false;
static float lastBattery = -1.0;

#define STATUSBAR_BG 0x2104

void drawStatusBar() {
  // 1. Determine current values
  bool currentBlink = (millis() / 500) % 2 == 0;
  long currentRssi = (currentState == STATE_CONNECTED) ? WiFi.RSSI() : 0;
  IPAddress currentIp;
  if (currentState == STATE_CONNECTED)
    currentIp = WiFi.localIP();
  else if (currentState == STATE_AP_MODE)
    currentIp = WiFi.softAPIP();

  // 2. Check for changes
  bool stateChanged = (currentState != lastState);
  bool rssiChanged = (currentRssi != lastRssi);
  bool ipChanged = (currentIp != lastIp);
  bool blinkChanged = (currentBlink != lastBlinkState);
  static int lastOtaProgress = -1;
  bool otaChanged =
      (currentState == STATE_OTA && otaProgress != lastOtaProgress);

  // Icon Positions (320px screen)
  int battX = 295; // Battery on the far right
  int iconX = 270; // Wifi next to it
  int iconY = 4;
  int battY = 4;

  // 3. Update Background and Border if state changed
  if (stateChanged) {
    tft.fillRect(0, 0, 320, 24, STATUSBAR_BG);
  }

  // 4. Update Icon area
  if (stateChanged || (currentState == STATE_CONNECTED && rssiChanged) ||
      (currentState == STATE_CONNECTING && blinkChanged)) {

    // Clear only icon area if not already cleared by stateChanged
    if (!stateChanged) {
      tft.fillRect(iconX, iconY, 24, 16, STATUSBAR_BG);
    }

    if (currentState == STATE_AP_MODE) {
      drawAPIcon(iconX, iconY);
    } else if (currentState == STATE_CONNECTING) {
      if (currentBlink)
        drawWifiIcon(iconX, iconY, -50);
    } else if (currentState == STATE_CONNECTED) {
      drawWifiIcon(iconX, iconY, currentRssi);
    } else if (currentState == STATE_OTA) {
      // Optional: Could blink or show something specific, but text is main
      // indicator
      if (currentBlink)
        drawWifiIcon(iconX, iconY, currentRssi);
    } else {
      tft.drawLine(iconX, iconY, iconX + 15, iconY + 15, TFT_RED);
      tft.drawLine(iconX + 15, iconY, iconX, iconY + 15, TFT_RED);
    }
  }

  // 4b. Update Battery area
  float currentBattery = getBatteryVoltage();
  bool batteryChanged = (abs(currentBattery - lastBattery) > 0.05);

  if (stateChanged || batteryChanged) {
    if (!stateChanged) {
      tft.fillRect(battX, battY, 22, 16, STATUSBAR_BG);
    }
    drawBatteryIcon(battX, battY, currentBattery);
  }

  // 5. Update IP Address / Status Text area
  if (stateChanged || ipChanged || otaChanged) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, STATUSBAR_BG);

    if (currentState == STATE_OTA) {
      // Clear area
      tft.fillRect(5, 4, 150, 20, STATUSBAR_BG);
      tft.setCursor(5, 4);
      tft.print("OTA");

      // Simple Progress Bar: | [=====     ] |
      int barX = 5;
      int barY = 16;
      int barW = 80;
      int barH = 6;

      tft.drawFastVLine(barX, barY - 1, barH + 2, TFT_WHITE); // Left |
      tft.drawFastVLine(barX + barW + 2, barY - 1, barH + 2,
                        TFT_WHITE); // Right |

      int progressW = (otaProgress * barW) / 100;
      tft.fillRect(barX + 2, barY, progressW, barH, TFT_GREEN);
      tft.fillRect(barX + 2 + progressW, barY, barW - progressW, barH,
                   TFT_BLACK);
    } else if (currentIp[0] == 0) {
      tft.fillRect(5, 4, 100, 20, STATUSBAR_BG);
    } else {
      tft.setCursor(5, 4);
      tft.printf("%d.%d  ", currentIp[0], currentIp[1]);

      tft.setCursor(5, 14);
      tft.printf("%d.%d  ", currentIp[2], currentIp[3]);
    }
  }

  // 6. Store current state
  lastState = currentState;
  lastRssi = currentRssi;
  lastIp = currentIp;
  lastBlinkState = currentBlink;
  lastBattery = currentBattery;
  lastOtaProgress = otaProgress;
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

float getBatteryVoltage() {
  int raw = analogRead(36);
  return (raw / 4095.0) * 2.0 * 3.3;
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
    tft.fillRect(x + (i * 4), y + (16 - h), 3, h, color);
  }
}
