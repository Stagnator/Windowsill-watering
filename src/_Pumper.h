/*============================================================\
| PUMPER                                                      |
\============================================================*/
//=====================================
#ifndef _Pumper_h
#define _Pumper_h
//=====================================
#include <Arduino.h>
#include <EEPROM.h>
#include <OneButton.h>
#include "Main.h"

/* Enums */

//pumps status
typedef enum  {
  OK,            // Ok (run)
  OUT_OF_WATER,  // Out of water (stop)
  SETUP,         // Setup mode
  LEAK_DT        // Leaking detected (stop)
} STATUS_OF_PUMP;


#pragma pack(push, 1)
struct pumpSetting {
  uint8_t pumpNumber;  // 0-1-2
  uint8_t minM;
  uint8_t maxM;
  uint8_t pumpTime;
  uint8_t pumpPause;
  uint8_t pumpCycls;
};  //6 bytes
#pragma pack(pop)

union tUnionSetting {
  pumpSetting D;
  byte B[sizeof(pumpSetting)];
};



//=====================================
class PUMPER {
private:
  OneButton pumpBtn;
  int sensPinNo, pumpPinNo;
  uint8_t alarmPinNo, buttonPinNo;
  uint8_t pumpNo;                  //number of pump
  STATUS_OF_PUMP pumpStatus;  // status
  tUnionSetting pumpSetup;         //myPump[i].pumpSetup.D.minM
  uint8_t currMoist;               //current moisture from capacitive sensor

  /* constants */

  //static constexpr uint8_t AK09916_ADDRESS{ 0x0C };


public:
  /* Constructors */
  PUMPER();
  explicit PUMPER(const int i, const int sensPin, const int pumpPin, const uint8_t alarmPin, const uint8_t buttonPin);

  /* Basic settings */



  void init();
  void stopIt();                  //Emergency stop of pumping
  WATERING_RESULT pumpIt();  //Executive function
  void onePump();                 //One time of pamping cycle (for calibrating purposes)
  void pumpGo();                  //Just start pumping
  void setupPump();               //Manual Setup and write to EEPROM
  bool isPumpLeak();              //Return leak sensor status
  uint8_t sensorRead();           //Return moisture from capacity sensor 0-100%


  void LongPressStart();
  void LongPressStop();
  void DuringLongPress();
  void ClickFunction();
  void DoubleClickFunction();


protected:

  uint8_t setUpCounter;  //Temporary for pump time count in sec


  void readDataE(int i);
  void writeDataE(int i);


};  //class
//========================================
#endif /* PUMPER */