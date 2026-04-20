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



// constants



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
  uint8_t setUpCounter;     // Temporary for pump time count in sec

  void readMoisture(); // Refresh current moisture from capacity sensor 0-100%
  void onePump();      // One time of shot pamping (for calibrating purposes)
  void pumpGo();       // Just start pumping
  
  bool isStorageEmpty();   // Check if water storage is empty (for resistive sensor)
  void readDataEPR();
  
  // Pump button functions
  void LongPressStart();
  void LongPressStop();
  void DuringLongPress();
  void ClickFunction();
  void DoubleClickFunction();

public:
  /* Constructors */
  PUMPER();
  explicit PUMPER(const int i, const int sensPin, const int pumpPin, const uint8_t alarmPin, const uint8_t buttonPin);

  /* Methods */
  void init();
  void stopIt();            // (Emergency) Stop of pumping
  EWateringResult pumpIt(); // Pump if moisture low
  uint8_t getMoisture();        // Return current moisture
  uint8_t getDesiredMoisture(); // Return desired moisture

}; // class
//========================================
#endif /* PUMPER */