#include "torrent_list_gui.h"
#include "display_utils.h"
#include "transmission_client.h"
#include <TFT_eSPI.h>

// External TFT reference
extern TFT_eSPI tft;

// State variables
static TorrentListState listState = TORRENT_LIST_BROWSING;
static TorrentFilter currentFilter = FILTER_ALL;
static int scrollOffset = 0;
static int selectedTorrent = 0;

// Search state
static String searchQuery = "";
static int kbRow = 0;
static int kbCol = 0;
static bool shiftActive = false;

// Filtered torrent indices
static int filteredIndices[MAX_TORRENTS];
static int filteredCount = 0;

// Last fetch time
static unsigned long lastFetchTime = 0;

// Virtual keyboard layouts (same as wifi_scan_gui)
static const char *kbRowsLower[] = {"1234567890", "qwertyuiop", "asdfghjkl",
                                    "zxcvbnm"};
static const char *kbRowsUpper[] = {"!@#$%^&*()", "QWERTYUIOP", "ASDFGHJKL",
                                    "ZXCVBNM"};
#define KB_ROWS 5 // 4 char rows + 1 action row

// Filter names
const char *getFilterName(TorrentFilter filter) {
  switch (filter) {
  case FILTER_ALL:
    return "All";
  case FILTER_DOWNLOADING:
    return "Down";
  case FILTER_QUEUED_DOWN:
    return "QDown";
  case FILTER_SEEDING:
    return "Seed";
  case FILTER_QUEUED_SEED:
    return "QSeed";
  case FILTER_PAUSED:
    return "Paused";
  case FILTER_COMPLETE:
    return "Done";
  case FILTER_INCOMPLETE:
    return "Partial";
  case FILTER_ACTIVE:
    return "Active";
  case FILTER_CHECKING:
    return "Check";
  default:
    return "???";
  }
}

// Check if torrent matches current filter and search
static bool matchesFilter(const TorrentInfo &t) {
  // Check search query first
  if (searchQuery.length() > 0) {
    String nameLower = t.name;
    nameLower.toLowerCase();
    String queryLower = searchQuery;
    queryLower.toLowerCase();
    if (nameLower.indexOf(queryLower) < 0) {
      return false;
    }
  }

  // Check status filter
  switch (currentFilter) {
  case FILTER_ALL:
    return true;
  case FILTER_DOWNLOADING:
    return t.status == TR_STATUS_DOWNLOAD;
  case FILTER_QUEUED_DOWN:
    return t.status == TR_STATUS_DOWNLOAD_WAIT;
  case FILTER_SEEDING:
    return t.status == TR_STATUS_SEED;
  case FILTER_QUEUED_SEED:
    return t.status == TR_STATUS_SEED_WAIT;
  case FILTER_PAUSED:
    return t.status == TR_STATUS_STOPPED;
  case FILTER_COMPLETE:
    return t.percentDone >= 1.0;
  case FILTER_INCOMPLETE:
    return t.percentDone < 1.0;
  case FILTER_ACTIVE:
    return t.rateDownload > 0 || t.rateUpload > 0;
  case FILTER_CHECKING:
    return t.status == TR_STATUS_CHECK || t.status == TR_STATUS_CHECK_WAIT;
  default:
    return true;
  }
}

// Rebuild filtered list
static void applyFilter() {
  filteredCount = 0;
  TorrentInfo *torrents = transmission.getTorrents();
  int total = transmission.getTorrentCount();

  for (int i = 0; i < total && filteredCount < MAX_TORRENTS; i++) {
    if (matchesFilter(torrents[i])) {
      filteredIndices[filteredCount++] = i;
    }
  }

  // Adjust selection if out of bounds
  if (selectedTorrent >= filteredCount) {
    selectedTorrent = filteredCount > 0 ? filteredCount - 1 : 0;
  }
  if (scrollOffset > selectedTorrent) {
    scrollOffset = selectedTorrent;
  }
}

