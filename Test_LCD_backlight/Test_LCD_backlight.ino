#include <LiquidCrystal.h>

#define BackLightPin 10 // Pins for LCD backlight

LiquidCrystal lcd(8, 9, 4, 5, 6, 7); // for LCD 16*2 with 6*key

int  adc_key_val[5] ={92, 240, 380, 600, 820 };  // 08, 98, 255, 409, 640
uint8_t NUM_KEYS = 5; // Number of keys
int key=-1;
int oldkey=-1;

uint8_t BLset[2] = {255, 80};

void setup() {
  pinMode(BackLightPin, OUTPUT);
  digitalWrite(BackLightPin, HIGH);
  lcd.begin(16, 2);
  lcd.noCursor();
  lcd.setCursor(0, 0);
  lcd.print("Backlight change");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("demo. Now: 255");
}

void digit3Print(int j) {
  if (j<100) lcd.print("0");
  if (j<10) lcd.print("0");
  lcd.print(j);
}

// Convert ADC value to key number
int get_key(int input) {
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

void Read_Key() {
  int adc_key_in = analogRead(0);    // read the value from the sensor  
  key = get_key(adc_key_in);            // convert into key press
  
  if (key != oldkey) {            // if keypress is detected
    int newkey = key;
    delay(50);    // wait for debounce time
    adc_key_in = analogRead(0);    // read the value from the sensor  
    key = get_key(adc_key_in);            // convert into key press
    if (key == newkey) {
      oldkey = key;
      //if ((key >= 0) && KeyToneOn) tone(TonePin, NOTE_DS6, 15);
    } else {
      key = -1;
      //digitalWrite(TonePin, LOW);
    }
  } //else digitalWrite(TonePin, LOW);
}

void MenuSelect() {
  analogWrite(BackLightPin, 255);  
  lcd.setCursor(0, 0);
  lcd.print(" Day mode:      ");
  lcd.setCursor(0, 1);
  lcd.print(" Night mode:    ");
  lcd.setCursor(13, 0);
  digit3Print(BLset[0]);
  lcd.setCursor(13, 1);
  digit3Print(BLset[1]);
}

void BLDisp(uint8_t CursPos) {
  String MenuItem[2]= {" Day            ", " Ngt            "};
  analogWrite(BackLightPin, BLset[CursPos]);  
  for (uint8_t i=0;i<2;i++) {
    lcd.setCursor(0, i);
    lcd.print(MenuItem[i]);
  }
  lcd.setCursor(0, CursPos);
  lcd.write(62);
  for (uint8_t i=0;i<2;i++) {
    lcd.setCursor(5, i);
    for (int j=0; j<12; j++) {
      if (25*j+5<=BLset[i]) lcd.write(255);
      else lcd.write(219); 
    }
  }    
}

void loop() {
  for (int i=255; i>=0; i--) {
    if (key>=0) break;
    analogWrite(BackLightPin, i);
    lcd.setCursor(11, 1);
    digit3Print(i);
    Read_Key();
    delay(100);
  }
  for (int i=0; i<256; i++) {
    if (key>=0) break;
    analogWrite(BackLightPin, i);
    lcd.setCursor(11, 1);
    digit3Print(i);
    delay(100);
  }
  if (key>=0) {
    uint8_t LastItem = 0;
    BLDisp(LastItem);
    delay(1000);
    while (true) {
      Read_Key();
      switch( key ) {
        case 0: // Right
          if (BLset[LastItem] <= 230) {
            BLset[LastItem] += 25;
            BLDisp(LastItem);
          }
          break;
        case 1: // Up
          LastItem = 0;
          BLDisp(LastItem);
          break;
        case 2: // Down
          LastItem = 1;
          BLDisp(LastItem);
          break;
        case 3: // Left
          if (BLset[LastItem] >= 30) {
            BLset[LastItem] -= 25;
            BLDisp(LastItem);
          }
          break;
        case 4: // Menu
          MenuSelect();
          break;
      }
      delay(500);
    }
  }
}
