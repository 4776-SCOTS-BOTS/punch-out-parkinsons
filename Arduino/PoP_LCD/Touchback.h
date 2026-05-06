/*
 * Touchback.h — Hardware abstraction and game logic for Punch Out Parkinson's (LCD-only version).
 *
 * Declares and defines the NeoPixel strip, MPR121 capacitive touch controller, scoring
 * globals, and all game-mode functions used by PoP_LCD.ino.
 *
 * NOTE on ledRANDOM(): unlike later versions of this sketch, ledRANDOM() here inlines
 * its per-pad lighting and touch logic directly inside a switch statement rather than
 * delegating to a helper function like sequentialFunction(). If you compare with
 * PoP_Full, that is the intentional structural difference.
 */

#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

// _BV(bit) produces a bitmask with a single bit set; used by the MPR121 library.
#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// ── Hardware pin / count constants ──────────────────────────────────────────
#define LED_PIN 12    // Data line for the NeoPixel strip
#define LED_COUNT 660 // Total LEDs: 4 segments × 165 LEDs each

// ── Hardware object instances ────────────────────────────────────────────────
// strip: 660-LED GRB NeoPixel chain on pin 12.
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// cap: MPR121 capacitive touch controller. Initialized at I2C address 0x5A in setup().
Adafruit_MPR121 cap = Adafruit_MPR121();

// ── Touch-state tracking (not used directly in this version but reserved) ────
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

// ── LED color presets ────────────────────────────────────────────────────────
uint32_t green = strip.Color(0, 255, 0);
uint32_t red   = strip.Color(255, 0, 0);
uint32_t blue  = strip.Color(0, 0, 255);

// ── Scoring globals ──────────────────────────────────────────────────────────
// scoreTot accumulates across all rounds of a game session; reset in showScore() after display.
unsigned long scoreTot = 0;

// timeout: the window (ms) the player has to hit the lit pad. Default 5000 ms (5 s).
// Adjusted at runtime by timeDelay() via the LCD "Time To Hit" sub-menu.
int timeout = 5000;

// divisor: computed once from the default timeout so that score = (timeout - timeToHit) / divisor
// yields a maximum of 100 points per hit. Recalculated if timeout changes.
int divisor = timeout / 100;

// ── Feedback LED animations ──────────────────────────────────────────────────

// greenLEDS: two quick green flashes on the full strip — played on a correct hit.
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

// redLEDS: one solid red flash — played when the player misses or hits the wrong pad.
void redLEDS() {
  strip.fill(red, 0, 660);
  strip.show();
  delay(750);
  strip.clear();
  strip.show();
  delay(500);
}

// ── Core pad-hit handler ─────────────────────────────────────────────────────
/*
 * sequentialFunction: lights a single pad's LED segment (blue), waits up to `timeout` ms
 * for a touch, then scores and gives feedback.
 *
 *   startLED — first LED index for this pad's segment (0 / 166 / 331 / 496)
 *   padNum   — expected cap.touched() bitmask value for this pad (1 / 2 / 4 / 8)
 *
 * Scoring: score = (timeout - timeToTouch) / divisor. Max score per round = 100.
 * A wrong pad or timeout scores 0 and triggers redLEDS().
 */
unsigned long sequentialFunction(uint16_t startLED, int padNum) {
  int currtouched = cap.touched();
  int lasttouched;
  unsigned long delayTime = timeout;
  unsigned long startTime;
  unsigned long timeToTouch;
  unsigned long score = 0;

  // Light the target pad's segment and start the countdown.
  strip.fill(blue, startLED, 165);
  strip.show();
  Serial.println(cap.touched());

  startTime = millis();
  // Spin until the pad is touched or the timeout expires.
  while ((millis() - startTime) < delayTime && cap.touched() == 0 ) {
  }
  timeToTouch = (millis() - startTime);
  currtouched = cap.touched();

  // Award points only if the correct pad (matching padNum bitmask) was hit.
  if (currtouched == padNum) {
    Serial.println(timeToTouch);
    score = (timeout - timeToTouch) / divisor;
    greenLEDS();
    scoreTot = scoreTot + score;
  } else {
    score = 0;
    redLEDS();
    scoreTot = scoreTot + score;
  }
}

// ── Sequential game mode ─────────────────────────────────────────────────────
/*
 * ledSEQUENTIAL: runs 8 rounds that cycle through pads 1→2→3→4→4→3→2→1
 * (base values 1–8 map to pad indices via the switch below).
 * Maximum total score = 8 rounds × 100 points = 800.
 *
 * The large commented-out blocks below each sequentialFunction() call are the
 * original inline logic that was refactored into sequentialFunction(). They are
 * kept as historical reference showing what the helper replaced.
 */
