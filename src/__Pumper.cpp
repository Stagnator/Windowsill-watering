#include "_Pumper.h"


PUMPER::PUMPER() {}

PUMPER::PUMPER(const int i, const int sensPin, const int pumpPin, const uint8_t alarmPin, const uint8_t buttonPin)
  : pumpNo(i), pumpBtn(buttonPin, true), sensPinNo(sensPin), pumpPinNo(pumpPin), alarmPinNo(alarmPin) {
  pinMode(sensPinNo, INPUT);
  pinMode(alarmPinNo, INPUT);
  pinMode(pumpPinNo, OUTPUT);

  pumpBtn.attachClick([](void *scope) {
    ((PUMPER *)scope)->ClickFunction();
  },
                      this);  //это п-дец ))))
  pumpBtn.attachDoubleClick([](void *scope) {
    ((PUMPER *)scope)->DoubleClickFunction();
  },
                            this);
  pumpBtn.attachLongPressStart([](void *scope) {
    ((PUMPER *)scope)->LongPressStart();
  },
                               this);
  pumpBtn.attachDuringLongPress([](void *scope) {
    ((PUMPER *)scope)->DuringLongPress();
  },
                                this);
  pumpBtn.attachLongPressStop([](void *scope) {
    ((PUMPER *)scope)->LongPressStop();
  },
                              this);
  
}


void PUMPER::init() {
    
  if (isPumpLeak()) { pumpStatus = _LEAK_DT; }
  

  pumpBtn.setLongPressIntervalMs(1000);
  //readDataE(i);
}

void PUMPER::stopIt() {
  digitalWrite(pumpPinNo, LOW);
} //

EWateringResult PUMPER::pumpIt() {
  Serial.print("Pump number ");
  Serial.println(pumpNo);
  pumpBtn.tick();
  if (pumpStatus == _OK) {
    return _PASS;
  } else {
    return LEAK;
  }
}

void PUMPER::onePump() {
  digitalWrite(pumpPinNo, HIGH);
  delay(pumpSetup.D.pumpTime * 1000);
  digitalWrite(pumpPinNo, LOW);
} //

void PUMPER::pumpGo() {
  digitalWrite(pumpPinNo, HIGH);
} //

bool PUMPER::isPumpLeak() {
  return digitalRead(alarmPinNo);
} //

uint8_t PUMPER::sensorRead() {
int soilMoistureValue = analogRead(sensPinNo);
return( map(soilMoistureValue, AirValue, WaterValue, 0, 100));
}

//-------------------------------------button----------------
void PUMPER::LongPressStart() {
  //Serial.print(((OneButton *)oneButton)->getPressedMs());
  Serial.println("\t - LongPressStart()");
  if (pumpStatus == _SETUP) {
    setUpCounter = 0;
    pumpGo();
  }
}

void PUMPER::LongPressStop() {
  stopIt();
  Serial.print(setUpCounter);
  Serial.println("\t - LongPressStop()\n");
}

void PUMPER::DuringLongPress() {
  //Serial.print(((OneButton *)oneButton)->getPressedMs());
  Serial.println("\t - DuringLongPress()");
  setUpCounter = +1;
}

void PUMPER::ClickFunction() {
  onePump();
}  // ClickFunction

void PUMPER::DoubleClickFunction() {

}  // DoubleClickFunction
