#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Update.h>

#include <WebServer.h>
#include <WiFi.h>

#include "display_utils.h"
#include "web_pages.h"

// --- Configuration ---
// VERSION is in web_pages.h
const char *const BUILD_DATE = "2026. jan. 01.";
const char *AP_SSID = "ODROID-GO_Config";
const char *CONFIG_FILE = "/config.json";

// --- Globals ---
WebServer server(80);
TFT_eSPI tft = TFT_eSPI();
String ssid = "";
String password = "";

// Transmission Settings
String transHost = "";
int transPort = 9091;
String transPath = "/transmission/rpc";
String transUser = "";
String transPass = "";

State currentState = STATE_AP_MODE;

// LED Control (ODROID-GO has Blue LED on IO2)
#define LED_BUILTIN 2

unsigned long lastBlinkTime = 0;
bool ledState = false;

// --- Function Prototypes ---
void loadConfig();
void saveConfig();

void deleteConfig();
void setupAP();
void setupServerRoutes();
void handleRoot();
void handleScan();
void handleSave();
void handleReset();
void handleRestart();
void handleStatus();
void updateLED();
void handleGetParams();
void handleSaveParams();
void handleTestTransmission();

// --- Setup ---
void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(3); // ODROID-GO Landscape
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(
      LED_BUILTIN,
      LOW); // Off? (Depends on circuit, usually active high for ODROID?)

  tft.fillRect(0, 0, 320, 24, TFT_DARKGREY); // ODROID width is 320
  tft.drawFastHLine(0, 24, 320, TFT_WHITE);
  // drawStatusBar(); // Don't call yet, WiFi not ready

  // --- OTA Setup (Delay begin until WiFi/AP is ready) ---
  ArduinoOTA.setHostname("ODROID-GO-Monitor");
  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA");
    currentState = STATE_OTA;
    drawStatusBar();
  });
  ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError(
      [](ota_error_t error) { Serial.printf("Error[%u]: ", error); });

  // LittleFS

  if (!LittleFS.begin(true)) { // Format if fail
    Serial.println("LittleFS mount failed");
  }

  loadConfig();
  setupServerRoutes();

  if (ssid != "") {
    currentState = STATE_CONNECTING;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Connecting to: ");
    Serial.println(ssid);
  } else {
    setupAP();
  }

  ArduinoOTA.begin();
  server.begin();
  Serial.println("HTTP server started");
  drawStatusBar(); // Now safe to call
}

// --- Loop ---
void loop() {
  ArduinoOTA.handle();
  updateLED();

  if (currentState == STATE_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      currentState = STATE_CONNECTED;
      Serial.println("\nConnected!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());

      tft.fillRect(0, 25, 320, 215, TFT_BLACK); // ODROID height 240
      drawStatusBar();

    } else {
      static unsigned long startAttemptInfo = millis();
      if (millis() - startAttemptInfo > 20000) {
        Serial.println("Connection timeout. Switching to AP.");
        setupAP();
      }
    }
  } else if (currentState == STATE_CONNECTED) {
    if (WiFi.status() != WL_CONNECTED) {
      currentState = STATE_CONNECTING;
    }
  }

  server.handleClient();

  static unsigned long lastStatusUpdate = 0;
  if (millis() - lastStatusUpdate > 500) {
    lastStatusUpdate = millis();
    drawStatusBar();
  }
}

// --- Implementation ---

void updateLED() {
  unsigned long currentMillis = millis();

  if (currentState == STATE_AP_MODE) {
    if (currentMillis - lastBlinkTime >= 1000) {
      lastBlinkTime = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    }
  } else if (currentState == STATE_CONNECTING) {
    if (currentMillis - lastBlinkTime >= 200) {
      lastBlinkTime = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    }
  } else if (currentState == STATE_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW); // Off
  }
}

void setupAP() {
  currentState = STATE_AP_MODE;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);

  Serial.println("Starting AP Mode");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  tft.fillRect(0, 25, 320, 215, TFT_BLUE);
  drawStatusBar();
}

