/*
  Binary Trainer - Arduino Nano + 0.96" I2C OLED
  Learn mode : shows numbers 0-15 with their binary pattern
  Reasoning  : convert a number to binary within a time limit, with score,
               reaction time and a saved high score

  Libraries: U8g2 (by olikraus). EEPROM and Wire come with the Arduino IDE.
*/

#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include <U8g2lib.h>

// 0.96" SSD1306 OLED. If your screen is 1.3" / looks shifted, use the SH1106 line instead:
// U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ===== YOUR SETTINGS =====
const int SW_BIT[4] = {2, 3, 4, 5};        // bit inputs: 8, 4, 2, 1
const int LED_P[4]  = {9, 10, 11, 12};     // LEDs: 8, 4, 2, 1
// const int LED_P[4] = {9, 10, 11, 8};    // use this line if the bit-1 LED is on D8
const int BTN_LEARN  = 7;                  // Learn button
const int BTN_REASON = 6;                  // Reason / Check button
const bool MOMENTARY_BITS = true;          // true = each press flips the bit and it stays
                                           // false = real latching toggle switches
const unsigned long TIME_LIMIT_MS = 5000;  // time allowed per question
// =========================

const int WEIGHT[4] = {8, 4, 2, 1};

enum Mode { MENU, LEARN, INTRO, RESET_SW, ASK, RESULT_OK, RESULT_WRONG };
Mode mode = MENU;

bool bits[4] = {false, false, false, false};
int learnNum = 0, target = 0, score = 0, highScore = 0;
int shownValue = 0;
bool newHigh = false, wrongPage2 = false, timeUp = false;
unsigned long modeStart = 0, lastAdvance = 0;
unsigned long questionStart = 0, lastTimerDraw = 0, reactionMs = 0;
bool needRedraw = true;

bool prevState[14];
unsigned long lastChange[14];

// returns true once per press (with debounce)
bool pressedEdge(int pin) {
  bool now = (digitalRead(pin) == LOW);
  bool edge = now && !prevState[pin] && (millis() - lastChange[pin] > 30);
  if (now != prevState[pin]) { prevState[pin] = now; lastChange[pin] = millis(); }
  return edge;
}

void clearBits() {
  for (int i = 0; i < 4; i++) bits[i] = false;
}

// the value currently set on the 4 bit inputs
int switchValue() {
  int v = 0;
  for (int i = 0; i < 4; i++) {
    bool on = MOMENTARY_BITS ? bits[i] : (digitalRead(SW_BIT[i]) == LOW);
    if (on) v += WEIGHT[i];
  }
  return v;
}

void setLeds(int v) {
  for (int i = 0; i < 4; i++) digitalWrite(LED_P[i], (v & WEIGHT[i]) ? HIGH : LOW);
}

// ---------- drawing helpers ----------
void sparkle(int x, int y) {
  u8g2.drawLine(x - 3, y, x + 3, y);
  u8g2.drawLine(x, y - 3, x, y + 3);
}

void drawSadFace(int cx, int cy) {
  u8g2.drawCircle(cx, cy, 20);
  u8g2.drawDisc(cx - 7, cy - 5, 2);
  u8g2.drawDisc(cx + 7, cy - 5, 2);
  u8g2.drawLine(cx - 11, cy - 9, cx - 4, cy - 12);
  u8g2.drawLine(cx + 4, cy - 12, cx + 11, cy - 9);
  for (int dx = -8; dx <= 8; dx++) {
    int y = cy + 10 - (64 - dx * dx) / 16;
    u8g2.drawPixel(cx + dx, y);
    u8g2.drawPixel(cx + dx, y + 1);
  }
  u8g2.drawDisc(cx + 8, cy + 1, 1);
  u8g2.drawPixel(cx + 8, cy + 4);
}

void drawWowFace(int cx, int cy) {
  u8g2.drawCircle(cx, cy, 20);
  u8g2.drawCircle(cx - 7, cy - 5, 4);
  u8g2.drawDisc(cx - 7, cy - 5, 1);
  u8g2.drawCircle(cx + 7, cy - 5, 4);
  u8g2.drawDisc(cx + 7, cy - 5, 1);
  u8g2.drawLine(cx - 11, cy - 12, cx - 3, cy - 13);
  u8g2.drawLine(cx + 3, cy - 13, cx + 11, cy - 12);
  u8g2.drawDisc(cx, cy + 9, 5);
  sparkle(cx + 25, cy - 16);
  sparkle(cx + 22, cy + 14);
}

void drawBoxes(int v) {
  for (int i = 0; i < 4; i++) {
    int x = 3 + i * 32;
    bool on = (v & WEIGHT[i]);

    char w[2] = { (char)('0' + WEIGHT[i]), 0 };
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(x + 10, 26, w);

    u8g2.drawFrame(x, 30, 26, 22);
    if (on) { u8g2.drawBox(x, 30, 26, 22); u8g2.setDrawColor(0); }
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(x + 8, 48, on ? "1" : "0");
    u8g2.setDrawColor(1);
  }
}

