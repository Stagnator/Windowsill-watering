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
  pumpStatus = _WAITING;
  pumpPinState = OFF;
  pinMode(sensPinNo, INPUT);
  pinMode(alarmPinNo, INPUT);
  readDataEPR();
  pumpOnOff(OFF); // OFF
  rawCurrMoist = 0;
  

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
  uint16_t mstReadB[3];    // Bufer for raw sensor readings to median
  uint16_t medianRaw;      // Raw sensor reading and median value for more stable readings

  for (uint8_t i = 0; i < 3; ++i)
  {
    medianRaw = analogRead(sensPinNo);
    mstReadB[i] = constrain(medianRaw, pumpSetup.D.sensWaterValue, pumpSetup.D.sensAirValue); // Constrain raw sensor reading to calibration values to avoid false triggering of pump due to sensor errors or out of range readings
    delay(SensorSampleDelayMs);
  }

  if ((mstReadB[0] <= mstReadB[1] && mstReadB[1] <= mstReadB[2]) ||
      (mstReadB[0] >= mstReadB[1] && mstReadB[1] >= mstReadB[2]))
  {
    medianRaw = mstReadB[1];
  }
  else if ((mstReadB[1] <= mstReadB[0] && mstReadB[0] <= mstReadB[2]) ||
           (mstReadB[1] >= mstReadB[0] && mstReadB[0] >= mstReadB[2]))
  {
    medianRaw = mstReadB[0];
  }
  else
  {
    medianRaw = mstReadB[2];
  }
  DEBUG_PRINT(F("Median raw sensor reading: "));
  DEBUG_PRINTLN(medianRaw);

  rawCurrMoist = ((alfaConst_x10 * medianRaw + (10 - alfaConst_x10) * rawCurrMoist)) / 10; // EMA filter for moisture readings (to stabilize the readings and avoid false triggering of pump)  
  DEBUG_PRINT(F("Filtered raw sensor reading: "));
  DEBUG_PRINTLN(rawCurrMoist);
  currMoist = constrain(map(rawCurrMoist, pumpSetup.D.sensWaterValue, pumpSetup.D.sensAirValue, 99, 0), 0, 99); // Map raw sensor reading to moisture percentage and constrain to 0-99%
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
  pumpSetup.D.pumpTime = constrain(pumpSetup.D.pumpTime, 0, 10);
  pumpSetup.D.pumpPause = constrain(pumpSetup.D.pumpPause, 0, 20);
  pumpSetup.D.sensAirValue = constrain(pumpSetup.D.sensAirValue, 500, 800); // Constrain sensor calibration values to reasonable range
  pumpSetup.D.sensWaterValue = constrain(pumpSetup.D.sensWaterValue, 100, 300);
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
  if (currentMillis - previousMillis >= interval)
  {
    pumpPinState = !pumpPinState;
    runUpCounter++;
    if (runUpCounter >= maxPumpCykles * 2)
    {
      pumpStatus = _ERROR;
      pumpPinState = OFF;
    }
    pumpOnOff(pumpPinState);
    previousMillis = currentMillis;
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
