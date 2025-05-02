#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

#define leftStripPin 10
#define rightStripPin 11

#define LED_COUNT 330

Adafruit_NeoPixel leftStrip(LED_COUNT, leftStripPin, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel rightStrip(LED_COUNT, rightStripPin, NEO_GRB + NEO_KHZ800);

Adafruit_MPR121 cap = Adafruit_MPR121();

uint16_t lasttouched = 0;
uint16_t currtouched = 0;

uint32_t greenL = leftStrip.Color(0, 255, 0);
uint32_t redL = leftStrip.Color(255, 0, 0);
uint32_t blueL = leftStrip.Color(0, 0, 255);
uint32_t whiteL = leftStrip.Color(255, 255, 255);
uint32_t blackL = leftStrip.Color(0, 0, 0);

uint32_t greenR = rightStrip.Color(0, 255, 0);
uint32_t redR = rightStrip.Color(255, 0, 0);
uint32_t blueR = rightStrip.Color(0, 0, 255);
uint32_t whiteR = rightStrip.Color(255, 255, 255);
uint32_t blackR = rightStrip.Color(0, 0, 0);

unsigned long scoreTot = 0;
unsigned long highSeqScore = 0;
unsigned long highRandScore = 0;

int timeout = 5000;

int divisor = timeout / 100;

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

unsigned long sequentialFunction(String side, uint16_t startLED, int padNum) {
  //padNum is the pin number on Adafruit board, goes 1, 2, 4, 8, etc.
  //startLED is the diode that you start on. There are 165 diodes per LED strip
  int currtouched = cap.touched();
  int lasttouched;
  unsigned long delayTime = timeout;
  unsigned long startTime;
  unsigned long timeToTouch;
  unsigned long score = 0;

  if (side.equals("left")) {
    leftStrip.fill(blueL, startLED, 165);
    leftStrip.show();
  } else if (side.equals("right")) {
    rightStrip.fill(blueR, startLED, 165);
    rightStrip.show();
  }
  // Serial.println("Blue light");
  // Serial.println(cap.touched());
  
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

unsigned long randomFunction(String side, int startLED, int padNum) {
  int currtouched = cap.touched();
  int lasttouched;
  int lastRand;
  int score = 0;
  unsigned long delayTime = 2500;
  unsigned long startTime;
  unsigned long timeToTouch;

  if (side.equals("left")) {
    leftStrip.fill(blueL, startLED, 165);
    leftStrip.show();
  } else if (side.equals("right")) {
    rightStrip.fill(blueR, startLED, 165);
    rightStrip.show();
  }

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

unsigned long ledRANDOM() {
  unsigned long score = 0;

  for (int i = 0; i < 10; i++) {
    int currtouched = cap.touched();
    int lasttouched;
    int lastRand;
    unsigned long delayTime = 2500;
    unsigned long startTime;
    unsigned long timeToTouch;

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