void sumText(int v, char* out, int n) {
  int k = snprintf(out, n, "%d = ", v);
  if (v == 0) { snprintf(out + k, n - k, "0"); return; }
  bool first = true;
  for (int i = 0; i < 4; i++) {
    if (v & WEIGHT[i]) {
      k += snprintf(out + k, n - k, first ? "%d" : "+%d", WEIGHT[i]);
      first = false;
    }
  }
}

void drawScreen() {
  char b[24];
  switch (mode) {
    case MENU:
      u8g2.setFont(u8g2_font_ncenB10_tr);
      u8g2.drawStr(14, 24, "Binary Trainer");
      u8g2.setFont(u8g2_font_6x10_tf);
      snprintf(b, sizeof(b), "Best: %d", highScore);
      u8g2.drawStr(0, 9, b);
      u8g2.drawStr(10, 44, "Button 1 : Learn");
      u8g2.drawStr(10, 58, "Button 2 : Reasoning");
      break;

    case LEARN:
      snprintf(b, sizeof(b), "Number: %d", learnNum);
      u8g2.setFont(u8g2_font_ncenB10_tr);
      u8g2.drawStr(0, 12, b);
      drawBoxes(learnNum);
      sumText(learnNum, b, sizeof(b));
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(0, 63, b);
      break;

    case INTRO:
      u8g2.setFont(u8g2_font_ncenB10_tr);
      u8g2.drawStr(4, 26, "Reasoning Level");
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(34, 44, "Get ready...");
      snprintf(b, sizeof(b), "%d sec per number", (int)(TIME_LIMIT_MS / 1000));
      u8g2.drawStr(22, 58, b);
      break;

    case RESET_SW:
      u8g2.setFont(u8g2_font_ncenB10_tr);
      u8g2.drawStr(10, 14, "Reset switches");
      drawBoxes(shownValue);
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(0, 63, "Flip all switches down");
      break;

    case ASK: {
      snprintf(b, sizeof(b), "Make: %d", target);
      u8g2.setFont(u8g2_font_ncenB10_tr);
      u8g2.drawStr(0, 12, b);
      snprintf(b, sizeof(b), "Pts:%d", score);
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(92, 10, b);
      drawBoxes(shownValue);

      unsigned long el = millis() - questionStart;       // countdown bar
      if (el > TIME_LIMIT_MS) el = TIME_LIMIT_MS;
      unsigned long left = TIME_LIMIT_MS - el;
      int w = (int)(98UL * left / TIME_LIMIT_MS);
      u8g2.drawFrame(0, 56, 100, 8);
      if (w > 0) u8g2.drawBox(1, 57, w, 6);
      snprintf(b, sizeof(b), "%ds", (int)((left + 999) / 1000));
      u8g2.drawStr(104, 63, b);
      break;
    }

    case RESULT_OK:
      drawWowFace(24, 32);
      u8g2.setFont(u8g2_font_ncenB14_tr);
      u8g2.drawStr(58, 26, "WOW!");
      u8g2.setFont(u8g2_font_6x10_tf);
      snprintf(b, sizeof(b), "Right! %d.%ds", (int)(reactionMs / 1000), (int)((reactionMs % 1000) / 100));
      u8g2.drawStr(58, 40, b);
      u8g2.drawStr(58, 52, "+1 point");
      snprintf(b, sizeof(b), "Score: %d", score);
      u8g2.drawStr(0, 63, b);
      break;

    case RESULT_WRONG:
      if (!wrongPage2) {
        drawSadFace(24, 32);
        u8g2.setFont(u8g2_font_ncenB10_tr);
        u8g2.drawStr(56, 24, timeUp ? "TIME UP!" : "WRONG!");
        u8g2.setFont(u8g2_font_6x10_tf);
        snprintf(b, sizeof(b), "Score: %d", score);
        u8g2.drawStr(56, 40, b);
        snprintf(b, sizeof(b), "Answer: %d", target);
        u8g2.drawStr(56, 52, b);
      } else if (newHigh) {
        sparkle(8, 10); sparkle(120, 12); sparkle(118, 44);
        u8g2.setFont(u8g2_font_ncenB14_tr);
        u8g2.drawStr(10, 24, "NEW HIGH");
        u8g2.drawStr(18, 42, "SCORE!");
        u8g2.setFont(u8g2_font_6x10_tf);
        snprintf(b, sizeof(b), "You scored %d", score);
        u8g2.drawStr(0, 54, b);
        u8g2.drawStr(0, 63, "1:Learn   2:Reason");
      } else {
        u8g2.setFont(u8g2_font_ncenB10_tr);
        u8g2.drawStr(26, 16, "Game Over");
        u8g2.setFont(u8g2_font_6x10_tf);
        snprintf(b, sizeof(b), "Your score: %d", score);
        u8g2.drawStr(0, 34, b);
        snprintf(b, sizeof(b), "High score: %d", highScore);
        u8g2.drawStr(0, 47, b);
        u8g2.drawStr(0, 63, "1:Learn   2:Reason");
      }
      break;
  }
}

