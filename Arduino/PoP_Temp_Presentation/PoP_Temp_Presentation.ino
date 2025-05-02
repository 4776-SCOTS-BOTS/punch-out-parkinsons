#include <LiquidCrystal.h>
#include "Touchback.h"

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

/*Main Variables*/
int lcd_key = 0;
int adc_key_in = 0;
int adc_key_old;
int item_num;
bool selectPressed = false;
int seqOptions = 1;
int display_options;
int analogPin = A0;  //Define the A0 as analogPin as integer type.

/*Define values for the buttons as integers*/
#define btnRIGHT 0
#define btnUP 1
#define btnDOWN 2
#define btnLEFT 3
#define btnSELECT 4
#define btnNONE 5

//Sets the value of adc_key_in to the value of the currently pressed key
int read_LCD_buttons() {
  adc_key_in = analogRead(0);

  if (adc_key_in > 1000) return btnNONE;

  if (adc_key_in < 50) return btnRIGHT;
  if (adc_key_in < 150) return btnUP;
  if (adc_key_in < 300) return btnDOWN;
  if (adc_key_in < 450) return btnLEFT;
  if (adc_key_in < 700) return btnSELECT;

  return btnNONE;
}

//Menu Switcher - Switch items
int switcher(int switchDirection) {
  lcd.clear();
  lcd.setCursor(0, 1);
  item_num = item_num + switchDirection;
  item_num = constrain(item_num, 1, 2);
  lcd.clear();
  lcd.setCursor(0, 1);
  //lcd.print(item_num);
  delay(250);

  if (item_num == 1) {
    lcd.setCursor(0, 0);
    lcd.print("   Sequential");
  } else if (item_num == 2) {
    lcd.setCursor(0, 0);
    lcd.print("     Random");
  }
}

void setup() {
  //VISUAL BEGINNING
  lcd.begin(16, 2);
  Serial.begin(9600);
  pinMode(10, OUTPUT);   // sets backlight pin-10 as PWM output
  analogWrite(10, 125);  // Set backlight to 50% brightness
  lcd.setCursor(0, 0);
  lcd.print("PoP");
  lcd.setCursor(0, 1);
  lcd.print(" Presenter Mode");

  lcd.setCursor(0, 1);
  adc_key_old = analogRead(analogPin);  // store the unpress key value
  //VISUAL ENDING
  //FUNCTIONAL BEGINNING
  randomSeed(analogRead(0));

  strip.begin();
  strip.show();
  strip.setBrightness(8);

  while (!Serial) {  // needed to keep leonardo/micro from starting too fast!
    delay(10);
  }

  Serial.println("S.C.O.T.S. Bots Punch Out Parkinsons (Sequence 1)");

  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    //while (1);
  }
  Serial.println("MPR121 found!");
  //FUNCTIONAL ENDING
}

void loop() {
  //VISUAL BEGINNING
  lcd_key = read_LCD_buttons();

  switch (lcd_key) {
    case btnSELECT:
      {
        selectPressed = true;

        if (item_num == 1) {
          scoreTot = 0;
          ledSEQUENTIAL();
          lcd.setCursor(0, 1);
          lcd.print("Your Score: ");
          lcd.print(scoreTot);
        }
        if (item_num == 2) {
          scoreTot = 0;
          ledRANDOM();
          lcd.setCursor(0, 1);
          lcd.print("Your Score: ");
          lcd.print(scoreTot);
        }
        break;
      }

    case btnRIGHT:
      {
        if (selectPressed == false) {
          switcher(1);
        }
        break;
      }

    case btnLEFT:
      {
        if (selectPressed == false) {
          switcher(-1);
        }
        break;
      }
  }
}
