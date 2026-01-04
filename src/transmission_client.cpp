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
  _torrentCount = 0;
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

int TransmissionClient::getTorrentCount() { return _torrentCount; }

TorrentInfo *TransmissionClient::getTorrents() { return _torrents; }

void TransmissionClient::fetchTorrents() {
  Serial.printf("fetchTorrents: host=%s, connected=%d\n", transHost.c_str(),
                _connected);

  if (transHost.length() == 0 || !_connected) {
    Serial.println("fetchTorrents: SKIPPED (no host or not connected)");
    return;
  }

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
  http.setTimeout(3000); // Longer timeout for torrent list

  // Request torrent list with needed fields
  String payload = "{\"method\":\"torrent-get\",\"arguments\":{\"fields\":["
                   "\"id\",\"name\",\"status\",\"percentDone\","
                   "\"rateDownload\",\"rateUpload\",\"uploadRatio\","
                   "\"bandwidthPriority\"]}}";

  const char *headerKeys[] = {"X-Transmission-Session-Id"};
  http.collectHeaders(headerKeys, 1);

  int httpCode = http.POST(payload);
  Serial.printf("fetchTorrents: POST httpCode=%d\n", httpCode);

  if (httpCode == 409) {
    _sessionId = http.header("X-Transmission-Session-Id");
    Serial.println("fetchTorrents: Got 409, retrying with new session");
    http.end();

    http.begin(url);
    if (transUser.length() > 0) {
      http.setAuthorization(transUser.c_str(), transPass.c_str());
    }
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Transmission-Session-Id", _sessionId);
    http.setTimeout(3000);
    httpCode = http.POST(payload);
    Serial.printf("fetchTorrents: Retry httpCode=%d\n", httpCode);
  }

  if (httpCode == 200) {
    String resp = http.getString();
    Serial.printf("fetchTorrents: Response len=%d\n", resp.length());

    // Use larger buffer for torrent list (response is ~30KB, need ~2x for
    // ArduinoJson)
    DynamicJsonDocument doc(65536);
    DeserializationError error = deserializeJson(doc, resp);

    if (error) {
      Serial.printf("fetchTorrents: JSON parse error: %s\n", error.c_str());
    }

    if (!error && doc["result"] == "success") {
      JsonArray torrents = doc["arguments"]["torrents"];
      _torrentCount = 0;

      for (JsonObject t : torrents) {
        if (_torrentCount >= MAX_TORRENTS)
          break;

        _torrents[_torrentCount].id = t["id"];
        _torrents[_torrentCount].name = t["name"].as<String>();
        _torrents[_torrentCount].status = t["status"];
        _torrents[_torrentCount].percentDone = t["percentDone"];
        _torrents[_torrentCount].rateDownload = t["rateDownload"];
        _torrents[_torrentCount].rateUpload = t["rateUpload"];
        _torrents[_torrentCount].uploadRatio = t["uploadRatio"];
        _torrents[_torrentCount].bandwidthPriority = t["bandwidthPriority"];
        _torrentCount++;
      }

      Serial.printf("Fetched %d torrents\n", _torrentCount);
    }
  }

  http.end();
}

void TransmissionClient::toggleTorrentPause(int torrentId) {
  if (transHost.length() == 0 || !_connected)
    return;

  // Find current status
  int currentStatus = -1;
  for (int i = 0; i < _torrentCount; i++) {
    if (_torrents[i].id == torrentId) {
      currentStatus = _torrents[i].status;
      break;
    }
  }

  if (currentStatus == -1)
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
  http.setTimeout(2000);

  // If stopped, start it. Otherwise, stop it.
  String method =
      (currentStatus == TR_STATUS_STOPPED) ? "torrent-start" : "torrent-stop";
  String payload = "{\"method\":\"" + method + "\",\"arguments\":{\"ids\":[" +
                   String(torrentId) + "]}}";

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
    Serial.printf("Torrent %d: %s\n", torrentId, method.c_str());
    // Refresh torrent list
    fetchTorrents();
  }

  http.end();
}

TransmissionClient transmission;
