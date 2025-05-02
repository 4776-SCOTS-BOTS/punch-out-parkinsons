#include <LiquidCrystal.h>
#include "separatedTouchback.h"

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int lcd_key = 0;
int adc_key_in = 0;
int adc_key_old;
int item_num;
int analogPin = A0;  //Define the A0 as analogPin as integer type.
bool selectPressed = false;

bool hasLCDButtons = false;

//External Button Pins (Digital)
int readSelect, readUp, readDown, readLeft, readRight;
int selectPin = A1;
int upPin = A2;
int downPin = A3;
int leftPin = A4;
int rightPin = A5;

//Menu Item Numbers
int seqOpt = 0;
int scoreOpt = 0;

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

  leftStrip.begin();
  leftStrip.setBrightness(4);
  leftStrip.fill(redL, 0, 660);
  leftStrip.show();
  delay(250);
  leftStrip.fill(blackL, 0, 660);
  leftStrip.show();
  leftStrip.clear();

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
  cap.setThresholds(6, 2);

  Serial.println("MPR121 found!");
  //FUNCTIONAL ENDING
}

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
