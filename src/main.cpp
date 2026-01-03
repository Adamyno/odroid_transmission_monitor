#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Update.h>

#include <WebServer.h>
#include <WiFi.h>

#include "battery_utils.h"
#include "config_utils.h"
#include "display_utils.h"
#include "gui_handler.h"
#include "input_handler.h"
#include "transmission_client.h"
#include "web_pages.h"
#include "web_server.h"

// --- Configuration ---
// VERSION is in web_pages.h
const char *const BUILD_DATE = __DATE__ " " __TIME__;
const char *AP_SSID = "ODROID-GO_Config";
// CONFIG_FILE is in config_utils.cpp

// --- Globals ---
// server is defined in web_server.cpp
TFT_eSPI tft = TFT_eSPI();
// Globals ssid, password, transHost... are in config_utils.cpp

State currentState = STATE_AP_MODE;
int otaProgress = 0;
unsigned long connectionStartTime = 0;

// --- Function Prototypes ---
void loadConfig();
void saveConfig();

void checkWiFiConnection(); // Helper for reconnection logic

void deleteConfig();
void setupAP(); // This will be replaced by startAPMode
void startAPMode();
void connectToWiFi();

TaskHandle_t transmissionTask;

void transmissionTaskLoop(void *pvParameters) {
  for (;;) {
    transmission.update();
    vTaskDelay(50 / portTICK_PERIOD_MS); // Small delay to yield
  }
}

// --- Setup ---
void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println("BOOT: Starting Full Firmware...");

  // Launch Transmission Task on Core 0
  xTaskCreatePinnedToCore(
      transmissionTaskLoop, /* Function to implement the task */
      "TransmissionTask",   /* Name of the task */
      10000,                /* Stack size in words */
      NULL,                 /* Task input parameter */
      1,                    /* Priority of the task */
      &transmissionTask,    /* Task handle. */
      0);                   /* Core where the task should run */

  setupInputs();    // Initialize buttons
  setupBattery();   // Initialize ADC
  setupBacklight(); // Initialize PWM for backlight

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  tft.fillRect(0, 0, 320, 24, 0x2104);

  // --- OTA Setup ---
  ArduinoOTA.setHostname("ODROID-GO-Monitor");
  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA");
    currentState = STATE_OTA;
    otaProgress = 0;
    drawStatusBar();
  });
  ArduinoOTA.onEnd([]() {
    otaProgress = 100;
    drawStatusBar();
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int p = (progress / (total / 100));
    if (p != otaProgress) {
      otaProgress = p;
      drawStatusBar();
    }
  });
  ArduinoOTA.onError(
      [](ota_error_t error) { Serial.printf("Error[%u]: ", error); });

  // LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  } else {
    Serial.println("LittleFS mounted");
  }

  loadConfig();
  setupServerRoutes();

  if (ssid != "") {
    Serial.println("Connecting to WiFi...");
    connectToWiFi();
  } else {
    Serial.println("Starting AP Mode...");
    startAPMode();
  }

  Serial.println("Starting OTA...");
  ArduinoOTA.begin();
  Serial.println("Starting Web Server...");
  server.begin();

  Serial.println("Initial drawStatusBar...");
  drawStatusBar();
  Serial.println("Setup Complete.");
}

