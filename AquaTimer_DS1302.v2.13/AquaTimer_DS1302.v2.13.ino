//********************************************************
// 5-channel timer for Aquarium. Version 2.13 (15.09.17) *
// creator: Owintsowsky Maxim     17 248 / 17 338 (1137) *
// VK group: https://vk.com/automation4house             *
// Telegram chat: https://t.me/aquatimer                 *
//********************************************************

#include <LiquidCrystal.h>
#include <EEPROM.h> 
#include <DS1302.h>
#include "pitches.h"

#define VibroPin 15 //Pins for feeding vibro-motor A1
#define TonePin 18 //Pins for pieze A4
#define BackLightPin 10 // Pins for LCD backlight

//#define TEST_MODE // State for test mode

LiquidCrystal lcd(8, 9, 4, 5, 6, 7); // for LCD 16*2 with 6 key

const char TVer[] = {"2.13"};
const char chDesc[4][3] = {": ", ":", "-", ":"};
const uint8_t RotaryChar[4] = {0x2D, 3, 0x7C, 0x2F}; // Rotary symbol codes

const char dn[7][4] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
//const char mn[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dec"};

boolean BeepOnOff = true; // Switch on/off beeper every hour
boolean KeyToneOn = true; // Switch on/off keys tone

int  adc_key_val[5] ={92, 240, 380, 600, 820 };  // 08, 98, 255, 409, 640
uint8_t NUM_KEYS = 5; // Number of keys
int key=-1;
int oldkey=-1;
uint8_t TimersNum = 18; // Number of timers
uint8_t MaxNumChannels = 5; //Max number of channels
uint8_t ChannelsNum = 4; // Number of relay channels                          Screen:      0123456789ABCDEF 
uint8_t ChPin[] = {11, 13, 16, 17, 19}; // Pins for channels D11, D13, A2, A3, A5
unsigned int FeedTime = 1181; // Start time vibro
uint8_t FeedDelay = 3; // Time for vibro * 100mS
boolean FeedOK = false; // Feeding held
uint8_t ChOnOff[6]; // Buffer for timers
// My symbols
uint8_t triang[8]  = {0x0,0x8,0xc,0xe,0xc,0x8,0x0};
uint8_t cosaya[8] = {0x0,0x10,0x8,0x4,0x2,0x1,0x0}; 
uint8_t mybell[8]  = {0x4,0xe,0xe,0xe,0x1f,0x0,0x4};
uint8_t nobell[8]  = {0x5,0xf,0xe,0xe,0x1f,0x8,0x10};
uint8_t check[8] = {0x0,0x1,0x3,0x16,0x1c,0x8,0x0};
uint8_t cross[8] = {0x0,0x1b,0xe,0x4,0xe,0x1b,0x0}; // x
uint8_t smboff[8] = {0x0,0x0,0xe,0xa,0xe,0x0,0x0};  // o
uint8_t setpic = 2; // Set-symbol number on table
uint8_t oldsec = 65; // Previous second value
int8_t TimeAdj = 0; // Time correction value

uint8_t TmrAddr = 16; // Timers start address in EEPROM (0xAA, 0x55, TimersNum, ChannelsNum, TimeEatHour, TimeEatMin, EatDelay, 1-RelayUp, HourBeep, KeyTone, TimeAdj, 0, 0, 0, 0, 0)
uint8_t daysofweek[] = {127, 31, 96, 1, 2, 4, 8, 16, 32, 64, 0}; // States in the week
uint8_t WeekStateNum; // Total state options in the week
uint8_t RotaryNum = 0; 
uint8_t RotaryMaxNum = 3;

uint8_t RelayUp = HIGH; // Relay type

// Hour beep tones
//int ToneBoy[] = {NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6};
//int ToneBoy[] = {NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5, NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6};
int ToneBoy[] = {NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6, NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7};
int TimeNow;
boolean BeepNow = false;

boolean Ch1NeedOn = false;
boolean Ch1NeedOff = false;
boolean Ch1OnOff = false;
boolean Ch2NeedOn = false;
boolean Ch2NeedOff = false;
boolean Ch2OnOff = false;
// variables for backlight
uint8_t BLset[2] = {255, 55}; // 255 - day mode, 55 - night mode
boolean BLNeedOn = false;
boolean BLNeedOff = false;
uint8_t BLNightState = 0; // 0 - Set day mode, 1 - Set night mode

#ifndef TEST_MODE
// Init the DS1302
DS1302 rtc(2, 3, 12);
// DS1302:  CE pin    -> Arduino Digital 2
//          I/O pin   -> Arduino Digital 3
//          SCLK pin  -> Arduino Digital 12

// Init a Time-data structure
Time t;
#endif

void setup() {
  for (uint8_t ij = 0 ; ij < MaxNumChannels ; ij++) {
    pinMode(ChPin[ij], OUTPUT);
    digitalWrite(ChPin[ij], LOW);
  }
  pinMode(VibroPin, OUTPUT);
  digitalWrite(VibroPin, LOW);
  pinMode(TonePin, OUTPUT);
  digitalWrite(TonePin, LOW);

  WeekStateNum = sizeof(daysofweek); // Number of states in the week
  
#ifndef TEST_MODE
  // Set the clock to run-mode, and disable the write protection
  rtc.halt(false);
  rtc.writeProtect(false);
#endif

  lcd.begin(16, 2);
  // Backlight On
  analogWrite(BackLightPin, 255);
  lcd.createChar(1, nobell);
  lcd.createChar(2, triang);
  lcd.createChar(3, cosaya);
  lcd.createChar(4, mybell);
  lcd.createChar(5, check);
  lcd.createChar(6, cross);
  lcd.createChar(7, smboff);
  lcd.home();
  lcd.noCursor();
  ReadWriteEEPROM();
  delay(2500);
  ShowFeedingTime();
  delay(2500);
}