void render() {
  u8g2.firstPage();
  do { drawScreen(); } while (u8g2.nextPage());
}

// ---------- mode changes ----------
void goLearn() {
  mode = LEARN;
  learnNum = 0;
  lastAdvance = millis();
  setLeds(learnNum);
  needRedraw = true;
}

void goIntro() {                       // starts a fresh run
  mode = INTRO;
  modeStart = millis();
  score = 0;
  clearBits();
  setLeds(0);
  needRedraw = true;
}

void goResetSw() {                     // latching switches only: wait until all are down
  mode = RESET_SW;
  shownValue = switchValue();
  setLeds(shownValue);
  needRedraw = true;
}

void newQuestion() {
  int old = target;
  do { target = random(1, 16); } while (target == old);
  mode = ASK;
  shownValue = switchValue();
  setLeds(shownValue);
  questionStart = millis();            // the timer starts here
  lastTimerDraw = millis();
  needRedraw = true;
}

void nextQuestion() {
  if (MOMENTARY_BITS) {
    clearBits();
    newQuestion();
  } else if (switchValue() == 0) {
    newQuestion();
  } else {
    goResetSw();
  }
}

void endRun(bool byTimeout) {          // wrong answer or time up
  mode = RESULT_WRONG;
  modeStart = millis();
  wrongPage2 = false;
  timeUp = byTimeout;
  newHigh = (score > highScore);
  if (newHigh) {
    highScore = score;
    EEPROM.update(0, highScore > 254 ? 254 : highScore);
  }
  setLeds(target);                     // show the right answer on the LEDs
  needRedraw = true;
}

void checkAnswer() {
  if (switchValue() == target) {
    reactionMs = millis() - questionStart;
    score++;
    mode = RESULT_OK;
    modeStart = millis();
    needRedraw = true;
  } else {
    endRun(false);
  }
}

void setup() {
  for (int i = 0; i < 4; i++) {
    pinMode(SW_BIT[i], INPUT_PULLUP);
    pinMode(LED_P[i], OUTPUT);
  }
  pinMode(BTN_LEARN, INPUT_PULLUP);
  pinMode(BTN_REASON, INPUT_PULLUP);

  // hold both mode buttons while powering on to reset the high score
  if (digitalRead(BTN_LEARN) == LOW && digitalRead(BTN_REASON) == LOW) {
    EEPROM.update(0, 0);
  }
  highScore = EEPROM.read(0);
  if (highScore == 255) highScore = 0;

  randomSeed(analogRead(A0));
  u8g2.begin();
  u8g2.setBusClock(400000);
  needRedraw = true;
}

void loop() {
  unsigned long now = millis();

  bool bitPressed[4];
  for (int i = 0; i < 4; i++) bitPressed[i] = pressedEdge(SW_BIT[i]);
  bool eLearn  = pressedEdge(BTN_LEARN);
  bool eReason = pressedEdge(BTN_REASON);

  switch (mode) {
    case MENU:
      if (eLearn) goLearn();
      else if (eReason) goIntro();
      break;

    case LEARN:
      if (eReason) {
        goIntro();
      } else if (eLearn || now - lastAdvance >= 2000) {
        learnNum = (learnNum + 1) % 16;
        lastAdvance = now;
        setLeds(learnNum);
        needRedraw = true;
      }
      break;

    case INTRO:
      if (now - modeStart >= 1500) nextQuestion();
      break;

    case RESET_SW: {                   // only used with latching switches
      int v = switchValue();
      if (v != shownValue) {
        shownValue = v;
        setLeds(v);
        needRedraw = true;
      }
      if (eLearn) goLearn();
      else if (v == 0) newQuestion();
      break;
    }

    case ASK: {
      if (eLearn) {
        goLearn();
      } else if (now - questionStart >= TIME_LIMIT_MS) {
        endRun(true);                  // time is up, the run ends
      } else if (eReason) {
        checkAnswer();
      } else {
        if (MOMENTARY_BITS) {
          for (int i = 0; i < 4; i++) {
            if (bitPressed[i]) bits[i] = !bits[i];
          }
        }
        int v = switchValue();
        if (v != shownValue) {
          shownValue = v;
          setLeds(v);
          needRedraw = true;
        }
        if (now - lastTimerDraw >= 200) {   // keep the countdown bar moving
          lastTimerDraw = now;
          needRedraw = true;
        }
      }
      break;
    }

    case RESULT_OK:
      if (eLearn) goLearn();
      else if (now - modeStart >= 2000) nextQuestion();
      break;

    case RESULT_WRONG:
      if (!wrongPage2 && now - modeStart >= 2500) {
        wrongPage2 = true;
        needRedraw = true;
      }
      if (eLearn) goLearn();
      else if (eReason) goIntro();
      break;
  }

  if (needRedraw) {
    needRedraw = false;
    render();
  }
}
