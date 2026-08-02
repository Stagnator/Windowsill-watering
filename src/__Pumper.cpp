#include <Arduino.h>
#include <EEPROM.h>
#include "debug.h"
#include "_Pumper.h"

PUMPER::PUMPER() {}

PUMPER::PUMPER(const uint8_t i, const uint8_t sensPin, const uint8_t pumpPin, const uint8_t alarmPin, const uint8_t buttonPin)
    : pumpNo(i), pumpBtn(buttonPin, true, true), sensPinNo(sensPin), pumpPinNo(pumpPin), alarmPinNo(alarmPin)
{
}

void PUMPER::init()
{
  pinMode(sensPinNo, INPUT);
  pinMode(alarmPinNo, INPUT);
  readDataEPR(); // Read pump settings from EEPROM
  
  rawCurrMoist = (uint32_t)pumpSetup.D.sensWaterValue << 8;
  prevMillisPump = 0;
  prevMillisSens = 0;

  pumpBtn.setPressMs(800);
  pumpBtn.attachClick(staticClickHandler, this);
  pumpBtn.attachDoubleClick(staticDoubleClickHandler, this);
  pumpBtn.attachLongPressStart(staticLongPressStartHandler, this);
  pumpBtn.attachDuringLongPress(staticDuringLongPressHandler, this);
  pumpBtn.attachLongPressStop(staticLongPressStopHandler, this);

  if (isStorageEmpty())
  {
    pumpStatus = _OUT_OF_WATER;
    DEBUG_PRINT(F("pump out of water: "));
    DEBUG_PRINTLN(pumpNo);
  }

  DEBUG_PRINT(F("pump ready: "));
  DEBUG_PRINTLN(pumpNo);
  pumpOnOff(ON); // check if pump is working (for LOW active pump)
  delay(500); // Wait for pump to stabilize
  pumpOnOff(OFF); // OFF
}

void PUMPER::pumpOnOff(bool on)
{
  if (on)
    pinMode(pumpPinNo, INPUT_PULLUP); // High impedance state, pump OFF (for LOW active pump). Got very sesetive modules
  else
  {
    pinMode(pumpPinNo, OUTPUT);
    digitalWrite(pumpPinNo, pumpPinState); // pumpPinState is LOW for ON, High impedance state for OFF (for LOW active pump)
  }
}

void PUMPER::pumpGo()
{

  if (pumpStatus == _WAITING)
  {
    pumpStatus = _RUNNING;
  }
  else if (pumpStatus == _RUNNING)
  {
    pumpStatus = _WAITING;
  }
}

void PUMPER::readMoisture()
{
  uint32_t currentMillis = millis();
  uint8_t underRunning = (pumpStatus == _RUNNING) ? 4 : 1;
  
  if (currentMillis - prevMillisSens >= SensorSampleIntervalMs/underRunning)
  {
    analogRead(sensPinNo);
    delayMicroseconds(100); // Delay to stabilize the analog input
    uint32_t rawSensRead = analogRead(sensPinNo);
    if (rawSensRead > pumpSetup.D.sensAirValue+150 || rawSensRead < pumpSetup.D.sensWaterValue-150) // If the sensor reading is significantly higher than the air calibration value, it may be due to residual charge in the sensor. Discharge it.
    {
      DEBUG_PRINTLN(F("Discharging sensor!"));
      pinMode(sensPinNo, OUTPUT); 
      digitalWrite(sensPinNo, LOW); // Set pin to LOW to discharge the sensor
      delayMicroseconds(100); // Wait for a short time to allow the sensor to discharge
      pinMode(sensPinNo, INPUT); // Set pin back to INPUT mode
      delayMicroseconds(100); // Wait for a short time to stabilize the analog input
      analogRead(sensPinNo); // Read the sensor value again
      delayMicroseconds(200); // Delay to stabilize the analog input
      rawSensRead = analogRead(sensPinNo); // Read the sensor value again
    }
    DEBUG_PRINT(F("Sensor "));
    DEBUG_PRINT(sensPinNo);
    DEBUG_PRINT(F(" reading: "));
    DEBUG_PRINTLN(rawSensRead);
    rawCurrMoist = rawCurrMoist + K * (rawSensRead - (rawCurrMoist >> 8)); // EMA filter for moisture readings (to stabilize the readings and avoid false triggering of pump)
    uint16_t filteredValue = rawCurrMoist >> 8;
    currMoist = constrain(map(filteredValue, pumpSetup.D.sensAirValue, pumpSetup.D.sensWaterValue, 0, 99), 0, 99); // Map raw sensor reading to moisture percentage and constrain to 0-99%
    prevMillisSens = currentMillis;
  }
}

