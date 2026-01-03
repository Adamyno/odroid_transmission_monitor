#ifndef TRANSMISSION_CLIENT_H
#define TRANSMISSION_CLIENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

class TransmissionClient {
public:
  TransmissionClient();
  void begin();
  void update(); // Call in loop

  bool isConnected();
  long getDownloadSpeed(); // Bytes/sec
  long getUploadSpeed();   // Bytes/sec

private:
  unsigned long _lastUpdate;
  unsigned long _interval; // Interval in ms
  bool _connected;
  long _dlSpeed;
  long _ulSpeed;
  String _sessionId;

  void fetchStats();
};

extern TransmissionClient transmission;

#endif
