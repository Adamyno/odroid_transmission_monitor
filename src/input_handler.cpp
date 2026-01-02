#include "input_handler.h"

bool btnMenuPressed = false;
bool btnSelectPressed = false;
bool btnUpPressed = false;
bool btnDownPressed = false;
bool btnAPressed = false;
bool btnBPressed = false;

// Debounce handling
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

void setupInputs() {
  pinMode(BTN_MENU, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  // Joypad usually requires analog read or specific pins, assuming digital for
  // now if they are directional switches Actually ODROID-GO Joypad is analog on
  // 34/35. For now effectively ignoring joypad or using just buttons if needed.
  // Let's implement Menu (13) acting as a toggle.
}

void readInputs() {
  // Reset one-shot flags
  btnMenuPressed = false;
  btnSelectPressed = false;
  btnUpPressed = false;
  btnDownPressed = false;
  btnAPressed = false;
  btnBPressed = false;

  // Global debounce/rate limiting
  if (millis() - lastDebounceTime < 50) { // Check inputs at 20Hz
    return;
  }

  if (digitalRead(BTN_MENU) == LOW) {
    btnMenuPressed = true;
    lastDebounceTime = millis();
  }

  // Joystick handling (Analog)
  int joyY = analogRead(BTN_JOY_Y); // 0-4095
  if (joyY < 1000) { // UP? or Down? verification needed. Usually 0 is one edge.
    // Assuming 0 is Up
    btnUpPressed = true;
    lastDebounceTime = millis();
  } else if (joyY > 3000) { // Down
    btnDownPressed = true;
    lastDebounceTime = millis();
  }

  // Select Button for "Enter"
  if (digitalRead(BTN_SELECT) == LOW) {
    btnSelectPressed = true;
    lastDebounceTime = millis();
  }

  // A Button also for "Enter"
  if (digitalRead(BTN_A) == LOW) {
    btnAPressed = true;
    lastDebounceTime = millis();
  }
}
