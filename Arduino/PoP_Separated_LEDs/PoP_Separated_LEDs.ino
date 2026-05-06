/*
 * PoP_Separated_LEDs.ino — Punch Out Parkinson's, Two-Strip Variant
 * S.C.O.T.S. Bots FRC Team
 *
 * WHAT THIS SKETCH IS
 * -------------------
 * This is the "Separated LEDs" architecture: the 660-LED chain is split
 * across two independent NeoPixel strips (left/right sides of the bag)
 * rather than one long chain. This is the recommended starting point for
 * scaling to a full competition bag, because you can control each side
 * independently without worrying about LED index offsets across 660 units.
 *
 * For hardware pinout, color constants, touch sensor config, and all game
 * logic, see separatedTouchback.h.
 *
 * GAME MODES
 * ----------
 *   Sequential: 4 pads × 2 passes = 8 rounds, fixed order. Max 800 pts.
 *   Random:     10 rounds, random pad each time (never same pad twice). Max 1000 pts.
 *
 * MENU SYSTEM
 * -----------
 * Two main menu items (item_num 1–2): Sequences and High Score.
 * Left/Right navigate between them. Down enters a sub-menu; Up/Down scroll
 * through options. Select launches the highlighted mode.
 *
 * Both the built-in LCD shield buttons (A0 resistor ladder) and five external
 * buttons (A1–A5, INPUT_PULLUP, LOW = pressed) drive the same menu logic.
 * hasLCDButtons gates the built-in button code — set it true if the shield's
 * buttons are physically present and working.
 *
 * THE selectPressed FLAG
 * ----------------------
 * seqOpt = 0 is the top-level "Sequences / Down For Options" header screen.
 * Once the user presses Down to enter the sub-menu, selectPressed is set true.
 * While it is true, sequences() clamps seqOpt to the range 1–3, preventing
 * the Up button from navigating back to seqOpt 0 (the header). Without this
 * clamp, pressing Up from option 1 could leave the menu in an inconsistent
 * state where the left/right buttons stop working.
 * selectPressed resets to false when the user chooses "Return To Menu" and
 * seqOpt is explicitly set back to 0.
 *
 * HIGH SCORES
 * -----------
 * Stored in RAM only (highSeqScore, highRandScore in separatedTouchback.h).
 * Lost on power cycle — no EEPROM write is implemented in this version.
 */

#include <LiquidCrystal.h>
#include "separatedTouchback.h"

// ── LCD shield setup ──────────────────────────────────────────────────────────
// Standard pin mapping for the common Arduino LCD keypad shield
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int lcd_key = 0;
int adc_key_in = 0;
int adc_key_old;
int item_num;
int analogPin = A0;  //Define the A0 as analogPin as integer type.

// Set true once the user enters a Sequences sub-menu (see file header for details)
bool selectPressed = false;

// Set true if the LCD shield's built-in buttons are physically connected and working
bool hasLCDButtons = false;

// ── External button pins (A1–A5, INPUT_PULLUP; LOW = pressed) ─────────────────
int readSelect, readUp, readDown, readLeft, readRight;
int selectPin = A1;
int upPin = A2;
int downPin = A3;
int leftPin = A4;
int rightPin = A5;

// ── Menu state ────────────────────────────────────────────────────────────────
int seqOpt = 0;    // Current Sequences sub-menu item (0 = header, 1–3 = options)
int scoreOpt = 0;  // Current High Score sub-menu item

// ── LCD shield button constants ───────────────────────────────────────────────
// The shield shares all five buttons on A0 through a resistor ladder.
// read_LCD_buttons() maps the ADC reading to these values.
#define lcdRIGHT  0
#define lcdUP     1
#define lcdDOWN   2
#define lcdLEFT   3
#define lcdSELECT 4
#define lcdNONE   5

// ── read_LCD_buttons ──────────────────────────────────────────────────────────
// Reads A0 and returns which shield button is pressed based on ADC thresholds.
// ADC thresholds are empirical — resistor ladder values vary slightly by board.
//Sets the value of adc_key_in to the value of the currently pressed key
int read_LCD_buttons() {
  adc_key_in = analogRead(0);

  if (adc_key_in > 1000) return lcdNONE;

  if (adc_key_in < 50)  return lcdRIGHT;
  if (adc_key_in < 150) return lcdUP;
  if (adc_key_in < 300) return lcdDOWN;
  if (adc_key_in < 450) return lcdLEFT;
  if (adc_key_in < 700) return lcdSELECT;

  return lcdNONE;
}

