// #define DEBUG
//==========================================================//
// Windowsill garden watering sysytem for three pumping zones//
//==========================================================//

/************************************************/
#define SketchVersion "v 0.46"
/************************************************/

#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include <OneButton.h>
#include <RotaryEncoder.h>
#include <LiquidCrystal_I2C.h>

#include "debug.h"

#include "_Pumper.h" //Class and setup for pumper unit

//------------------------------------------------

#define NB_OF_PUMPS 3 // Quantity of pump units
/*=============================================\
|One Unit include:                             |
| -Capacitive Moisture Sensor (analog input)   |
| -Resestive Sensor for watertank's            |
|    empty control (HIGH is empty)             |
| -Pump ON/OFF reley module (ON-LOW, OFF-HIGH) |
|   I've very sensitive modules, so need to set|
|   pinMode to INPUT_PULLUP for OFF state and  |
|   OUTPUT with LOW for ON state               |
| -Pump control button                         |
\=============================================*/

// Pins definitions
static constexpr uint8_t pinOfSensor[NB_OF_PUMPS] = {A3, A6, A7};   // Pins connected to capacity sensors 1-2-3
static constexpr uint8_t pinOfPump[NB_OF_PUMPS] = {A0, A1, A2};     // Pins connected to pump relays 1-2-3
static constexpr uint8_t pinOfAlarmSensor[NB_OF_PUMPS] = {7, 8, 9}; // Pins connected to leak resestive sensors 1-2-3 in digital mode
static constexpr uint8_t pinOfCntrlButton[NB_OF_PUMPS] = {4, 5, 6}; // Pins connected to control buttons 1-2-3
static constexpr uint8_t pinOfEncoder[3] = {10, 11, 12};            // Pins connected to encoder (A, B, last number is encbutton)
static constexpr uint8_t pinINT0StopButton = 2;                     // Pin of Emergency STOP button (and START too)
static constexpr uint8_t pinINT1AlarmSensors = 3;                   // Pin of Emergency STOP form Resestive Leak Sensors
static constexpr uint8_t pinAlarmLED = 13;                          // Pin of Alarm LED
// A4 - SDA, A5 - SCL, LCD connection

/* Enums */

// machine status
typedef enum
{
  _STOP,       // System halted
  _RUN,        // Sysytem works
  _SETUP_MODE, // System in setup mode
  _ALARM,      // Leaking detected
  _SENS_CALIB  // Sensor calibration mode
} ECurrStatus;

// initial data for pumping setting
tUnionSetting initPumpSetup[NB_OF_PUMPS]{
    // MinM(%), MaxM(%), PumpTime(sec), PumpPause(src), SensorAirValue, SensorWaterValue
    {{5, 50, 2, 10, 800, 200}},  // Pump 1 settings
    {{5, 50, 2, 10, 800, 200}},  // Pump 2 settings
    {{5, 50, 2, 10, 800, 200}}}; // Pump 3 settings

static String nameOfSetting[6] = {"minMo", "MAXMo", "PumpT", "PumpP", "SnAir", "SnWat"};
//=====================================

// hardware assignements
LiquidCrystal_I2C lcd(0x27, 16, 2);
RotaryEncoder encoder(pinOfEncoder[0], pinOfEncoder[1], RotaryEncoder::LatchMode::TWO03);
OneButton encoderBtn(pinOfEncoder[2], true);
OneButton startStopButton(pinINT0StopButton, true, true); // true for active LOW, true for pullup
// Global variables
uint8_t selectedPump = 0;
uint8_t settingIndex = 0;
uint32_t previousMillis = 0;
bool ledState = LOW;
bool backLightState = LOW;
String oldString1, oldString2;
String newString1, newString2;
uint8_t oldValue, newValue;
volatile ECurrStatus currentStatus = _STOP;

// PUMPER *myPump = new PUMPER[NB_OF_PUMPS];
PUMPER myPump[NB_OF_PUMPS];