void initTorrentListGui() {
  listState = TORRENT_LIST_BROWSING;
  currentFilter = FILTER_ALL;
  scrollOffset = 0;
  selectedTorrent = 0;
  searchQuery = "";
  filteredCount = 0;
  lastFetchTime = 0;
}

// Format speed for display
static String formatSpeed(long bytesPerSec) {
  if (bytesPerSec < 1024) {
    return String(bytesPerSec) + "B/s";
  } else if (bytesPerSec < 1024 * 1024) {
    return String(bytesPerSec / 1024) + "K/s";
  } else {
    return String(bytesPerSec / (1024 * 1024), 1) + "M/s";
  }
}

// Get priority string
static const char *getPriorityStr(int priority) {
  if (priority > 0)
    return "Hi";
  if (priority < 0)
    return "Lo";
  return "Md";
}

// Draw progress bar
static void drawProgressBar(int x, int y, int w, int h, float percent) {
  // Background
  tft.fillRect(x, y, w, h, UI_GREY);
  // Filled portion
  int fillW = (int)(w * percent);
  if (fillW > 0) {
    tft.fillRect(x, y, fillW, h, TFT_GREEN);
  }
}

// Draw filter bar at top
static void drawFilterBar() {
  int y = 24;
  tft.fillRect(0, y, 320, 20, UI_TAB_BG);

  tft.setTextSize(1);

  // Search indicator
  tft.setTextColor(UI_GREY, UI_TAB_BG);
  tft.setCursor(5, y + 6);
  if (searchQuery.length() > 0) {
    tft.print("\"");
    // Truncate long search
    if (searchQuery.length() > 8) {
      tft.print(searchQuery.substring(0, 8));
      tft.print("..");
    } else {
      tft.print(searchQuery);
    }
    tft.print("\"");
  } else {
    tft.print("[VOL:Search]");
  }

  // Filter name
  tft.setTextColor(UI_CYAN, UI_TAB_BG);
  tft.setCursor(110, y + 6);
  tft.print(getFilterName(currentFilter));

  // Count
  int total = transmission.getTorrentCount();
  tft.setTextColor(UI_WHITE, UI_TAB_BG);
  tft.setCursor(260, y + 6);
  tft.printf("%d/%d", filteredCount, total);
}

// Draw a single torrent row
static void drawTorrentRow(int listIdx, int screenY, bool selected) {
  if (listIdx >= filteredCount)
    return;

  int idx = filteredIndices[listIdx];
  TorrentInfo *torrents = transmission.getTorrents();
  const TorrentInfo &t = torrents[idx];

  int rowH = 36;

  // Background
  uint16_t bgColor = selected ? UI_SELECTED_BG : UI_BG;
  tft.fillRect(0, screenY, 320, rowH, bgColor);

  // Line 1: Name (truncated)
  tft.setTextSize(1);
  tft.setTextColor(UI_WHITE, bgColor);
  tft.setCursor(5, screenY + 3);

  String name = t.name;
  if (name.length() > 42) {
    name = name.substring(0, 42);
  }
  tft.print(name);

  // Status icon based on status
  const char *statusIcon = "";
  uint16_t statusColor = UI_GREY;
  switch (t.status) {
  case TR_STATUS_DOWNLOAD:
    statusIcon = "v";
    statusColor = TFT_GREEN;
    break;
  case TR_STATUS_SEED:
    statusIcon = "^";
    statusColor = UI_CYAN;
    break;
  case TR_STATUS_STOPPED:
    statusIcon = "||";
    statusColor = TFT_YELLOW;
    break;
  case TR_STATUS_CHECK:
  case TR_STATUS_CHECK_WAIT:
    statusIcon = "?";
    statusColor = TFT_ORANGE;
    break;
  case TR_STATUS_DOWNLOAD_WAIT:
  case TR_STATUS_SEED_WAIT:
    statusIcon = "Q";
    statusColor = UI_GREY;
    break;
  }
  tft.setTextColor(statusColor, bgColor);
  tft.setCursor(305, screenY + 3);
  tft.print(statusIcon);

  // Line 2: Stats
  int y2 = screenY + 16;
  tft.setTextColor(UI_GREY, bgColor);
  tft.setCursor(5, y2);

  // U: and D: speeds
  tft.print("U:");
  tft.setTextColor(UI_CYAN, bgColor);
  tft.print(formatSpeed(t.rateUpload));

  tft.setTextColor(UI_GREY, bgColor);
  tft.print(" D:");
  tft.setTextColor(TFT_GREEN, bgColor);
  tft.print(formatSpeed(t.rateDownload));

  // Priority
  tft.setTextColor(UI_GREY, bgColor);
  tft.setCursor(150, y2);
  tft.print("Pri:");
  tft.setTextColor(UI_WHITE, bgColor);
  tft.print(getPriorityStr(t.bandwidthPriority));

  // Ratio
  tft.setTextColor(UI_GREY, bgColor);
  tft.setCursor(195, y2);
  tft.print("R:");
  tft.setTextColor(UI_WHITE, bgColor);
  if (t.uploadRatio < 0) {
    tft.print("-");
  } else {
    tft.printf("%.1f", t.uploadRatio);
  }

  // Percent
  int pct = (int)(t.percentDone * 100);
  tft.setTextColor(UI_WHITE, bgColor);
  tft.setCursor(235, y2);
  tft.printf("%3d%%", pct);

  // Progress bar
  drawProgressBar(270, y2, 45, 8, t.percentDone);

  // Divider line
  tft.drawFastHLine(5, screenY + rowH - 1, 310, UI_GREY);
}

