/*
 * PoP_Full.ino — Main sketch for Punch Out Parkinson's (most feature-complete version).
 *
 * Wires together the LCD shield menu system and external navigation buttons with the
 * game engine declared in Touchback.h. Key features over earlier sketches:
 *   - EEPROM persistence: high scores survive power cycles.
 *   - Adjustable hit window: "Time To Hit" menu item lets the therapist set delayTime
 *     from 1 to 60 seconds without reflashing.
 *   - Long-press score reset: hold the external Select button for 10 seconds to wipe
 *     both high scores; the strip flashes red as confirmation.
 *
 * Menu structure (navigated with Left/Right on the top level):
 *   1. Sequences  → Sequential | Random | Return To Menu
 *   2. High Score → Sequential score | Random score | Return To Menu
 *   3. Time To Hit (Select to enter edit mode, Up/Down to adjust, Select to confirm)
 */

#include <LiquidCrystal.h>
#include <EEPROM.h>
#include "Touchback.h"

// EEPROM byte addresses where the two high scores (unsigned long, 4 bytes each) are stored.
#define ADDR_SEQ_SCORE  0
#define ADDR_RAND_SCORE 4

// LCD shield uses digital pins 4–9; do not reassign those pins for other purposes.
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// --------------------------------------------------------------------------
// LCD shield built-in button state
// --------------------------------------------------------------------------

int lcd_key     = 0;
int adc_key_in  = 0;
int adc_key_old;
int item_num;
int analogPin = A0;  //Define the A0 as analogPin as integer type.

// hasLCDButtons guards the built-in button branch in loop(); set true if the
// shield's onboard buttons are in use (false by default — external buttons preferred).
bool hasLCDButtons = false;

// --------------------------------------------------------------------------
// External button pins (INPUT_PULLUP — LOW means pressed)
// --------------------------------------------------------------------------

//External Button Pins (Digital)
int readSelect, readUp, readDown, readLeft, readRight;
int selectPin = A1;
int upPin     = A2;
int downPin   = A3;
int leftPin   = A4;
int rightPin  = A5;

// --------------------------------------------------------------------------
// Long-press detection for high-score reset
// --------------------------------------------------------------------------

//Long press select button stuff
unsigned long pressStartTime     = 0;
unsigned long pressDuration      = 10000;  // Hold Select for 10 s to reset scores.
bool          hasLongPressOccurred = false;

// --------------------------------------------------------------------------
// Menu state variables
// --------------------------------------------------------------------------

// seqOpt, scoreOpt, touchOpt each track position within their respective sub-menus.
// All three being 0 means the user is at the top level of the main menu (see zero()).
//Menu Item Numbers
int seqOpt   = 0;
int scoreOpt = 0;
int touchOpt = 0;

// zero — returns true only when the user is at the top level of every menu dimension,
// used to gate Left/Right navigation so sub-menu state doesn't bleed into menu switching.
bool zero() {
  if (seqOpt == 0 && scoreOpt == 0 && touchOpt == 0) {
    return true;
  }
  return false;
}

// --------------------------------------------------------------------------
// Built-in LCD button constants (mapped from A0 resistor-ladder voltages)
// --------------------------------------------------------------------------

//Define values for the built in buttons as integers
#define lcdRIGHT  0
#define lcdUP     1
#define lcdDOWN   2
#define lcdLEFT   3
#define lcdSELECT 4
#define lcdNONE   5

