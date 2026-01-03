# Changelog

## [1.10.0] - 2026-01-03
### Added
- **Free Space Display**: The status bar now shows the free disk space of the Transmission download directory (e.g., "123G") with a yellow folder icon.

## [1.9.0] - 2026-01-03
### Changed
- **Turtle Icon**: Enhanced the alt-speed turtle icon on the status bar. It is now green with shell details, a head with an eye, and four visible legs.

## [1.8.2] - 2026-01-03
### Fixed
- **Alt-Speed Sync**: Fixed the JSON buffer size for parsing the `session-get` response (512 -> 4096 bytes). The alt-speed icon now correctly updates when the setting is changed from the Transmission Web UI.

## [1.8.1] - 2026-01-03
### Fixed
- **Status Bar**: Fixed alt-speed (turtle) icon not updating when the setting is changed externally (e.g., via Web UI). The display now correctly tracks and updates based on the server's state.

## [1.8.0] - 2026-01-03
### Added
- **Alt-Speed Toggle**: Press the speaker button (Select, GPIO 27) to toggle Transmission's alternative speed mode.
- **Turtle Icon**: A cyan turtle icon now appears on the status bar (left of download speed) when alt-speed mode is active.

## [1.7.10] - 2026-01-03
### Fixed
- **Status Bar**: Fixed a 1-pixel visual artifact from the upload icon remaining after disconnection.
- **OTA**: Restored the green progress bar on the status bar during firmware updates.

## [1.7.9] - 2026-01-03
### Changed
- **Branding**: Updated Project link and creator name to "Adamyno" on the About page.
- **Repository**: Pointed project link to the new repository: `https://github.com/Adamyno/odroid_transmission_monitor`.

## [1.7.8] - 2026-01-03
### Changed
- **Transmission Icon**: Redesigned to resemble an **Automatic Gearshift (T-shape)** as requested.
- **Input Responsiveness**: Improved button response time by removing global input polling delay.

## [1.7.1] - 2026-01-03
### Changed
- **Status Bar Layout**: Refined the layout to maximize space.
    - Right-aligned elements: Battery Icon, Battery %, Signal Icon.
    - Added **Transmission Icon** (Small) to the left of the Signal Icon.
    - Speed stats now appear to the left of the Transmission Icon.
    - Elements are dynamically positioned to fit seamlessly.

## [1.7.0] - 2026-01-03
### Added
- **Real-time Monitoring**:
    - **Dashboard**: Now displays a Transmission Logo that is colored when connected and grey when disconnected. Shows "Connected" or "Searching..." status text.
    - **Status Bar**: Displays real-time **Download** (Green Down Arrow) and **Upload** (Red Up Arrow) speeds when connected. Updates occur every ~2 seconds.
    - **Auto-Reconnect**: Device periodically checks for the Transmission server and automatically recovers connection.

## [1.6.2] - 2026-01-03
### Changed
- **UI Polish**:
    - **Brightness Control**: Replaced "< Adj >" text with arrows ("<" and ">") around the slider used when adjustments are active.
    - **Layout**: Moved "Edit details via Web UI" notice to the bottom of the Settings screen.

## [1.6.1] - 2026-01-03
### Changed
- **Settings Navigation**: Refined menu behavior. Settings page starts inactive to allow easy tab switching. Press DOWN to activate settings (Brightness, then Test). Press UP until inactive to exit settings mode and resume tab navigation.

## [1.6.0] - 2026-01-03
### Added
- **TFT Settings Page**: Implemented a functional Settings page on the device menu.
    - **Brightness Control**: Adjustable via Left/Right buttons.
    - **Transmission Info**: Displays Host and Port.
    - **Connection Test**: "Test Connection" button to verify Transmission connectivity directly from the device.
    - **UI Update**: Removed redundant "Settings" header for a cleaner look.

## [1.5.16] - 2026-01-03
### Fixed
- **Web Interface**: Restored "Transmission Config" and "Firmware Update" cards that were accidentally hidden in the previous update. "AP Mode Config" remains hidden in Station Mode.

## [1.5.15] - 2026-01-03
### Changed
- **Web Interface**: Hidden "AP Mode Config" in the Settings tab when in Station Mode, as these settings are irrelevant and potentially confusing when connected to a network.

## [1.5.14] - 2026-01-03
### Added
- **Battery Percentage**: Added battery percentage text next to the battery icon in the top status bar.
### Changed
- **Status Bar Layout**: Shifted the WiFi/AP Signal icon to the left to make room for the battery percentage text.

## [1.5.13] - 2026-01-03
### Fixed
- **Connection Fallback**: Implemented a 20-second timeout for WiFi connections. If the device cannot connect to the configured network (e.g., wrong password or router offline) within 20 seconds, it will automatically switch to AP Mode, ensuring the device remains accessible.

