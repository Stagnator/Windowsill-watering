#include <Arduino.h>
#include <EEPROM.h>
#include "_Pumper.h"

PUMPER::PUMPER() {}

PUMPER::PUMPER(const int i, const int sensPin, const int pumpPin, const uint8_t alarmPin, const uint8_t buttonPin)
    : pumpNo(i), pumpBtn(buttonPin), sensPinNo(sensPin), pumpPinNo(pumpPin), alarmPinNo(alarmPin)
{
  pinMode(sensPinNo, INPUT);
  pinMode(alarmPinNo, INPUT);
  pinMode(pumpPinNo, OUTPUT);

  pumpBtn.attachClick([](void *scope)
                      { ((PUMPER *)scope)->ClickFunction(); },
                      this); // это п-дец ))))
  pumpBtn.attachDoubleClick([](void *scope)
                            { ((PUMPER *)scope)->DoubleClickFunction(); },
                            this);
  pumpBtn.attachLongPressStart([](void *scope)
                               { ((PUMPER *)scope)->LongPressStart(); },
                               this);
  pumpBtn.attachDuringLongPress([](void *scope)
                                { ((PUMPER *)scope)->DuringLongPress(); },
                                this);
  pumpBtn.attachLongPressStop([](void *scope)
                              { ((PUMPER *)scope)->LongPressStop(); },
                              this);
}

void PUMPER::init()
{
  pumpStatus = _WAITING;
  pumpPinState = LOW;
  currMoist = 0;
  readMoisture();
  readDataEPR();
  pumpBtn.setLongPressIntervalMs(800);
  if (isStorageEmpty())
  {
    pumpStatus = _OUT_OF_WATER;
  }
}

void PUMPER::pumpGo()
{
  if (pumpStatus == _OUT_OF_WATER)
    pumpStatus = _WAITING;
  if (isStorageEmpty())
  {
    pumpStatus = _OUT_OF_WATER;
    return;
  }
  else
  {
    if(pumpStatus == _WAITING)
    {
      pumpStatus = _RUNNING;
    } else if (pumpStatus == _RUNNING)
    {
      pumpStatus = _WAITING;
    }
  }
}

void PUMPER::readMoisture()
{
  int soilMoistureValue = analogRead(sensPinNo);
  currMoist = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
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
  pumpPinState = HIGH;
  digitalWrite(pumpPinNo, pumpPinState);
  delay(pumpSetup.D.pumpTime * 1000);
  pumpPinState = LOW;
  digitalWrite(pumpPinNo, pumpPinState);
  
} //

bool PUMPER::isStorageEmpty()
{
  return digitalRead(alarmPinNo);
} //

void PUMPER::stopIt()
{
  pumpStatus =  _STOP_PUMP;
  pumpPinState = LOW;
  digitalWrite(pumpPinNo, pumpPinState);
} //

void PUMPER::readDataEPR()
{
  EEPROM.get(pumpNo * sizeof(tUnionSetting), pumpSetup);
}

void PUMPER::diasablePump()
{
  digitalWrite(pumpPinNo, LOW);
  pumpStatus = _STOP_PUMP;
} //

void PUMPER::pumpIt()
{
  Serial.print("Pump number ");
  Serial.println(pumpNo);
  pumpBtn.tick();
  if (isPumpLeak())
  {
    pumpStatus = _LEAK_DT;
    return _LEAK;
  }

  if (pumpStatus == _OK)
  {
    readMoisture();
    if (currMoist < pumpSetup.minM)
    {
      return _DONE;
    }
    else
    {
      return _PASS;
    }
  }
  else
  {
    return _LEAK;
  }
}

uint8_t PUMPER::getMoisture()
{
  return currMoist;
}

EStatusOfPump PUMPER::getStatus()
{
  return EStatusOfPump();
}

uint8_t PUMPER::getDesiredMoisture()
{
  if (pumpStatus == _RUNNING)
  {
    return pumpSetup.D.maxM;
  }
  else
  {
    return pumpSetup.D.minM;
  }
}
//-------------------------------------button----------------
void PUMPER::LongPressStart()
{
  // Serial.print(((OneButton *)oneButton)->getPressedMs());
  Serial.println("\t - LongPressStart()");
  if (pumpStatus == _SETUP)
  {
    setUpCounter = 0;
    pumpGo();
  }
}

void PUMPER::LongPressStop()
{
  stopIt();
  Serial.print(setUpCounter);
  Serial.println("\t - LongPressStop()\n");
}

void PUMPER::DuringLongPress()
{
  // Serial.print(((OneButton *)oneButton)->getPressedMs());
  Serial.println("\t - DuringLongPress()");
  setUpCounter = +1;
}

void PUMPER::ClickFunction()
{
  if (pumpStatus == _OUT_OF_WATER)
  {
    pumpStatus = _OK;
  }
  else
  {
    onePump();
  }
} // ClickFunction

void PUMPER::DoubleClickFunction()
{

} // DoubleClickFunction
