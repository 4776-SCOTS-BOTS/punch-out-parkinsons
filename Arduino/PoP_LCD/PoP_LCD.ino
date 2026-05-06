/*
 * PoP_LCD.ino — Punch Out Parkinson's (LCD-button-only version).
 *
 * This is an intermediate version of the sketch that uses ONLY the five built-in
 * buttons on the LCD shield (no external wired buttons, no EEPROM). High scores
 * are lost on power cycle. It is the best starting point for understanding the
 * LCD menu structure before the full PoP_Full version added external buttons and
 * persistent storage.
 *
 * Hardware summary:
 *   - 16×2 LCD shield via LiquidCrystal (pins 4–9)
 *   - All five buttons share analog pin A0 (resistor-ladder voltage divider)
 *   - Backlight brightness controlled by PWM on pin 10
 *   - NeoPixel strip and MPR121 touch controller configured in Touchback.h
 */

#include <LiquidCrystal.h>
#include "Touchback.h"

// ── LCD object ───────────────────────────────────────────────────────────────
// Standard Arduino LCD shield wiring: RS=8, EN=9, D4–D7=4–7.
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ── Button-reading state ─────────────────────────────────────────────────────
int lcd_key     = 0;
int adc_key_in  = 0;
int adc_key_old;
int analogPin   = A0;  //Define the A0 as analogPin as integer type.

bool on = false; // True after setup() completes; reserved for future use.

// ── Menu state variables ─────────────────────────────────────────────────────
//Menu Item Numbers
int seqOptions     = 0; // Sub-menu position within the Sequence menu (0=root, 1=Sequential, 2=Random, 3=Return)
int hitOptions     = 0; // 1 when the "Time To Hit" sub-menu is active; enables left/right to adjust timeout
int displayOptions = 0; // Reserved for a future "Time To Display" menu item
int mobileOptions  = 0; // Reserved for a future "Mobility" menu item

// ── Button constants ─────────────────────────────────────────────────────────
//Define values for the built in buttons as integers
#define lcdRIGHT  0
#define lcdUP     1
#define lcdDOWN   2
#define lcdLEFT   3
#define lcdSELECT 4
#define lcdNONE   5

// ── Button reader ────────────────────────────────────────────────────────────
/*
 * read_LCD_buttons: samples the resistor-ladder voltage on A0 and returns the
 * corresponding button constant. Each button pulls A0 to a different voltage;
 * the thresholds below are midpoints between those voltages.
 * Returns lcdNONE when no button is pressed (ADC reads ~1023).
 */
//Sets the value of adc_key_in to the value of the currently pressed key
int read_LCD_buttons() {
  adc_key_in = analogRead(0);

  if (adc_key_in > 1000) return lcdNONE;

  if (adc_key_in < 50)  return lcdRIGHT;
  if (adc_key_in < 150) return lcdUP;
  if (adc_key_in < 300) return lcdDOWN;
  if (adc_key_in < 450) return lcdLEFT;
  if (adc_key_in < 700) return lcdSELECT;

  Serial.println(adc_key_in);
  return lcdNONE;
}

// ── Post-game score display ───────────────────────────────────────────────────
// showScore: prints the accumulated scoreTot to the LCD second row, then resets it.
void showScore() {
  lcd.setCursor(0, 1);
  lcd.print("Your Score: ");
  lcd.print(scoreTot);
  if (scoreTot > 0) {
    scoreTot = 0;
  }
}

// ── Timeout adjustment ────────────────────────────────────────────────────────
/*
 * timeDelay: increments or decrements `timeout` by `direction` seconds, then
 * converts the new value to milliseconds for use by game functions.
 * Called by the left/right buttons while hitOptions == 1.
 *
 * BUG NOTE: timeout is already in milliseconds when this function is called on
 * subsequent presses, but the function re-multiplies by 1000 each time. This
 * means repeated presses compound incorrectly. In PoP_Full this was corrected
 * by storing the raw-seconds value separately.
 */
void timeDelay(int direction) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Timeout delay");
  lcd.setCursor(0, 1);
  lcd.print(timeout + direction);

  timeout = timeout + direction;
  //Changes timeout from seconds to milliseconds
  timeout = timeout * 1000;
}

// ── Sequence sub-menu ─────────────────────────────────────────────────────────
/*
 * sequences: scrolls through the Sequence sub-menu (Sequential / Random / Return)
 * by adding `direction` (+1 = down, -1 = up) to seqOptions.
 * Wraps back to item 1 if the user scrolls past item 3.
 */
//Sequence Options
void sequences(int direction) {
  seqOptions = seqOptions + direction;

  //Loop menu back to first item
  if (seqOptions > 3) {
    seqOptions = 1;
  }

  switch (seqOptions) {
    case 0: {
      if (item_num == 1) {
        //delay(250);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("   Sequences");
        lcd.setCursor(0, 1);
        lcd.print("Down For Options");
      }
      break;
    }
    case 1: {
      delay(250);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("   Sequential");
      break;
    }
    case 2: {
      delay(250);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("     Random");
      break;
    }
    case 3: {
      delay(250);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" Return To Menu");
      break;
    }
  }
}

// ── Time-to-hit setter (unused in main loop) ──────────────────────────────────
/*
 * hitTimes: computes a new timeout in milliseconds from `hitOptions + direction`
 * seconds and writes it to the global `timeout`. Not wired into the main loop
 * in this version — timeDelay() is used instead.
 */
