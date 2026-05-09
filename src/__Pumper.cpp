#include <Arduino.h>
#include <EEPROM.h>
#include "debug.h"
#include "_Pumper.h"


PUMPER::PUMPER() {}

PUMPER::PUMPER(const int i, const int sensPin, const int pumpPin, const uint8_t alarmPin, const uint8_t buttonPin)
    : pumpNo(i), pumpBtn(buttonPin, true, true), sensPinNo(sensPin), pumpPinNo(pumpPin), alarmPinNo(alarmPin)
{
}

void PUMPER::init()
{
  pumpStatus = _WAITING;
  pumpPinState = LOW;
  currMoist = 0;
  readMoisture();
  readDataEPR();

  pinMode(sensPinNo, INPUT);
  pinMode(alarmPinNo, INPUT);
  pinMode(pumpPinNo, OUTPUT);
  pumpBtn.setLongPressIntervalMs(800);
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

void PUMPER::pumpGo()
{
  if (isStorageEmpty())
  {
    pumpStatus = _OUT_OF_WATER;
    return;
  }
  else
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
}

void PUMPER::readMoisture()
{
  int soilMoistureValue = analogRead(sensPinNo);
  currMoist = constrain(map(soilMoistureValue, AirValue, WaterValue, 0, 100), 1, 99);
}

void PUMPER::onePump()
{
  if (pumpStatus == _OUT_OF_WATER)
    pumpStatus = _WAITING;

  if (isStorageEmpty())
  {
    pumpStatus = _OUT_OF_WATER;
    return;
  }
  nonBlockingPumpRun(pumpSetup.D.pumpTime * 1000, 0);
} //

bool PUMPER::isStorageEmpty()
{
  return digitalRead(alarmPinNo);
} //

void PUMPER::readDataEPR()
{
  EEPROM.get(pumpNo * sizeof(tUnionSetting), pumpSetup);
  pumpSetup.D.minM = constrain(pumpSetup.D.minM, 0, 99); // Validate settings read from EEPROM
  pumpSetup.D.maxM = constrain(pumpSetup.D.maxM, 0, 99);
  if (pumpSetup.D.maxM < pumpSetup.D.minM)
  {
    pumpSetup.D.maxM = pumpSetup.D.minM;
  }
  pumpSetup.D.pumpTime = constrain(pumpSetup.D.pumpTime, 0, 10);
  pumpSetup.D.pumpPause = constrain(pumpSetup.D.pumpPause, 0, 20);
}

void PUMPER::diasableEnablePump()
{
  if (pumpStatus == _STOP_PUMP || pumpStatus == _ERROR)
  {
    pumpStatus = _WAITING;
    runUpCounter = 0;
  }
  else
  {
    pumpStatus = _STOP_PUMP;
    pumpPinState = LOW;
    digitalWrite(pumpPinNo, pumpPinState);
  }

} //

void PUMPER::nonBlockingPumpRun(unsigned long onTime, unsigned long offTime)
{
  unsigned long currentMillis = millis();
  unsigned long interval = pumpPinState ? onTime : offTime;
  if (currentMillis - previousMillis >= interval)
  {
    pumpPinState = !pumpPinState;
    runUpCounter++;
    if (runUpCounter >= maxPumpCykles * 2)
    {
      pumpStatus = _ERROR;
      pumpPinState = LOW;
    }
    digitalWrite(pumpPinNo, pumpPinState);
    previousMillis = currentMillis;
  }
}

void PUMPER::stopIt()
{
  pumpStatus = _STOP_PUMP;
  runUpCounter = 0;
  pumpPinState = LOW;
  digitalWrite(pumpPinNo, pumpPinState);
  pumpBtn.tick();
}

void PUMPER::tick()
{
  pumpBtn.tick();
}

void PUMPER::pumpIt()
{
  pumpBtn.tick();
  readMoisture();
  if (pumpStatus == _RUNNING)
  {
    if (currMoist >= pumpSetup.D.maxM)
    {
      pumpStatus = _WAITING;
      pumpPinState = LOW;
      digitalWrite(pumpPinNo, pumpPinState);
      runUpCounter = 0;
    }
    else
    {
      if (isStorageEmpty())
      {
        pumpStatus = _OUT_OF_WATER;
        pumpPinState = LOW;
        digitalWrite(pumpPinNo, pumpPinState);
        return;
      }
      nonBlockingPumpRun(pumpSetup.D.pumpTime * 1000, pumpSetup.D.pumpPause * 1000);
    }
  }
  else if (pumpStatus == _WAITING)
  {
    if (currMoist <= pumpSetup.D.minM)
    {
      pumpStatus = _RUNNING;
    }
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
