// #define DEBUG
//==========================================================//
// Windowsill garden watering sysytem for three pumping zones//
//==========================================================//

/************************************************/
#define SketchVersion "v5_20260420"
/************************************************/

// #define USE_LGT_EEPROM_API
#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include <OneButton.h>
#include <RotaryEncoder.h>
#include <LiquidCrystal_I2C.h>

#include "_Pumper.h" //Class and setup for pumper unit

//------------------------------------------------

#define NB_OF_PUMPS 3 // Quantity of pump units
/*=============================================\
|One Unit include:                             |
| -Capacitive Moisture Sensor (analog input)   |
| -Resestive Sensor for watertank's            |
|    empty control (HIGH is empty)             |
| -Pump ON/OFF reley                           |
| -Pump control button                         |
\=============================================*/

// Capacitive Sensor RAW data
// Dry: (565 430]
// Wet: (430 350]
// Water: (350 205]
static constexpr int AirValue = 565;   // Calibration of sensors needed!
static constexpr int WaterValue = 205; // Calibration of sensors needed!

// Pins definitions
static constexpr int pinOfSensor[NB_OF_PUMPS] = {A3, A6, A7};       // Pins connected to capacity sensors 1-2-3
static constexpr int pinOfPump[NB_OF_PUMPS] = {A0, A1, A2};         // Pins connected to pump relays 1-2-3
static constexpr uint8_t pinOfAlarmSensor[NB_OF_PUMPS] = {7, 8, 9}; // Pins connected to leak resestive sensors 1-2-3 in digital mode
static constexpr uint8_t pinOfCntrlButton[NB_OF_PUMPS] = {4, 5, 6}; // Pins connected to control buttons 1-2-3
static constexpr uint8_t pinOfEncoder[3] = {10, 11, 12};            // Pins connected to encoder (A, B, last number is encbutton)
static constexpr uint8_t pinINT0StopButton = 2;                     // Pin of Emergency STOP button (and START too)
static constexpr uint8_t pinINT1AlarmSensors = 3;                   // Pin of Emergency STOP form Resestive Leak Sensors
static constexpr uint8_t pinAlarmLED = 13;                          // Pin of Alarm LED
// A4 - SDA, A5 - SCL, LCD con

static constexpr uint8_t maxPumpCykles = 50; // Max count of pump cykles

/* Enums */

// machine status
typedef enum
{
  _STOP,       // System halted
  _RUN,        // Sysytem works
  _SETUP_MODE, // System in setup mode
  _ALARM       // Leaking detected
} ECurrStatus;

// Result of watering attempt
typedef enum
{
  _PASS,       // Dont need watering
  _DONE,       // Watering succesful
  _CANT_REACH, // Watering failed
  _LEAK        // Leaking detected
} EWateringResult;

#pragma pack(push, 1)
struct pumpSetting
{
  uint8_t minM;      // Min moisture to start watering
  uint8_t maxM;      // Max moisture to stop watering
  uint8_t pumpTime;  // Time of pumping in seconds
  uint8_t pumpPause; // Time of pause between pump cykles in seconds
}; // 4 bytes (32 bits)
#pragma pack(pop)

union tUnionSetting
{
  pumpSetting D;
  byte B[sizeof(pumpSetting)];
};

// initial data for pumping setting
tUnionSetting initPumpSetup[NB_OF_PUMPS]{
    // MinM(%), MaxM(%), PumpTime(sec), PumpPause(src)
    {{20, 60, 2, 10}},
    {{20, 60, 2, 10}},
    {{20, 60, 2, 10}}}; // array of pumps settings

static String nameOfSetting[sizeof(tUnionSetting) / sizeof(uint8_t)] = {"minMo", "MAXMo", "PumpTime", "PumpPause"};
//=====================================

// hardware assignements
LiquidCrystal_I2C lcd(0x27, 16, 2);
RotaryEncoder encoder(pinOfEncoder[0], pinOfEncoder[1], RotaryEncoder::LatchMode::TWO03);
OneButton encoderBtn(pinOfEncoder[2], true);
int selectedPump = 0;
int settingIndex = 0;
ECurrStatus currentStatus = _STOP;
EWateringResult waterignResult = _PASS;