// Draw virtual keyboard for search
static void drawSearchKeyboard() {
  tft.fillRect(0, 24, 320, 216, UI_BG);

  tft.setTextSize(2);
  tft.setTextColor(UI_CYAN, UI_BG);
  tft.setCursor(10, 30);
  tft.print("Search:");

  // Search box
  tft.fillRect(100, 25, 210, 24, UI_CARD_BG);
  tft.setTextSize(1);
  tft.setTextColor(UI_WHITE, UI_CARD_BG);
  tft.setCursor(105, 33);
  String displayQ = searchQuery;
  if (displayQ.length() > 24)
    displayQ = displayQ.substring(displayQ.length() - 24);
  tft.print(displayQ);
  tft.print("_");

  // Keyboard
  int keyW = 28;
  int keyH = 22;
  int startY = 60;

  const char **rows = shiftActive ? kbRowsUpper : kbRowsLower;

  for (int row = 0; row < 4; row++) {
    int rowLen = strlen(rows[row]);
    int startX = (320 - rowLen * keyW) / 2;

    for (int col = 0; col < rowLen; col++) {
      int x = startX + col * keyW;
      int y = startY + row * (keyH + 4);

      bool sel = (kbRow == row && kbCol == col);

      if (sel) {
        tft.fillRoundRect(x, y, keyW - 2, keyH, 3, UI_CYAN);
        tft.setTextColor(TFT_BLACK, UI_CYAN);
      } else {
        tft.fillRoundRect(x, y, keyW - 2, keyH, 3, UI_CARD_BG);
        tft.setTextColor(UI_WHITE, UI_CARD_BG);
      }

      tft.setCursor(x + 10, y + 6);
      tft.print(rows[row][col]);
    }
  }

  // Action row
  int actY = startY + 4 * (keyH + 4);
  const char *actions[] = {"Shift", "Space", "Clear"};
  int actW = 80;
  int actX = (320 - 3 * actW) / 2;

  for (int i = 0; i < 3; i++) {
    bool sel = (kbRow == 4 && kbCol == i);
    int x = actX + i * actW;

    if (sel) {
      tft.fillRoundRect(x, actY, actW - 4, keyH, 3, UI_CYAN);
      tft.setTextColor(TFT_BLACK, UI_CYAN);
    } else {
      tft.fillRoundRect(x, actY, actW - 4, keyH, 3, UI_CARD_BG);
      tft.setTextColor(UI_WHITE, UI_CARD_BG);
    }

    tft.setCursor(x + 10, actY + 6);
    tft.print(actions[i]);
  }

  // Hint bar
  tft.fillRect(0, 220, 320, 20, UI_TAB_BG);
  tft.setTextColor(UI_GREY, UI_TAB_BG);
  tft.setCursor(10, 225);
  tft.print("A:Type  B:Back  VOL:Done  START:Clear");
}

