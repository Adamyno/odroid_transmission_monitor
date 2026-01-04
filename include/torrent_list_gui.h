#ifndef TORRENT_LIST_GUI_H
#define TORRENT_LIST_GUI_H

#include <Arduino.h>

// Filter modes
enum TorrentFilter {
  FILTER_ALL = 0,
  FILTER_DOWNLOADING,
  FILTER_QUEUED_DOWN,
  FILTER_SEEDING,
  FILTER_QUEUED_SEED,
  FILTER_PAUSED,
  FILTER_COMPLETE,
  FILTER_INCOMPLETE,
  FILTER_ACTIVE,
  FILTER_CHECKING,
  FILTER_COUNT // Number of filters
};

// State enum for torrent list UI
enum TorrentListState { TORRENT_LIST_BROWSING, TORRENT_LIST_SEARCHING };

// Initialize the torrent list GUI
void initTorrentListGui();

// Draw the torrent list screen
void drawTorrentList();

// Handle input for the torrent list
// Returns true if screen needs redraw
bool handleTorrentListInput(bool up, bool down, bool left, bool right, bool a,
                            bool b, bool start, bool select, bool volume);

// Enter search mode (opens virtual keyboard)
void enterTorrentSearchMode();

// Check if currently in search mode
bool isInTorrentSearchMode();

// Get filter name as string
const char *getFilterName(TorrentFilter filter);

#endif