void ShowChannels() {
  lcd.setCursor(0, 0);
  lcd.print("Out-channels hex");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  for (uint8_t ij = 0 ; ij < MaxNumChannels ; ij++) {
    if (ChPin[ij] < 10) lcd.write(48 + ChPin[ij]);
    else {
      if (ChPin[ij] > 15) {
        lcd.write(49);
        lcd.write(32 + ChPin[ij]);
      } else lcd.write(ChPin[ij]+55);
    }
    if ((ij+1) < MaxNumChannels) lcd.write(32);
    else if (((ij+1) == MaxNumChannels) && (ChannelsNum < MaxNumChannels)) lcd.write(41);
    if (((ij+1) == ChannelsNum) && (ChannelsNum < MaxNumChannels)) 
      lcd.write(40);
  }
}

void ShowFeedingTime() {
    lcd.setCursor(0, 0);
    lcd.print("Feed time: ");
    lcd.print(lid1Zero(EEPROM.read(4)));
    lcd.write(58); // :
    lcd.print(lid1Zero(EEPROM.read(5)));
    lcd.setCursor(0, 1);
    lcd.write(4);
    lcd.print(" duration");
    if (FeedDelay < 100) lcd.write(32);
    lcd.print(FeedDelay/10);
    lcd.write(46); // .
    lcd.print(FeedDelay%10);
    lcd.print(" S");
}

String lid1Zero(uint8_t val) {
  if (val<10) return "0" + String(val);
  else return String(val);
}

// Written massive to EEPROM from adderess <addr> and size of <dtlng>
void eeWrite(uint8_t val[], unsigned int addr, unsigned int dtlng) {
  for(uint8_t i=0; i<dtlng; i++) {
    EEPROM.write(i+addr, val[i]);
  }
}

void EEwritedef() { // Written default value to EEPROM for all timers
  //                             0     1     2          3            4            5           6         7        8         8        10       11 12 13 14 15
  // default vilues for EEPROM: (0xAA, 0x55, TimersNum, ChannelsNum, TimeEatHour, TimeEatMin, EatDelay, RelayUp, HourBeep, KeyBeep, TimeAdj, BackLightDay, BackLightNight, 0, 0, 0, Timers defult setting)
  //                      0     1     2   3  4   5   6  7  8  9  10 11   12  13 14 15 |<--    Timer1    -->|<--      Timer2     -->|<--    Timer3    -->|<--   Timer4   -->|
  uint8_t ChOnOffDef[] = {0xAA, 0x55, 18, 4, 19, 41, 3, 0, 1, 1, 0, 255, 55, 0, 0, 0, 31, 0, 7, 20, 21, 0, 127, 1, 16, 40, 19, 20, 96, 0, 9, 30, 21, 0, 42, 2, 7, 55, 21, 55};
  ChOnOffDef[2] = TimersNum;
  ChOnOffDef[3] = ChannelsNum;
  ChOnOffDef[8] = BeepOnOff;
  ChOnOffDef[7] = KeyToneOn;
  ChOnOffDef[10] = TimeAdj;
  eeWrite(ChOnOffDef, 0, sizeof(ChOnOffDef));
  uint8_t ChOnOffNull[6] = {85, 4, 22, 0, 23, 0};
  for (uint8_t i = 4;i < TimersNum;i++) { // write other timers to EEPROM
#ifdef TEST_MODE
    ChOnOffNull[0] = daysofweek[i-2]; // Delete it!!! Only for demo.
    if (i>4) {
      ChOnOffNull[2] = i-4;           // Delete it!!! Only for demo.
      ChOnOffNull[4] = i-3;           // Delete it!!! Only for demo.
    }
#endif
    eeWrite(ChOnOffNull, TmrAddr + i*6, sizeof(ChOnOffNull));
  }
}

void EEreadTimer(uint8_t NumTmr) { // read desired timer
  for (uint8_t i=0; i<6 ; i++) {
    ChOnOff[i] = EEPROM.read(TmrAddr + 6*NumTmr + i);
  }
}

void ReadWriteEEPROM() { // check the contents EEPROM
  boolean eeOK = false;
  uint8_t eeValue = EEPROM.read(0);
  if (eeValue == 0xAA) {
    eeValue = EEPROM.read(1);
    if (eeValue == 0x55) {
      eeValue = EEPROM.read(2);
      if (eeValue == TimersNum) {
        eeOK = true;  // Change to 'true' !!!
      }
    }
  }
  lcd.setCursor(0, 0);
  if (eeOK) {
    RelayUp = HIGH - EEPROM.read(7); //Inverted relay
    FeedTime = EEPROM.read(4)*60 + EEPROM.read(5);
    FeedDelay = EEPROM.read(6);
    ChannelsNum = EEPROM.read(3);
    BeepOnOff = EEPROM.read(8);
    KeyToneOn = EEPROM.read(9);
    TimeAdj = EEPROM.read(10);
    if (EEPROM.read(11) > 0) {
      BLset[0] = EEPROM.read(11);
      BLset[1] = EEPROM.read(12);
    }
    lcd.setCursor(0, 0);
    lcd.print(lid1Zero(TimersNum));
    lcd.print(" on/off timers");
    lcd.setCursor(0, 1);
    lcd.print("for ");
    lcd.print(ChannelsNum);
    lcd.write(47);
    lcd.print(MaxNumChannels);
    lcd.print(" channels");
    tone(TonePin, NOTE_A6, 200);
    delay(220);
    tone(TonePin, NOTE_E7, 100);
    delay(1780);
    lcd.setCursor(0, 0);
    lcd.print("Version  ");
    lcd.print(TVer);
    lcd.print(" by");
    lcd.setCursor(0, 1);
    lcd.print("  vk.cc/6Z2GQ7  "); // link to https://vk.cc/6Z2GQ7
    delay(1500);
  } else {
    EEwritedef();
    lcd.print("Written time for");
    lcd.setCursor(0, 1);
    lcd.print(ChannelsNum);
    lcd.print(" channel EEPROM");
    delay(2500);
    ShowChannels();
    tone(TonePin, NOTE_E7, 200);
    delay(220);
    tone(TonePin, NOTE_A6, 100);
    delay(2000);
  }
}

