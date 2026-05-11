// #define DEBUG
//==========================================================//
// Windowsill garden watering sysytem for three pumping zones//
//==========================================================//

/************************************************/
#define SketchVersion "v 0.40"
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
|   I have very sesetiv modules, so need to set|
|   pinMode to INPUT_PULLUP for OFF state and  |
|   OUTPUT with LOW for ON state               |
| -Pump control button                         |
\=============================================*/

// Pins definitions
static constexpr int pinOfSensor[NB_OF_PUMPS] = {A3, A6, A7};       // Pins connected to capacity sensors 1-2-3
static constexpr int pinOfPump[NB_OF_PUMPS] = {A0, A1, A2};         // Pins connected to pump relays 1-2-3
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
  _WATER_OUT,  // Water storage is empty
  _SETUP_MODE, // System in setup mode
  _ALARM       // Leaking detected
} ECurrStatus;

// initial data for pumping setting
tUnionSetting initPumpSetup[NB_OF_PUMPS]{
    // MinM(%), MaxM(%), PumpTime(sec), PumpPause(src)
    {{20, 60, 2, 10}},
    {{20, 60, 2, 10}},
    {{20, 60, 2, 10}}}; // array of pumps settings

static String nameOfSetting[sizeof(tUnionSetting) / sizeof(uint8_t)] = {"minMo", "MAXMo", "PumpT", "PumpP"};
//=====================================

// hardware assignements
LiquidCrystal_I2C lcd(0x27, 16, 2);
RotaryEncoder encoder(pinOfEncoder[0], pinOfEncoder[1], RotaryEncoder::LatchMode::TWO03);
OneButton encoderBtn(pinOfEncoder[2], true);
OneButton startStopButton(pinINT0StopButton, true, true); // true for active LOW, true for pullup
// Global variables
uint8_t selectedPump = 0;
uint8_t settingIndex = 0;
unsigned long previousMillis = 0;
bool ledState = LOW;
String oldString1, oldString2;
String newString1, newString2;
uint8_t oldValue, newValue;
volatile ECurrStatus currentStatus = _STOP;

PUMPER *myPump = new PUMPER[NB_OF_PUMPS];

const unsigned long WRITTEN_SIGNATURE = 0xBEEFDEED;
tUnionSetting pumpSetupFromEPR[NB_OF_PUMPS]; // array of pumps settings read from EEPROM
const int eepromSize = EEPROM.length();
const unsigned int storedAddress = sizeof(tUnionSetting) * NB_OF_PUMPS;

void memoryInit()
{
  DEBUG_PRINTLN(F("\nStart StoreFlashData on "));
  DEBUG_PRINT(F("EEPROM length: "));
  DEBUG_PRINTLN(eepromSize);
  // Check signature at address
  unsigned long a = 0;
  EEPROM.get(storedAddress, a);
  if (a != WRITTEN_SIGNATURE)
  {
    DEBUG_PRINTLN(F("SignTURE not found, writing initial data to EEPROM"));
    EEPROM.put(0, initPumpSetup);
    EEPROM.put(storedAddress, WRITTEN_SIGNATURE);
  }
}

void memoryReset()
{
  EEPROM.put(storedAddress, 0x66); // Clear signature to force re-writing of initial data on next start
  DEBUG_PRINTLN(F("Memory reset, signature cleared"));
  memoryInit();
}

void startStop()
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

void leakAlarmOn()
{
  currentStatus = _ALARM;
}

// Non-blocking LED blink using millis()