void drawTorrentList() {
  // Fetch immediately on first draw, then every 3 seconds
  if (lastFetchTime == 0 || millis() - lastFetchTime > 3000) {
    transmission.fetchTorrents();
    applyFilter();
    lastFetchTime = millis();
  }

  if (listState == TORRENT_LIST_SEARCHING) {
    drawSearchKeyboard();
    return;
  }

  // Clear content area
  tft.fillRect(0, 24, 320, 196, UI_BG);

  // Draw filter bar
  drawFilterBar();

  // Draw torrent rows
  int rowH = 36;
  int contentY = 44;
  int maxVisible = 5; // 180 / 36 = 5 rows

  for (int i = 0; i < maxVisible && (scrollOffset + i) < filteredCount; i++) {
    int listIdx = scrollOffset + i;
    bool selected = (listIdx == selectedTorrent);
    drawTorrentRow(listIdx, contentY + i * rowH, selected);
  }

  // Empty state
  if (filteredCount == 0) {
    tft.setTextSize(1);
    tft.setTextColor(UI_GREY, UI_BG);
    tft.setCursor(100, 120);
    if (transmission.getTorrentCount() == 0) {
      tft.print("No torrents found");
    } else {
      tft.print("No matches for filter");
    }
  }

  // Scroll indicator
  if (filteredCount > maxVisible) {
    int barH = 150;
    int barY = 50;
    int thumbH = max(20, barH * maxVisible / filteredCount);
    int thumbY = barY + (barH - thumbH) * scrollOffset /
                            max(1, filteredCount - maxVisible);

    tft.fillRect(316, barY, 4, barH, UI_GREY);
    tft.fillRect(316, thumbY, 4, thumbH, UI_CYAN);
  }

  // Hint bar
  tft.fillRect(0, 220, 320, 20, UI_TAB_BG);
  tft.setTextSize(1);
  tft.setTextColor(UI_GREY, UI_TAB_BG);
  tft.setCursor(5, 225);
  tft.print("MENU:");
  tft.setTextColor(UI_WHITE, UI_TAB_BG);
  tft.print("Menu ");
  tft.setTextColor(UI_GREY, UI_TAB_BG);
  tft.print("VOL:");
  tft.setTextColor(UI_WHITE, UI_TAB_BG);
  tft.print("Srch ");
  tft.setTextColor(UI_GREY, UI_TAB_BG);
  tft.print("L/R:");
  tft.setTextColor(UI_WHITE, UI_TAB_BG);
  tft.print("Filt ");
  tft.setTextColor(UI_GREY, UI_TAB_BG);
  tft.print("SEL:");
  tft.setTextColor(UI_WHITE, UI_TAB_BG);
  tft.print("Spd ");
  tft.setTextColor(UI_GREY, UI_TAB_BG);
  tft.print("START:");
  tft.setTextColor(UI_WHITE, UI_TAB_BG);
  tft.print("Pause");
}