void HourBeep(int CurTime) { // makes a beep each hour
  if (BeepOnOff) {
    if (CurTime % 60 == 0) {
      if (!BeepNow) { // if not peak yet
        CurTime = CurTime / 60;
        CurTime = CurTime % 12;
        tone(TonePin, ToneBoy[CurTime], 31);
        BeepNow = true;
        delay(35);
        digitalWrite(TonePin, LOW);
      }
    } else BeepNow = false;
  }
}

int get_key(int input) { // Convert ADC value to key number
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
      if ((key >= 0) && KeyToneOn) tone(TonePin, NOTE_DS6, 15);
    } else {
      key = -1;
      digitalWrite(TonePin, LOW);
    }
  } else digitalWrite(TonePin, LOW);
}

void TimerDisp() {
  lcd.setCursor(2, 0);
  uint8_t kk = 1;
  for(uint8_t j=0;j<7;j++) {
    lcd.write(32);
    if (kk & ChOnOff[0]) lcd.print(j+1);
    else lcd.write(7); // smboff
    kk <<= 1;
  }
  lcd.setCursor(0, 1);
  lcd.print("Ch");
  lcd.print(ChOnOff[1]+1);
  for(uint8_t j=0;j<4;j++) {
    lcd.print(chDesc[j]);
    lcd.print(lid1Zero(ChOnOff[j+2]));
  }
}

void ShowTimer(uint8_t i) {
  lcd.setCursor(0, 0);
  lcd.print(lid1Zero(i+1));
  EEreadTimer(i);
  TimerDisp();
  lcd.setCursor(1, 0);
  lcd.blink();
}

void TimerOnOffDisp(uint8_t ChState) {
  uint8_t kk = 1;
  if (Ch1NeedOn) {
    if (ChState & 1) Ch1NeedOn = false;
    else ChState |= 1;
  }
  if (Ch1NeedOff) {
    if (ChState & 1) ChState &= 14;
    else Ch1NeedOff = false;
  }
  BLNightState = !(ChState & 1);
  if (BLNeedOn) {
    if (ChState & 1) BLNeedOn = false;
    else BLNightState = 0;
  }
  if (BLNeedOff) {
    if (ChState & 1) BLNightState = 1;
    else BLNeedOff = false;
  }
  if (Ch2NeedOn) {
    if (ChState & 2) Ch2NeedOn = false;
    else ChState |= 2;
  }
  if (Ch2NeedOff) {
    if (ChState & 2) ChState &= 13;
    else Ch2NeedOff = false;
  }
  for(uint8_t i=0;i<MaxNumChannels;i++) {
    if (i < ChannelsNum) { // Only working channels
      if (kk & ChState) {
        if (i==1) { // Pump channel
          lcd.write(RotaryChar[RotaryNum]);
          RotaryNum++;
          if (RotaryNum > RotaryMaxNum) RotaryNum = 0;
        } else {
          lcd.write(223); // Zaboy
        }
        digitalWrite(ChPin[i], RelayUp);
        if (i==0) {
          Ch1OnOff = true;
        }
        if (i==1) {
          Ch2OnOff = true;
        }
      } else {
        lcd.write(161);
        digitalWrite(ChPin[i], HIGH-RelayUp);
        if (i==0) {
          Ch1OnOff = false;
        }
        if (i==1) {
          Ch2OnOff = false;
        }
      }
    } else { // not working channels
      lcd.write(6); // Cross
    }
    kk <<= 1;
  }
  for(uint8_t i=MaxNumChannels;i<6;i++) lcd.write(32); // for five and less channel configuration
  if (BeepOnOff) lcd.write(4); // Bell
  else lcd.write(1); // noBell
  analogWrite(BackLightPin, BLset[BLNightState]);
}

uint8_t StateChannels(int CurTime, uint8_t DayOfWeek) {
  uint8_t ChState = 0;
  uint8_t bitDay = 1;
  if (DayOfWeek > 1) bitDay <<= (DayOfWeek-1);
  for (uint8_t i=0 ; i<TimersNum ; i++) {
    EEreadTimer(i);
    if (ChOnOff[0] & bitDay) {
      if((CurTime >= (ChOnOff[2]*60+ChOnOff[3])) && (CurTime < (ChOnOff[4]*60+ChOnOff[5]))){
        ChState |= (1 << ChOnOff[1]);
      }
    }
  } 
  return ChState;
}

#ifndef TEST_MODE
void TimeToLCD(uint8_t DoW, uint8_t Day, uint8_t Month, int Year, uint8_t Hour, uint8_t Min, uint8_t PosCur) {
  lcd.setCursor(0, 1);
  lcd.print(DoW);
  lcd.write(32);
  lcd.print(lid1Zero(Day));
  lcd.write(46); // .
  lcd.print(lid1Zero(Month));
  lcd.write(46); // .
  lcd.print(lid1Zero(Year));
  lcd.write(32);
  lcd.print(lid1Zero(Hour));
  lcd.write(58); // :
  lcd.print(lid1Zero(Min));
  lcd.setCursor(PosCur, 1);
  delay(300);
}
#endif

