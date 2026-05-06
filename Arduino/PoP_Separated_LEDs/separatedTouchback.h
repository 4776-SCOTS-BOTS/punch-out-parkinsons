/*
 * separatedTouchback.h — Hardware definitions and game logic for PoP_Separated_LEDs
 *
 * TWO-STRIP ARCHITECTURE
 * ----------------------
 * Unlike earlier PoP sketches that chain all 660 LEDs on a single data pin,
 * this version splits the bag into two independent NeoPixel strips:
 *   - leftStrip  (pin 10, 330 LEDs) — covers pads 1 and 4
 *   - rightStrip (pin 11, 330 LEDs) — covers pads 2 and 8
 *
 * Each strip has 165 LEDs per pad segment. Pad layout as of 3/19/25:
 *   Left strip:  startLED 166 = pad 1  |  startLED 0 = pad 4
 *   Right strip: startLED   0 = pad 2  |  startLED 166 = pad 8
 *
 * WHY COLORS ARE DUPLICATED PER STRIP
 * ------------------------------------
 * NeoPixel color values (uint32_t) are produced by calling .Color() on a
 * specific strip instance and are bound to that instance's internal format.
 * Passing a color created from leftStrip to rightStrip.fill() can produce
 * wrong colors. So each logical color exists twice: a *L variant for the
 * left strip and a *R variant for the right strip.
 *
 * TOUCH CONTROLLER
 * ----------------
 * MPR121 capacitive sensor on I2C at 0x5A. cap.touched() returns a bitmask:
 *   pad 1 = 0x01, pad 2 = 0x02, pad 4 = 0x04, pad 8 = 0x08
 * Sensitivity is set to (6, 2) — lower thresholds mean higher sensitivity
 * compared to PoP_Full (10, 5).
 *
 * HIGH SCORES
 * -----------
 * highSeqScore and highRandScore are in-memory only. They reset on power cycle.
 */

#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// ── Pin and strip configuration ───────────────────────────────────────────────
#define leftStripPin 10
#define rightStripPin 11

#define LED_COUNT 330  // LEDs per strip; 165 per pad segment