const constexpr uint32_t WRITTEN_SIGNATURE = 0xBEEFDEED;
tUnionSetting pumpSetupFromEPR[NB_OF_PUMPS]; // array of pumps settings read from EEPROM
const uint32_t eepromSize = EEPROM.length();
const uint32_t storedAddress = sizeof(tUnionSetting) * NB_OF_PUMPS;

void memoryInit()
{
  DEBUG_PRINTLN(F("\nStart StoreFlashData on "));
  DEBUG_PRINT(F("EEPROM length: "));
  DEBUG_PRINTLN(eepromSize);
  // Check signature at address
  uint32_t a = 0;
  EEPROM.get(storedAddress, a);
  DEBUG_PRINTLN(F("read from EEPROM"));
  if (a != WRITTEN_SIGNATURE)
  {
    DEBUG_PRINTLN(F("Signature not found, writing initial data to EEPROM"));
    EEPROM.put(0, initPumpSetup);
    EEPROM.put(storedAddress, WRITTEN_SIGNATURE);
  }
}

void memoryReset()
{
  EEPROM.put(storedAddress, 0x66); // Clear signature to force re-writing of initial data on next start
  DEBUG_PRINTLN(F("\nMemory reset, signature cleared"));
  memoryInit();
}

void leakAlarmOn()
{
  delayMicroseconds(10);
  if (digitalRead(pinINT1AlarmSensors) == LOW) // Check if the pin is still LOW after debounce delay
  {
    currentStatus = _ALARM;
  }
}

void handleSensorsCalibration()
{
  uint16_t sensorValue;
  uint16_t sensReadB[3];

  for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
  {
    for (uint8_t j = 0; j < 10; j++) // Take 10 readings for more stable calibration values
    {
      sensReadB[j % 3] = analogRead(pinOfSensor[i]);
      delay(2);
    }

    if ((sensReadB[0] <= sensReadB[1] && sensReadB[1] <= sensReadB[2]) ||
        (sensReadB[0] >= sensReadB[1] && sensReadB[1] >= sensReadB[2]))
    {
      sensorValue = sensReadB[1];
    }
    else if ((sensReadB[1] <= sensReadB[0] && sensReadB[0] <= sensReadB[2]) ||
             (sensReadB[1] >= sensReadB[0] && sensReadB[0] >= sensReadB[2]))
    {
      sensorValue = sensReadB[0];
    }
    else
    {
      sensorValue = sensReadB[2];
    }

    if (sensorValue > 300) // Sensor in air!
    {
      pumpSetupFromEPR[i].D.sensAirValue = sensorValue;
    }
    else
    {
      pumpSetupFromEPR[i].D.sensWaterValue = sensorValue;
    }

    delay(10);
  }
}

// Non-blocking LED blink using millis()

void alarmLedBlink(uint32_t onTime, uint32_t offTime)
{
  uint32_t currentMillis = millis();
  uint32_t interval = ledState ? onTime : offTime;
  if (currentMillis - previousMillis >= interval)
  {
    ledState = !ledState;
    digitalWrite(pinAlarmLED, ledState);
    previousMillis = currentMillis;
  }
}

void displayInitPrint()
{
  ledState = HIGH;
  digitalWrite(pinAlarmLED, ledState);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Windowsill Water");
  lcd.setCursor(0, 1);
  lcd.print("Stagnator ");
  lcd.print(SketchVersion);
  delay(2000);
  ledState = LOW;
  digitalWrite(pinAlarmLED, ledState);
  lcd.clear();
}

void handleLED()
{
  if (currentStatus == _ALARM)
  {
    alarmLedBlink(200, 200);
    return;
  }

  for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
  {
    if (myPump[i].getStatus() == _OUT_OF_WATER || myPump[i].getStatus() == _ERROR)
    {
      alarmLedBlink(200, 2000);
      return;
    }
  }

  if (currentStatus == _STOP)
  {
    alarmLedBlink(2000, 200);
    return;
  }
  ledState = LOW;
  digitalWrite(pinAlarmLED, ledState);
}

