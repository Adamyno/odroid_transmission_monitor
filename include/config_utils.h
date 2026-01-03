#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// Global Configuration Variables
extern String ssid;
extern String password;

extern String apSSID;
extern String apPassword;

extern String transHost;
extern int transPort;
extern String transPath;
extern String transUser;
extern String transPass;

extern int brightness;

// Functions
void loadConfig();
void saveConfig();
void deleteConfig();
void factoryReset();

#endif
