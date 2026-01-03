#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "display_utils.h" // For State enum? Or forward declare?
#include <ArduinoJson.h>
#include <WebServer.h>
// Needs access to various globals or callbacks...
// For now, let's keep it simple and declare external dependencies if needed.

extern WebServer server;

void setupServerRoutes();
void handleRoot();
void handleScanWifi(); // New JSON handler
void handleScan();
void handleSave();
void handleReset();
void handleRestart();
void handleStatus();
void handleGetParams();
void handleSaveParams();
void handleSaveParams();
void handleTestTransmission();
String testTransmission(const String &host, int port, const String &path,
                        const String &user, const String &pass);

#endif
