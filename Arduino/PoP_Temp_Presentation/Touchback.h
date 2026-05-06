/*
 * Touchback.h — Hardware & Game-Logic Header (Presentation Version)
 *
 * This header is included by PoP_Temp_Presentation.ino. It owns everything
 * that touches hardware (NeoPixel strip, MPR121 capacitive sensor) and
 * implements the two game modes: ledSEQUENTIAL() and ledRANDOM().
 *
 * Hardware layout:
 *   NeoPixel strip — 660 LEDs on pin 12, divided into 4 segments of 165 LEDs:
 *     Pad 1: LEDs   0–164  (startLED =   0)
 *     Pad 2: LEDs 166–330  (startLED = 166)
 *     Pad 3: LEDs 331–495  (startLED = 331)
 *     Pad 4: LEDs 496–660  (startLED = 496)
 *
 *   MPR121 — capacitive touch controller on I2C at 0x5A.
 *   cap.touched() returns a bitmask: pad1=1, pad2=2, pad3=4, pad4=8.
 *
 * Scoring formula (per pad hit):
 *   score = (timeGiven - timeToHit) / divisor
 *   divisor = timeGiven / 100   →  max 100 pts per hit
 *   Sequential max = 800 pts (8 rounds), Random max = 1000 pts (10 rounds).
 *
 * NOTE — known inconsistency: case 1/case 8 of ledSEQUENTIAL() divides by
 * the hardcoded literal 50 instead of the `divisor` variable. All other cases
 * use `divisor` correctly. With timeGiven=5000, divisor=50, so the results are
 * identical right now — but if timeGiven is ever changed, case 1/8 will score
 * differently from every other case. This is an early-dev leftover; fix by
 * replacing the literal 50 with `divisor` in that case block.
 */

#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

/* --------------------------------------------------------------------------
 * Compatibility shim
 * _BV(bit) shifts a 1 into position `bit`. Defined here in case the AVR
 * headers haven't already provided it (they usually do on Uno/Mega).
 * -------------------------------------------------------------------------- */
#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

/* --------------------------------------------------------------------------
 * NeoPixel strip configuration
 * -------------------------------------------------------------------------- */
#define LED_PIN   12   // Data pin for the NeoPixel strip
#define LED_COUNT 660  // Total LEDs across all 4 pad segments

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

/* --------------------------------------------------------------------------
 * MPR121 capacitive touch sensor
 * Default I2C address 0x5A (ADDR pin floating or tied to GND).
 * -------------------------------------------------------------------------- */
Adafruit_MPR121 cap = Adafruit_MPR121();

/* --------------------------------------------------------------------------
 * Touch-state tracking
 * lasttouched/currtouched hold the previous and current cap.touched() bitmasks.
 * They are re-declared as locals inside each game function; these globals are
 * kept for potential future use.
 * -------------------------------------------------------------------------- */
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

/* --------------------------------------------------------------------------
 * Pre-computed color values for the NeoPixel strip
 * -------------------------------------------------------------------------- */
uint32_t green = strip.Color(0, 255, 0);
uint32_t red   = strip.Color(255, 0, 0);
uint32_t blue  = strip.Color(0, 0, 255);

/* --------------------------------------------------------------------------
 * Scoring state
 * scoreTot accumulates across all rounds of one game session.
 * timeGiven is fixed at 5000 ms — the window a player has to hit each pad.
 * divisor normalises the score so a perfect hit is worth 100 pts.
 * -------------------------------------------------------------------------- */
unsigned long scoreTot = 0;
int timeGiven = 5000;
int divisor   = timeGiven / 100;  // = 50; score = (timeGiven - timeToHit) / divisor → max 100

/* --------------------------------------------------------------------------
 * greenLEDS() — hit-confirmation feedback
 * Flashes the entire strip green twice to signal a successful pad hit.
 * -------------------------------------------------------------------------- */
void greenLEDS() {
  strip.fill(green, 0, 660);
  strip.show();
  delay(250);
  strip.clear();
  strip.show();
  delay(250);
  strip.fill(green, 0, 660);
  strip.show();
  delay(250);
  strip.clear();
  strip.show();
}

/* --------------------------------------------------------------------------
 * redLEDS() — miss feedback
 * Flashes the entire strip red once to signal a missed pad.
 * -------------------------------------------------------------------------- */
void redLEDS() {
  strip.fill(red, 0, 660);
  strip.show();
  delay(750);
  strip.clear();
  strip.show();
  delay(500);
}

/* --------------------------------------------------------------------------
 * ledSEQUENTIAL() — Sequential game mode
 *
 * Runs 8 rounds (base 1–8) in a fixed order using a symmetric pattern:
 *   rounds 1&8 → pad 1, rounds 2&7 → pad 2, rounds 3&6 → pad 3, rounds 4&5 → pad 4
 * Each round lights the target pad's LED segment blue, then polls until the
 * player hits any pad or timeGiven (5000 ms) expires.
 *
 * Hit detection: cap.touched() bitmask is compared to the expected pad value.
 * On a correct hit → greenLEDS() + score added to scoreTot.
 * On a miss/wrong pad → redLEDS() + 0 points.
 *
 * INCONSISTENCY — case 1/case 8 use hardcoded `/50` instead of `/divisor`.
 * With timeGiven=5000 this produces the same result, but is fragile. See file
 * header note for details.
 * -------------------------------------------------------------------------- */
