/*
 * Touchback.h — Hardware abstraction and game engine for Punch Out Parkinson's.
 *
 * Declares and drives the two core hardware peripherals (MPR121 capacitive touch
 * controller and the 660-LED NeoPixel strip) and implements all game-logic functions:
 * sequential and random gameplay modes, per-round scoring, and LED feedback animations.
 * Included by PoP_Full.ino, which owns the LCD menu and external button handling.
 *
 * Hardware summary:
 *   - MPR121 on I2C at 0x5A; cap.touched() returns a uint16_t bitmask (bit 0 = pad 1,
 *     bit 1 = pad 2, bit 2 = pad 3, bit 3 = pad 4).
 *   - NeoPixel strip: 660 LEDs on pin 12, four 165-LED segments (one per pad).
 *     Segment offsets: 0, 166, 331, 496. The +1 gap between segments leaves one LED
 *     dark at each boundary — this is intentional.
 */

#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// Pin 12 drives the NeoPixel data line.
// NEVER USE PIN 4, 5, 6, 7, 8, 9, 10 — those are claimed by the LCD shield.
#define LED_PIN 12

#define LED_COUNT 660

// --------------------------------------------------------------------------
// Hardware object declarations
// --------------------------------------------------------------------------

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_MPR121 cap = Adafruit_MPR121();

// --------------------------------------------------------------------------
// Touch state and color constants
// --------------------------------------------------------------------------

// lasttouched / currtouched hold successive bitmask snapshots for edge detection.
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

uint32_t green  = strip.Color(0,   255, 0);
uint32_t red    = strip.Color(255, 0,   0);
uint32_t blue   = strip.Color(0,   0,   255);
uint32_t white  = strip.Color(255, 255, 255);
uint32_t black  = strip.Color(0,   0,   0);
uint32_t yellow = strip.Color(255, 255, 0);

// --------------------------------------------------------------------------
// Scoring state
// --------------------------------------------------------------------------

// scoreTot accumulates points within a single game run; reset to 0 after showScore().
unsigned long scoreTot      = 0;
unsigned long highSeqScore  = 0;
unsigned long highRandScore = 0;

// delayTime: the hit window in milliseconds. Default 5 s; user-adjustable 1–60 s.
unsigned long delayTime = 5000;

// scoreDivisor normalises the per-round max to 100 pts regardless of delayTime.
// Calculated as delayTime / 100 before each game run.
int scoreDivisor;

// --------------------------------------------------------------------------
// LED feedback animations
// --------------------------------------------------------------------------

// greenLEDS — double-flash green across all LEDs: correct hit feedback.
void greenLEDS() {
  strip.fill(green, 0, LED_COUNT);
  strip.show();
  delay(250);
  strip.clear();
  strip.show();
  delay(250);
  strip.fill(green, 0, LED_COUNT);
  strip.show();
  delay(250);
  strip.clear();
  strip.show();
}

// redLEDS — single long red pulse: missed or wrong pad feedback.
void redLEDS() {
  strip.fill(red, 0, LED_COUNT);
  strip.show();
  delay(750);
  strip.clear();
  strip.show();
  delay(500);
}

// highScoreBlink — two-second white hold: used as a simpler high-score flash
// (currently superseded by highScoreRotateCog in PoP_Full.ino).
void highScoreBlink() {
  strip.fill(white, 0, LED_COUNT);
  strip.show();
  delay(2000);
  strip.clear();
  strip.show();
  delay(2000);
}

// highScoreRotate — a 165-LED white window sweeps once around the full strip.
// Available as an alternative celebration animation; not called by the main sketch.
void highScoreRotate() {
  int rotations = 1;
  int speed = 10;
  int segmentSize = 165;  // how many LEDs are in the "rotating" segment
  int totalSteps = LED_COUNT;

  for (int r = 0; r < rotations; r++) {
    for (int i = 0; i < totalSteps; i++) {
      strip.clear();

      // Light up the segment of LEDs
      for (int j = 0; j < segmentSize; j++) {
        int ledIndex = (i + j) % LED_COUNT;
        strip.setPixelColor(ledIndex, white);
      }

      strip.show();
      delay(speed);
    }
  }

  strip.clear();
  strip.show();
}