int hitTimes(int direction) {
  //Adds or subtracts from the value of hitOptions, then multiplies that value by 1000 to get it in milliseconds
  hitOptions = (hitOptions + direction) * 1000;

  timeout = hitOptions;
  return timeout;
}

// ── Main menu navigation ──────────────────────────────────────────────────────
/*
 * switcher: moves through the top-level menu by adding `switchDirection` to
 * item_num, clamped to [1, 3]. Items 4 (Mobility) is defined in the switch but
 * unreachable because item_num is constrained to 3 — it exists for a planned
 * future menu item.
 */
//Menu Switcher - Switch items on main menu
void switcher(int switchDirection) {
  lcd.clear();
  lcd.setCursor(0, 1);
  item_num = item_num + switchDirection;
  item_num = constrain(item_num, 1, 3); // Caps navigation at 3 items; item 4 is unreachable.
  lcd.clear();
  //lcd.setCursor(16, 0);
  //lcd.print(item_num);
  delay(250);

  switch (item_num) {
    case 1:
      {
        lcd.setCursor(0, 0);
        lcd.print("    Sequence");
        lcd.setCursor(0, 1);
        lcd.print("Down For Options");

        break;
      }
    case 2: {
      lcd.setCursor(0, 0);
      lcd.print("  Time To Hit");
      lcd.setCursor(0, 1);
      lcd.print("Down For Options");

      break;
    }
    case 3:
      {
        lcd.setCursor(0, 0);
        lcd.print("Time To Display");
        lcd.setCursor(0, 1);
        lcd.print("Down For Options");

        break;
      }

    case 4: // Planned "Mobility" menu item — currently unreachable (item_num capped at 3).
      {
        lcd.setCursor(0, 0);
        lcd.print("    Mobility");
        lcd.setCursor(0, 1);
        lcd.print("Down For Options");

        break;
      }
  }
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
  //VISUAL BEGINNING
  lcd.begin(16, 2);
  Serial.begin(9600);
  pinMode(10, OUTPUT);   // sets backlight pin-10 as PWM output
  analogWrite(10, 125);  // Set backlight to 50% brightness
  lcd.setCursor(0, 0);
  lcd.print("   Punch Out");
  lcd.setCursor(0, 1);
  lcd.print("  Parkinson's");

  lcd.setCursor(0, 1);
  adc_key_old = analogRead(analogPin);  // store the unpress key value

  on = true;
  //VISUAL ENDING

  //FUNCTIONAL BEGINNING
  // Seed the RNG from floating analog noise so random pad sequences differ each power-on.
  randomSeed(analogRead(0));

  strip.begin();
  strip.show();
  strip.setBrightness(8); // Low brightness (out of 255) — safe for indoor clinical use.


  while (!Serial) {  // needed to keep leonardo/micro from starting too fast!
    delay(10);
  }
  delay(1000);
  Serial.println("S.C.O.T.S. Bots Punch Out Parkinson's");

  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    //while (1);
    //while (!cap.begin(0x5A));
  }
  Serial.println("MPR121 found!");
  //FUNCTIONAL ENDING
}

// ── Main loop ─────────────────────────────────────────────────────────────────
/*
 * loop: reads the currently pressed LCD button each iteration and dispatches to
 * the appropriate menu action or game mode.
 *
 * Control flow summary:
 *   SELECT — launches whichever game mode seqOptions points to (1=Sequential, 2=Random,
 *             3=Return-to-menu). Does nothing if seqOptions == 0.
 *   RIGHT / LEFT — navigate the top-level menu (when no sub-menu is active) OR
 *                  adjust the timeout (when hitOptions == 1).
 *   UP / DOWN — scroll through the Sequence sub-menu when item_num == 1.
 */
void loop() {
  lcd_key = read_LCD_buttons();
  //Serial.println("Loop works");
  Serial.println(cap.touched());

  switch (lcd_key) {
    case lcdSELECT:
      {
        // Launch the game mode currently highlighted in the Sequence sub-menu.
        switch (seqOptions) {
          case 1:
            {
              ledSEQUENTIAL();
              showScore();
              break;
            }
          case 2:
            {
              ledRANDOM();
              showScore();
              break;
            }
          case 3:
            {
              // "Return To Menu" — reset seqOptions and redisplay the Sequences header.
              seqOptions = 0;
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("   Sequences");
              lcd.setCursor(0, 1);
              lcd.print("Down For Options");
            }
        }
        break;
      }

    case lcdRIGHT:
      {
        // Top-level navigation OR timeout increment, depending on active context.
        if (seqOptions == 0 && hitOptions == 0) {
          switcher(1);
        } else if (hitOptions == 1) {
            timeDelay(1);
        }
        break;
      }

    case lcdLEFT:
      {
        // Top-level navigation OR timeout decrement, depending on active context.
        if (seqOptions == 0 && hitOptions == 0) {
          switcher(-1);
        } else if (hitOptions == 1) {
            timeDelay(-1);
        }
        break;
      }

    case lcdUP:
      {
        // Scroll up through the Sequence sub-menu (only active on menu item 1).
        if (item_num == 1) {
          sequences(-1);
        }
        break;
      }

    case lcdDOWN:
      {
        // Scroll down through the Sequence sub-menu (only active on menu item 1).
        if (item_num == 1) {
          sequences(1);
        }
        break;
      }
  }
}