void SetOneTimer(uint8_t CurTimer) { // Setting current timer parametrs
  uint8_t CurWeekMode;
  uint8_t CurPos = 0;
  uint8_t CurTab[6] = {0, 2, 6, 9, 12, 15};
  for (CurWeekMode=0; CurWeekMode < WeekStateNum; CurWeekMode++) {
    if (ChOnOff[0] == daysofweek[CurWeekMode]) break;
  }
  lcd.blink();
  lcd.setCursor(2, 0);
  lcd.write(setpic);
  lcd.setCursor(2, 0);
  delay(450);
  unsigned long TimeStart=millis();
  for(uint8_t MenuExit = 0; MenuExit < 1 ; ) {
    Read_Key();
    if (key >= 0) TimeStart=millis();
    switch( key ) {
      case 0: // Right
        if (CurPos > 4) CurPos = 0;
        else CurPos++;
        if (CurPos > 0) {
          lcd.setCursor(2, 0);
          lcd.write(32);
          lcd.setCursor(CurTab[CurPos], 1);
        }
        else {
          lcd.setCursor(2, 0);
          lcd.write(setpic);
          lcd.setCursor(2, 0);
        }
        break;
      case 1: // Up
        if (CurPos > 0) {
          if (CurPos > 1) {
            if (ChOnOff[CurPos]<(23+(CurPos%2)*32)) { // 23 or 55
              ChOnOff[CurPos]+=((CurPos%2)*4+1);      // 1 or 5
            } else {
              ChOnOff[CurPos]=0;
            }
          } else {
            if (ChOnOff[1] < (ChannelsNum-1)) ChOnOff[1]++;
            else ChOnOff[1]=0;
          }
          TimerDisp();
          lcd.setCursor(CurTab[CurPos], 1);
        } else {
          if (CurWeekMode < (WeekStateNum-1)) CurWeekMode++;
          else CurWeekMode = 0;
          ChOnOff[0] = daysofweek[CurWeekMode];
          TimerDisp();
          lcd.setCursor(2, 0);
          lcd.write(setpic);
          lcd.setCursor(2, 0);
        }
        break;
      case 2: // Down
        if (CurPos > 0) {
          if (CurPos > 1) {
            if (ChOnOff[CurPos]>0) { // 23 or 55
              ChOnOff[CurPos]-=((CurPos%2)*4+1);      // 1 or 5
            } else {
              ChOnOff[CurPos]=(23+(CurPos%2)*32);
            }
          } else {
            if (ChOnOff[1] > 0) ChOnOff[1]--;
            else ChOnOff[1]=ChannelsNum-1;
          }
          TimerDisp();
          lcd.setCursor(CurTab[CurPos], 1);
        } else {
          if (CurWeekMode > 0) CurWeekMode--;
          else CurWeekMode = (WeekStateNum-1);
          ChOnOff[0] = daysofweek[CurWeekMode];
          TimerDisp();
          lcd.setCursor(2, 0);
          lcd.write(setpic);
          lcd.setCursor(2, 0);
        }
        break;
      case 3: // Left
        if (CurPos > 0) CurPos--;
        else CurPos = 5;
        if (CurPos > 0) {
          lcd.setCursor(2, 0);
          lcd.write(32);
          lcd.setCursor(CurTab[CurPos], 1);
        }
        else {
          lcd.setCursor(2, 0);
          lcd.write(setpic);
          lcd.setCursor(2, 0);
        }
        break;
      case 4: // Menu
        if ((60*ChOnOff[4]+ChOnOff[5]) > (60*ChOnOff[2]+ChOnOff[3])) {
          eeWrite(ChOnOff, TmrAddr + CurTimer*sizeof(ChOnOff), sizeof(ChOnOff));
        } else {
          lcd.setCursor(0, 0);
          lcd.print("Timer set error!");
          lcd.setCursor(0, 1);
          lcd.print("Write old value.");
        }
        MenuExit = 1;
        break;
    }
    delay(330);
    if((millis()-TimeStart) > 60000) MenuExit = 1;
  }    
}

void SetTimers() { // Select Timer from Menu
  uint8_t CurTimer = 0;
  ShowTimer(CurTimer);
  delay(450);
  unsigned long TimeStart=millis();
  for(uint8_t MenuExit = 0; MenuExit < 1 ; ) {
    Read_Key();
    if (key >= 0) TimeStart=millis();
    switch( key ) {
      case 0: // Right
        SetOneTimer(CurTimer);
        ShowTimer(CurTimer);
        delay(500);
        break;
      case 1: // Up
        if (CurTimer < (TimersNum-1)) CurTimer++;
        else CurTimer=0;
        ShowTimer(CurTimer);
        break;
      case 2: // Down
        if (CurTimer > 0) CurTimer--;
        else CurTimer = TimersNum - 1;
        ShowTimer(CurTimer);
        break;
      case 3: // Left
        MenuExit = 1;
        break;
      case 4: // Menu
        MenuExit = 1;
        break;
    }
    delay(330);
    if((millis()-TimeStart) > 30000) MenuExit = 1;
  }    
  lcd.noBlink();
}