unsigned long ledSEQUENTIAL() {
  int currtouched = cap.touched();
  int lasttouched;
  unsigned long delayTime = timeout;
  unsigned long startTime;
  unsigned long timeToTouch;
  unsigned long score = 0;

  // Loop 8 rounds. Cases are paired (1&8, 2&7, 3&6, 4&5) so pads mirror back.
  for (int base = 1; base < 9; base++) {

    switch (base) {
      case 1:
      case 8:
          sequentialFunction(0, 1); // Pad 1: segment offset 0, bitmask 1
        // strip.fill(blue, 0, 165);
        // strip.show();
        // startTime = millis();
        // while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        // }
        // timeToTouch = (millis() - startTime);
        // currtouched = cap.touched();
        // if (currtouched == 1) {
        //   //Serial.println(timeToTouch);
        //   score = (timeout - timeToTouch) / divisor;
        //   greenLEDS();
        //   scoreTot = scoreTot + score;
        // } else {
        //   score = 0;
        //   redLEDS();
        //   scoreTot = scoreTot + score;
        // }
        break;
      case 2:
      case 7:
        sequentialFunction(166, 2); // Pad 2: segment offset 166, bitmask 2
        // strip.fill(blue, 166, 165);
        // strip.show();
        // startTime = millis();
        // while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        // }
        // timeToTouch = (millis() - startTime);
        // currtouched = cap.touched();
        // if (currtouched == 2) {
        //   //Serial.println(timeToTouch);
        //   score = (timeout - timeToTouch) / divisor;
        //   greenLEDS();
        //   scoreTot = scoreTot + score;
        // } else {
        //   score = 0;
        //   redLEDS();
        //   scoreTot = scoreTot + score;
        // }
        break;
      case 3:
      case 6:
        sequentialFunction(331, 4); // Pad 3: segment offset 331, bitmask 4
        // strip.fill(blue, 331, 165);
        // strip.show();
        // startTime = millis();
        // while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        // }
        // timeToTouch = (millis() - startTime);
        // currtouched = cap.touched();
        // if (currtouched == 4) {
        //   //Serial.println(timeToTouch);
        //   score = (timeout - timeToTouch) / divisor;
        //   greenLEDS();
        //   scoreTot = scoreTot + score;
        // } else {
        //   score = 0;
        //   redLEDS();
        //   scoreTot = scoreTot + score;
        // }
        break;
      case 4:
      case 5:
        sequentialFunction(496, 8); // Pad 4: segment offset 496, bitmask 8
        // strip.fill(blue, 496, 165);
        // strip.show();
        // startTime = millis();
        // while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        // }
        // timeToTouch = (millis() - startTime);
        // currtouched = cap.touched();
        // if (currtouched == 8) {
        //   //Serial.println(timeToTouch);
        //   score = (timeout - timeToTouch) / divisor;
        //   greenLEDS();
        //   scoreTot = scoreTot + score;
        // } else {
        //   score = 0;
        //   redLEDS();
        //   scoreTot = scoreTot + score;
        // }
        break;
    }
  }
}

// ── Random game mode ─────────────────────────────────────────────────────────
/*
 * ledRANDOM: runs 10 rounds with a randomly chosen pad each round.
 * The same pad cannot be chosen twice in a row (lastRand guard).
 * Maximum total score = 10 rounds × 100 points = 1000.
 *
 * IMPORTANT: unlike ledSEQUENTIAL(), this function does NOT delegate to
 * sequentialFunction(). Each case inlines its own strip.fill / touch-wait /
 * score logic. This reflects an earlier stage of development. If you refactor,
 * extract the repeated block into a helper the same way sequentialFunction()
 * was written for the sequential mode.
 *
 * Note: delayTime is hard-coded to 2500 ms here, ignoring the global `timeout`.
 * This was likely an oversight — future versions should use `timeout` for
 * consistency with the sequential mode and the "Time To Hit" menu option.
 */
unsigned long ledRANDOM() {
  unsigned long score = 0;

  for (int i = 0; i < 10; i++) {
    int currtouched = cap.touched();
    int lasttouched;
    int lastRand;
    unsigned long delayTime = 2500; // Hard-coded; does not respect the global `timeout`.
    unsigned long startTime;
    unsigned long timeToTouch;

    // Pick a random pad (1–4); re-roll if it matches the previous round's pad.
    int rand = random(1, 5);
    while (rand == lastRand) {
      rand = random(1, 5);
    }

    // Each case: light the pad's segment, wait for touch, score, give feedback.
    // pad bitmask values: pad1=1, pad2=2, pad3=4, pad4=8 (MPR121 bitmask).
    switch (rand) {
      case 1: // Pad 1 — segment offset 0, bitmask 1
        strip.fill(blue, 0, 165);
        strip.show();
        startTime = millis();
        while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        }
        timeToTouch = (millis() - startTime);
        currtouched = cap.touched();
        if (currtouched == 1) {
          score = (timeout - timeToTouch) / divisor;
          greenLEDS();
          scoreTot = scoreTot + score;
          Serial.println(scoreTot);
        } else {
          score = 0;
          redLEDS();
          scoreTot = scoreTot + score;
        }
        break;
      case 2: // Pad 2 — segment offset 166, bitmask 2
        strip.fill(blue, 166, 165);
        strip.show();
        startTime = millis();
        while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        }
        timeToTouch = (millis() - startTime);
        currtouched = cap.touched();
        if (currtouched == 2) {
          score = (timeout - timeToTouch) / divisor;
          greenLEDS();
          scoreTot = scoreTot + score;
          Serial.println(scoreTot);
        } else {
          score = 0;
          redLEDS();
          scoreTot = scoreTot + score;
        }
        break;
      case 3: // Pad 3 — segment offset 331, bitmask 4
        strip.fill(blue, 331, 165);
        strip.show();
        startTime = millis();
        while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        }
        timeToTouch = (millis() - startTime);
        currtouched = cap.touched();
        if (currtouched == 4) {
          score = (timeout - timeToTouch) / divisor;
          greenLEDS();
          scoreTot = scoreTot + score;
          Serial.println(scoreTot);
        } else {
          score = 0;
          redLEDS();
          scoreTot = scoreTot + score;
        }
        break;
      case 4: // Pad 4 — segment offset 496, bitmask 8
        strip.fill(blue, 496, 165);
        strip.show();
        startTime = millis();
        while ((millis() - startTime) < delayTime && cap.touched() == 0) {
        }
        timeToTouch = (millis() - startTime);
        currtouched = cap.touched();
        if (currtouched == 8) {
          score = (timeout - timeToTouch) / divisor;
          greenLEDS();
          scoreTot = scoreTot + score;
          Serial.println(scoreTot);
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
  Serial.print("Final Score = ");
  Serial.print(scoreTot);
  return scoreTot;
}
