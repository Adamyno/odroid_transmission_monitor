#ifndef TRANSMISSION_CLIENT_H
#define TRANSMISSION_CLIENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// Torrent status enum (matches Transmission API)
enum TorrentStatus {
  TR_STATUS_STOPPED = 0,
  TR_STATUS_CHECK_WAIT = 1,
  TR_STATUS_CHECK = 2,
  TR_STATUS_DOWNLOAD_WAIT = 3,
  TR_STATUS_DOWNLOAD = 4,
  TR_STATUS_SEED_WAIT = 5,
  TR_STATUS_SEED = 6
};

// Max torrents we can store
#define MAX_TORRENTS 200

// Torrent data structure
struct TorrentInfo {
  int id;
  String name;
  int status;
  float percentDone; // 0.0 - 1.0
  long rateDownload; // bytes/sec
  long rateUpload;   // bytes/sec
  float uploadRatio;
  int bandwidthPriority; // -1=Low, 0=Normal, 1=High
};

class TransmissionClient {
public:
  TransmissionClient();
  void begin();
  void update(); // Call in loop

  bool isConnected();
  long getDownloadSpeed(); // Bytes/sec
  long getUploadSpeed();   // Bytes/sec
  bool isAltSpeedEnabled();
  long long getFreeSpace(); // Bytes
  void toggleAltSpeed();

  // Torrent list methods
  int getTorrentCount();
  TorrentInfo *getTorrents();
  void fetchTorrents();
  void toggleTorrentPause(int torrentId);

private:
  unsigned long _lastUpdate;
  unsigned long _interval; // Interval in ms
  bool _connected;
  long _dlSpeed;
  long _ulSpeed;
  bool _altSpeedEnabled;
  long long _freeSpace;
  String _sessionId;

  // Torrent storage
  TorrentInfo _torrents[MAX_TORRENTS];
  int _torrentCount;

  void fetchStats();
};

extern TransmissionClient transmission;

#endif