/*
 * read_LCD_buttons — reads the resistor-ladder voltage on A0 and returns the
 * corresponding button constant. Each button shorts a different resistor combination,
 * producing a distinct ADC range. Values above 1000 mean no button is pressed.
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

  return lcdNONE;
}

// --------------------------------------------------------------------------
// EEPROM high-score persistence
// --------------------------------------------------------------------------

// saveHighScores — writes both high scores to EEPROM at their fixed byte addresses.
void saveHighScores() {
  Serial.println("saveHighScores triggered");
  EEPROM.put(ADDR_SEQ_SCORE,  highSeqScore);
  EEPROM.put(ADDR_RAND_SCORE, highRandScore);
}

// loadHighScores — reads both high scores from EEPROM on startup so they survive resets.
void loadHighScores() {
  EEPROM.get(ADDR_SEQ_SCORE,  highSeqScore);
  EEPROM.get(ADDR_RAND_SCORE, highRandScore);
}

// resetHighScores — zeroes both scores, saves to EEPROM, and flashes red as confirmation.
// Triggered by holding the external Select button for pressDuration (10 s).
void resetHighScores() {
  highSeqScore  = 0;
  highRandScore = 0;
  saveHighScores();

  strip.fill(red, 0, LED_COUNT);  // flash red as a visual indicator
  strip.show();
  delay(500);
  strip.clear();
  strip.show();
  delay(500);
  strip.clear();
  strip.show();
  delay(500);
  strip.clear();
  strip.show();
}

// --------------------------------------------------------------------------
// Post-game score display and high-score detection
// --------------------------------------------------------------------------

/*
 * showScore — prints the run score on the LCD second row, checks for a new high
 * score (mode-aware via seqOpt), triggers the high-score animation if needed,
 * saves to EEPROM, then resets scoreTot for the next run.
 *
 * seqOpt == 1 → sequential game just finished; seqOpt == 2 → random game just finished.
 */
void showScore() {
  lcd.setCursor(0, 1);
  lcd.print("Your Score: ");
  lcd.print(scoreTot);

  if (scoreTot > highSeqScore && seqOpt == 1) {
    Serial.println("sequential high score set");
    highSeqScore = scoreTot;
    saveHighScores();
    highScoreRotateCog(4000);
  }
  if (scoreTot > highRandScore && seqOpt == 2) {
    Serial.println("random high score set");
    highRandScore = scoreTot;
    Serial.println(highRandScore);
    saveHighScores();
    highScoreRotateCog(4000);
  }

  if (scoreTot > 0) {
    scoreTot = 0;
  }
}

// --------------------------------------------------------------------------
// Sub-menu navigation functions
// --------------------------------------------------------------------------

/*
 * sequences — navigates the Sequences sub-menu (seqOpt 1 = Sequential,
 * 2 = Random, 3 = Return To Menu). seqOpt wraps around; 0 is the top-level
 * header and is skipped during normal up/down navigation so the user must
 * use "Return To Menu" to exit rather than scrolling past the top.
 */
