# Changelog

## [1.3.9] - 2026-01-02
### Fixed
- AP Mode: Resolved issue where custom SSID/Password settings were persistent in config but not actually used when starting the Access Point. 
- Web UI: Fixed a CSS bug on the Status page where the middle bar of the signal icon was grey instead of cyan in the -60 to -69 dBm range.

## [1.3.8] - 2026-01-02
### Added
- Settings: AP Mode Config card to customize Access Point SSID and Password.
- WiFi Scan: Signal strength bars (4-level icon) in AP mode network list.
- Battery: Now displayed in both AP and STA modes.
### Fixed
- Signal Strength Icon: Corrected boundary conditions (`>=` instead of `>`) for accurate icon display at -60, -70, -80 dBm thresholds.
- Button Feedback: Replaced `alert()` dialogs with inline visual feedback (encoding-safe) for Restart, Reset, and Save buttons.

## [1.3.7] - 2026-01-02
### Added
- Transmission connection test: Integrated CSRF handling and real-time speed reporting (DL/UL).
- Calibrated battery monitoring: Using factory eFuse calibration and 64x multi-sampling for stable readings.
### Fixed
- Web OTA: Improved reliability with response delays and fixed progress bar calculation using `Content-Length`.
- Screen UI: Fixed WiFi signal icon visibility and activity during OTA updates.
### Changed
- Web Dashboard: Enhanced OTA UI with AJAX-based progress on the button and visual feedback (blue color).

## [1.3.6] - 2026-01-01
### Changed
- WiFi Connection Logic: Removed physical LED blinking.
- Status Bar: Added detailed feedback for "Connecting:" (Grey icon) and "DHCP request..." (Green icon).
- Connection Timeout: Implemented a 30-second timeout for DHCP to fallback to AP mode.

## [1.3.5] - 2026-01-01
### Changed
- Refined status bar: Completely removed the last remaining white line from boot setup.
- OTA Display: Updated label to "OTA UPGRADE..." for better clarity.

## [1.3.4] - 2026-01-01
### Changed
- ODROID-GO Display: Darkened status bar background and removed the bottom white boundary line.

## [1.3.3] - 2026-01-01
### Changed
- OTA Update display: Replaced magenta background with a clean "OTA" text and a progress bar with boundary markers.

## [1.3.2] - 2026-01-01
### Changed
- Web UI: Improved battery percentage text readability (white color, dark shadow, centered alignment).

## [1.3.1] - 2026-01-01
### Fixed
- Corrected battery voltage reading (removed incorrect calibration factor).
### Changed
- Web UI: Battery icon moved next to nav buttons; percentage shown inside the icon.

## [1.3.0] - 2026-01-01
### Added
- Battery voltage and dynamic icon in the web dashboard (Navigation bar and Status tab).
- Color-coded battery alerts on Web UI (Yellow < 25%, Red < 10%).

## [1.2.0] - 2026-01-01
### Added
- "OTA UPDATE..." status indicator on the display when a firmware update is in progress (Web or OTA).

## [1.1.0] - 2026-01-01
### Added
- Battery status indicator on the status bar (far right).
- Repositioned WiFi signal icon to the left of the battery indicator.

## [1.0.1] - 2026-01-01
### Changed
- Relocated WiFi signal strength icon to the far right of the status bar for better visibility.
- Improved web interface "About" tab to display version and build date dynamically.

### Added
- Created this CHANGELOG.md file.

## [1.0.0] - 2026-01-01
- Initial release for ODROID-GO (ESP32).
- Migration from ESP8266 to ESP32.
- Added TFT display support.
- Added OTA update support.
- Added Web Dashboard with Transmission settings.