Adafruit_NeoPixel leftStrip(LED_COUNT, leftStripPin, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel rightStrip(LED_COUNT, rightStripPin, NEO_GRB + NEO_KHZ800);

// ── Touch sensor ──────────────────────────────────────────────────────────────
Adafruit_MPR121 cap = Adafruit_MPR121();

uint16_t lasttouched = 0;
uint16_t currtouched = 0;

// ── Color constants (duplicated per strip — see file header) ──────────────────
uint32_t greenL = leftStrip.Color(0, 255, 0);
uint32_t redL   = leftStrip.Color(255, 0, 0);
uint32_t blueL  = leftStrip.Color(0, 0, 255);
uint32_t whiteL = leftStrip.Color(255, 255, 255);
uint32_t blackL = leftStrip.Color(0, 0, 0);

uint32_t greenR = rightStrip.Color(0, 255, 0);
uint32_t redR   = rightStrip.Color(255, 0, 0);
uint32_t blueR  = rightStrip.Color(0, 0, 255);
uint32_t whiteR = rightStrip.Color(255, 255, 255);
uint32_t blackR = rightStrip.Color(0, 0, 0);

// ── Scoring globals ───────────────────────────────────────────────────────────
unsigned long scoreTot = 0;       // Accumulates score for the current game run
unsigned long highSeqScore = 0;   // All-time best for Sequential mode (in-memory only)
unsigned long highRandScore = 0;  // All-time best for Random mode (in-memory only)

// ── Timing constants ──────────────────────────────────────────────────────────
int timeout = 5000;               // ms the player has to hit the lit pad

// Scoring formula: score = (timeout - timeToHit) / divisor
// divisor = timeout / 100 → max possible score per pad is 100 pts
int divisor = timeout / 100;

// ── Feedback animations ───────────────────────────────────────────────────────

// Correct hit: double green flash on both strips
void greenLEDS() {
  leftStrip.fill(greenL, 0, LED_COUNT);
  rightStrip.fill(greenR, 0, LED_COUNT);
  leftStrip.show();
  rightStrip.show();
  delay(250);
  leftStrip.clear();
  rightStrip.clear();
  leftStrip.show();
  rightStrip.show();
  delay(250);
  leftStrip.fill(greenL, 0, LED_COUNT);
  rightStrip.fill(greenR, 0, LED_COUNT);
  leftStrip.show();
  rightStrip.show();
  delay(250);
  leftStrip.clear();
  rightStrip.clear();
  leftStrip.show();
  rightStrip.show();
}

// Miss or wrong pad: red flash on both strips
void redLEDS() {
  leftStrip.fill(redL, 0, LED_COUNT);
  rightStrip.fill(redR, 0, LED_COUNT);
  leftStrip.show();
  rightStrip.show();
  delay(750);
  leftStrip.clear();
  rightStrip.clear();
  leftStrip.show();
  rightStrip.show();
  delay(500);
}

// New high score: white blink on both strips
void highScoreBlink() {
  leftStrip.fill(whiteL, 0, LED_COUNT);
  rightStrip.fill(whiteR, 0, LED_COUNT);
  leftStrip.show();
  rightStrip.show();
  delay(100);
  leftStrip.clear();
  rightStrip.clear();
  leftStrip.show();
  rightStrip.show();
  delay(100);
}

// ── sequentialFunction ────────────────────────────────────────────────────────
// Lights one pad segment, waits for a touch, and scores the result.
//
// Parameters:
//   side     — "left" or "right", selects which physical strip to use
//   startLED — first LED index within that strip for this pad's 165-LED segment
//   padNum   — MPR121 bitmask value for this pad (1, 2, 4, or 8)
//
// The player has `timeout` ms to hit the correct pad.
// Score = (timeout - timeToHit) / divisor; added to global scoreTot.
// Wrong pad or timeout scores 0.
unsigned long sequentialFunction(String side, uint16_t startLED, int padNum) {
  //padNum is the pin number on Adafruit board, goes 1, 2, 4, 8, etc.
  //startLED is the diode that you start on. There are 165 diodes per LED strip
  int currtouched = cap.touched();
  int lasttouched;
  unsigned long delayTime = timeout;
  unsigned long startTime;
  unsigned long timeToTouch;
  unsigned long score = 0;

  // Light the correct 165-LED segment on the appropriate strip
  if (side.equals("left")) {
    leftStrip.fill(blueL, startLED, 165);
    leftStrip.show();
  } else if (side.equals("right")) {
    rightStrip.fill(blueR, startLED, 165);
    rightStrip.show();
  }
  // Serial.println("Blue light");
  // Serial.println(cap.touched());

  // Wait until the player touches any pad or the timeout expires
  startTime = millis();
  while ((millis() - startTime) < delayTime && cap.touched() == 0 ) {
  }
  timeToTouch = (millis() - startTime);
  currtouched = cap.touched();
  if (currtouched == padNum) {
    Serial.println(currtouched);
    score = (timeout - timeToTouch) / divisor;
    greenLEDS();
    scoreTot = scoreTot + score;
  } else {
    score = 0;
    redLEDS();
    scoreTot = scoreTot + score;
  }
  lasttouched == currtouched;
}

// ── ledSEQUENTIAL ─────────────────────────────────────────────────────────────
// Runs one full Sequential game: 8 rounds, hitting all 4 pads twice each
// in a fixed order (1, 2, 4, 8, 8, 4, 2, 1). Max score = 8 × 100 = 800 pts.
//
// The loop variable `base` maps to pad cases so pads are visited symmetrically:
//   cases 1&8 → pad 1 (left, startLED 166)
//   cases 2&7 → pad 2 (right, startLED 0)
//   cases 3&6 → pad 4 (left, startLED 0)
//   cases 4&5 → pad 8 (right, startLED 166)
unsigned long ledSEQUENTIAL() {
  int currtouched = cap.touched();
  int lasttouched;
  unsigned long delayTime = timeout;
  unsigned long startTime;
  unsigned long timeToTouch;
  unsigned long score = 0;

  for (int base = 1; base < 9; base++) {

    switch (base) {
      case 1:
      case 8:
          //As of 3/19/25, last in line for left side
          sequentialFunction("left", 166, 1); // Pin 1 (startLED 0 before 3/16/25)
        break;
      case 2:
      case 7:
        //As of 3/19/25, first in line for right side
        sequentialFunction("right", 0, 2); // Pin 2 (startLED 166 before 3/16/25)
        break;
      case 3:
      case 6:
        //As of 3/19/25, first in line for left side
        sequentialFunction("left", 0, 4); // Pin 3 (startLED 331 before 3/16/25)
        break;
      case 4:
      case 5:
        //As of 3/19/25, last in line for right side
        sequentialFunction("right", 166, 8); // Pin 4 (startLED 496 before 3/16/25)
        break;
    }
  }
}

// ── randomFunction ────────────────────────────────────────────────────────────
// Same single-pad logic as sequentialFunction, used by ledRANDOM.
// Uses a shorter 2500 ms timeout so Random mode feels faster-paced.
// Also checks for a new high Random score after each pad.
unsigned long randomFunction(String side, int startLED, int padNum) {
  int currtouched = cap.touched();
  int lasttouched;
  int lastRand;
  int score = 0;
  unsigned long delayTime = 2500;  // Shorter window than Sequential (5000 ms)
  unsigned long startTime;
  unsigned long timeToTouch;

  // Light the correct 165-LED segment on the appropriate strip
  if (side.equals("left")) {
    leftStrip.fill(blueL, startLED, 165);
    leftStrip.show();
  } else if (side.equals("right")) {
    rightStrip.fill(blueR, startLED, 165);
    rightStrip.show();
  }

  // Wait until the player touches any pad or the timeout expires
  startTime = millis();
  while ((millis() - startTime) < delayTime && cap.touched() == 0) {}
  timeToTouch = (millis() - startTime);
  currtouched = cap.touched();
  if (currtouched == padNum) {
    score = (timeout - timeToTouch) / divisor;
    greenLEDS();
    scoreTot = scoreTot + score;
    //Serial.println(scoreTot);
  } else {
    score = 0;
    redLEDS();
    scoreTot = scoreTot + score;
  }

  if (scoreTot > highRandScore) {
    highRandScore = scoreTot;
  }
}

// ── ledRANDOM ─────────────────────────────────────────────────────────────────
// Runs one full Random game: 10 rounds, each with a randomly chosen pad.
// Never lights the same pad twice in a row. Max score = 10 × 100 = 1000 pts.
//
// rand is a value 1–4 that maps to the four pads:
//   1 → pad 1 (left,  startLED 166)
//   2 → pad 2 (right, startLED 0)
//   3 → pad 4 (left,  startLED 0)
//   4 → pad 8 (right, startLED 166)
unsigned long ledRANDOM() {
  unsigned long score = 0;

  for (int i = 0; i < 10; i++) {
    int currtouched = cap.touched();
    int lasttouched;
    int lastRand;
    unsigned long delayTime = 2500;
    unsigned long startTime;
    unsigned long timeToTouch;

    // Pick a random pad; re-roll if it's the same as the previous round
    int rand = random(1, 5);
    while (rand == lastRand) {
      rand = random(1, 5);
    }

    switch (rand) {
      case 1: {
        //As of 3/19/25, last in line for left side
        randomFunction("left", 166, 1);
        break;
      }
      //As of 3/19/25, first in line for right side
      case 2: {
        randomFunction("right", 0, 2);
        break;
      }
      //As of 3/19/25, first in line for left side
      case 3: {
        randomFunction("left", 0, 4);
        break;
      }
      //As of 3/19/25, last in line for right side
      case 4: {
        randomFunction("right", 166, 8);
        break;
      }
    }
    lasttouched = currtouched;
    lastRand = rand;
  }
  //Serial.print("Final Score = ");
  //Serial.print(scoreTot);
  return scoreTot;
}
