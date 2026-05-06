/*
 * PoP_Temp_Presentation.ino — Punch Out Parkinson's (Presentation Version)
 *
 * This is the simplest and earliest version of the PoP sketch. It was built
 * for live demonstrations and presentations, so the interface is intentionally
 * minimal. If you are new to this project, start here before reading more
 * complex versions.
 *
 * Key characteristics of this version:
 *   - Flat menu: LEFT/RIGHT on the LCD shield cycles between Sequential and
 *     Random modes only. No submenus, no settings screens.
 *   - LCD shield buttons only: no external wired buttons.
 *   - No high score tracking: scoreTot is reset to 0 at the start of every game.
 *   - All pad logic is inline in ledSEQUENTIAL() / ledRANDOM() (Touchback.h).
 *
 * selectPressed flag:
 *   Set to true when SELECT is pressed (game starts). While true, LEFT and
 *   RIGHT button presses are ignored, preventing the player from switching
 *   modes mid-game. It is never reset to false in this version — once a game
 *   has been played, the mode is locked for the rest of the session. This is
 *   intentional for demo use (operator picks the mode once, then hands the
 *   device to patients).
 *
 * Hardware:
 *   LCD shield — 16×2 display. All five buttons share analog pin A0 via a
 *   resistor ladder; read_LCD_buttons() converts the ADC reading to a button ID.
 *   Pin 10 controls LCD backlight via PWM.
 *   NeoPixel strip, MPR121 sensor — see Touchback.h for details.
 */

#include <LiquidCrystal.h>
#include "Touchback.h"

/* --------------------------------------------------------------------------
 * LCD shield pin assignment
 * RS=8, Enable=9, D4=4, D5=5, D6=6, D7=7 — standard DFRobot/Adafruit shield.
 * -------------------------------------------------------------------------- */
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

/* --------------------------------------------------------------------------
 * Main Variables
 * -------------------------------------------------------------------------- */
int lcd_key     = 0;
int adc_key_in  = 0;
int adc_key_old;
int item_num;                // Currently selected menu item (1=Sequential, 2=Random)
bool selectPressed = false;  // True once a game has started; locks LEFT/RIGHT navigation
int seqOptions     = 1;
int display_options;
int analogPin = A0;  //Define the A0 as analogPin as integer type.

/* --------------------------------------------------------------------------
 * Button ID constants
 * The LCD shield resistor ladder produces distinct ADC ranges for each button.
 * These constants are used in read_LCD_buttons() and the loop() switch.
 * -------------------------------------------------------------------------- */
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

/* --------------------------------------------------------------------------
 * read_LCD_buttons()
 * Reads analog pin A0 and maps the voltage to a button ID.
 * Each button pulls A0 to a different voltage via a resistor ladder, so the
 * ADC value falls in a predictable range per button. Returns btnNONE when
 * no button is pressed (ADC > 1000).
 * -------------------------------------------------------------------------- */
//Sets the value of adc_key_in to the value of the currently pressed key
int read_LCD_buttons() {
  adc_key_in = analogRead(0);

  if (adc_key_in > 1000) return btnNONE;

  if (adc_key_in < 50)  return btnRIGHT;
  if (adc_key_in < 150) return btnUP;
  if (adc_key_in < 300) return btnDOWN;
  if (adc_key_in < 450) return btnLEFT;
  if (adc_key_in < 700) return btnSELECT;

  return btnNONE;
}

/* --------------------------------------------------------------------------
 * switcher()
 * Advances the menu selection by switchDirection (+1 = right, -1 = left).
 * item_num is clamped to [1, 2] so it can't go out of range.
 * Redraws the LCD with the name of the newly selected mode.
 * The 250 ms delay debounces the button.
 * -------------------------------------------------------------------------- */
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

/* --------------------------------------------------------------------------
 * setup()
 * Initialises the LCD, serial port, backlight, NeoPixel strip, and MPR121.
 * randomSeed(analogRead(0)) uses floating analog noise for non-repeating
 * random sequences in ledRANDOM().
 * -------------------------------------------------------------------------- */
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

/* --------------------------------------------------------------------------
 * loop()
 * Reads the LCD buttons on every iteration.
 *
 *   SELECT — starts the selected game mode. Resets scoreTot to 0, runs the
 *            game function (blocking), then displays the final score on row 1.
 *            Also sets selectPressed = true to lock mode switching.
 *
 *   RIGHT/LEFT — calls switcher() to change the displayed mode, but only if
 *                selectPressed is false (i.e., no game has been played yet).
 * -------------------------------------------------------------------------- */
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