// ── showScore ─────────────────────────────────────────────────────────────────
// Displays the current run's total score on the LCD bottom row, then resets
// scoreTot. Also updates the mode high score if this run beat it.
void showScore() {
  lcd.setCursor(0, 1);
  lcd.print("Your Score: ");
  lcd.print(scoreTot);
  if (scoreTot > 0) {
    scoreTot = 0;
  }

  if (scoreTot > highSeqScore && seqOpt == 1) {
    highSeqScore = scoreTot;
    highScoreBlink();
  } else if (scoreTot > highRandScore && seqOpt == 2) {
    highRandScore = scoreTot;
    highScoreBlink();
  }
}

// ── sequences ─────────────────────────────────────────────────────────────────
// Handles navigation within the Sequences sub-menu.
// direction = +1 (Down) or -1 (Up).
//
// seqOpt values:
//   0 — top-level header ("Sequences / Down For Options")
//   1 — Sequential mode
//   2 — Random mode
//   3 — Return To Menu
//
// The selectPressed clamp prevents Up from reaching seqOpt 0 once the user
// is inside the sub-menu. Without it, reaching 0 via Up leaves left/right
// navigation broken until the board resets.
//Sequence Options
void sequences(int direction) {
  seqOpt = seqOpt + direction;

  //Keep the main menu item from being an option to up-button back to
    //Forces the Return To Menu item to be used
    //Bugs out slightly if up-buttoned back to, with left/right buttons not working
  if (selectPressed == true) {
    seqOpt = constrain(seqOpt, 1, 3);
  }

  //Loop menu back to first item from last item
  if (seqOpt > 3) {
    seqOpt = 1;
  }

  switch (seqOpt) {
    case 0: {
      if (item_num == 1) {
        selectPressed = false;
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

// ── highScore ─────────────────────────────────────────────────────────────────
// Handles navigation within the High Score sub-menu.
// Shows recorded high scores for Sequential (max 800) and Random (max 1000).
void highScore(int direction) {
  scoreOpt = scoreOpt + direction;
  switch (scoreOpt) {
    case 0: {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("   High Score");
      lcd.setCursor(0, 1);
      lcd.print("Down For Options");
      break;
    }
    case 1: {
      delay(250);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sequential = ");
      Serial.println(highSeqScore);
      lcd.print(highSeqScore);
      lcd.setCursor(0, 1);
      lcd.print("Max Score = 800");

      break;
    }
    case 2: {
      delay(250);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("  Random = ");
      Serial.println(highRandScore);
      lcd.print(highRandScore);
      lcd.setCursor(0, 1);
      lcd.print("Max Score = 1000");
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

// ── switcher ──────────────────────────────────────────────────────────────────
// Moves between top-level menu items (item_num 1 = Sequences, 2 = High Score).
// Only reachable when no sub-menu is open (seqOpt == 0 && scoreOpt == 0).
//Menu Switcher - Switch items on main menu
void switcher(int switchDirection) {
  lcd.clear();
  lcd.setCursor(0, 1);
  item_num = item_num + switchDirection;
  item_num = constrain(item_num, 1, 2);
  lcd.clear();
  //lcd.setCursor(16, 0);
  //lcd.print(item_num);
  delay(250);

  switch (item_num) {
    case 1:
      {
        lcd.setCursor(0, 0);
        lcd.print("   Sequences");
        lcd.setCursor(0, 1);
        lcd.print("Down For Options");

        break;
      }
    case 2: {
      lcd.setCursor(0, 0);
      lcd.print("   High Score");
      lcd.setCursor(0, 1);
      lcd.print("Down For Options");

      break;
    }
  }
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
  //VISUAL BEGINNING
  lcd.begin(16, 2);
  Serial.begin(9600);

  pinMode(10, OUTPUT);   // sets backlight pin-10 as PWM output

  // External Button Pins
  pinMode(selectPin, INPUT_PULLUP);
  pinMode(upPin, INPUT_PULLUP);
  pinMode(downPin, INPUT_PULLUP);
  pinMode(leftPin, INPUT_PULLUP);
  pinMode(rightPin, INPUT_PULLUP);

  analogWrite(10, 125);  // Set backlight to 50% brightness
  lcd.setCursor(0, 0);
  lcd.print("   Punch Out");
  lcd.setCursor(0, 1);
  lcd.print("  Parkinson's");

  lcd.setCursor(0, 1);
  adc_key_old = analogRead(analogPin);  // store the unpress key value

  //VISUAL ENDING

  //FUNCTIONAL BEGINNING
  randomSeed(analogRead(0));

  // Initialize left strip: brief red flash to confirm wiring, then clear
  leftStrip.begin();
  leftStrip.setBrightness(4);
  leftStrip.fill(redL, 0, 660);
  leftStrip.show();
  delay(250);
  leftStrip.fill(blackL, 0, 660);
  leftStrip.show();
  leftStrip.clear();

  // Initialize right strip: same startup sequence
  rightStrip.begin();
  rightStrip.setBrightness(4);
  rightStrip.fill(redR, 0, 660);
  rightStrip.show();
  delay(250);
  rightStrip.fill(blackR, 0, 660);
  rightStrip.show();
  rightStrip.clear();

  while (!Serial) {  // needed to keep leonardo/micro from starting too fast
    delay(10);
  }

  delay(1000);
  Serial.println("S.C.O.T.S. Bots Punch Out Parkinson's");

  // Initialize MPR121 touch controller at default I2C address 0x5A.
  // Alternative addresses: 0x5B (3.3V), 0x5C (SDA), 0x5D (SCL)
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    //while (1);
    while (!cap.begin(0x5A)){}
  }

  //Sensitivity
  // Threshold Number Logic:
    // Higher touch number = lower sensitivity
  cap.setThresholds(6, 2);  // (touch, release) — more sensitive than PoP_Full (10, 5)

  Serial.println("MPR121 found!");
  //FUNCTIONAL ENDING
}

// ── loop ──────────────────────────────────────────────────────────────────────
// Polls both the LCD shield buttons and the five external buttons each cycle.
// hasLCDButtons gates the built-in button block — set it true if the shield
// buttons are physically present.
// External buttons always active; select sets selectPressed before dispatching.
void loop() {
  //LCD Buttons
  lcd_key = read_LCD_buttons();
  // Serial.print("Down  ");
  // Serial.println(readDown);
  // Serial.print("Up  ");
  // Serial.println(readUp);
  // Serial.print("Left  ");
  // Serial.println(readLeft);
  // Serial.print("Right  ");
  // Serial.println(readRight);
  // Serial.print("Select  ");
  // Serial.println(readSelect);

  //Serial.println(cap.touched());

  switch (lcd_key) {
    if (hasLCDButtons == true) {
      case lcdSELECT:
        {
          switch (seqOpt) {
            case 1:
              {
                lcd.clear();
                delay(125);
                lcd.print("   Sequential");
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
                // "Return To Menu" chosen — reset sub-menu state
                seqOpt = 0;
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
          if (seqOpt == 0) {
            switcher(1);
          }
          break;
        }

      case lcdLEFT:
        {
          if (seqOpt == 0) {
            switcher(-1);
          }
          break;
        }

      case lcdUP:
        {
          if (item_num == 1) {
            sequences(-1);
          }
          if (item_num == 2) {
            highScore(-1);
          }
          break;
        }

      case lcdDOWN:
        {
          if (item_num == 1) {
            sequences(1);
          }
          if (item_num == 2) {
            highScore(1);
          }
          break;
        }
    }
  }

  // ── External buttons ───────────────────────────────────────────────────────
  // All five pins are INPUT_PULLUP; LOW means pressed.
  // Select sets selectPressed so sequences() knows the user is inside the sub-menu.

  readSelect = digitalRead(selectPin);
  if (readSelect == LOW) {
    selectPressed = true;
    switch (seqOpt) {
          case 1:
            {
              ledSEQUENTIAL();
              showScore();
              leftStrip.clear();
              rightStrip.clear();
              break;
            }
          case 2:
            {
              ledRANDOM();
              showScore();
              leftStrip.clear();
              rightStrip.clear();
              break;
            }
          case 3:
            {
              // "Return To Menu" chosen — reset sub-menu state
              seqOpt = 0;
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("   Sequences");
              lcd.setCursor(0, 1);
              lcd.print("Down For Options");
              break;
            }
    }
    Serial.println("Ext. Select Pressed");
  }

  readUp = digitalRead(upPin);
  if (readUp == LOW) {
    switch (item_num) {
      case 1: {
        sequences(-1);
        break;
      }
      case 2: {
        highScore(-1);
        break;
      }
    }
    Serial.println("Ext. Up Pressed");
  }

  readDown = digitalRead(downPin);
  if (readDown == LOW) {
    switch (item_num) {
      case 1: {
        sequences(1);
        break;
      }
      case 2: {
        highScore(1);
        break;
      }
    }
    Serial.println("Ext. Down Pressed");
  }

  readLeft = digitalRead(leftPin);
  if (readLeft == LOW) {
    if (seqOpt == 0 && scoreOpt == 0) {
      switcher(-1);
    }
    Serial.println("Ext. Left Pressed");
  }

  readRight = digitalRead(rightPin);
  if (readRight == LOW) {
    if (seqOpt == 0 && scoreOpt == 0) {
      switcher(1);
    }
    Serial.println("Ext. Right Pressed");
  }
}
