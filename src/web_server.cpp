#include "web_server.h"
#include "battery_utils.h" // For battery voltage
#include "config_utils.h"  // For ssid, password, etc.
#include "web_pages.h"
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>

WebServer server(80);

extern int otaProgress;

const char *update_html = R"(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { font-family: sans-serif; padding: 20px; background: #222; color: #fff; text-align: center; }
form { background: #333; padding: 20px; border-radius: 8px; display: inline-block; }
input { padding: 10px; margin: 10px 0; width: 100%; box-sizing: border-box; }
input[type=submit] { background: #d35400; color: white; border: none; cursor: pointer; font-weight: bold; }
</style>
</head>
<body>
<h1>Firmware Update</h1>
<form id='upload_form' method='POST' action='/update' enctype='multipart/form-data'>
<input type='file' name='update' id='file_input'>
<input type='submit' value='Update' id='update_btn'>
</form>
<script>
var form = document.getElementById('upload_form');
form.onsubmit = function(event) {
  event.preventDefault();
  var fileInput = document.getElementById('file_input');
  var file = fileInput.files[0];
  if (!file) return;
  var formData = new FormData();
  formData.append('update', file);
  var xhr = new XMLHttpRequest();
  xhr.open('POST', '/update', true);
  xhr.upload.onprogress = function(e) {
    if (e.lengthComputable) {
      var percent = Math.round((e.loaded / e.total) * 100);
      document.getElementById('update_btn').value = '[' + percent + '%] Uploading...'; 
      document.getElementById('update_btn').style.backgroundColor = '#2980b9'; // Change color to blue
    }
  };
  xhr.onload = function() {
    if (xhr.status === 200) {
      document.body.innerHTML = '<h1>' + xhr.responseText + '</h1>';
    } else {
      document.getElementById('update_btn').value = 'Failed';
    }
  };
  xhr.send(formData);
};
</script>
</body>
</html>
)";

void handleUpdate() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(200, "text/html", update_html);
}

void handleUpdateUpload() {
  server.send(200, "text/plain",
              (Update.hasError()) ? "Update Failed"
                                  : "Update Success! Rebooting...");
  delay(1000); // Give time for the client to receive the response
  ESP.restart();
}

void handleUpdateMultipart() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    currentState = STATE_OTA; // Trigger UI update
    otaProgress = 0;
    drawStatusBar(); // Force initial draw

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // start with max available size
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    } else {
      // Calculate progress
      // Use Content-Length header if available
      size_t total = 0;
      if (server.hasHeader("Content-Length")) {
        total = server.header("Content-Length").toInt();
      }

      if (total > 0) {
        int p = (int)((float)Update.progress() / (float)total * 100.0);
        if (p > 100)
          p = 100;

        if (p != otaProgress) {
          otaProgress = p;
          Serial.printf("OTA Progress: %d%%\n", otaProgress);
          drawStatusBar();
        }
      }
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) { // true to set the size to the current progress
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      otaProgress = 100;
      drawStatusBar();
    } else {
      Update.printError(Serial);
    }
  }
}

// Forward declarations
void handleRoot();
void handleScanWifi();
void handleScan();
void handleSave();
void handleReset();
void handleRestart();
void handleStatus();
void handleGetParams();
void handleSaveParams();
void handleTestTransmission();

// ...
void setupServerRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/scan_wifi", HTTP_GET, handleScanWifi); // JSON handler
  server.on("/scan", HTTP_GET, handleScan);          // HTML Page
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/restart", HTTP_POST, handleRestart);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/get_params", HTTP_GET, handleGetParams);
  server.on("/save_params", HTTP_POST, handleSaveParams);
  server.on("/test_transmission", HTTP_POST, handleTestTransmission);

  // Firmware Update Handlers
  const char *headerKeys[] = {"Content-Length"};
  server.collectHeaders(headerKeys, 1);
  server.on("/update", HTTP_GET, handleUpdate);
  server.on("/update", HTTP_POST, handleUpdateUpload, handleUpdateMultipart);
}

