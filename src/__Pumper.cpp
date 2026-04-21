#include "_Pumper.h"

PUMPER::PUMPER() {}

PUMPER::PUMPER(const int i, const int sensPin, const int pumpPin, const uint8_t alarmPin, const uint8_t buttonPin)
    : pumpNo(i), pumpBtn(buttonPin, true), sensPinNo(sensPin), pumpPinNo(pumpPin), alarmPinNo(alarmPin)
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
  pumpStatus = _OK;
  currMoist = 0;
  readMoisture();
  readDataEPR();
  //pumpSetup = initPumpSetup[pumpNo];
  pumpBtn.setLongPressIntervalMs(800);
  if (isPumpLeak())
  {
    pumpStatus = _LEAK_DT;
  }
}

void PUMPER::pumpGo()
{
  digitalWrite(pumpPinNo, HIGH);
}

void PUMPER::stopIt()
{
  digitalWrite(pumpPinNo, LOW);
} //

EWateringResult PUMPER::pumpIt()
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

void PUMPER::onePump()
{
  digitalWrite(pumpPinNo, HIGH);
  delay(pumpSetup.D.pumpTime * 1000);
  digitalWrite(pumpPinNo, LOW);
} //

bool PUMPER::isStorageEmpty()
{
  return digitalRead(alarmPinNo);
} //

void readMoisture()
{
  int soilMoistureValue = analogRead(sensPinNo);
  this.currMoist = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
}

uint8_t PUMPER::getMoisture()
{
  return currMoist;
}

uint8_t getDesiredMoisture()
{
  return pumpSetup.maxM;
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