void TimeSetup() {
#ifndef TEST_MODE
  uint8_t CurTab[6] = {0, 3, 6, 9, 12, 15};
  uint8_t CurPos = 0;
  t = rtc.getTime();
  uint8_t SetDoW = t.dow;
  uint8_t SetDay = t.date;
  uint8_t SetMonth = t.mon;
  int SetYear = t.year - 2000;
  uint8_t SetHour = t.hour;
  uint8_t SetMin = t.min;
  lcd.setCursor(0, 0);
  lcd.print("Set Date & Time:");
  TimeToLCD(SetDoW, SetDay, SetMonth, SetYear, SetHour, SetMin, CurTab[CurPos]);
  delay(1000);
  lcd.blink();
  unsigned long TimeStart=millis();
  for(uint8_t MenuExit = 0; MenuExit < 1 ; ) {
    Read_Key();
    if (key >= 0) TimeStart=millis();
    switch( key ) {
      case 0: // Right
        if (CurPos < 5) {
          CurPos++; 
        }
        TimeToLCD(SetDoW, SetDay, SetMonth, SetYear, SetHour, SetMin, CurTab[CurPos]);
        break;
      case 1: // Up
        switch( CurPos ) {
          case 0:
            if (SetDoW < 7) {
              SetDoW++;
            } else {
              SetDoW = 1;
            }
            break;
          case 1:
            if (SetDay < 31) {
              SetDay++;
            } else {
              SetDay = 1;
            }
            break;
          case 2:
            if (SetMonth < 12) {
              SetMonth++;
            } else {
              SetMonth = 1;
            }
            break;
          case 3:
            if (SetYear < 99) {
              SetYear++;
            } else {
              SetYear = 0;
            }
            break;
          case 4:
            if (SetHour < 23) {
              SetHour++;
            } else {
              SetHour = 0;
            }
            break;
          case 5:
            if (SetMin < 59) {
              SetMin++;
            } else {
              SetMin = 0;
              if (SetHour < 23) {
                SetHour++;
              } else {
                SetHour = 0;
              }
            }
            break;
        }
        TimeToLCD(SetDoW, SetDay, SetMonth, SetYear, SetHour, SetMin, CurTab[CurPos]);
        break;
      case 2: // Down
        switch( CurPos ) {
          case 0:
            if (SetDoW > 1) {
              SetDoW--;
            } else {
              SetDoW = 7;
            }
            break;
          case 1:
            if (SetDay > 1) {
              SetDay--;
            } else {
              SetDay = 31;
            }
            break;
          case 2:
            if (SetMonth > 1) {
              SetMonth--;
            } else {
              SetMonth = 12;
            }
            break;
          case 3:
            if (SetYear > 0) {
              SetYear--;
            } else {
              SetYear = 99;
            }
            break;
          case 4:
            if (SetHour > 0) {
              SetHour--;
            } else {
              SetHour = 23;
            }
            break;
          case 5:
            if (SetMin > 0) {
              SetMin--;
            } else {
              SetMin = 59;
              if (SetHour > 0) {
                SetHour--;
              } else {
                SetHour = 23;
              }
            }
            break;
        }
        TimeToLCD(SetDoW, SetDay, SetMonth, SetYear, SetHour, SetMin, CurTab[CurPos]);
        break;
      case 3: // Left
        if (CurPos > 0) {
          CurPos--; 
        }
        TimeToLCD(SetDoW, SetDay, SetMonth, SetYear, SetHour, SetMin, CurTab[CurPos]);
        break;
      case 4: // Menu
        rtc.setTime(SetHour, SetMin, 0);     // Set the time (24hr format)
        rtc.setDate(SetDay, SetMonth, (2000 + SetYear));   // Set the date
        rtc.setDOW(SetDoW);        // Set Day-of-Week
        MenuExit = 1;
        break;
    }
  delay(250);
  if((millis()-TimeStart) > 130000) MenuExit = 1;
  }
#endif
  lcd.noBlink();
}

// Displaying time and duration of feeding
void FeedDisp(uint8_t Pos) {
  uint8_t CurTab[3] = {1, 4, 13};
  lcd.setCursor(0, 0);
  lcd.print("< Feeding time >");
  lcd.setCursor(0, 1);
  lcd.print(lid1Zero(EEPROM.read(4)));
  lcd.write(58); // :
  lcd.print(lid1Zero(EEPROM.read(5)));
  if (FeedDelay < 100) lcd.write(32);
  lcd.print(" t = ");
  lcd.print(FeedDelay/10);
  lcd.write(46); // .
  lcd.print(FeedDelay%10);
  lcd.print(" S");
  lcd.setCursor(CurTab[Pos], 1);
}

// Setting the time and duration of feeding
void FeedMenu() {
  lcd.blink();
  uint8_t CurPos = 0;
  uint8_t FeedHour = EEPROM.read(4);
  uint8_t FeedMin = EEPROM.read(5);
  FeedDisp(CurPos);
  delay(500);
  unsigned long TimeStart=millis();
  for(uint8_t MenuExit = 0; MenuExit < 1 ; ) {
    Read_Key();
    if (key >= 0) TimeStart=millis();
    switch( key ) {
      case 0: // Right
        if (CurPos > 1) CurPos = 0;
        else CurPos++;
        FeedDisp(CurPos);
        break;
      case 1: // Up
        switch( CurPos ) {
          case 0:
            if (FeedHour < 23) FeedHour++;
            else FeedHour = 0;
            EEPROM.write(4, FeedHour);
            break;
          case 1:
            if (FeedMin < 59) FeedMin++;
            else FeedMin = 0;
            EEPROM.write(5, FeedMin);
            break;
          case 2:
            FeedDelay++;
            EEPROM.write(6, FeedDelay);
            break;
        }
        FeedDisp(CurPos);
        break;
      case 2: // Down
        switch( CurPos ) {
          case 0:
            if (FeedHour > 0) FeedHour--;
            else FeedHour = 23;
            EEPROM.write(4, FeedHour);
            break;
          case 1:
            if (FeedMin > 0) FeedMin--;
            else FeedMin = 59;
            EEPROM.write(5, FeedMin);
            break;
          case 2:
            FeedDelay--;
            EEPROM.write(6, FeedDelay);
            break;
        }
        FeedDisp(CurPos);
        break;
      case 3: // Left
        if (CurPos < 1) CurPos = 2;
        else CurPos--;
        FeedDisp(CurPos);
        break;
      case 4: // Menu
        MenuExit = 1;
        break;
    }
    delay(350);
    if((millis()-TimeStart) > 30000) MenuExit = 1;
  } 
  FeedTime = FeedHour*60 + FeedMin;
  lcd.noBlink();
}