// M% 99 55 33 +RUN
// ST SP 66 22 ER WT
// STOP RUN WAIT ERROR WATER
void handleLCD()
{
  newString1 = "M% ";
  for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
  {
    newString1 += String(myPump[i].getMoisture() < 10 ? "0" : "");
    newString1 += String(myPump[i].getMoisture());
    if (i < NB_OF_PUMPS - 1)
      newString1 += " ";
  }
  switch (currentStatus)
  {
  case _STOP:
    newString1 += " STOP";
    break;
  case _RUN:
    newString1 += " +RUN";
    break;
  case _SETUP_MODE:
    newString1 += " -SET";
    break;
  case _ALARM:
    newString1 += " ALRM";
    break;
  }
  if (newString1 != oldString1)
  {
    lcd.setCursor(0, 0);
    lcd.print(newString1.c_str());
    oldString1 = newString1;
  }

  newString2 = "ST ";

  for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
  {
    switch (myPump[i].getStatus())
    {
    case _WAITING:
      newString2 += String(myPump[i].getDesiredMoisture() < 10 ? "0" : "");
      newString2 += String(myPump[i].getDesiredMoisture()) + "W";
      break;
    case _RUNNING:
      newString2 += String(myPump[i].getDesiredMoisture() < 10 ? "0" : "");
      newString2 += String(myPump[i].getDesiredMoisture()) + "R";
      break;
    case _OUT_OF_WATER:
      newString2 += "WT ";
      break;
    case _STOP_PUMP:
      newString2 += "SP ";
      break;
    case _ERROR:
      newString2 += "ER ";
      break;
    case _ONE_TIME:
      newString2 += "OT ";
      break;
    }
    if (i < NB_OF_PUMPS - 1)
      newString2 += " ";
  }

  if (newString2 != oldString2)
  {
    lcd.setCursor(0, 1);
    lcd.print(newString2.c_str());
    oldString2 = newString2;
  }
}

void handleLCDSetupMode()
{
  lcd.clear();
  lcd.setCursor(0, 0);

  lcd.print("Pump N: ");
  lcd.print(selectedPump + 1);

  lcd.setCursor(0, 1);

  lcd.print("Set ");
  lcd.print(nameOfSetting[settingIndex]);
  lcd.print(": ");
  lcd.print(oldValue);
  if (settingIndex == 0 || settingIndex == 1) // For minMo and MAXMo settings, show percentage sign
  {
    lcd.print("%  ");
  }
  else
  {
    lcd.print("s  ");
  }
}

// buttons
void staticStartStopISR()
{
  startStopButton.tick();
}

void startStopButtonClick()
{
  if (currentStatus == _STOP)
  {
    currentStatus = _RUN;
    DEBUG_PRINTLN(F("Start button pressed, system started"));
  }
  else
  {
    currentStatus = _STOP;
    DEBUG_PRINTLN(F("Stop button pressed, system stopped"));
  }
}

void startStopButtonLongPress()
{
  memoryReset();
  for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
  {
    myPump[i].init();
  }
  lcd.clear();
  lcd.print("Memory reset!");
  delay(1000);
  lcd.clear();
  handleLCD();
}

void startStopButtonDoubleClick()
{
  if (backLightState == LOW)
  {
    backLightState = HIGH;
    lcd.backlight();
  }
  else
  {
    backLightState = LOW;
    lcd.noBacklight();
  }
}

void encBtnClick()
{
  switch (currentStatus)
  {
  case _SETUP_MODE:
    settingIndex++;
    if (settingIndex >= 4)
    {
      settingIndex = 0;
    }
    oldValue = pumpSetupFromEPR[selectedPump].B[settingIndex];
    encoder.setPosition(oldValue);
    handleLCDSetupMode();
    break;

  case _SENS_CALIB:
    handleSensorsCalibration();
    break;
  }
}