void PUMPER::onePump()
{
  pumpStatus = _ONE_TIME;
} //

bool PUMPER::isStorageEmpty()
{
  return digitalRead(alarmPinNo);
} //

void PUMPER::readDataEPR()
{
  EEPROM.get(pumpNo * sizeof(tUnionSetting), pumpSetup);

  pumpSetup.D.minM = constrain(pumpSetup.D.minM, 1, 99); // Validate settings read from EEPROM
  pumpSetup.D.maxM = constrain(pumpSetup.D.maxM, 1, 99);
  if (pumpSetup.D.maxM < pumpSetup.D.minM)
  {
    pumpSetup.D.maxM = pumpSetup.D.minM;
  }
  pumpSetup.D.pumpTime = constrain(pumpSetup.D.pumpTime, 0, 99);
  pumpSetup.D.pumpPause = constrain(pumpSetup.D.pumpPause, 0, 99);
  // pumpSetup.D.sensAirValue = constrain(pumpSetup.D.sensAirValue, 700, 1024); // Constrain sensor calibration values to reasonable range
  // pumpSetup.D.sensWaterValue = constrain(pumpSetup.D.sensWaterValue, 100, 690);
  DEBUG_PRINT(F("Pump read from EEPROM, pump "));
  DEBUG_PRINT(pumpNo);
  DEBUG_PRINT(F(": "));
  DEBUG_PRINT(pumpSetup.D.minM);
  DEBUG_PRINT(F("%, "));
  DEBUG_PRINT(pumpSetup.D.maxM);
  DEBUG_PRINT(F("%, "));
  DEBUG_PRINT(pumpSetup.D.pumpTime);
  DEBUG_PRINT(F("s, "));
  DEBUG_PRINT(pumpSetup.D.pumpPause);
  DEBUG_PRINTLN(F("s"));
  DEBUG_PRINT(F("Sensor values: Air="));
  DEBUG_PRINT(pumpSetup.D.sensAirValue);
  DEBUG_PRINT(F(", Water="));
  DEBUG_PRINTLN(pumpSetup.D.sensWaterValue);
}

void PUMPER::diasableEnablePump()
{
  if (pumpStatus == _STOP_PUMP || pumpStatus == _ERROR)
  {
    runUpCounter = 0;
    pumpStatus = _WAITING;
  }
  else
  {
    if (pumpPinState != OFF)
    {
      pumpPinState = OFF;
      pumpOnOff(pumpPinState);
    }
    pumpStatus = _STOP_PUMP;
  }

} //

void PUMPER::nonBlockingPumpRun(uint32_t onTime, uint32_t offTime)
{
  uint32_t currentMillis = millis();
  uint32_t interval = !pumpPinState ? onTime : offTime;
  DEBUG_PRINTLN(currentMillis - prevMillisPump);
  if (currentMillis - prevMillisPump >= interval)
  {
    pumpPinState = !pumpPinState;
    runUpCounter++;
    if (runUpCounter >= maxPumpCykles * 2)
    {
      pumpStatus = _ERROR;
      pumpPinState = OFF;
    }
    pumpOnOff(pumpPinState);
    DEBUG_PRINT(F("Pump NO: "));
    DEBUG_PRINTLN(pumpNo);
    DEBUG_PRINT(F("Pump state: "));
    DEBUG_PRINTLN(!pumpPinState ? F("ON") : F("OFF"));
    prevMillisPump = currentMillis;
  }
}

void PUMPER::stopIt()
{
  if (pumpPinState != OFF)
  {
    pumpPinState = OFF;
    pumpOnOff(pumpPinState);
  }
  runUpCounter = 0;
  pumpStatus = _STOP_PUMP;
}