// --- Loop ---
void loop() {
  static unsigned long dhcpStartTime = 0;
  static unsigned long lastStatusUpdate = 0;

  // Periodic Status Bar Update
  if (millis() - lastStatusUpdate > 2000) {
    drawStatusBar();
    lastStatusUpdate = millis();
  }

  if (currentState == STATE_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi Connected! Requesting IP...");
      currentState = STATE_DHCP;
      dhcpStartTime = millis();
      drawStatusBar();
    } else {
      if (millis() - connectionStartTime > 20000) { // 20s Timeout
        Serial.println("Connection Timeout! Switching to AP Mode.");
        startAPMode();
      }
    }
  }

  if (currentState == STATE_DHCP) {
    if (WiFi.localIP()[0] != 0) {
      Serial.println("IP Obtained!");
      currentState = STATE_CONNECTED;
      drawStatusBar();
      drawDashboard();
    } else {
      if (millis() - dhcpStartTime > 30000) {
        Serial.println("DHCP Timeout! Switching to AP Mode.");
        startAPMode();
      }
    }
  }

  // Check connection status periodically (unless in AP mode or OTA)
  if (currentState != STATE_AP_MODE && currentState != STATE_OTA &&
      currentState != STATE_DHCP) {
    checkWiFiConnection();
  }

  if (currentState == STATE_AP_MODE || currentState == STATE_CONNECTED ||
      currentState == STATE_MENU || currentState == STATE_ABOUT ||
      currentState == STATE_SETTINGS) {
    server.handleClient();
  }

  if (currentState == STATE_AP_MODE || currentState == STATE_CONNECTED ||
      currentState == STATE_OTA || currentState == STATE_MENU ||
      currentState == STATE_ABOUT || currentState == STATE_SETTINGS) {
    ArduinoOTA.handle();
  }

  // Web Server Handle
  server.handleClient();

  // Real-time updates now handled in task
  // transmission.update();

  // --- Input & GUI Handling ---
  readInputs();

  // Menu toggle
  if (btnMenuPressed) {
    if (currentState == STATE_MENU || currentState == STATE_ABOUT ||
        currentState == STATE_SETTINGS) {
      // Exit tabbed interface
      if (WiFi.status() == WL_CONNECTED)
        currentState = STATE_CONNECTED;
      else
        currentState = STATE_AP_MODE;
      drawStatusBar();
      drawDashboard();
    } else {
      // Enter tabbed interface
      menuIndex = 0;
      currentState = STATE_MENU;
      drawStatus();
    }
  }

  // Tab navigation
  if (currentState == STATE_MENU || currentState == STATE_ABOUT ||
      currentState == STATE_SETTINGS) {

    // Pass input to settings page if active
    bool processed = false; // Flag to indicate if input was handled by settings
    if (currentState == STATE_SETTINGS) {
      if (btnUpPressed || btnDownPressed || btnLeftPressed || btnRightPressed ||
          btnAPressed || btnBPressed) {
        // If handleSettingsInput returns true, it handled the input (e.g.
        // brightness adj). If false (inactive state), allowed to fall through
        // to Tab Switch.
        if (handleSettingsInput(btnUpPressed, btnDownPressed, btnLeftPressed,
                                btnRightPressed, btnAPressed, btnBPressed)) {
          // Handled, do not process tab switch
          processed = true;
        }
      }
    }

    // Normal Tab Switch Logic (if not processed by Settings)
    if (!processed) {
      bool tabChanged = false;
      if (btnLeftPressed && menuIndex > 0) {
        menuIndex--;
        tabChanged = true;
      }
      if (btnRightPressed && menuIndex < 2) {
        menuIndex++;
        tabChanged = true;
      }

      if (tabChanged) {
        if (menuIndex == 0) {
          currentState = STATE_MENU;
          drawStatus();
        } else if (menuIndex == 1) {
          currentState = STATE_SETTINGS;
          resetSettingsMenu(); // Reset to inactive state on entry
          drawSettings();
        } else if (menuIndex == 2) {
          currentState = STATE_ABOUT;
          drawAbout();
        }
      }
    }

    // B button also exits (Global back)
    if (btnBPressed &&
        currentState != STATE_SETTINGS) { // Allow B in Settings for Test? No, A
                                          // for Test. B for Back?
      if (WiFi.status() == WL_CONNECTED)
        currentState = STATE_CONNECTED;
      else
        currentState = STATE_AP_MODE;
      drawStatusBar();
      drawDashboard();
    }
    // Handle B in settings explicitly?
    if (btnBPressed && currentState == STATE_SETTINGS) {
      // Go back to Dashboard
      if (WiFi.status() == WL_CONNECTED)
        currentState = STATE_CONNECTED;
      else
        currentState = STATE_AP_MODE;
      drawStatusBar();
      drawDashboard();
    }
  }

  // Update Status Bar throttled
  static unsigned long lastDrawTime = 0;
  static unsigned long lastTabUpdate = 0;

  if (millis() - lastDrawTime > 250) {
    drawStatusBar();
    lastDrawTime = millis();
  }

  // Periodic Tab Refresh (e.g. for live Status updates)
  if (millis() - lastTabUpdate > 5000) {
    if (currentState == STATE_MENU) {
      updateStatusValues(); // Partial update to prevent flickering
    }
    lastTabUpdate = millis();
  }

  delay(10);
}

// --- Implementation ---

void startAPMode() {
  Serial.println("Starting AP Mode");
  WiFi.mode(WIFI_AP_STA);
  if (apPassword.length() >= 8) {
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
  } else {
    WiFi.softAP(apSSID.c_str());
  }
  currentState = STATE_AP_MODE;
  drawStatusBar();
  drawDashboard();

  handleScan();
}

void connectToWiFi() {
  Serial.printf("Connecting to %s\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  currentState = STATE_CONNECTING;
  connectionStartTime = millis();
  drawStatusBar();
}

void setupAP() {
  currentState = STATE_AP_MODE;
  WiFi.mode(WIFI_AP);
  if (apPassword.length() >= 8) {
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
  } else {
    WiFi.softAP(apSSID.c_str());
  }
  drawStatusBar();
}

void checkWiFiConnection() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();
    if (WiFi.status() != WL_CONNECTED && currentState != STATE_CONNECTING) {
      Serial.println("WiFi Check: Not Connected. Reconnecting...");
      WiFi.disconnect();
      WiFi.begin(ssid.c_str(), password.c_str());
      if (currentState == STATE_CONNECTED) {
        currentState = STATE_CONNECTING;
        drawStatusBar();
      }
    }
  }
}