void encBtnDoubleClick()
{
  if (currentStatus != _SETUP_MODE)
  {
    settingIndex = 0;
    selectedPump = 0;
    oldValue = pumpSetupFromEPR[selectedPump].B[settingIndex];
    encoder.setPosition(oldValue);
    currentStatus = _SETUP_MODE;
    handleLCDSetupMode();
  }
  else
  {
    EEPROM.put(0, pumpSetupFromEPR);
    lcd.clear();

    for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
    {
      DEBUG_PRINT(F("Pump write to EEPROM, pump "));
      DEBUG_PRINT(i);
      DEBUG_PRINT(F(": "));
      DEBUG_PRINT(pumpSetupFromEPR[i].D.minM);
      DEBUG_PRINT(F("%, "));
      DEBUG_PRINT(pumpSetupFromEPR[i].D.maxM);
      DEBUG_PRINT(F("%, "));
      DEBUG_PRINT(pumpSetupFromEPR[i].D.pumpTime);
      DEBUG_PRINT(F("s, "));
      DEBUG_PRINT(pumpSetupFromEPR[i].D.pumpPause);
      DEBUG_PRINTLN(F("s"));
      DEBUG_PRINT(F("Sensor values: Air="));
      DEBUG_PRINT(pumpSetupFromEPR[i].D.sensAirValue);
      DEBUG_PRINT(F(", Water="));
      DEBUG_PRINTLN(pumpSetupFromEPR[i].D.sensWaterValue);
      myPump[i].init();
    }
    currentStatus = _RUN;
  }
}

void encBtnLongPressStart()
{
  switch (currentStatus)
  {
  case _STOP:
    currentStatus = _SENS_CALIB;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ALL sensrs CALBR"); 
    /* To calibrate sensors, put ALL sensors in water then pressing short button, then  
    put ALL sensors in air and press short button again. To save values and exit calibration mode*/
    lcd.setCursor(0, 1);
    lcd.print("Calibrating...  ");
    break;

  case _SETUP_MODE:
    selectedPump++;
    if (selectedPump >= NB_OF_PUMPS)
    {
      selectedPump = 0;
    }
    oldValue = pumpSetupFromEPR[selectedPump].B[settingIndex];
    encoder.setPosition(oldValue);
    handleLCDSetupMode();
    break;

  case _SENS_CALIB:
    EEPROM.put(0, pumpSetupFromEPR);
    lcd.clear();
    lcd.print("Calibr finished!");
    delay(1000);

    for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
    {
      DEBUG_PRINT(F("Calibr write to EEPROM, pump "));
      DEBUG_PRINT(i);
      DEBUG_PRINT(F(": "));
      DEBUG_PRINT(pumpSetupFromEPR[i].D.minM);
      DEBUG_PRINT(F("%, "));
      DEBUG_PRINT(pumpSetupFromEPR[i].D.maxM);
      DEBUG_PRINT(F("%, "));
      DEBUG_PRINT(pumpSetupFromEPR[i].D.pumpTime);
      DEBUG_PRINT(F("s, "));
      DEBUG_PRINT(pumpSetupFromEPR[i].D.pumpPause);
      DEBUG_PRINTLN(F("s"));
      DEBUG_PRINT(F("Sensor values: Air="));
      DEBUG_PRINT(pumpSetupFromEPR[i].D.sensAirValue);
      DEBUG_PRINT(F(", Water="));
      DEBUG_PRINTLN(pumpSetupFromEPR[i].D.sensWaterValue);
      myPump[i].init();
    }
    currentStatus = _STOP;
    break;

  default:
    break;
  }
}

