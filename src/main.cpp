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
#include "web_pages.h"
#include "web_server.h"

// --- Configuration ---
// VERSION is in web_pages.h
const char *const BUILD_DATE = "2026. jan. 01.";
const char *AP_SSID = "ODROID-GO_Config";
// CONFIG_FILE is in config_utils.cpp

// --- Globals ---
// server is defined in web_server.cpp
TFT_eSPI tft = TFT_eSPI();
// Globals ssid, password, transHost... are in config_utils.cpp

State currentState = STATE_AP_MODE;
int otaProgress = 0;

// --- Function Prototypes ---
void loadConfig();
void saveConfig();

void deleteConfig();
void setupAP(); // This will be replaced by startAPMode, but keeping for now as
                // it's called in setupServerRoutes
void startAPMode();
void connectToWiFi();
// setupServerRoutes and handlers are now in web_server.cpp

// --- Setup ---
void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println("BOOT: Starting Full Firmware...");

  setupInputs();  // Initialize buttons
  setupBattery(); // Initialize ADC

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
  // --- Main Loop Logic ---
  // 1. WiFi Status Management
  static unsigned long dhcpStartTime = 0;

  if (currentState == STATE_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi Connected! Requesting IP...");
      currentState = STATE_DHCP;
      dhcpStartTime = millis();
      drawStatusBar();
    } else if (WiFi.status() == WL_CONNECT_FAILED ||
               WiFi.status() == WL_NO_SSID_AVAIL) {
      // Automatic retry handled by ESP32, but we could timeout here
    }
  }

  if (currentState == STATE_DHCP) {
    if (WiFi.localIP()[0] != 0) {
      Serial.println("IP Obtained!");
      currentState = STATE_CONNECTED;
      drawStatusBar();
    } else {
      if (millis() - dhcpStartTime > 30000) {
        Serial.println("DHCP Timeout! Switching to AP Mode.");
        startAPMode();
      }
    }
  }

  if (currentState == STATE_CONNECTED) {
    if (WiFi.status() != WL_CONNECTED) {
      currentState = STATE_CONNECTING;
      drawStatusBar();
    }
  }

  if (currentState == STATE_AP_MODE || currentState == STATE_CONNECTED) {
    server.handleClient();
  }

  if (currentState == STATE_AP_MODE || currentState == STATE_CONNECTED ||
      currentState == STATE_OTA) {
    ArduinoOTA.handle();
  }

  // --- Input & GUI Handling ---
  readInputs();

  if (btnMenuPressed) {
    if (currentState == STATE_MENU) {
      // Exit Menu
      if (WiFi.status() == WL_CONNECTED)
        currentState = STATE_CONNECTED;
      else
        currentState = STATE_AP_MODE;
      drawStatusBar();
    } else {
      currentState = STATE_MENU;
      drawMenu();
    }
  }

  // Menu Navigation
  if (currentState == STATE_MENU) {
    if (btnUpPressed) {
      Serial.println("UP");
      menuUp();
    }
    if (btnDownPressed) {
      Serial.println("DOWN");
      menuDown();
    }
    if (btnAPressed || btnSelectPressed) {
      Serial.println("SELECT");
      // Handle selection
      // menuIndex is extern from gui_handler.h
      if (menuIndex == 0) { // Status (Connected View)
        Serial.println("Selected Status (No-op)");
      } else if (menuIndex == 1) { // Settings
        currentState = STATE_SETTINGS;
        drawSettings();
      } else if (menuIndex == 2) { // About
        currentState = STATE_ABOUT;
        drawAbout();
      }
    }
  } else if (currentState == STATE_ABOUT || currentState == STATE_SETTINGS) {
    if (btnMenuPressed || btnBPressed) {
      currentState = STATE_MENU;
      drawMenu();
    }
  }

  // Update Status Bar (throttled to avoid WDT / heavy bus usage)
  static unsigned long lastDrawTime = 0;
  if (millis() - lastDrawTime > 250) {
    drawStatusBar();
    lastDrawTime = millis();
  }

  delay(10); // Yield to system/WDT
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

  // Show Setup Page
  handleScan();
}

void connectToWiFi() {
  Serial.printf("Connecting to %s\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  currentState = STATE_CONNECTING;
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

  Serial.println("Starting AP Mode");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  tft.fillRect(0, 25, 320, 215, TFT_BLUE);
  drawStatusBar();
}

// Helper to get back to main (removed huge server block)
// Moved all handlers to web_server.cpp
