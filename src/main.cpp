// #define DEBUG
//==========================================================//
// Windowsill garden watering sysytem for three pumping zones//
//==========================================================//

/************************************************/
#define SketchVersion v5_20260420
/************************************************/

#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include <OneButton.h>
#include <RotaryEncoder.h>
#include <LiquidCrystal_I2C.h>

#include "_Pumper.h" //Class and setup for pumper unit
//------------------------------------------------

#define NB_OF_PUMPS 3  //Quantity of pump units
/*=============================================\
|One Unit include:                             |
| -Capacitive Moisture Sensor (analog input)   |
| -Resestive Sensor for watertank              |
|    empty control (HIGH is empty)             |
| -Pump ON/OFF reley                           |
| -Pump control button                         |
\=============================================*/


//Capacitive Sensor RAW data
//Dry: (520 430]
//Wet: (430 350]
//Water: (350 260]
static constexpr int AirValue = 620;  //Calibration of sensors needed!
static constexpr int WaterValue = 310; //Calibration of sensors needed!

//Pins definitions
static constexpr int pinOfSensor[NB_OF_PUMPS] = { A3, A6, A7 };        //Pins connected to capacity sensors 1-2-3
static constexpr int pinOfPump[NB_OF_PUMPS] = { A0, A1, A2 };          //Pins connected to pump relays 1-2-3
static constexpr uint8_t pinOfAlarmSensor[NB_OF_PUMPS] = { 7, 8, 9 };  //Pins connected to leak resestive sensors 1-2-3 in digital mode
static constexpr uint8_t pinOfCntrlButton[NB_OF_PUMPS] = { 4, 5, 6 };  //Pins connected to control buttons 1-2-3
static constexpr uint8_t pinOfEncoder[3] = { 10, 11, 12 };          //Pins connected to encoder (A, B, last number is encbutton)
static constexpr uint8_t pinINT0StopButton = 2;                     //Pin of Emergency STOP button (and START too)
static constexpr uint8_t pinINT1AlarmSensors = 3;                   //Pin of Emergency STOP form Resestive Leak Sensors
static constexpr uint8_t pinAlarmLED = 13;                          //Pin of Alarm LED
// A4 - SDA, A5 - SCL, LCD con  

/* Enums */

//machine status
typedef enum  {
  _STOP,        // System halted
  _RUN,         // Sysytem works
  _SETUP_MODE,  // System in setup mode
  _ALARM        // Leaking detected
} ECurrStatus;

// Result of watering attempt
typedef enum  {
  _PASS,        // Dont need watering
  _DONE,        // Watering succesful
  _CANT_REACH,  // Watering failed
  _LEAK         // Leaking detected
} EWateringResult;

//=====================================

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