/*void PUMPER::tick()
{
  pumpBtn.tick();
}*/

void PUMPER::handlePump()
{
  pumpBtn.tick();
  readMoisture();

  if (isStorageEmpty())
  {
    if (pumpStatus != _OUT_OF_WATER)
    {
      pumpStatus = _OUT_OF_WATER;
      DEBUG_PRINT(F("pump out of water: "));
      DEBUG_PRINTLN(pumpNo);
      if (pumpPinState != OFF)
      {
        pumpPinState = OFF;
        pumpOnOff(pumpPinState);
      }
      runUpCounter = 0;
    }
  }

  switch (pumpStatus)
  {

  case _ERROR:

  case _STOP_PUMP:

    break;

  case _OUT_OF_WATER:
    if (!isStorageEmpty())
    {
      pumpStatus = _WAITING;
    }

    break;

  case _WAITING:
    if (pumpPinState != OFF)
    {
      pumpPinState = OFF;
      pumpOnOff(pumpPinState);
      runUpCounter = 0;
    }
    if (currMoist <= pumpSetup.D.minM)
    {
      pumpStatus = _RUNNING;
    }
    break;

  case _RUNNING:
    if (currMoist >= pumpSetup.D.maxM)
    {
      pumpPinState = OFF;
      pumpOnOff(pumpPinState);
      runUpCounter = 0;
      pumpStatus = _WAITING;
    }
    else
    {
      nonBlockingPumpRun(pumpSetup.D.pumpTime * 1000, pumpSetup.D.pumpPause * 1000);
    
    }
    break;

  case _ONE_TIME:
    nonBlockingPumpRun(pumpSetup.D.pumpTime * 1000, 0);
    if (pumpPinState == OFF)
    {
      runUpCounter = 0;
      pumpStatus = _STOP_PUMP;
    }
    break;
  }
}

EStatusOfPump PUMPER::getStatus()
{
  readMoisture();
  return pumpStatus;
}

uint8_t PUMPER::getMoisture()
{
  return currMoist;
}

uint8_t PUMPER::getDesiredMoisture()
{
  if (pumpStatus == _WAITING)
    return pumpSetup.D.minM;
  else
    return pumpSetup.D.maxM;
}

// Статический посредник
void PUMPER::staticClickHandler(void *scope)
{
  // Приводим указатель обратно к типу нашего класса
  PUMPER *instance = (PUMPER *)scope;
  instance->ClickFunction(); // Вызываем обычный метод
}

void PUMPER::staticDoubleClickHandler(void *scope)
{
  PUMPER *instance = (PUMPER *)scope;
  instance->DoubleClickFunction();
}

void PUMPER::staticLongPressStartHandler(void *scope)
{
  PUMPER *instance = (PUMPER *)scope;
  instance->LongPressStart();
}

void PUMPER::staticDuringLongPressHandler(void *scope)
{
  PUMPER *instance = (PUMPER *)scope;
  instance->DuringLongPress();
}

void PUMPER::staticLongPressStopHandler(void *scope)
{
  PUMPER *instance = (PUMPER *)scope;
  instance->LongPressStop();
}

//-------------------------------------button----------------
void PUMPER::LongPressStart()
{

  DEBUG_PRINT(F("\t - LongPressStart()"));
  DEBUG_PRINTLN(pumpNo);
  diasableEnablePump();
}

void PUMPER::LongPressStop()
{
  DEBUG_PRINT(F("\t - LongPressStop()\n"));
  DEBUG_PRINTLN(pumpNo);
}

void PUMPER::DuringLongPress()
{
  DEBUG_PRINT(F("\t - DuringLongPress()"));
  DEBUG_PRINTLN(pumpNo);
}

void PUMPER::ClickFunction()
{
  onePump();
  DEBUG_PRINT(F("\t - ClickFunction()"));
  DEBUG_PRINTLN(pumpNo);
} // ClickFunction

void PUMPER::DoubleClickFunction()
{
  pumpGo();
  DEBUG_PRINT(F("\t - DoubleClickFunction()"));
  DEBUG_PRINTLN(pumpNo);
} // DoubleClickFunction