// highScoreRotateCog — the primary high-score celebration animation.
// Every other LED lights up (the "cog" pattern) and the whole pattern rotates
// for completeTime milliseconds. Segments 1 & 4 are green; segments 2 & 3 are yellow.
void highScoreRotateCog(unsigned long completeTime) {
  int speed = 50;
  int step = 2;  // Every other LED is lit up, creating a "cog" effect
  int position = 0;
  unsigned long startTime = millis();

  Serial.println("highScoreRotateCog triggered");

  while (millis() - startTime < completeTime) {
    strip.clear();  // Clear the strip to reset the pattern

    // Light up every other LED to create the "cog" effect on each strip
    for (int i = 0; i < LED_COUNT; i += step) {
      int ledIndex = (position + i) % LED_COUNT;

      // Determine the color for each strip
      if (ledIndex < 165) {
        // First strip (Green)
        strip.setPixelColor(ledIndex, green);
      } else if (ledIndex < 330) {
        // Second strip (Yellow)
        strip.setPixelColor(ledIndex, yellow);
      } else if (ledIndex < 495) {
        // Third strip (Yellow)
        strip.setPixelColor(ledIndex, yellow);
      } else {
        // Fourth strip (Green)
        strip.setPixelColor(ledIndex, green);
      }
    }

    strip.show();
    delay(speed);

    // Rotate the pattern by shifting the position
    position = (position + 1) % LED_COUNT;
  }

  strip.clear();
  strip.show();  // Turn off the LEDs after the rotation is complete
}

// --------------------------------------------------------------------------
// Core game functions
// --------------------------------------------------------------------------

/*
 * sequentialFunction — lights one pad's LED segment blue, then waits up to
 * delayTime ms for the player to touch the correct pad.
 *
 * Parameters:
 *   startLED — first LED index of the pad's segment (0 / 166 / 331 / 496).
 *   padNum   — MPR121 bitmask value for the expected pad (1 / 2 / 4 / 8).
 *
 * Scoring: score = (delayTime - timeToHit) / scoreDivisor, max 100 per round.
 * A correct touch triggers greenLEDS(); any other result triggers redLEDS().
 * The running total is accumulated in scoreTot.
 */
unsigned long sequentialFunction(uint16_t startLED, int padNum) {
  //padNum is the pin number on Adafruit board, goes 1, 2, 4, 8, etc.
  //startLED is the diode that you start on. There are 165 diodes per LED strip
  int currtouched = cap.touched();
  int lasttouched;
  unsigned long startTime;
  unsigned long timeToTouch;
  unsigned long score = 0;
  //delayTime = delayTime * 1000;
  scoreDivisor = delayTime / 100;

  strip.fill(blue, startLED, 165);
  strip.show();
  // Serial.println("Blue light");
  // Serial.println(cap.touched());

  startTime = millis();
  // Wait until either the hit window expires or any pad is touched.
  while ((millis() - startTime) < delayTime && cap.touched() == 0 ) {
  }
  timeToTouch = (millis() - startTime);
  currtouched = cap.touched();
  if (currtouched == padNum) {
    Serial.println(currtouched);
    score = (delayTime - timeToTouch) / scoreDivisor;
    greenLEDS();
    scoreTot = scoreTot + score;
  } else {
    score = 0;
    redLEDS();
    scoreTot = scoreTot + score;
  }
  lasttouched == currtouched;
}

/*
 * ledSEQUENTIAL — runs one full sequential game: 8 rounds cycling through
 * all 4 pads twice (pads 1→2→3→4→4→3→2→1 via the switch mapping below).
 * Maximum possible score is 800 (8 rounds × 100 pts).
 *
 * The case comments note physical wiring changes made on 3/16/25 that shifted
 * which LED segment corresponds to each pad number — do not reorder these
 * without verifying the physical strip layout.
 */