void SubMenuDisp(uint8_t CursPos, uint8_t MenuPos) { // Display system menu items
  String MenuItem[2][4] = {"Positive relay  ", "Hour Beep Off   ", "Keys Tone Off   ", " Day            ", "Inverted relay  ", "Hour Beep On    ", "Keys Tone On    ", " Ngt            "};
  for (uint8_t i=0;i<2;i++) {
    lcd.setCursor(0, i);
    lcd.print(MenuItem[i][MenuPos]);
  }
  switch( MenuPos ) {
    case 0: // Relay type
      lcd.setCursor(15, HIGH-RelayUp);
      break;
    case 1: // Hour beep
      lcd.setCursor(15, BeepOnOff);
      break;
    case 2: // Keys tone
      lcd.setCursor(15, KeyToneOn);
      break;
  }
  lcd.write(5);
  lcd.setCursor(15, CursPos);
  lcd.blink();
}

void SubChangeMenu(uint8_t SysMenuNum) { // System submenu
  uint8_t LastItem = 0;
  SubMenuDisp(LastItem, SysMenuNum);
  delay(500);
  unsigned long TimeStart=millis();
  for(uint8_t MenuExit = 0; MenuExit < 1 ; ) {
    Read_Key();
    if (key >= 0) TimeStart=millis();
    switch( key ) {
      case 0: // Right
        EEPROM.write(7+SysMenuNum, LastItem);
        delay(100);
        RelayUp=HIGH-EEPROM.read(7);
        BeepOnOff=EEPROM.read(8);
        KeyToneOn=EEPROM.read(9);
        SubMenuDisp(LastItem, SysMenuNum);
        delay(700);
        MenuExit = 1;
        break;
      case 1: // Up
        LastItem = 0;
        SubMenuDisp(LastItem, SysMenuNum);
        break;
      case 2: // Down
        LastItem = 1;
        SubMenuDisp(LastItem, SysMenuNum);
        break;
      case 3: // Left
        MenuExit = 1;
        break;
      case 4: // Menu
        MenuExit = 1;
        break;
    }
    delay(500);
    if((millis()-TimeStart) > 20000) MenuExit = 1;
  }
  lcd.noBlink();
}

void DispNumOfChannels() {
  lcd.setCursor(13, 1);
  lcd.print(ChannelsNum);
}

void MenuChannelsNum() {
  unsigned long TimeStart=millis();
  lcd.setCursor(0, 0);
  lcd.print("Set the number  ");
  lcd.setCursor(0, 1);
  lcd.print("of channels:    ");
  DispNumOfChannels();
  delay(350);
  for(uint8_t MenuExit = 0; MenuExit < 1 ; ) {
    Read_Key();
    if (key >= 0) TimeStart=millis();
    switch( key ) {
      case 0: // Right
        EEPROM.write(3, ChannelsNum);
        MenuExit = 1;
        break;
      case 1: // Up
        if (ChannelsNum < MaxNumChannels) ChannelsNum++;
        else ChannelsNum = 2;
        DispNumOfChannels();
        break;
      case 2: // Down
        if (ChannelsNum > 2) ChannelsNum--;
        else ChannelsNum = MaxNumChannels;
        DispNumOfChannels();
        break;
      case 3: // Left
        ChannelsNum = EEPROM.read(3);
        MenuExit = 1;
        break;
      case 4: // Menu
        EEPROM.write(3, ChannelsNum);
        MenuExit = 1;
        break;
    }
    delay(500);
    if((millis()-TimeStart) > 15000) {
      ChannelsNum = EEPROM.read(3);
      MenuExit = 1;
    }
  }    
  ShowChannels();
  delay(1200);
}

void MenuSetTimeAdjust() {
  unsigned long TimeStart=millis();
  lcd.setCursor(0, 0);
  lcd.print("Set the value of");
  lcd.setCursor(0, 1);
  lcd.print("time adjust:    ");
  DispTimeAdjValue();
  delay(350);
  for(uint8_t MenuExit = 0; MenuExit < 1 ; ) {
    Read_Key();
    if (key >= 0) TimeStart=millis();
    switch( key ) {
      case 0: // Right
        EEPROM.write(10, TimeAdj);
        MenuExit = 1;
        break;
      case 1: // Up
        if (TimeAdj < 59) TimeAdj++;
        else tone(TonePin, NOTE_E7, 40);
        DispTimeAdjValue();
        break;
      case 2: // Down
        if (TimeAdj > -60) TimeAdj--;
        else tone(TonePin, NOTE_E7, 40);
        DispTimeAdjValue();
        break;
      case 3: // Left
        TimeAdj = EEPROM.read(10);
        DispTimeAdjValue();
        MenuExit = 1;
        break;
      case 4: // Menu
        EEPROM.write(10, TimeAdj);
        MenuExit = 1;
        break;
    }
    delay(250);
    if((millis()-TimeStart) > 15000) {
      TimeAdj = EEPROM.read(10);
      MenuExit = 1;
    }
  }    
}

void DispTimeAdjValue() {
  uint8_t CurPos = 15;
  if (TimeAdj>9) CurPos=14;
  if (TimeAdj<0) {
    CurPos=14;
    if (TimeAdj<-9) CurPos=13;
  }
  lcd.setCursor(12, 1);
  lcd.print("   ");
  lcd.setCursor(CurPos, 1);
  lcd.print(TimeAdj);
}

