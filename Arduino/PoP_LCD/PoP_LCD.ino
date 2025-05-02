#include <LiquidCrystal.h>
#include "Touchback.h"

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int lcd_key = 0;
int adc_key_in = 0;
int adc_key_old;
int item_num;
int analogPin = A0;  //Define the A0 as analogPin as integer type.

bool on = false;

//Menu Item Numbers
int seqOptions = 0;
int hitOptions = 0;
int displayOptions = 0;
int mobileOptions = 0;

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

  Serial.println(adc_key_in);
  return lcdNONE;
}

void showScore() {
  lcd.setCursor(0, 1);
  lcd.print("Your Score: ");
  lcd.print(scoreTot);
  if (scoreTot > 0) {
    scoreTot = 0;
  }
}

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

int hitTimes(int direction) {
  //Adds or subtracts from the value of hitOptions, then multiplies that value by 1000 to get it in milliseconds
  hitOptions = (hitOptions + direction) * 1000;

  timeout = hitOptions;
  return timeout;
}

//Menu Switcher - Switch items on main menu
void switcher(int switchDirection) {
  lcd.clear();
  lcd.setCursor(0, 1);
  item_num = item_num + switchDirection;
  item_num = constrain(item_num, 1, 3);
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

    case 4:
      {
        lcd.setCursor(0, 0);
        lcd.print("    Mobility");
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
  randomSeed(analogRead(0));

  strip.begin();
  strip.show();
  strip.setBrightness(8);


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

void loop() {
  lcd_key = read_LCD_buttons();
  //Serial.println("Loop works");
  Serial.println(cap.touched());

  switch (lcd_key) {
    case lcdSELECT:
      {
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
        if (seqOptions == 0 && hitOptions == 0) {
          switcher(1);
        } else if (hitOptions == 1) {
            timeDelay(1);
        }
        break;
      }

    case lcdLEFT:
      {
        if (seqOptions == 0 && hitOptions == 0) {
          switcher(-1);
        } else if (hitOptions == 1) {
            timeDelay(-1);
        }
        break;
      }

    case lcdUP:
      {
        if (item_num == 1) {
          sequences(-1);
        }
        break;
      }

    case lcdDOWN:
      {
        if (item_num == 1) {
          sequences(1);
        }
        break;
      }
  }
}
