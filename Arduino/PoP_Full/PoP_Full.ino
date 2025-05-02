#include <LiquidCrystal.h>
#include <EEPROM.h>
#include "Touchback.h"

#define ADDR_SEQ_SCORE 0
#define ADDR_RAND_SCORE 4

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int lcd_key = 0;
int adc_key_in = 0;
int adc_key_old;
int item_num;
int analogPin = A0;  //Define the A0 as analogPin as integer type.

bool hasLCDButtons = false;

//External Button Pins (Digital)
int readSelect, readUp, readDown, readLeft, readRight;
int selectPin = A1;
int upPin = A2;
int downPin = A3;
int leftPin = A4;
int rightPin = A5;

//Long press select button stuff
unsigned long pressStartTime = 0;
unsigned long pressDuration = 10000;
bool hasLongPressOccurred = false;

//Menu Item Numbers
int seqOpt = 0;
int scoreOpt = 0;
int touchOpt = 0;

bool zero() {
  if (seqOpt == 0 && scoreOpt == 0 && touchOpt == 0) {
    return true;
  }
  return false;
}

//Define values for the built in buttons as integers
#define lcdRIGHT 0
#define lcdUP 1
#define lcdDOWN 2
#define lcdLEFT 3
#define lcdSELECT 4
#define lcdNONE 5

//Sets the value of adc_key_in to the value of the currently pressed key
int read_LCD_buttons() {
  adc_key_in = analogRead(0);

  if (adc_key_in > 1000) return lcdNONE;

  if (adc_key_in < 50) return lcdRIGHT;
  if (adc_key_in < 150) return lcdUP;
  if (adc_key_in < 300) return lcdDOWN;
  if (adc_key_in < 450) return lcdLEFT;
  if (adc_key_in < 700) return lcdSELECT;

  return lcdNONE;
}

void saveHighScores() {
  Serial.println("saveHighScores triggered");
  EEPROM.put(ADDR_SEQ_SCORE, highSeqScore);
  EEPROM.put(ADDR_RAND_SCORE, highRandScore);
}

void loadHighScores() {
  EEPROM.get(ADDR_SEQ_SCORE, highSeqScore);
  EEPROM.get(ADDR_RAND_SCORE, highRandScore);
}

void resetHighScores() {
  highSeqScore = 0;
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

  strip.begin();
  strip.setBrightness(4);
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
    while (!cap.begin(0x5A)){}
  }

  //Load the previously saved high scores
  loadHighScores();

  //Sensitivity - higher number = lower sensitivity
  cap.setThresholds(10, 5);

  Serial.println("MPR121 found!");
  //FUNCTIONAL ENDING
}

void loop() {
  //LCD Buttons
  lcd_key = read_LCD_buttons();

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

  //External Buttons
  static bool lastButtonState = HIGH;
  readSelect = digitalRead(selectPin);
  if (readSelect == LOW) {
    delay(250);
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
          seqOpt = 0;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("   Sequences");
          lcd.setCursor(0, 1);
          lcd.print("Down For Options");
          break;
        }
    }

    if (scoreOpt == 3) {
      scoreOpt = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("   High Score");
      lcd.setCursor(0, 1);
      lcd.print("Down For Options");
    }

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

    if (lastButtonState == HIGH) {
      pressStartTime = millis(); //Record button press time
    }

    Serial.println("Ext. Select Pressed");
  }
  if (readSelect == HIGH && lastButtonState == LOW) {
    if (millis() - pressStartTime >= pressDuration) {
      resetHighScores();
    }
  }
  lastButtonState = readSelect;

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
          delayTime = delayTime + 1000;
          touchTime();
        }
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
      case 3: {
        if (touchOpt == 1) {
          delay(200);
          delayTime = delayTime - 1000;
          touchTime();
        }
      }
    }
    Serial.println("Ext. Down Pressed");
  }

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
