#include "transmission_client.h"
#include "config_utils.h" // For transHost, etc.
#include <WiFi.h>

TransmissionClient::TransmissionClient() {
  _lastUpdate = 0;
  _interval = 2000; // Update every 2 seconds by default
  _connected = false;
  _dlSpeed = 0;
  _ulSpeed = 0;
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
  http.setTimeout(1500); // Short timeout to avoid blocking UI

  String payload = "{\"method\":\"session-stats\"}";

  // We need to collect the session ID header if 409
  const char *headerKeys[] = {"X-Transmission-Session-Id"};
  http.collectHeaders(headerKeys, 1);

  int httpCode = http.POST(payload);

  if (httpCode == 409) {
    // CSRF Error, get new token and retry immediately
    _sessionId = http.header("X-Transmission-Session-Id");
    http.end();

    // Retry
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
}

TransmissionClient transmission;
