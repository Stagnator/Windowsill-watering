// #define DEBUG
//==========================================================//
// Windowsill garden watering sysytem for three pumping zones//
//==========================================================//

/************************************************/
#define SketchVersion "v 0.30"
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
// A4 - SDA, A5 - SCL, LCD connection

static constexpr uint8_t maxPumpCykles = 50; // Max count of pumping cykles to reach desired moisture level (for safety reasons)

/* Enums */

// machine status
typedef enum
{
  _STOP,       // System halted
  _RUN,        // Sysytem works
  _SETUP_MODE, // System in setup mode
  _ALARM       // Leaking detected
} ECurrStatus;

#pragma pack(push, 1)
struct pumpSetting
{
  uint8_t minM;      // Min moisture to start watering (0-99)
  uint8_t maxM;      // Max moisture to stop watering (0-99)
  uint8_t pumpTime;  // Time of pumping in seconds (0-10)
  uint8_t pumpPause; // Time of pause between pump cykles in seconds (0-20)
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
// Global variables
uint8_t selectedPump = 0;
uint8_t settingIndex = 0;
unsigned long previousMillis = 0;
bool ledState = LOW;
String oldString1, oldString2;
String newString1, newString2;
ECurrStatus currentStatus = _STOP;

PUMPER *myPump = new PUMPER[NB_OF_PUMPS];

const unsigned int WRITTEN_SIGNATURE = 0xBEEFDEED;
tUnionSetting pumpSetupFromEPR[NB_OF_PUMPS]; // array of pumps settings read from EEPROM

void memoryInit()
{
  // Check signature at address
  unsigned int storedAddress = sizeof(tUnionSetting) * NB_OF_PUMPS;
  unsigned int a = 0;
  EEPROM.get(storedAddress, a);
  if (a != WRITTEN_SIGNATURE)
  {
    EEPROM.put(0, initPumpSetup);
    EEPROM.put(storedAddress, WRITTEN_SIGNATURE);
  }
  EEPROM.get(0, pumpSetupFromEPR);
  for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
  {
    Serial.print("Pump read from EEPROM, pump ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(pumpSetupFromEPR[i].D.minM);
    Serial.print("%, ");
    Serial.print(pumpSetupFromEPR[i].D.maxM);
    Serial.print("%, ");
    Serial.print(pumpSetupFromEPR[i].D.pumpTime);
    Serial.print("s, ");
    Serial.print(pumpSetupFromEPR[i].D.pumpPause);
    Serial.println("s");
  }
}

void startStop()
{
  if (currentStatus == _STOP)
  {
    currentStatus = _RUN;
  }
  else
  {
    currentStatus = _STOP;
  }
}

void leakAlarmOn()
{
  currentStatus = _ALARM;
}

void displayInitPrint()
{
  lcd.setCursor(0, 0);
  lcd.print("Windowsill Water");
  lcd.setCursor(0, 1);
  lcd.print("Stagnator ");
  lcd.print(SketchVersion);
  delay(2000);
}

// M% 99 55 33 44 5
// ST SP 66 22 ER WT
// STOP RUN WAIT ERROR WATER
void handleLCD()
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
    break;
  case _RUN:
    newString1 += " RUN";
    break;
  case _SETUP_MODE:
    newString1 += " SET";
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
      newString2 += String(myPump[i].getMoisture());
      break;
    case _RUNNING:
      newString2 += String(myPump[i].getMoisture());
      break;
    case _OUT_OF_WATER:
      newString2 += "WT";
      alarmLedBlink(1000);
      break;
    case _STOP_PUMP:
      newString2 += "SP";
      break;
    case _ERROR:
      newString2 += "ER";
      alarmLedBlink(500);
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

// Non-blocking LED blink using millis()
void alarmLedBlink(unsigned long interval)
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    ledState = !ledState;
    digitalWrite(pinAlarmLED, ledState);
  }
}

void setup()
{
  Wire.begin();
  Serial.begin(115200); // Init serial output for debug
  while (!Serial)
    ; // Needed only for built-in USB ports.

  pinMode(pinAlarmLED, OUTPUT);

  Serial.println("StartStart_ver: " + String(SketchVersion));
  memoryInit();
  lcd.init();
  lcd.backlight();
  // lcd.noBacklight();

  for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
    myPump[i] = PUMPER(i, pinOfSensor[i], pinOfPump[i], pinOfAlarmSensor[i], pinOfCntrlButton[i]);
  for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
    myPump[i].init();

  displayInitPrint();

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

  currentStatus = _RUN;

  pinMode(pinINT0StopButton, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinINT0StopButton), startStop, FALLING);

  pinMode(pinINT1AlarmSensors, INPUT);
  attachInterrupt(digitalPinToInterrupt(pinINT1AlarmSensors), leakAlarmOn, FALLING);
}

void loop()
{
  if (currentStatus == _ALARM)
  {
    for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
      myPump[i].stopIt();
    alarmLedBlink(200);
  }

  if (currentStatus == _RUN)
  {
    ledState = LOW;
    digitalWrite(pinAlarmLED, ledState);
    for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
      myPump[i].pumpIt();
  }

  if (currentStatus == _STOP)
  {
    for (uint8_t i = 0; i < NB_OF_PUMPS; i++)
      myPump[i].stopIt();
    ledState = HIGH;
    digitalWrite(pinAlarmLED, ledState);
  }

  if (currentStatus == _SETUP_MODE)
  {
    encoder.tick();

    int oldValue;
    int newValue = encoder.getPosition();

    pumpSetupFromEPR[selectedPump].B[settingIndex] = newValue;

    lcd.setCursor(0, 0);
    lcd.print("Pump: ");
    lcd.print(selectedPump);
    lcd.setCursor(0, 1);
    lcd.print("Setting ");
    lcd.print(nameOfSetting[settingIndex]);
    lcd.print(": ");
    lcd.print(newValue);
    oldValue = newValue;
  }

  handleLCD();
  encoderBtn.tick();
}