void alarmLedBlink(unsigned long onTime, unsigned long offTime)
{
  unsigned long currentMillis = millis();
  unsigned long interval = ledState ? onTime : offTime;
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

// M% 99 55 33 44 5
// ST SP 66 22 ER WT
// STOP RUN WAIT ERROR WATER
void handleLCDandLED()
{
  newString1 = "M% ";
  for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
  {
    newString1 += String(myPump[i].getMoisture());
    if (i < NB_OF_PUMPS - 1)
      newString1 += " ";
  }
  switch (currentStatus)
  {
  case _STOP:
    newString1 += " STOP";
    alarmLedBlink(1000, 200);
    break;
  case _RUN:
    newString1 += " +RUN";
    break;
  case _SETUP_MODE:
    newString1 += " -SET";
    break;
  case _ALARM:
    newString1 += " ALRM";
    alarmLedBlink(200, 200);
    break;
  case _WATER_OUT:
    newString1 += " +RUN";
    alarmLedBlink(200, 1000);
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
      newString2 += String(myPump[i].getDesiredMoisture()) + "W";
      break;
    case _RUNNING:
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

  lcd.print("s-%");
}

// buttons
void staticStartStopISR()
{
  startStopButton.tick();
}

void startStopButtonClick()
{
  startStop();
}

void startStopButtonLongPress()
{
  memoryReset();
  for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
  {
    myPump[i].init();
  }
  handleLCDandLED();
}

void encBtnClick()
{
  settingIndex++;
  if (settingIndex >= sizeof(tUnionSetting) / sizeof(uint8_t))
  {
    settingIndex = 0;
  }
  oldValue = pumpSetupFromEPR[selectedPump].B[settingIndex];
  encoder.setPosition(oldValue);
  handleLCDSetupMode();
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

    handleLCDSetupMode();

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
      myPump[i].init();
    }
    currentStatus = _RUN;
  }
}

void encBtnLongPressStart()
{
  selectedPump++;
  if (selectedPump >= NB_OF_PUMPS)
  {
    selectedPump = 0;
  }
  oldValue = pumpSetupFromEPR[selectedPump].B[settingIndex];
  encoder.setPosition(oldValue);
  handleLCDSetupMode();
}

void setup()
{
  Wire.begin();
  Wire.setClock(100000); // Set I2C clock to 100kHz
                         // Wire.setWireTimeout(3000, true); // Таймаут 3мс, сбрасывать шину при зависании
#ifdef DEBUG_ENABLE
  Serial.begin(57600); // Init serial output for debug
  delay(2000);         // 2 seconds delay for stable start and to read initial debug messages
#endif
  EEPROM.begin(); // Init EEPROM for LGT8F328P

  pinMode(pinAlarmLED, OUTPUT);

  DEBUG_PRINT(F("StartStart_ver: "));
  DEBUG_PRINTLN(String(SketchVersion));
  memoryInit();

  lcd.init();
  lcd.backlight();
  // lcd.noBacklight();
  displayInitPrint();

  for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
  {
    myPump[i] = PUMPER(i, pinOfSensor[i], pinOfPump[i], pinOfAlarmSensor[i], pinOfCntrlButton[i]);
    myPump[i].init();
  }

  // Setup encoder button
  encoderBtn.setLongPressIntervalMs(800);
  encoderBtn.attachClick(encBtnClick);
  encoderBtn.attachDoubleClick(encBtnDoubleClick);
  encoderBtn.attachLongPressStart(encBtnLongPressStart);
  // Setup external interrupts for STOP button and leak sensors
  pinMode(pinINT0StopButton, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinINT0StopButton), staticStartStopISR, FALLING);
  pinMode(pinINT1AlarmSensors, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinINT1AlarmSensors), leakAlarmOn, FALLING);
  startStopButton.attachClick(startStopButtonClick);
  startStopButton.attachLongPressStart(startStopButtonLongPress);

  EEPROM.get(0, pumpSetupFromEPR);

  /*for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
  {
    DEBUG_PRINT(F("Pump read from EEPROM, pump "));
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

  }*/

  currentStatus = _STOP;

  oldString1.reserve(16);
  oldString1 = "                ";
  oldString2.reserve(16);
  oldString2 = "                ";
  newString1.reserve(16);
  newString2.reserve(16);
  handleLCDandLED();
}

void loop()
{
  switch (currentStatus)
  {
  case _ALARM:
    for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
      myPump[i].stopIt();
    handleLCDandLED();
    break;

  case _RUN:
    for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
      myPump[i].handlePump();
    handleLCDandLED();
    break;

  case _STOP:
    for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
      myPump[i].stopIt();
    handleLCDandLED();
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
  }

  encoderBtn.tick();
  startStopButton.tick();
  delay(10);
}