//Sequence Options
void sequences(int direction) {
  seqOpt = seqOpt + direction;
  seqOpt = constrain(seqOpt, 0, 4);

  //Keep the main menu item from being an option to up-button back to
    //Forces the Return To Menu item to be used
    if (seqOpt < 1) {
      seqOpt = 3;
    }

  //Loop menu back to first item from last item
  if (seqOpt > 3) {
    seqOpt = 1;
  }

  switch (seqOpt) {
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

/*
 * highScore — navigates the High Score sub-menu (scoreOpt 1 = sequential score,
 * 2 = random score, 3 = Return To Menu). Same wrap-and-skip logic as sequences().
 */
void highScore(int direction) {
  scoreOpt = scoreOpt + direction;
  scoreOpt = constrain(scoreOpt, 0, 4);

  //Keep the main menu item from being an option to up-button back to
    //Forces the Return To Menu item to be used
    if (scoreOpt < 1) {
      scoreOpt = 3;
    }

  //Loop menu back to first item from last item
  if (scoreOpt > 3) {
    scoreOpt = 1;
  }

  switch (scoreOpt) {
    case 0: {
      //scoreOpt = constrain(scoreOpt, 1, 4);
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

/*
 * touchTime — renders the current delayTime value on the LCD.
 * Called whenever delayTime changes (Up/Down while touchOpt == 1) and when
 * the user first enters edit mode. delayTime is clamped to 1–60 s before display.
 */
void touchTime() {
  touchOpt = constrain(touchOpt, 0, 1);
  delayTime = constrain(delayTime, 1000, 60000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Time To Hit");
  lcd.setCursor(0, 1);
  lcd.print("Delay: ");
  lcd.print(delayTime/1000);
  lcd.print(" sec");
}

/*
 * switcher — scrolls the top-level main menu left or right (item_num 1–3,
 * wrapping). Called only when zero() is true so sub-menu state cannot interfere.
 *   item_num 1 → Sequences
 *   item_num 2 → High Score
 *   item_num 3 → Time To Hit
 */
//Menu Switcher - Switch items on main menu
void switcher(int switchDirection) {
  lcd.clear();
  lcd.setCursor(0, 1);
  item_num = item_num + switchDirection;
  item_num = constrain(item_num, 0, 4);
  lcd.clear();
  //lcd.setCursor(16, 0);
  //lcd.print(item_num);
  delay(250);

  //Loop the menu
  if (item_num < 1) {
    item_num = 3;
  }
  if (item_num > 3) {
    item_num = 1;
  }

  switch (item_num) {
    case 1: {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("   Sequences");
        lcd.setCursor(0, 1);
        lcd.print("Down For Options");
        break;
      }
    case 2: {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("   High Score");
      lcd.setCursor(0, 1);
      lcd.print("Down For Options");
      break;
    }
    case 3: {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("  Time To Hit");
      lcd.setCursor(0, 1);
      lcd.print("Select To Change");
    }
  }
}

// --------------------------------------------------------------------------
// Arduino lifecycle
// --------------------------------------------------------------------------

void setup() {
  //VISUAL BEGINNING
  lcd.begin(16, 2);
  Serial.begin(9600);

  pinMode(10, OUTPUT);   // sets backlight pin-10 as PWM output

  // External Button Pins
  pinMode(selectPin, INPUT_PULLUP);
  pinMode(upPin,     INPUT_PULLUP);
  pinMode(downPin,   INPUT_PULLUP);
  pinMode(leftPin,   INPUT_PULLUP);
  pinMode(rightPin,  INPUT_PULLUP);

  analogWrite(10, 125);  // Set backlight to 50% brightness
  lcd.setCursor(0, 0);
  lcd.print("   Punch Out");
  lcd.setCursor(0, 1);
  lcd.print("  Parkinson's");

  lcd.setCursor(0, 1);
  adc_key_old = analogRead(analogPin);  // store the unpress key value

  //VISUAL ENDING

  //FUNCTIONAL BEGINNING

  // Seed the RNG from floating analog noise so random pad order differs each power-on.
  randomSeed(analogRead(0));

  strip.begin();
  strip.setBrightness(4);  // Low brightness (max 255) to avoid overdrawing USB power.
  strip.fill(red, 0, LED_COUNT);
  strip.show();
  delay(250);
  strip.fill(black, 0, LED_COUNT);
  strip.show();
  strip.clear();

  while (!Serial) {  // needed to keep leonardo/micro from starting too fast
    delay(10);
  }

  delay(1000);
  Serial.println("S.C.O.T.S. Bots Punch Out Parkinson's");

  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    //while (1);
    while (!cap.begin(0x5A)){}  // Block until the sensor is found rather than crashing.
  }

  //Load the previously saved high scores
  loadHighScores();

  // Touch threshold 10, release threshold 5. Higher values = less sensitive.
  // Tune these if pads trigger spuriously or miss touches in the target environment.
  //Sensitivity - higher number = lower sensitivity
  cap.setThresholds(10, 5);

  Serial.println("MPR121 found!");
  //FUNCTIONAL ENDING
}

/*
 * loop — main event loop, polled continuously.
 *
 * Two input sources are handled in order:
 *   1. Built-in LCD shield buttons (A0 resistor ladder) — gated by hasLCDButtons.
 *   2. External buttons (A1–A5, INPUT_PULLUP, LOW = pressed) — always active.
 *
 * Select: launches the highlighted game mode, confirms menu actions, or enters/exits
 *         Time To Hit edit mode. Long-press (10 s) resets high scores.
 * Up/Down: scroll sub-menus or adjust delayTime when in Time To Hit edit mode.
 * Left/Right: scroll the top-level main menu (only when zero() is true).
 */
void loop() {
  //LCD Buttons
  lcd_key = read_LCD_buttons();

  switch (lcd_key) {
    if (hasLCDButtons == true) {
      case lcdSELECT:
        {
          // Launch the game mode currently highlighted in the Sequences sub-menu,
          // or exit back to the Sequences header if "Return To Menu" is selected.
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

  // --------------------------------------------------------------------------
  // External button handling
  // --------------------------------------------------------------------------

  //External Buttons
  static bool lastButtonState = HIGH;
  readSelect = digitalRead(selectPin);
  if (readSelect == LOW) {
    delay(250);  // Debounce.

    // If a Sequences sub-option is highlighted, Select launches or exits it.
    switch (seqOpt) {
      case 1:
        {
          ledSEQUENTIAL();
          showScore();
          strip.clear();
          break;
        }
      case 2:
        {
          ledRANDOM();
          showScore();
          strip.clear();
          break;
        }
      case 3:
        {
          // "Return To Menu" selected — collapse back to the Sequences header.
          seqOpt = 0;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("   Sequences");
          lcd.setCursor(0, 1);
          lcd.print("Down For Options");
          break;
        }
    }

    // Collapse the High Score sub-menu back to its header when "Return To Menu" is confirmed.
    if (scoreOpt == 3) {
      scoreOpt = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("   High Score");
      lcd.setCursor(0, 1);
      lcd.print("Down For Options");
    }

    // Toggle Time To Hit edit mode: touchOpt 0 = display only, 1 = editable with Up/Down.
    if (item_num == 3) {
      switch(touchOpt) {
        case 0: {
          touchTime();
          touchOpt = 1;
          Serial.println("touchOpt:");
          Serial.println(touchOpt);
          break;
        }
        case 1: {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("  Time To Hit");
          lcd.setCursor(0, 1);
          lcd.print("Select To Change");
          touchOpt = 0;
          Serial.println("touchOpt:");
          Serial.println(touchOpt);
          break;
        }
      }
    }

    // Start timing the press for long-press detection (reset fires on release).
    if (lastButtonState == HIGH) {
      pressStartTime = millis(); //Record button press time
    }

    Serial.println("Ext. Select Pressed");
  }

  // On Select release: if the button was held for pressDuration, reset high scores.
  if (readSelect == HIGH && lastButtonState == LOW) {
    if (millis() - pressStartTime >= pressDuration) {
      resetHighScores();
    }
  }
  lastButtonState = readSelect;

  // Up: scroll sub-menus backward, or increment delayTime in Time To Hit edit mode.
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
      case 3: {
        if (touchOpt == 1) {
          delay(200);
          delayTime = delayTime + 1000;  // Increment by 1 second.
          touchTime();
        }
      }
    }
    Serial.println("Ext. Up Pressed");
  }

  // Down: scroll sub-menus forward, or decrement delayTime in Time To Hit edit mode.
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
      case 3: {
        if (touchOpt == 1) {
          delay(200);
          delayTime = delayTime - 1000;  // Decrement by 1 second.
          touchTime();
        }
      }
    }
    Serial.println("Ext. Down Pressed");
  }

  // Left/Right navigate the top-level menu; zero() prevents this when inside a sub-menu.
  readLeft = digitalRead(leftPin);
  if (readLeft == LOW) {
    if (zero()) {
      switcher(-1);
    }
    Serial.println("Ext. Left Pressed");
  }

  readRight = digitalRead(rightPin);
  if (readRight == LOW) {
    if (zero()) {
      switcher(1);
    }
    Serial.println("Ext. Right Pressed");
  }
}