bool handleTorrentListInput(bool up, bool down, bool left, bool right, bool a,
                            bool b, bool start, bool select, bool volume) {
  bool update = false;

  if (listState == TORRENT_LIST_SEARCHING) {
    // Keyboard navigation
    if (up && kbRow > 0) {
      kbRow--;
      int maxCol = (kbRow < 4) ? strlen(kbRowsLower[kbRow]) - 1 : 2;
      if (kbCol > maxCol)
        kbCol = maxCol;
      update = true;
    } else if (down && kbRow < KB_ROWS - 1) {
      kbRow++;
      int maxCol = (kbRow < 4) ? strlen(kbRowsLower[kbRow]) - 1 : 2;
      if (kbCol > maxCol)
        kbCol = maxCol;
      update = true;
    } else if (left) {
      int maxCol = (kbRow < 4) ? strlen(kbRowsLower[kbRow]) - 1 : 2;
      if (kbCol > 0)
        kbCol--;
      else
        kbCol = maxCol;
      update = true;
    } else if (right) {
      int maxCol = (kbRow < 4) ? strlen(kbRowsLower[kbRow]) - 1 : 2;
      if (kbCol < maxCol)
        kbCol++;
      else
        kbCol = 0;
      update = true;
    } else if (a) {
      if (kbRow < 4) {
        // Type character
        const char **rows = shiftActive ? kbRowsUpper : kbRowsLower;
        if (searchQuery.length() < 30) {
          searchQuery += rows[kbRow][kbCol];
        }
      } else {
        // Action row
        if (kbCol == 0) {
          shiftActive = !shiftActive;
        } else if (kbCol == 1) {
          if (searchQuery.length() < 30)
            searchQuery += ' ';
        } else if (kbCol == 2) {
          searchQuery = "";
        }
      }
      update = true;
    } else if (b) {
      // Backspace - delete last character
      if (searchQuery.length() > 0) {
        searchQuery = searchQuery.substring(0, searchQuery.length() - 1);
      }
      update = true;
    } else if (volume) {
      // Done searching (same button that opened search)
      listState = TORRENT_LIST_BROWSING;
      applyFilter();
      update = true;
    } else if (start) {
      // Clear search
      searchQuery = "";
      update = true;
    }
  } else {
    // Browsing mode
    if (up && selectedTorrent > 0) {
      selectedTorrent--;
      if (selectedTorrent < scrollOffset) {
        scrollOffset = selectedTorrent;
      }
      update = true;
    } else if (down && selectedTorrent < filteredCount - 1) {
      selectedTorrent++;
      int maxVisible = 5;
      if (selectedTorrent >= scrollOffset + maxVisible) {
        scrollOffset = selectedTorrent - maxVisible + 1;
      }
      update = true;
    } else if (left) {
      // Previous filter
      int f = (int)currentFilter;
      f = (f - 1 + FILTER_COUNT) % FILTER_COUNT;
      currentFilter = (TorrentFilter)f;
      scrollOffset = 0;
      selectedTorrent = 0;
      applyFilter();
      update = true;
    } else if (right) {
      // Next filter
      int f = (int)currentFilter;
      f = (f + 1) % FILTER_COUNT;
      currentFilter = (TorrentFilter)f;
      scrollOffset = 0;
      selectedTorrent = 0;
      applyFilter();
      update = true;
      // SELECT is reserved for Alt-speed toggle globally - don't handle here
    } else if (false) {
      // Removed: SELECT conflict with Alt-speed
    } else if (start) {
      // Toggle pause/resume
      if (filteredCount > 0 && selectedTorrent < filteredCount) {
        int idx = filteredIndices[selectedTorrent];
        TorrentInfo *torrents = transmission.getTorrents();
        transmission.toggleTorrentPause(torrents[idx].id);
        update = true;
      }
    } else if (volume) {
      // Open search
      listState = TORRENT_LIST_SEARCHING;
      kbRow = 0;
      kbCol = 0;
      shiftActive = false;
      update = true;
    } else if (a) {
      // Select torrent (future: details view)
      // For now, just toggle pause
      if (filteredCount > 0 && selectedTorrent < filteredCount) {
        int idx = filteredIndices[selectedTorrent];
        TorrentInfo *torrents = transmission.getTorrents();
        transmission.toggleTorrentPause(torrents[idx].id);
        update = true;
      }
    }
    // Note: B is handled in main.cpp to return to menu
  }

  return update;
}

void enterTorrentSearchMode() {
  listState = TORRENT_LIST_SEARCHING;
  kbRow = 0;
  kbCol = 0;
  shiftActive = false;
}

bool isInTorrentSearchMode() { return listState == TORRENT_LIST_SEARCHING; }