void setupServerRoutes() {
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/restart", HTTP_POST, handleRestart);
  server.on("/status", handleStatus);
  server.on("/getParams", handleGetParams);
  server.on("/saveParams", HTTP_POST, handleSaveParams);
  server.on("/testTransmission", HTTP_POST, handleTestTransmission);

  // Web Config Handler
  server.on("/scan", HTTP_GET, []() {
    String json = "[";
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i) {
      if (i)
        json += ",";
      json += "{\"ssid\":\"" + WiFi.SSID(i) +
              "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    json += "]";
    server.send(200, "application/json", json);
  });

  // Web OTA Handler
  server.on(
      "/update", HTTP_POST,
      []() {
        server.send(200, "text/plain",
                    (Update.hasError()) ? "Update Failed"
                                        : "Update Success! Rebooting...");
        delay(100);
        ESP.restart();
      },
      []() {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
          Serial.setDebugOutput(true);
          Serial.printf("Update: %s\n", upload.filename.c_str());
          if (!Update.begin(
                  UPDATE_SIZE_UNKNOWN)) { // start with max available size
            Update.printError(Serial);
          }
          currentState = STATE_OTA;
          drawStatusBar();
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (Update.write(upload.buf, upload.currentSize) !=
              upload.currentSize) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(true)) {
            Serial.printf("Update Success: %u\nRebooting...\n",
                          upload.totalSize);
          } else {
            Update.printError(Serial);
          }
          Serial.setDebugOutput(false);
        }
      });
}

void loadConfig() {
  if (LittleFS.exists(CONFIG_FILE)) {
    File file = LittleFS.open(CONFIG_FILE, "r");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, file);
    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();
    if (doc.containsKey("t_host")) {
      transHost = doc["t_host"].as<String>();
      transPort = doc["t_port"] | 9091;
      transPath = doc["t_path"].as<String>();
      transUser = doc["t_user"].as<String>();
      transPass = doc["t_pass"].as<String>();
    }
    file.close();
    Serial.println("Config loaded.");
  }
}

void saveConfig() {
  DynamicJsonDocument doc(512);
  doc["ssid"] = ssid;
  doc["password"] = password;
  doc["t_host"] = transHost;
  doc["t_port"] = transPort;
  doc["t_path"] = transPath;
  doc["t_user"] = transUser;
  doc["t_pass"] = transPass;

  File file = LittleFS.open(CONFIG_FILE, "w");
  serializeJson(doc, file);
  file.close();
}

void deleteConfig() { LittleFS.remove(CONFIG_FILE); }

void handleRoot() {
  if (currentState == STATE_CONNECTED) {
    String page = FPSTR(dashboard_html);
    page.replace("%SSID%", WiFi.SSID());

    page.replace("%IP%", WiFi.localIP().toString());
    page.replace("%RSSI%", String(WiFi.RSSI()));
    page.replace("%MAC%", WiFi.macAddress());
    page.replace("%VERSION%", VERSION);
    page.replace("%BUILD_DATE%", BUILD_DATE);
    server.send(200, "text/html", page);
  } else {
    server.send(200, "text/html", index_html);
  }
}

void handleScan() {
  int n = WiFi.scanNetworks();
  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < n; ++i) {
    JsonObject obj = arr.createNestedObject();
    obj["ssid"] = WiFi.SSID(i);
    obj["rssi"] = WiFi.RSSI(i);
  }

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    ssid = server.arg("ssid");
    password = server.arg("password");
    saveConfig();

    server.send(200, "text/plain", "Saved");
    delay(1000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing args");
  }
}

void handleReset() {
  deleteConfig();
  server.send(200, "text/plain", "Reset");
  delay(1000);
  ESP.restart();
}

void handleRestart() {
  server.send(200, "text/plain", "Restarting...");
  delay(1000);
  ESP.restart();
}

void handleStatus() {
  DynamicJsonDocument doc(64);
  doc["rssi"] = WiFi.RSSI();
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleGetParams() {
  DynamicJsonDocument doc(256);
  doc["host"] = transHost;
  doc["port"] = transPort;
  doc["path"] = transPath;
  doc["user"] = transUser;
  doc["pass"] = transPass;
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleSaveParams() {
  transHost = server.arg("host");
  transPort = server.arg("port").toInt();
  transPath = server.arg("path");
  transUser = server.arg("user");
  transPass = server.arg("pass");
  saveConfig();
  server.send(200, "text/plain", "Params saved!");
}

// --- Base64 Helper ---
// ESP32 doesn't need PROGMEM for char arrays efficiently usually but it's fine
static const char b64_alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
String base64Encode(String input) {
  String output = "";
  int i = 0, j = 0, len = input.length();
  unsigned char char_array_3[3], char_array_4[4];

  while (len--) {
    char_array_3[i++] = input[j++];
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] =
          ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] =
          ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;
      for (i = 0; i < 4; i++)
        output += b64_alphabet[char_array_4[i]];
      i = 0;
    }
  }
  if (i) {
    for (j = i; j < 3; j++)
      char_array_3[j] = '\0';
    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] =
        ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] =
        ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;
    for (j = 0; j < i + 1; j++)
      output += b64_alphabet[char_array_4[j]];
    while (i++ < 3)
      output += '=';
  }
  return output;
}

