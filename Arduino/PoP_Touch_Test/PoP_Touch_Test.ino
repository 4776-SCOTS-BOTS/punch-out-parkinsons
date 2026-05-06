/*
 * PoP_Touch_Test — capacitive touch sensor test stub
 *
 * PURPOSE
 * -------
 * Placeholder sketch reserved for MPR121 touch sensor validation.
 * The intent is to initialize the MPR121, then stream raw touch/release
 * values to Serial so you can verify wiring, I2C address, and sensitivity
 * thresholds before running PoP_Full.
 *
 * WHEN TO USE
 * -----------
 * - After rewiring or replacing a touch pad, confirm the correct electrode
 *   fires when struck.
 * - When tuning the MPR121 touch/release thresholds (registers 0x41–0x5A),
 *   watch Serial output to find stable values with no false triggers.
 * - Sanity check before a competition or demo — ensure all pads respond.
 *
 * This file is currently empty. Add MPR121 initialization in setup() and
 * Serial.print() calls in loop() to implement the test.
 */

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