// Scan handler that returns JSON for web_pages.h JS
void handleScanWifi() {
  Serial.println("Hit /scan_wifi");
  int n = WiFi.scanNetworks();
  Serial.printf("Scanned %d networks\n", n);
  DynamicJsonDocument doc(2048);
  JsonArray array = doc.to<JsonArray>();

  if (n > 0) {
    for (int i = 0; i < n; ++i) {
      JsonObject obj = array.createNestedObject();
      obj["ssid"] = WiFi.SSID(i);
      obj["rssi"] = WiFi.RSSI(i);
      obj["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
  }

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleRoot() {
  // Fix 1: Add Cache-Control headers
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  String page = dashboard_html;

  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    page.replace("%MODE%", "AP");
    page.replace("%IP%", WiFi.softAPIP().toString());
  } else {
    page.replace("%MODE%", "STA");
    page.replace("%IP%", WiFi.localIP().toString());
  }

  // Common Replacements
  page.replace("%SSID%",
               (WiFi.getMode() == WIFI_STA) ? WiFi.SSID() : "ODROID-AP");
  page.replace("%RSSI%",
               (WiFi.getMode() == WIFI_STA) ? String(WiFi.RSSI()) : "0");
  page.replace("%MAC%", WiFi.macAddress());
  page.replace("%VERSION%", VERSION);
  page.replace("%BUILD_DATE%", __DATE__ " " __TIME__);

  server.send(200, "text/html", page);
}

// /scan endpoint was previously serving index_html.
// Since we unified the UI, we can redirect /scan to / or remove it.
// However, to avoid broken links if any, let's redirect to root.
void handleScan() {
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    ssid = server.arg("ssid");
    password = server.arg("password");
    saveConfig();
    String response = "Saved. Restarting...";
    server.send(200, "text/plain", response);
    delay(1000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing ssid or password");
  }
}

void handleReset() {
  deleteConfig();
  String response = "Configuration deleted. Restarting...";
  server.send(200, "text/plain", response);
  delay(1000);
  ESP.restart();
}

void handleRestart() {
  server.send(200, "text/plain", "Restarting...");
  delay(1000);
  ESP.restart();
}

void handleStatus() {
  DynamicJsonDocument doc(512); // Increased size
  doc["rssi"] = WiFi.RSSI();
  doc["ip"] = WiFi.localIP().toString();

  // Prioritize "STA" mode if connected, even if AP is running in background
  // (WIFI_AP_STA)
  bool isAPMode =
      (WiFi.getMode() == WIFI_AP) ||
      ((WiFi.getMode() == WIFI_AP_STA) && (WiFi.status() != WL_CONNECTED));

  if (isAPMode) {
    doc["mode"] = "AP";
    doc["ap_ssid"] = apSSID;
    doc["ap_password"] = apPassword;
    doc["ip"] = WiFi.softAPIP().toString(); // Correct IP for AP mode
  } else {
    doc["mode"] = "STA";
  }

  // Battery
  float battV = getBatteryVoltage();
  doc["batt"] = battV;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleGetParams() {
  DynamicJsonDocument doc(512);
  doc["host"] = transHost;
  doc["port"] = transPort;
  doc["path"] = transPath;
  doc["user"] = transUser;
  doc["pass"] = transPass;
  doc["ap_ssid"] = apSSID;
  doc["ap_password"] = apPassword;
  doc["brightness"] = brightness;
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleSaveParams() {
  if (server.hasArg("host"))
    transHost = server.arg("host");
  if (server.hasArg("port"))
    transPort = server.arg("port").toInt();
  if (server.hasArg("path"))
    transPath = server.arg("path");
  if (server.hasArg("user"))
    transUser = server.arg("user");
  if (server.hasArg("pass"))
    transPass = server.arg("pass");
  bool apChanged = false;
  if (server.hasArg("ap_ssid")) {
    String newApSSID = server.arg("ap_ssid");
    if (newApSSID != apSSID) {
      apSSID = newApSSID;
      apChanged = true;
    }
  }
  if (server.hasArg("ap_password")) {
    String newApPass = server.arg("ap_password");
    if (newApPass != apPassword) {
      apPassword = newApPass;
      apChanged = true;
    }
  }
  if (server.hasArg("brightness")) {
    brightness = server.arg("brightness").toInt();
    setBrightness(brightness);
  }

  saveConfig();
  server.send(200, "text/plain", "Saved params");
  delay(100); // Give time for response to flush

  // Apply AP changes immediately if in AP or AP_STA mode
  if (apChanged) {
    if (apPassword.length() >= 8) {
      WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    } else {
      WiFi.softAP(apSSID.c_str());
    }
    Serial.println("AP Settings updated via WebUI");
  }
}

String formatSpeed(long bytes) {
  if (bytes < 1024)
    return String(bytes) + " B/s";
  if (bytes < 1048576)
    return String(bytes / 1024.0, 1) + " KB/s";
  return String(bytes / 1048576.0, 1) + " MB/s";
}

String testTransmission(const String &host, int port, const String &path,
                        const String &user, const String &pass) {
  if ((WiFi.status() != WL_CONNECTED) && (WiFi.getMode() != WIFI_AP_STA)) {
    return "WiFi Not Connected";
  }

  HTTPClient http;
  String url = "http://" + host + ":" + String(port) + path;
  Serial.println("Testing: " + url);

  // 1. Prepare Request to catch CSRF Token
  http.begin(url);
  if (user.length() > 0)
    http.setAuthorization(user.c_str(), pass.c_str());
  http.addHeader("Content-Type", "application/json");

  // Fix 2: Removed duplicate header

  const char *headerKeys[] = {"X-Transmission-Session-Id", "Content-Length"};
  http.collectHeaders(headerKeys, 2);

  String payload = "{\"method\":\"session-stats\"}";
  int httpCode = http.POST(payload);

  String sessionId = "";

  // 2. Handle CSRF (409)
  if (httpCode == 409) {
    sessionId = http.header("X-Transmission-Session-Id");
    Serial.println("Got CSRF Token: " + sessionId);
    http.end(); // Close first connection

    // Retry with Token
    http.begin(url);
    if (user.length() > 0)
      http.setAuthorization(user.c_str(), pass.c_str());
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Transmission-Session-Id", sessionId);
    httpCode = http.POST(payload);
  }

  String result = "";
  if (httpCode == 200) {
    String resp = http.getString();
    // Parse JSON
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, resp);
    if (!error) {
      if (doc.containsKey("result")) {
        String status = doc["result"].as<String>();
        if (status == "success") {
          long dl = doc["arguments"]["downloadSpeed"];
          long ul = doc["arguments"]["uploadSpeed"];
          result =
              "Success! DL: " + formatSpeed(dl) + ", UL: " + formatSpeed(ul);
        } else {
          result = "RPC Error: " + status;
        }
      } else {
        result = "Invalid JSON structure";
      }
    } else {
      result = "JSON Error";
    }
  } else if (httpCode == 401) {
    result = "Auth Failed (401)";
  } else {
    result = "Error: " + String(httpCode);
    if (httpCode < 0)
      result += " (" + http.errorToString(httpCode) + ")";
  }

  http.end();
  return result;
}

void handleTestTransmission() {
  String t_host = server.arg("host");
  int t_port = server.arg("port").toInt();
  String t_path = server.arg("path");
  String t_user = server.arg("user");
  String t_pass = server.arg("pass");

  String result = testTransmission(t_host, t_port, t_path, t_user, t_pass);
  server.send(200, "text/plain", result);
}
