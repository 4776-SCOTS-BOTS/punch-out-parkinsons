/*
 * EEPROM Clear — Punch Out Parkinson's utility sketch
 *
 * PURPOSE
 * -------
 * Wipes every byte of the Arduino's EEPROM to 0. Use this when you want to
 * reset the high score data that PoP_Full stores across power cycles.
 * It is NOT part of normal gameplay — upload it only when a full score reset
 * is needed, then re-upload PoP_Full immediately afterward (this sketch does
 * nothing in loop() and will leave the board sitting idle).
 *
 * HOW TO KNOW IT'S DONE
 * ----------------------
 * Pin 10 (the LCD backlight pin) is driven HIGH at the end of setup().
 * If the backlight turns on, the wipe completed successfully.
 *
 * WORKFLOW
 * --------
 * 1. Upload this sketch.
 * 2. Wait for the LCD backlight to turn on.
 * 3. Re-upload PoP_Full — scores now start from zero.
 *
 * Sets all of the bytes of the EEPROM to 0.
 * Please see eeprom_iteration for a more in depth
 * look at how to traverse the EEPROM.
 *
 * This example code is in the public domain.
 */

#include <EEPROM.h>

void setup() {
  // initialize the LED pin as an output.
  pinMode(10, OUTPUT);

  /***
    Iterate through each byte of the EEPROM storage.
    Larger AVR processors have larger EEPROM sizes, E.g:
    - Arduino Duemilanove: 512 B EEPROM storage.
    - Arduino Uno:         1 kB EEPROM storage.
    - Arduino Mega:        4 kB EEPROM storage.
    Rather than hard-coding the length, you should use the pre-provided length function.
    This will make your code portable to all AVR processors.
  ***/

  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }

  // turn the LED on when we're done
  digitalWrite(10, HIGH);
}

void loop() {
  /** Empty loop. **/
}