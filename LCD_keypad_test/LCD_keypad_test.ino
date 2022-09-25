//************************************
// LCD 1602 w/KeyPad Test (28.01.16) *
// creator: Owintsowsky Maxim        *
//************************************
//#include <Wire.h>
#include <LiquidCrystal.h>

#define BackLightPin 10

//Key message
char msgs[5][14] = {"Right Key OK ", 
                    "Up Key OK    ", 
                    "Down Key OK  ", 
                    "Left Key OK  ", 
                    "Menu Key OK  " };
//int  adc_key_val[5] ={85, 219, 394, 600, 760 };  // 47, 132, 307, 482, 723
int  adc_key_val[5] ={93, 245, 380, 600, 700};  // 08, 98, 255, 409, 640
int NUM_KEYS = 5;
int adc_key_in;
int key=-1;
int oldkey=-1;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

void setup() {
  pinMode(13, OUTPUT);  //we'll use the debug LED to output a heartbeat
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.setCursor(0, 0);
  lcd.print("Hello my friend!");
  // initialize the LED as an output:
  delay(1300);
  digitalWrite(BackLightPin, LOW);
  lcd.setCursor(0, 1);
  lcd.print("LCD KeyPad Test ");
  delay(200);
  digitalWrite(BackLightPin, HIGH);
  delay(1500);
  lcd.setCursor(0, 0);
  lcd.print("LCD KeyPad Test ");
  lcd.setCursor(0, 1);
  lcd.print(" Press any key  ");
}

void loop() {
  adc_key_in = analogRead(0);    // read the value from the sensor  
  digitalWrite(13, HIGH);  
  key = get_key(adc_key_in);		        // convert into key press
	
  if (key != oldkey) {				    // if keypress is detected
    delay(50);		// wait for debounce time
    adc_key_in = analogRead(0);    // read the value from the sensor  
    key = get_key(adc_key_in);		        // convert into key press
    if (key != oldkey) {				
      oldkey = key;
      if (key >=0) {
        lcd.setCursor(0, 1);
      	lcd.print(msgs[key]);
        if (adc_key_in < 100) {
          lcd.print(" ");
        }
        lcd.print(adc_key_in);
      }
    }
  }
}

// Convert ADC value to key number
int get_key(unsigned int input) {
  int k;

  for (k = 0; k < NUM_KEYS; k++){
    if (input < adc_key_val[k]){
      return k;
    }
  }
  if (k >= NUM_KEYS)
    k = -1;     // No valid key pressed
  return k;
}