void setup()
{
  Wire.begin();
  Wire.setClock(100000); // Set I2C clock to 100kHz

#ifdef DEBUG_ENABLE
  Serial.begin(57600); // Init serial output for debug
  delay(2000);         // 2 seconds delay for stable start and to read initial debug messages
#endif

  pinMode(pinAlarmLED, OUTPUT);

  DEBUG_PRINT(F("StartStart_ver: "));
  DEBUG_PRINTLN(String(SketchVersion));
  /*#if defined(DEBUG_ENABLE)
    DEBUG_PRINTLN(F("Debug mode enabled Resetting memory for testing purposes"));
    memoryReset(); // Clear EEPROM for testing purposes, comment out in production
  #endif*/
  memoryInit();
  EEPROM.get(0, pumpSetupFromEPR);

  lcd.init();
  backLightState = HIGH;
  lcd.backlight();

  displayInitPrint();

  pinMode(pinOfEncoder[0], INPUT); // my encoder does not work withouot this settings!
  pinMode(pinOfEncoder[1], INPUT);
  // Setup encoder button
  encoderBtn.setLongPressIntervalMs(900);
  encoderBtn.attachClick(encBtnClick);
  encoderBtn.attachDoubleClick(encBtnDoubleClick);
  encoderBtn.attachLongPressStart(encBtnLongPressStart);
  // Setup external interrupts for STOP button and leak sensors
  pinMode(pinINT0StopButton, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinINT0StopButton), staticStartStopISR, FALLING);
  pinMode(pinINT1AlarmSensors, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinINT1AlarmSensors), leakAlarmOn, FALLING);
  startStopButton.setLongPressIntervalMs(3000);
  startStopButton.attachClick(startStopButtonClick);
  startStopButton.attachLongPressStart(startStopButtonLongPress);
  startStopButton.attachDoubleClick(startStopButtonDoubleClick);

  for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
  {
    myPump[i] = PUMPER(i, pinOfSensor[i], pinOfPump[i], pinOfAlarmSensor[i], pinOfCntrlButton[i]);
    myPump[i].init();
  }

#ifdef DEBUG_ENABLE
  currentStatus = _STOP;
#else
  currentStatus = _RUN;
#endif

  oldString1.reserve(16);
  oldString1 = "                ";
  oldString2.reserve(16);
  oldString2 = "                ";
  newString1.reserve(16);
  newString2.reserve(16);
  handleLCD();
}

void loop()
{
  encoderBtn.tick();
  startStopButton.tick();
  switch (currentStatus)
  {
  case _ALARM:
  case _STOP:
    for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
      if (myPump[i].getStatus() != _STOP_PUMP)
      {
        myPump[i].stopIt();
      }
    handleLCD();
    break;

  case _RUN:
    for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
      myPump[i].handlePump();
    handleLCD();
    break;

  case _SETUP_MODE:

    encoder.tick();
    long encPos = encoder.getPosition();
    newValue = constrain(encPos, 0, 99);

    if (newValue != oldValue)
    {

      pumpSetupFromEPR[selectedPump].B[settingIndex] = newValue;

      lcd.setCursor(0, 0);
      DEBUG_PRINT(F("Pump N: "));
      DEBUG_PRINTLN(selectedPump + 1);
      lcd.print("Pump N: ");
      lcd.print(selectedPump + 1);

      lcd.setCursor(0, 1);
      DEBUG_PRINT(F("Set "));
      DEBUG_PRINT(nameOfSetting[settingIndex]);
      lcd.print("Set ");
      lcd.print(nameOfSetting[settingIndex]);
      DEBUG_PRINT(": ");
      DEBUG_PRINT(newValue);
      lcd.print(": ");
      lcd.print(newValue);
      DEBUG_PRINTLN(F("sec(%)"));
      lcd.print("s-%");
      oldValue = newValue;
    }
    break;
  case _SENS_CALIB:
    
    break;
  }

  handleLED();
#ifdef DEBUG_ENABLE
  delay(20); // Small delay in debug mode to avoid flooding the serial output
#endif
}