void handleTestTransmission() {
  String host = transHost;
  int port = transPort;
  String path = transPath;
  String user = transUser;
  String pass = transPass;

  if (server.hasArg("host"))
    host = server.arg("host");
  if (server.hasArg("port"))
    port = server.arg("port").toInt();
  if (server.hasArg("path"))
    path = server.arg("path");
  if (server.hasArg("user"))
    user = server.arg("user");
  if (server.hasArg("pass"))
    pass = server.arg("pass");

  if (host == "") {
    server.send(400, "text/plain", "Host invalid");
    return;
  }

  WiFiClient client;
  if (!client.connect(host.c_str(), port)) {
    server.send(200, "text/plain", "Conn Failed (TCP)");
    return;
  }

  // Helper lambda to send request
  auto sendRequest = [&](String sessionId) {
    String auth = "";
    if (user != "" || pass != "") {
      auth = "Authorization: Basic " + base64Encode(user + ":" + pass) + "\r\n";
    }

    // JSON RPC payload for session-stats
    String payload = "{\"method\":\"session-stats\"}";

    client.print(
        "POST " + path + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + auth +
        (sessionId != "" ? "X-Transmission-Session-Id: " + sessionId + "\r\n"
                         : "") +
        "Content-Type: application/json\r\n" +
        "Content-Length: " + String(payload.length()) + "\r\n" +
        "Connection: close\r\n\r\n" + payload);
  };

  // 1. First attempt (likely to fail with 409 or 401)
  sendRequest("");

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 3000) {
      client.stop();
      server.send(200, "text/plain", "Timeout (1)");
      return;
    }
  }

  String line = client.readStringUntil('\n');
  String sessionId = "";
  bool unauthorized = false;

  // Parse headers
  while (line.length() > 1) { // while not empty line
    if (line.indexOf("409") > 0) {
      // Conflict - good, look for session id
    }
    if (line.indexOf("401") > 0) {
      unauthorized = true;
    }
    if (line.startsWith("X-Transmission-Session-Id: ")) {
      sessionId = line.substring(27);
      sessionId.trim();
    }
    line = client.readStringUntil('\n');
  }

  // Clear buffer
  while (client.available())
    client.read();
  client.stop(); // Transmission usually closes on 409 anyway

  if (unauthorized) {
    server.send(200, "text/plain", "Auth Failed (401)");
    return;
  }

  if (sessionId == "") {
    // Maybe it worked first time? (Unlikely for Transmission)
    // Or invalid path?
    server.send(200, "text/plain", "No Session ID (Path?)");
    return;
  }

  // 2. Second attempt with Session ID
  if (!client.connect(host.c_str(), port)) {
    server.send(200, "text/plain", "Conn Failed (Reconnect)");
    return;
  }

  sendRequest(sessionId);

  timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 3000) {
      client.stop();
      server.send(200, "text/plain", "Timeout (2)");
      return;
    }
  }

  // Read response
  String response = client.readString();
  client.stop();

  // Simple parsing
  int jsonStart = response.indexOf("{");
  if (jsonStart > -1) {
    String jsonBody = response.substring(jsonStart);
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonBody);
    if (!error) {
      if (doc["result"] == "success") {
        auto formatSpeed = [](long speed) -> String {
          float kmb = speed / 1024.0;
          return (kmb > 1024) ? String(kmb / 1024.0, 1) + " MB/s"
                              : String(kmb, 0) + " KB/s";
        };

        long downSpeed = doc["arguments"]["downloadSpeed"];
        long upSpeed = doc["arguments"]["uploadSpeed"];

        server.send(200, "text/plain",
                    "Success! DL: " + formatSpeed(downSpeed) +
                        " | UL: " + formatSpeed(upSpeed));
      } else {

        server.send(200, "text/plain",
                    "RPC Error: " + doc["result"].as<String>());
      }
    } else {
      server.send(200, "text/plain", "JSON Parse Err");
    }
  } else {
    server.send(200, "text/plain", "Invalid Resp Body");
  }
}
