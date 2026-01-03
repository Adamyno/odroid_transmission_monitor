#include "transmission_client.h"
#include "config_utils.h" // For transHost, etc.
#include <WiFi.h>

TransmissionClient::TransmissionClient() {
  _lastUpdate = 0;
  _interval = 2000; // Update every 2 seconds by default
  _connected = false;
  _dlSpeed = 0;
  _ulSpeed = 0;
  _altSpeedEnabled = false;
  _freeSpace = 0;
  _sessionId = "";
}

void TransmissionClient::begin() {
  // Initial setup if needed
}

void TransmissionClient::update() {
  if (WiFi.status() != WL_CONNECTED) {
    _connected = false;
    return;
  }

  if (millis() - _lastUpdate > _interval) {
    _lastUpdate = millis();
    fetchStats();
  }
}

bool TransmissionClient::isConnected() { return _connected; }

long TransmissionClient::getDownloadSpeed() { return _dlSpeed; }

long TransmissionClient::getUploadSpeed() { return _ulSpeed; }

bool TransmissionClient::isAltSpeedEnabled() { return _altSpeedEnabled; }

long long TransmissionClient::getFreeSpace() { return _freeSpace; }

void TransmissionClient::toggleAltSpeed() {
  if (transHost.length() == 0 || !_connected)
    return;

  HTTPClient http;
  String url = "http://" + transHost + ":" + String(transPort) + transPath;

  http.begin(url);
  if (transUser.length() > 0) {
    http.setAuthorization(transUser.c_str(), transPass.c_str());
  }
  http.addHeader("Content-Type", "application/json");
  if (_sessionId.length() > 0) {
    http.addHeader("X-Transmission-Session-Id", _sessionId);
  }
  http.setTimeout(1500);

  // Toggle to opposite of current state
  String payload =
      "{\"method\":\"session-set\",\"arguments\":{\"alt-speed-enabled\":";
  payload += (_altSpeedEnabled ? "false" : "true");
  payload += "}}";

  const char *headerKeys[] = {"X-Transmission-Session-Id"};
  http.collectHeaders(headerKeys, 1);

  int httpCode = http.POST(payload);

  if (httpCode == 409) {
    _sessionId = http.header("X-Transmission-Session-Id");
    http.end();

    http.begin(url);
    if (transUser.length() > 0) {
      http.setAuthorization(transUser.c_str(), transPass.c_str());
    }
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Transmission-Session-Id", _sessionId);
    httpCode = http.POST(payload);
  }

  if (httpCode == 200) {
    // Toggle was successful, update local state
    _altSpeedEnabled = !_altSpeedEnabled;
    Serial.println(_altSpeedEnabled ? "Alt Speed ON" : "Alt Speed OFF");
  }

  http.end();
}

void TransmissionClient::fetchStats() {
  if (transHost.length() == 0)
    return;

  HTTPClient http;
  String url = "http://" + transHost + ":" + String(transPort) + transPath;

  http.begin(url);
  if (transUser.length() > 0) {
    http.setAuthorization(transUser.c_str(), transPass.c_str());
  }
  http.addHeader("Content-Type", "application/json");
  if (_sessionId.length() > 0) {
    http.addHeader("X-Transmission-Session-Id", _sessionId);
  }
  http.setTimeout(1500);

  // Request session-stats for speeds
  String payload = "{\"method\":\"session-stats\"}";

  const char *headerKeys[] = {"X-Transmission-Session-Id"};
  http.collectHeaders(headerKeys, 1);

  int httpCode = http.POST(payload);

  if (httpCode == 409) {
    _sessionId = http.header("X-Transmission-Session-Id");
    http.end();

    http.begin(url);
    if (transUser.length() > 0) {
      http.setAuthorization(transUser.c_str(), transPass.c_str());
    }
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Transmission-Session-Id", _sessionId);
    httpCode = http.POST(payload);
  }

  if (httpCode == 200) {
    String resp = http.getString();
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, resp);

    if (!error && doc["result"] == "success") {
      _connected = true;
      _dlSpeed = doc["arguments"]["downloadSpeed"];
      _ulSpeed = doc["arguments"]["uploadSpeed"];
    } else {
      _connected = false;
    }
  } else {
    _connected = false;
  }

  http.end();

  // Now fetch session-get for alt-speed-enabled
  if (_connected) {
    http.begin(url);
    if (transUser.length() > 0) {
      http.setAuthorization(transUser.c_str(), transPass.c_str());
    }
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Transmission-Session-Id", _sessionId);
    http.setTimeout(1500);

    String payload2 = "{\"method\":\"session-get\",\"arguments\":{\"fields\":["
                      "\"alt-speed-enabled\",\"download-dir-free-space\"]}}";
    int code2 = http.POST(payload2);

    if (code2 == 200) {
      String resp2 = http.getString();
      DynamicJsonDocument doc2(4096); // Larger buffer for full session response
      DeserializationError err2 = deserializeJson(doc2, resp2);
      if (!err2 && doc2["result"] == "success") {
        _altSpeedEnabled = doc2["arguments"]["alt-speed-enabled"] | false;
        _freeSpace = doc2["arguments"]["download-dir-free-space"] | 0LL;
      }
    }

    http.end();
  }
}

TransmissionClient transmission;
