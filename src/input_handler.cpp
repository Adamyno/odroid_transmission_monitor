#include "input_handler.h"

bool btnMenuPressed = false;
bool btnSelectPressed = false;
bool btnStartPressed = false;
bool btnUpPressed = false;
bool btnDownPressed = false;
bool btnLeftPressed = false;
bool btnRightPressed = false;
bool btnAPressed = false;
bool btnBPressed = false;

// State tracking for edge detection
static bool prevBtnMenu = true;
static bool prevBtnSelect = true;
static bool prevBtnStart = true;
static bool prevBtnA = true;
static bool prevBtnB = true;
static bool prevUp = false;
static bool prevDown = false;
static bool prevLeft = false;
static bool prevRight = false;

// Debounce handling
unsigned long lastDebounceTime = 0;

void setupInputs() {
  pinMode(BTN_MENU, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
}

void readInputs() {
  // Reset one-shot flags
  btnMenuPressed = false;
  btnSelectPressed = false;
  btnStartPressed = false;
  btnUpPressed = false;
  btnDownPressed = false;
  btnLeftPressed = false;
  btnRightPressed = false;
  btnAPressed = false;
  btnBPressed = false;

  // No global rate limiting - poll every loop
  // Debouncing is handled by state logic or external timing if needed
  // lastDebounceTime = millis();

  // --- Digital Buttons (Active LOW) ---
  bool curMenu = digitalRead(BTN_MENU);
  if (curMenu == LOW && prevBtnMenu == HIGH)
    btnMenuPressed = true;
  prevBtnMenu = curMenu;

  bool curSelect = digitalRead(BTN_SELECT);
  if (curSelect == LOW && prevBtnSelect == HIGH)
    btnSelectPressed = true;
  prevBtnSelect = curSelect;

  bool curStart = digitalRead(BTN_START);
  if (curStart == LOW && prevBtnStart == HIGH)
    btnStartPressed = true;
  prevBtnStart = curStart;

  bool curA = digitalRead(BTN_A);
  if (curA == LOW && prevBtnA == HIGH)
    btnAPressed = true;
  prevBtnA = curA;

  bool curB = digitalRead(BTN_B);
  if (curB == LOW && prevBtnB == HIGH)
    btnBPressed = true;
  prevBtnB = curB;

  // --- Joystick handling (Analog) ---
  int joyY = analogRead(BTN_JOY_Y);
  bool curUp = (joyY > 3000);
  bool curDown = (joyY > 1000 && joyY <= 3000);

  if (curUp && !prevUp)
    btnUpPressed = true;
  if (curDown && !prevDown)
    btnDownPressed = true;

  prevUp = curUp;
  prevDown = curDown;

  int joyX = analogRead(BTN_JOY_X);
  bool curLeft = (joyX > 3000);
  bool curRight = (joyX > 1000 && joyX <= 3000);

  if (curLeft && !prevLeft)
    btnLeftPressed = true;
  if (curRight && !prevRight)
    btnRightPressed = true;

  prevLeft = curLeft;
  prevRight = curRight;
}
