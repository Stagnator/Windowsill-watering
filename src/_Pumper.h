/*============================================================\
| PUMPER                                                      |
\============================================================*/
//=====================================
#ifndef PUMPER_H
#define PUMPER_H
//=====================================

/* Enums */

// pumps status
typedef enum
{
  _OK,           // Ok (can operate)
  _OUT_OF_WATER, // Out of water (cant operate)
  _SETUP,        // Setup mode
  _LEAK_DT       // Leaking detected (stop)
} EStatusOfPump;

#pragma pack(push, 1)
struct pumpSetting
{
  uint8_t pumpNumber; // 0-1-2
  uint8_t minM;
  uint8_t maxM;
  uint8_t pumpTime;
  uint8_t pumpPause;
  uint8_t pumpCycls;
}; // 6 bytes
#pragma pack(pop)

union tUnionSetting
{
  pumpSetting D;
  byte B[sizeof(pumpSetting)];
};

// constants

// initial data for pumping setting
tUnionSetting initPumpSetup[NB_OF_PUMPS]{
    // PumpNumber, MinM(%), MaxM(%), PumpTime(sec), PumpPause(src), PumpCycls
    {{0, 20, 60, 5, 10, 10}},
    {{1, 20, 60, 5, 10, 10}},
    {{2, 20, 60, 5, 10, 10}}}; // array of pumps settings

//=====================================
class PUMPER
{
private:
  OneButton pumpBtn;
  int sensPinNo, pumpPinNo;
  uint8_t alarmPinNo, buttonPinNo;
  uint8_t pumpNo;           // number of pump
  EStatusOfPump pumpStatus; // status
  tUnionSetting pumpSetup;  // myPump[i].pumpSetup.D.minM
  uint8_t currMoist;        // current moisture from capacitive sensor


  void readMoisture(); // Refresh current moisture from capacity sensor 0-100%
  void onePump();      // One time of shot pamping (for calibrating purposes)
  void pumpGo();       // Just start pumping to rech desire moisture
  void setupPump();    // Manual Setup and write to EEPROM (under constraction)
  bool isPumpLeak();   // Return leak sensor status

public:
  /* Constructors */
  PUMPER();
  explicit PUMPER(const int i, const int sensPin, const int pumpPin, const uint8_t alarmPin, const uint8_t buttonPin);

  /* Basic settings */

  void init();
  void stopIt();            // Emergency stop of pumping
  EWateringResult pumpIt(); // Pump if moisture low

  uint8_t getMoisture();        // Return current moisture
  uint8_t getDesiredMoisture(); // Return desired moisture

  // Pump button functions
  void LongPressStart();
  void LongPressStop();
  void DuringLongPress();
  void ClickFunction();
  void DoubleClickFunction();

protected:
  uint8_t setUpCounter; // Temporary for pump time count in sec

  void readDataEPR();
  void writeDataEPR();

}; // class
//========================================
#endif /* PUMPER */