void BackLightDisp(uint8_t CursPos) {
  analogWrite(BackLightPin, BLset[CursPos]);
  lcd.setCursor(0, CursPos);
  lcd.write(setpic);
  lcd.setCursor(0, 1 - CursPos);
  lcd.write(32);
  for (uint8_t i=0;i<2;i++) {
    lcd.setCursor(5, i);
    for (int j=0; j<12; j++) {
      if (25*j+5<=BLset[i]) lcd.write(255);
      else lcd.write(219); 
    }
  }    
}

void MenuBackLightSet() {
  uint8_t LastItem = 0;
  SubMenuDisp(LastItem, 3);
  lcd.noBlink();
  BackLightDisp(LastItem);
  delay(500);
  unsigned long TimeStart=millis();
  for(uint8_t MenuExit = 0; MenuExit < 1 ; ) {
    Read_Key();
    if (key >= 0) TimeStart=millis();
    switch( key ) {
      case 0: // Right
        if (BLset[LastItem] <= 230) {
          BLset[LastItem] += 25;
          BackLightDisp(LastItem);
        }
        break;
      case 1: // Up
        LastItem = 0;
        BackLightDisp(LastItem);
        break;
      case 2: // Down
        LastItem = 1;
        BackLightDisp(LastItem);
        break;
      case 3: // Left
        if (BLset[LastItem] >= 30) {
          BLset[LastItem] -= 25;
          BackLightDisp(LastItem);
        }
        break;
      case 4: // Menu
        EEPROM.write(11, BLset[0]);
        EEPROM.write(12, BLset[1]);
        analogWrite(BackLightPin, 255);
        MenuExit = 1;
        break;
    }
    delay(500);
    if((millis()-TimeStart) > 15000) {
//      ChannelsNum = EEPROM.read(3);
      if (EEPROM.read(11) > 0) {
        BLset[0] = EEPROM.read(11);
        BLset[1] = EEPROM.read(12);
      } else {
        BLset[0] = 255;
        BLset[1] = 55;
      }
      analogWrite(BackLightPin, 255);
      MenuExit = 1;
    }
    key = -1;
  }    
  
}

void MenuDisp(uint8_t CursPos, uint8_t DispPos) { // Display main menu
  String MenuItem[] = {" Set date & time", " Setting timers ", " Feeding task   ", " System menu    ", " Menu exit      "};
  for (uint8_t i=0;i<2;i++) {
    lcd.setCursor(0, i);
    lcd.print(MenuItem[DispPos+i]);
  }
  lcd.setCursor(0, (CursPos-DispPos));
  lcd.write(setpic);
}

void MenuSelect() { // main menu function
  unsigned long TimeStart=millis();
  uint8_t LastItem = 0;
  uint8_t LastDisp = 0;
  uint8_t MaxItem = 4; // 5 items max
  analogWrite(BackLightPin, 255);
  lcd.noCursor();
  lcd.noBlink();
  MenuDisp(LastItem, LastDisp);
  delay(500);
  for (uint8_t MenuExit = 0; MenuExit < 1;) {
    Read_Key();
    if (key >= 0) TimeStart=millis();
    switch( key ) {
      case 0: // Right
        switch( LastItem ) {
          case 0: // * >Set Date&Time *
            TimeSetup(); 
            MenuExit = 1;
            break;
          case 1: // *  Set Timers    *
            SetTimers();
            break;
          case 2: // *  Set eat time  *
            FeedMenu();
            break;
          case 3: // *  System menu   *
            SysMenuSelect();
            break;
          case 4: // *  Exit menu     *
            MenuExit = 1;
            break;
        }
        key = -1;
        TimeStart=millis()-15000;
        MenuDisp(LastItem, LastDisp);
        break;
      case 1: // Up
        if (LastItem > 0) {
          if (LastItem <= LastDisp) LastDisp--;
          LastItem--;
        }
        MenuDisp(LastItem, LastDisp);
        break;
      case 2: // Down
        if (LastItem < MaxItem) {
          if (LastItem > LastDisp) LastDisp++;
          LastItem++;
        }
        MenuDisp(LastItem, LastDisp);
        break;
      case 3: // Left
        LastItem = MaxItem;
        LastDisp = MaxItem - 1;
        MenuDisp(LastItem, LastDisp);
        break;
      case 4: // Menu
        MenuExit = 1;
        break;
    }
    delay(350);
    if((millis()-TimeStart) > 45000) MenuExit = 1;
  }
}

void SysMenuDisp(uint8_t CursPos, uint8_t DispPos) { // Demonstration of the additional menu
  char MenuItem[8][17] = {" Set hour beep  ", " Set key tone   ", " Set relay type ", " Set num channel", " Set time adjust", " Set backlight  ", " Default value  ", " Main menu      "};
  for (uint8_t i=0;i<2;i++) {
    lcd.setCursor(0, i);
    lcd.print(MenuItem[DispPos+i]);
  }
  lcd.setCursor(0, (CursPos-DispPos));
  lcd.write(setpic);
}

