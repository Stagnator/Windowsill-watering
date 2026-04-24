// #define DEBUG
//==========================================================//
// Windowsill garden watering sysytem for three pumping zones//
//==========================================================//

/************************************************/
#define SketchVersion v4_20250811
/************************************************/

#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include <OneButton.h>
#include <RotaryEncoder.h>
#include <LiquidCrystal_I2C.h>

#include "Main.h"    //Main settings
#include "_Pumper.h" //Class and setup for pumper unit
//------------------------------------------------

// hardware assignements
LiquidCrystal_I2C lcd(0x27, 16, 2);
RotaryEncoder encoder(pinOfEncoder[0], pinOfEncoder[1], RotaryEncoder::LatchMode::TWO03);
OneButton encoderBtn(pinOfEncoder[2], true);
ECurrStatus currentStatus = _STOP;
EWateringResult waterignResult = _PASS;

PUMPER *myPump = new PUMPER[NB_OF_PUMPS];

void startStop()
{
  if (currentStatus == _STOP)
  {
    currentStatus = _RUN;
    alarmLedOff();
  }
  else
  {
    currentStatus = _STOP;
    for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
    {
      myPump[i].stopIt();
    }
    alarmLedOn();
  }
}

void leakAlarmOn()
{
  currentStatus = _ALARM;
}

void setup()
{
  Wire.begin();
  Serial.begin(115200); // Init serial output for debug
  while (!Serial)
  {
  } // Needed only for built-in USB ports.

  pinMode(pinAlarmLED, OUTPUT);

  pinMode(pinINT0StopButton, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinINT0StopButton), startStop, RISING);

  pinMode(pinINT1AlarmSensors, INPUT);
  attachInterrupt(digitalPinToInterrupt(pinINT1AlarmSensors), leakAlarmOn, RISING);

  Serial.println("StartStart11");

  lcd.init();
  lcd.backlight();

  currentStatus = _RUN;

  for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
  {
    myPump[i] = PUMPER(i, pinOfSensor[i], pinOfPump[i], pinOfAlarmSensor[i], pinOfCntrlButton[i]); // Как получить i из класса не передавая его явно?
    myPump[i].init();
    if (myPump[i].pumpStatus == _LEAK_DT)
    {
      currentStatus = _ALARM;
    }
  }
  displayPrint(currentStatus);
}

void loop()
{
  if (currentStatus == _ALARM)
  {
    alalrmLedBlink();
  }

  if (currentStatus == _RUN || currentStatus == _ALARM)
  {
    for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
    {
      waterignResult = myPump[i].pumpIt();
      displayPrint(currentStatus);
    }
  }

  if (currentStatus == _SETUP_MODE)
  {
  }

  displayPrint(currentStatus);
}

void alarmLedBlink()
{
  digitalWrite(pinAlarmLED, HIGH);
  delay(1000);
  pinMode(pinAlarmLED, LOW);
}

void alarmLedOn()
{
  digitalWrite(pinAlarmLED, HIGH);
}

void alarmLedOff()
{
  digitalWrite(pinAlarmLED, LOW);
}

void displayPrint(ECurrStatus cStat)
{
  /* Print a message to the LCD.
    lcd.setCursor(3,0);
    lcd.print("Hello, world!");
    lcd.setCursor(2,1);
    lcd.print("Ywrobot Arduino!");
     lcd.setCursor(0,2);
    lcd.print("Arduino LCM IIC 2004");
     lcd.setCursor(2,3);
    lcd.print("Power By Ec-yuan!");*/
}