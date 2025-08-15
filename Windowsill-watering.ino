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

//Pins definitions
const int pinOfSensor[NbOfPump] = { A0, A1, A2 };        //Pins connected to capacity sensors 1-2-3
const int pinOfPump[NbOfPump] = { A3, A6, A7 };          //Pins connected to pump relays 1-2-3
const uint8_t pinOfAlarmSensor[NbOfPump] = { 7, 8, 9 };  //Pins connected to leak resestive sensors 1-2-3 in digital mode
const uint8_t pinOfCntrlButton[NbOfPump] = { 4, 5, 6 };  //Pins connected to control buttons 1-2-3
const uint8_t pinOfEncoder[3] = { 10, 11, 12 };          //Pins connected to encoder (last number is encbutton)
const uint8_t pinINT0StopButton = 2;                     //Pin of Emergency STOP button (and START too)
const uint8_t pinINT1AlarmSensors = 3;                   //Pin of Emergency STOP form Resestive Leak Sensors
const uint8_t pinAlarmLED = 13;                          //Pin of Alarm LED

//hardware assignements
LiquidCrystal_I2C lcd(0x27, 16, 2);
RotaryEncoder encoder(pinOfEncoder[0], pinOfEncoder[1], RotaryEncoder::LatchMode::TWO03);
OneButton encoderBtn (pinOfEncoder[2], true);
enum CURR_STATUS currentStatus = STOP;
enum WATERING_RESULT waterignResult = PASS;

PUMPER* myPump = new PUMPER[NbOfPump];

void setup() {
  Wire.begin();
  Serial.begin(115200);  // Init serial output for debug
  while (!Serial) {}     // Needed only for built-in USB ports.

  pinMode(pinAlarmLED, OUTPUT);

  pinMode(pinINT0StopButton, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinINT0StopButton), startStop, FALLING);

  pinMode(pinINT1AlarmSensors, INPUT);
  attachInterrupt(digitalPinToInterrupt(pinINT1AlarmSensors), leakAlarm, HIGH);

  
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

void startStop() {
  uint8_t longPress = 1000;
}

void leakAlarm() {
  currentStatus = ALARM;
}