void SysMenuSelect() { // System menu
  unsigned long TimeStart=millis();
  uint8_t LastItem = 0;
  uint8_t LastDisp = 0;
  uint8_t MaxItem = 7; // 8 items max
  lcd.noCursor();
  lcd.noBlink();
  SysMenuDisp(LastItem, LastDisp);
  delay(500);
  for(uint8_t MenuExit = 0; MenuExit < 1; ) {
    Read_Key();
    if (key >= 0) TimeStart=millis();
    switch( key ) {
      case 0: // Right
        switch( LastItem ) { 
          case 0: // * >Hour beep on  *
            SubChangeMenu(1); 
            break;
          case 1: // *  Key tone on   *
            SubChangeMenu(2); 
            break;
          case 2: // *  Set relay type *
            SubChangeMenu(0);
            break;
          case 3: // *  Set num channel*
            MenuChannelsNum();
            break;
          case 4: // *  Set time adjust value*
            MenuSetTimeAdjust();
            break;
          case 5: // *  Set backlight value*
            MenuBackLightSet();
            break;
          case 6: // *  Default value *
            EEPROM.write(0,1);
            ReadWriteEEPROM();
            MenuExit = 1;
            break;
          case 7: // *  Exit menu     *
            MenuExit = 1;
            break;
        }
        key = -1;
        TimeStart=millis()-15000;
        SysMenuDisp(LastItem, LastDisp);
        break;
      case 1: // Up
        if (LastItem > 0) {
          if (LastItem <= LastDisp) LastDisp--;
          LastItem--;
        }
        SysMenuDisp(LastItem, LastDisp);
        break;
      case 2: // Down
        if (LastItem < MaxItem) {
          if (LastItem > LastDisp) LastDisp++;
          LastItem++;
        }
        SysMenuDisp(LastItem, LastDisp);
        break;
      case 3: // Left
        LastItem = MaxItem;
        LastDisp = MaxItem - 1;
        SysMenuDisp(LastItem, LastDisp);
        break;
      case 4: // Menu
        MenuExit = 1;
        break;
    }
    delay(350);
    if((millis()-TimeStart) > 45000) MenuExit = 1;
  }
}

boolean NotFeeding(unsigned int CurrTime) { // Verification of the need for feeding
  uint8_t ttt = 1;
#ifdef TEST_MODE
  ttt = 5;
#endif
  if ((CurrTime >= FeedTime) && (CurrTime < (FeedTime+ttt))) {
    if (FeedOK) return true;
    else {
      FeedOK = true;
      return false;
    }
  } else {
    FeedOK = false;
    return true;
  }
}

void FeedStart() {
  lcd.print("Feeding");
  tone(TonePin, NOTE_A5, 100);
  delay(120);
  tone(TonePin, NOTE_A5, 500);
  digitalWrite(VibroPin, HIGH);
  delay(FeedDelay*100);
  digitalWrite(VibroPin, LOW);
  delay(2500);
}

void TimeAdjusting() {
  if (TimeAdj < 0) {
    lcd.print("Adjusting");
    lcd.setCursor(0, 1);
    rtc.halt(true);
    delay(1000*abs(TimeAdj));
    rtc.halt(false);
    delay(1000);
  } else rtc.setTime(3, 30, TimeAdj);
}

void loop() {
#ifndef TEST_MODE
  // Get data from the DS1302
  t = rtc.getTime();
  if (oldsec != t.sec) {
    oldsec = t.sec;

    lcd.setCursor(0, 0);
    lcd.print(dn[t.dow-1]);
    lcd.print(", ");
    lcd.print(lid1Zero(t.date));
    lcd.write(32);
    lcd.print(rtc.getMonthStr());
    lcd.setCursor(11, 0);
    lcd.write(32);
    lcd.print(t.year, DEC);
    lcd.setCursor(0, 1);
    TimeNow = t.hour*60+t.min;
    if ((TimeNow == 210) && (t.sec == 0) && (TimeAdj != 0)) TimeAdjusting();
    lcd.print(rtc.getTimeStr());
    lcd.write(32);
    if (NotFeeding(TimeNow)) {
      TimerOnOffDisp(StateChannels(TimeNow, t.dow));
      if (!BLNightState) HourBeep(TimeNow);
    } else FeedStart();
  } else {
    lcd.setCursor(9, 1);
    TimerOnOffDisp(StateChannels(TimeNow, t.dow));
  }
  delay(240);
#else // for test mode
  for (uint8_t dw = 1; dw < 8 ; dw++) {
    for (int ct = 0 ; ct < 12*24 ; ct++) {
      lcd.setCursor(0, 0);
      lcd.print(dn[dw-1]);
      lcd.print(", 1");
      lcd.print(dw);
      lcd.print(" Sen 2017");
      lcd.setCursor(0, 1);
      lcd.print(lid1Zero(ct/12));
      lcd.write(58); // :
      lcd.print(lid1Zero((ct*5)%60));
      lcd.print(":00 ");
      if (NotFeeding(5*ct)) {
        TimerOnOffDisp(StateChannels(5*ct, dw));
        if (!BLNightState) HourBeep(5*ct);
      } else FeedStart();
      delay(50);
#endif
      Read_Key();
      switch( key ) {
        case 0: // Right
          if (Ch2OnOff) {
            if (!Ch2NeedOn) Ch2NeedOff = true;
            else Ch2NeedOn = false;
          } else {
            if (!Ch2NeedOff) Ch2NeedOn = true;
            else Ch2NeedOff = false;
          }
#ifdef TEST_MODE
          delay(500);
#else
          delay(250);
#endif
          break;
        case 1: // Up
          if (BLNightState == 0) {
            if (!BLNeedOn) BLNeedOff = true;
            else BLNeedOn = false;
          } else {
            if (!BLNeedOff) BLNeedOn = true;
            else BLNeedOff = false;
          }
          delay(250);
          break;
        case 2: // Down
          lcd.setCursor(9, 1);
          FeedStart();
          break;
        case 3: // Left
          if (Ch1OnOff) {
            if (!Ch1NeedOn) Ch1NeedOff = true;
            else Ch1NeedOn = false;
          } else {
            if (!Ch1NeedOff) Ch1NeedOn = true;
            else Ch1NeedOff = false;
          }
#ifdef TEST_MODE
          delay(500);
#else
          delay(250);
#endif
          break;
        case 4: // Menu
          MenuSelect();
          break;
      }
      key=-1;
#ifdef TEST_MODE
    }
  }
#endif
}