PUMPER *myPump = new PUMPER[NB_OF_PUMPS];

unsigned int storedAddress = sizeof(tUnionSetting) * NB_OF_PUMPS;
const unsigned int WRITTEN_SIGNATURE = 0xBEEFDEED;
tUnionSetting pumpSetupFromEPR[NB_OF_PUMPS];

void memoryRead()
{
  // Check signature at address
  unsigned int a = 0;
  EEPROM.get(storedAddress, a);
  if (a != WRITTEN_SIGNATURE)
  {
    EEPROM.put(0, initPumpSetup);
    EEPROM.put(storedAddress, WRITTEN_SIGNATURE);
  }
  EEPROM.get(0, pumpSetupFromEPR);
}

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

void handleLCD()
{
  String oldString1, oldString2;
  String newString1, newString2;
  switch (currentStatus)
  {
  case _STOP:
    newString1 = "System STOPPED";
    newString2 = "Press START";
    break;
  case _RUN:
    newString1 = "System RUNNING";
    newString2 = "All is OK";
    break;
  case _SETUP_MODE:
    newString1 = "SETUP MODE";
    newString2 = "Use encoder";
    break;
  case _ALARM:
    newString1 = "LEAKING DETECTED!";
    newString2 = "Check sensors!";
    break;
  }
  if (newString1 != oldString1 || newString2 != oldString2)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(newString1.c_str());
    lcd.setCursor(0, 1);
    lcd.print(newString2.c_str());
    oldString1 = newString1;
    oldString2 = newString2;
  }
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
  attachInterrupt(digitalPinToInterrupt(pinINT0StopButton), startStop, FALLING);

  pinMode(pinINT1AlarmSensors, INPUT);
  attachInterrupt(digitalPinToInterrupt(pinINT1AlarmSensors), leakAlarmOn, FALLING);

  Serial.println("StartStart_ver: " + String(SketchVersion));

  lcd.init();
  lcd.backlight();
  // lcd.noBacklight();

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
 
  // Setup encoder button
  encoderBtn.setLongPressIntervalMs(800);
  encoderBtn.attachClick([]()
                         {
    settingIndex++;
    if (settingIndex >= sizeof(tUnionSetting) / sizeof(int)) {
      settingIndex = 0;
    }
    encoder.setPosition(pumpSetupFromEPR[selectedPump].B[settingIndex]); });

  encoderBtn.attachDoubleClick([]()
                               {
                                 if (currentStatus != _SETUP_MODE)
                                 {
                                   settingIndex = 0;
                                   selectedPump = 0;
                                   encoder.setPosition(pumpSetupFromEPR[selectedPump].B[settingIndex]);
                                   currentStatus = _SETUP_MODE;
                                 }
                                 else
                                 {
                                   EEPROM.put(0, pumpSetupFromEPR);
                                   for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
                                   {
                                     myPump[i].init();
                                   }
                                   currentStatus = _RUN;
                                 } });

  encoderBtn.attachLongPressStart([]()
                                  {
    selectedPump++;
    if (selectedPump >= NB_OF_PUMPS) {
      selectedPump = 0;
    }
    encoder.setPosition(pumpSetupFromEPR[selectedPump].B[settingIndex]); });
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

// Function to modify pump settings interactively
void modifyPumpSettings()
{

  while (true)
  {
    encoder.tick();

    int oldValue;
    int newValue = encoder.getPosition();

    pumpSetupFromEPR[selectedPump].settings[settingIndex] = newValue;

    lcd.setCursor(0, 0);
    lcd.print("Pump: ");
    lcd.print(selectedPump);
    lcd.setCursor(0, 1);
    lcd.print("Setting ");
    lcd.print(nameOfSetting[settingIndex]);
    lcd.print(": ");
    lcd.print(newValue);
    oldValue = newValue;

    delay(100);
    encoderBtn.tick();
  }
}