unsigned long ledSEQUENTIAL() {
  int currtouched = cap.touched();
  int lasttouched;
  unsigned long startTime;
  unsigned long timeToTouch;
  unsigned long score = 0;

  for (int base = 1; base < 9; base++) {

    switch (base) {
      case 1:
      case 8:
          //As of 3/16/25, last in LED line due to hardware issues on output
          sequentialFunction(0, 1); // Pin 1 (startLED originally 0)
        break;
      case 2:
      case 7:
        //As of 3/16/25, first in LED line due to hardware issues on output on pad 1 (top left)
        sequentialFunction(166, 2); // Pin 2 (startLED originally 166)
        break;
      case 3:
      case 6:
        //As of 3/16/25, third in LED line due to hardware issues on output on pad 1 (top left)
        sequentialFunction(331, 4); // Pin 3 (startLED did not change)
        break;
      case 4:
      case 5:
        //As of 3/16/25, second in LED line due to hardware issues on output on pad 1 (top left)
        sequentialFunction(496, 8); // Pin 4 (startLED originally 496)
        break;
    }
  }
}

/*
 * randomFunction — single-round helper for ledRANDOM; identical scoring logic
 * to sequentialFunction but called with a pad chosen by the parent loop.
 *
 * Parameters:
 *   startLED — first LED index of the pad's segment.
 *   padNum   — MPR121 bitmask value for the expected pad.
 */
unsigned long randomFunction(int startLED, int padNum) {
  //padNum is the pin number on Adafruit board, goes 1, 2, 4, 8, etc.
  //startLED is the diode that you start on. There are 165 diodes per LED strip
  int currtouched = cap.touched();
  int lasttouched;
  int lastRand;
  unsigned long startTime;
  unsigned long timeToTouch;
  int score = 0;
  //delayTime = delayTime * 1000;
  scoreDivisor = delayTime / 100;

  strip.fill(blue, startLED, 165);
  strip.show();

  startTime = millis();
  // Wait until either the hit window expires or any pad is touched.
  while ((millis() - startTime) < delayTime && cap.touched() == 0) {}
  timeToTouch = (millis() - startTime);
  currtouched = cap.touched();
  if (currtouched == padNum) {
    score = (delayTime - timeToTouch) / scoreDivisor;
    greenLEDS();
    scoreTot = scoreTot + score;
    //Serial.println(scoreTot);
  } else {
    score = 0;
    redLEDS();
    scoreTot = scoreTot + score;
  }

  Serial.print("randomFunction Score: ");
  Serial.println(scoreTot);
  // if (scoreTot > highRandScore) {
  //   highRandScore = scoreTot;
  // }
}

/*
 * ledRANDOM — runs one full random game: 10 rounds, each pad chosen randomly
 * with the constraint that the same pad cannot appear twice in a row.
 * Maximum possible score is 1000 (10 rounds × 100 pts).
 * Returns the final scoreTot (caller is responsible for resetting it afterward).
 */
unsigned long ledRANDOM() {
  unsigned long score = 0;

  for (int i = 0; i < 10; i++) {
    int currtouched = cap.touched();
    int lasttouched;
    int lastRand;
    unsigned long startTime;
    unsigned long timeToTouch;

    // Pick a random pad 1–4, re-rolling if it matches the previous pad.
    int rand = random(1, 5);
    while (rand == lastRand) {
      rand = random(1, 5);
    }

    // Map random pad number to the correct LED segment offset and MPR121 bitmask.
    switch (rand) {
      case 1: {
        randomFunction(0, 1);
        break;
      }
      case 2: {
        randomFunction(166, 2);
        break;
      }
      case 3: {
        randomFunction(331, 4);
        break;
      }
      case 4: {
        randomFunction(496, 8);
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
