#include "config_utils.h"

// Define globals
String ssid = "";
String password = "";
String apSSID = "ODROID-GO";
String apPassword = "";

String transHost = "";
int transPort = 9091;
String transPath = "/transmission/rpc";
String transUser = "";
String transPass = "";

int brightness = 255; // Default max

const char *CONFIG_FILE = "/config.json";

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
    if (doc.containsKey("ap_ssid")) {
      apSSID = doc["ap_ssid"].as<String>();
      apPassword = doc["ap_password"].as<String>();
    }
    if (doc.containsKey("brightness")) {
      brightness = doc["brightness"] | 255;
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
  doc["ap_ssid"] = apSSID;
  doc["ap_password"] = apPassword;
  doc["brightness"] = brightness;

  File file = LittleFS.open(CONFIG_FILE, "w");
  serializeJson(doc, file);
  file.close();
}

void deleteConfig() { LittleFS.remove(CONFIG_FILE); }