## [1.5.12] - 2026-01-03
### Fixed
- **Status Bar Icon**: Applied the dual-mode display fix to the top status bar as well. It now correctly displays the Station WiFi signal strength icon instead of the AP icon when the device is connected to a network, even if AP mode is active.

## [1.5.11] - 2026-01-03
### Fixed
- **Dual Mode Display**: Fixed an issue where the device would display "AP Mode" status on the screen and web interface when simply saving AP settings in Station mode. Now, if the device is connected to a WiFi network (Station), it will prioritize displaying Station information even if AP mode is active in the background.

## [1.5.10] - 2026-01-03
### Fixed
- **Missing Reset Button**: Restored the "Reset" button in the AP Mode interface, which was previously only visible in Station Mode.

## [1.5.9] - 2026-01-03
### Fixed
- **Web Status Update**: Fixed an issue where the AP Mode status (SSID/Password) was not updating on the web interface because the data fetch loop was disabled in AP mode.

## [1.5.8] - 2026-01-03
### Added
- **Web Status AP Info**: Now displaying Current SSID and Password on the Web Interface Status tab when in AP mode.

## [1.5.7] - 2026-01-03
### Fixed
- **AP Settings UX**: Fixed the "Error" message when saving AP settings by modifying the web server response order.
- **Real-time Password Update**: The AP password on the Status page now updates immediately when changed.
- **UI Tweaks**: Removed the "Open" text when no password is set for the AP; now displays an empty field.

## [1.5.6] - 2026-01-03
### Fixed
- **AP Settings Application**: Fixed an issue where changing the AP SSID or Password via the web interface would not apply the new settings immediately until a device reboot.

## [1.5.5] - 2026-01-02
### Fixed
- **AP Mode Display**: Fixed an issue where the Status Bar icon would incorrectly show WiFi signal strength in Menu mode even when acting as an Access Point.
- **Status Page for AP**: The Status tab now correctly displays the AP Name, Password, and IP Address when in AP mode, replacing the Signal Strength indicator.

## [1.5.4] - 2026-01-02
### Removed
- **Status Bar IP**: Removed the IP address display from the top status bar to reduce clutter and focus on the detailed Status tab.

## [1.5.3] - 2026-01-02
### Fixed
- **Status Page Layout**: Aligned the MAC address value horizontally with the other status metrics.

## [1.5.2] - 2026-01-02
### Changed
- **Status Page Optimization**: Implemented flicker-free updating for the Status page. Only changed values (SSID, IP, RSSI, Battery) are redrawn instead of the entire screen.

## [1.5.1] - 2026-01-02
### Added
- **Real-Time Status Updates**: The physical display's Status tab now refreshes every 5 seconds to show live SSID, RSSI, and Battery metrics.

## [1.5.0] - 2026-01-02
### Added
- **Detailed Status Tab**: Physical display now shows SSID, IP Address, live RSSI (with icon), MAC address, and precise battery voltage.
- **Dashboard (Main Screen)**: Added a "Home" screen for the physical display showing connection status and IP address.
- **Build Date on Display**: Added the build date to the physical "About" page, synchronized with the web interface using compiler macros.
- **Improved WiFi Reconnection**: Explicit monitoring logic to handle connection drops more robustly.

### Changed
- **Menu Exit Logic**: Pressing the MENU or B button now correctly exits the tabbed interface and returns to the Dashboard.
- **Version bump**: Jumped to 1.5.0 to reflect major feature completion for the physical UI.

## [1.4.2] - 2026-01-02
### Added
- Backlight Control: Screen brightness can now be adjusted from the web interface.
- Persistence: Brightness settings are saved and restored on boot.
### Changed
- UI Polish: Removed "footer" instructions from physical display for a cleaner look.
- Version bump to 1.4.2.

## [1.4.1] - 2026-01-02
### Added
- Physical Display: Web-like tabbed interface with Status, Settings, and About pages.
- Physical Display: LEFT/RIGHT joystick navigation between tabs.
- Physical Display: Card-style layout with dark theme and cyan accents.
### Changed
- Physical Display: Removed MAIN MENU in favor of direct tab navigation.

## [1.3.11] - 2026-01-02
### Changed
- Added edge detection (one-shot logic) to buttons and joystick to prevent accidental multiple triggers (menu skipping).
- Fixed WiFi icon behavior in menu states.
- Enabled Web Dashboard accessibility while menu is open.
### Added
- Added new "About" page to physical display.
- Implemented Tab-based navigation on physical display.
- Synchronized physical UI colors with web interface.
- Fixed WiFi icon RSSI update logic.
- Optimized battery status bar updates.
- Refactored physical display state management into `gui_handler`.
- Added developer and version info to "About" card.
- Version bump to 1.3.10.

## [1.3.10] - 2026-01-02
### Fixed
- Status Bar: Implemented periodic refresh (every 2 seconds) to ensure Battery and WiFi signal icons update dynamically on the physical display.

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
