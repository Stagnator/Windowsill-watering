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
  _WAITING,      // Wait for start of pumping
  _RUNNING,      // Pump is running
  _OUT_OF_WATER, // Out of water (cant operate)
  _STOP_PUMP,         // STOP mode
  _ERROR,        // Error (cant reach desired moisture level for some reason)
} EStatusOfPump;

// constants

//=====================================
class PUMPER
{
private:
  OneButton pumpBtn;
  int sensPinNo, pumpPinNo;
  uint8_t alarmPinNo, buttonPinNo;
  uint8_t pumpNo;           // self number of pump
  EStatusOfPump pumpStatus; // status
  tUnionSetting pumpSetup;  // pumpSetup.D.minM
  uint8_t currMoist;        // current moisture from capacitive sensor
  uint8_t runUpCounter;     // Runs up counter to avoid infinite pumping (for safety reasons)
  unsigned long previousRunMillis, previousPauseMillis; // For counting time in millis

  void readMoisture(); // Refresh current moisture from capacity sensor 0-100%  
  void onePump();      // One time of shot pamping (for calibrating purposes)
  void pumpGo();       // Just start pumping to desired moisture level
  bool isStorageEmpty(); // Check if water storage is empty (for resistive sensor)
  void readDataEPR(); // Read pump settings from EEPROM
  void diasablePump(); // Disable pump (for example, when leak is detected)

  // Pump button functions
  void LongPressStart(); //Disable or enable pump
  void LongPressStop();
  void DuringLongPress();
  void ClickFunction(); //Start one time of shot pumping (for calibrating purposes)
  void DoubleClickFunction(); //Start pumping to desired moisture level (for calibrating purposes)

public:
  /* Constructors */
  PUMPER();
  explicit PUMPER(const int i, const int sensPin, const int pumpPin, const uint8_t alarmPin, const uint8_t buttonPin);

  /* Methods */
  void init();
  void stopIt();             // (Emergency) Stop of pumping
  void pumpIt();  // handle Pump
  uint8_t getMoisture();     // Return current moisture
  EStatusOfPump getStatus(); // Return status

}; // class
//========================================
#endif /* PUMPER */