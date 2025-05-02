#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

#define LED_PIN 12

#define LED_COUNT 660

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_MPR121 cap = Adafruit_MPR121();

uint16_t lasttouched = 0;
uint16_t currtouched = 0;
uint32_t green = strip.Color(0, 255, 0);
uint32_t red = strip.Color(255, 0, 0);
uint32_t blue = strip.Color(0, 0, 255);

unsigned long scoreTot = 0;
int timeout = 5000;

int divisor = timeout / 100;

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


void redLEDS() {
  strip.fill(red, 0, 660);
  strip.show();
  delay(750);
  strip.clear();
  strip.show();
  delay(500);
}

unsigned long sequentialFunction(uint16_t startLED, int padNum) {
  int currtouched = cap.touched();
  int lasttouched;
  unsigned long delayTime = timeout;
  unsigned long startTime;
  unsigned long timeToTouch;
  unsigned long score = 0;

  strip.fill(blue, startLED, 165);
  strip.show();
  Serial.println(cap.touched());
  
  startTime = millis();
  while ((millis() - startTime) < delayTime && cap.touched() == 0 ) {
  }
  timeToTouch = (millis() - startTime);
  currtouched = cap.touched();
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
          sequentialFunction(0, 1);
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
        sequentialFunction(166, 2);
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
        sequentialFunction(331, 4);
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
        sequentialFunction(496, 8);
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
      case 1:
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
      case 2:
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
      case 3:
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
      case 4:
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