#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

//NEVER USE PIN 4, 5, 6, 7, 8, 9, 10
#define LED_PIN 12

#define LED_COUNT 660

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_MPR121 cap = Adafruit_MPR121();

uint16_t lasttouched = 0;
uint16_t currtouched = 0;
uint32_t green = strip.Color(0, 255, 0);
uint32_t red = strip.Color(255, 0, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t white = strip.Color(255, 255, 255);
uint32_t black = strip.Color(0, 0, 0);
uint32_t yellow = strip.Color(255, 255, 0);

unsigned long scoreTot = 0;
unsigned long highSeqScore = 0;
unsigned long highRandScore = 0;

unsigned long delayTime = 5000;

int scoreDivisor;

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

void redLEDS() {
  strip.fill(red, 0, LED_COUNT);
  strip.show();
  delay(750);
  strip.clear();
  strip.show();
  delay(500);
}

void highScoreBlink() {
  strip.fill(white, 0, LED_COUNT);
  strip.show();
  delay(2000);
  strip.clear();
  strip.show();
  delay(2000);
}

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

unsigned long ledRANDOM() {
  unsigned long score = 0;

  for (int i = 0; i < 10; i++) {
    int currtouched = cap.touched();
    int lasttouched;
    int lastRand; 
    unsigned long startTime;
    unsigned long timeToTouch;

    int rand = random(1, 5);
    while (rand == lastRand) {
      rand = random(1, 5);
    }

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