unsigned long ledSEQUENTIAL() {
  int currtouched = cap.touched();
  int lasttouched;
  unsigned long delayTime = timeGiven;
  unsigned long startTime;
  unsigned long timeToTouch;
  unsigned long score = 0;
  //unsigned long scoreTot = 0;

  for (int base = 1; base < 9; base++) {

    switch (base) {
      case 1:
      case 8:
        // Target: pad 1 — segment 0..164, bitmask value 1
        strip.fill(blue, 0, 165);
        strip.show();
        startTime = millis();
        while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        }
        timeToTouch = (millis() - startTime);
        currtouched = cap.touched();
        if (currtouched == 1) {
          Serial.println(timeToTouch);
          score = (timeGiven - timeToTouch) / 50;  // BUG: should be / divisor (see file header)
          greenLEDS();
          scoreTot = scoreTot + score;
        } else {
          score = 0;
          redLEDS();
          scoreTot = scoreTot + score;
        }
        break;
      case 2:
      case 7:
        // Target: pad 2 — segment 166..330, bitmask value 2
        strip.fill(blue, 166, 165);
        strip.show();
        startTime = millis();
        while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        }
        timeToTouch = (millis() - startTime);
        currtouched = cap.touched();
        if (currtouched == 2) {
          Serial.println(timeToTouch);
          score = (timeGiven - timeToTouch) / divisor;
          greenLEDS();
          scoreTot = scoreTot + score;
        } else {
          score = 0;
          redLEDS();
          scoreTot = scoreTot + score;
        }
        break;
      case 3:
      case 6:
        // Target: pad 3 — segment 331..495, bitmask value 4
        strip.fill(blue, 331, 165);
        strip.show();
        startTime = millis();
        while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        }
        timeToTouch = (millis() - startTime);
        currtouched = cap.touched();
        if (currtouched == 4) {
          Serial.println(timeToTouch);
          score = (timeGiven - timeToTouch) / divisor;
          greenLEDS();
          scoreTot = scoreTot + score;
        } else {
          score = 0;
          redLEDS();
          scoreTot = scoreTot + score;
        }
        break;
      case 4:
      case 5:
        // Target: pad 4 — segment 496..660, bitmask value 8
        strip.fill(blue, 496, 165);
        strip.show();
        startTime = millis();
        while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        }
        timeToTouch = (millis() - startTime);
        currtouched = cap.touched();
        if (currtouched == 8) {
          Serial.println(timeToTouch);
          score = (timeGiven - timeToTouch) / divisor;
          greenLEDS();
          scoreTot = scoreTot + score;
        } else {
          score = 0;
          redLEDS();
          scoreTot = scoreTot + score;
        }
        break;
    }
  }
}

/* --------------------------------------------------------------------------
 * ledRANDOM() — Random game mode
 *
 * Runs 10 rounds. Each round picks a random pad (1–4), avoiding repeating the
 * same pad twice in a row (re-rolls until rand != lastRand).
 * Hit detection and scoring are identical to ledSEQUENTIAL().
 * Uses `divisor` consistently — no hardcoded literal.
 * Max possible score: 1000 pts (10 rounds × 100 pts).
 * -------------------------------------------------------------------------- */
unsigned long ledRANDOM() {
  unsigned long score = 0;
  //unsigned long scoreTot = 0;

  for (int i = 0; i < 10; i++) {
    int currtouched = cap.touched();
    int lasttouched;
    int lastRand;
    unsigned long delayTime = timeGiven;
    unsigned long startTime;
    unsigned long timeToTouch;

    /* Pick a random pad, re-rolling if it matches the previous one */
    int rand = random(1, 5);
    while (rand == lastRand) {
      rand = random(1, 5);
    }

    switch (rand) {
      case 1:
        // Target: pad 1 — segment 0..164, bitmask value 1
        strip.fill(blue, 0, 165);
        strip.show();
        startTime = millis();
        while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        }
        timeToTouch = (millis() - startTime);
        currtouched = cap.touched();
        if (currtouched == 1) {
          score = (timeGiven - timeToTouch) / divisor;
          greenLEDS();
          scoreTot = scoreTot + score;
        } else {
          score = 0;
          redLEDS();
          scoreTot = scoreTot + score;
        }
        break;
      case 2:
        // Target: pad 2 — segment 166..330, bitmask value 2
        strip.fill(blue, 166, 165);
        strip.show();
        startTime = millis();
        while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        }
        timeToTouch = (millis() - startTime);
        currtouched = cap.touched();
        if (currtouched == 2) {
          score = (timeGiven - timeToTouch) / divisor;
          greenLEDS();
          scoreTot = scoreTot + score;
        } else {
          score = 0;
          redLEDS();
          scoreTot = scoreTot + score;
        }
        break;
      case 3:
        // Target: pad 3 — segment 331..495, bitmask value 4
        strip.fill(blue, 331, 165);
        strip.show();
        startTime = millis();
        while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        }
        timeToTouch = (millis() - startTime);
        currtouched = cap.touched();
        if (currtouched == 4) {
          score = (timeGiven - timeToTouch) / divisor;
          greenLEDS();
          scoreTot = scoreTot + score;
        } else {
          score = 0;
          redLEDS();
          scoreTot = scoreTot + score;
        }
        break;
      case 4:
        // Target: pad 4 — segment 496..660, bitmask value 8
        strip.fill(blue, 496, 165);
        strip.show();
        startTime = millis();
        while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        }
        timeToTouch = (millis() - startTime);
        currtouched = cap.touched();
        if (currtouched == 8) {
          score = (timeGiven - timeToTouch) / divisor;
          greenLEDS();
          scoreTot = scoreTot + score;
        } else {
          score = 0;
          redLEDS();
          scoreTot = scoreTot + score;
        }
        break;
    }
    lasttouched = currtouched;
    lastRand = rand;
  }
}
