//#define DEBUG
//==========================================================//
//Windowsill garden watering sysytem for three pumping zones//
//==========================================================//

/************************************************/
#define SketchVersion v4_20250811
/************************************************/

#include <Wire.h>
#include <OneButton.h>
#include <RotaryEncoder.h>
#include <LiquidCrystal_I2C.h>
#include "Main.h" //Main settings
#include "_Pumper.h" //Class for pumper unit
//------------------------------------------------



//hardware assignements
LiquidCrystal_I2C lcd(0x27, 16, 2);
RotaryEncoder encoder(pinOfEncoder[0], pinOfEncoder[1], RotaryEncoder::LatchMode::TWO03);
OneButton encoderBtn (pinOfEncoder[2], true);
CURR_STATUS currentStatus = STOP;
WATERING_RESULT waterignResult = PASS;

PUMPER* myPump = new PUMPER[NbOfPump];

void startStop() {
  int longPress = 1000;
  longPress=longPress+1;
}

void leakAlarm() {
  currentStatus = ALARM;
}

void setup() {
  Wire.begin();
  Serial.begin(115200);  // Init serial output for debug
  while (!Serial) {}     // Needed only for built-in USB ports.

  pinMode(pinAlarmLED, OUTPUT);

  pinMode(pinINT0StopButton, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinINT0StopButton), startStop, FALLING);

  pinMode(pinINT1AlarmSensors, INPUT);
  attachInterrupt(digitalPinToInterrupt(pinINT1AlarmSensors), leakAlarm, HIGH);
  Serial.println("StartStart11");

  
  //lcd.init();
  //lcd.backlight();
  /* Print a message to the LCD.
  lcd.setCursor(3,0);
  lcd.print("Hello, world!");
  lcd.setCursor(2,1);
  lcd.print("Ywrobot Arduino!");
   lcd.setCursor(0,2);
  lcd.print("Arduino LCM IIC 2004");
   lcd.setCursor(2,3);
  lcd.print("Power By Ec-yuan!");*/


  for (uint8_t i = 0; i < NbOfPump; i++) {
    myPump[i] = PUMPER(i, pinOfSensor[i], pinOfPump[i], pinOfAlarmSensor[i], pinOfCntrlButton[i]);  //Как получить i из класса не передавая его явно?
    //pumpBtn[i].setup(pinOfCntrlButton[i], INPUT_PULLUP, true);
  }
  currentStatus = RUN;
}

void loop() {
  for (uint8_t i = 0; i < NbOfPump; i++) {
    waterignResult = myPump[i].pumpIt();
  }
}

void alarmLedBlink() {
  digitalWrite(pinAlarmLED, HIGH);
  delay(1000);
  pinMode(pinAlarmLED, LOW);
}

void alarmLedOn() {
  digitalWrite(pinAlarmLED, HIGH);
}

void alarmLedOff() {
  digitalWrite(pinAlarmLED, LOW);
}