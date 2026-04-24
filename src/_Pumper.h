/*============================================================\
| PUMPER                                                      |
\============================================================*/
//=====================================
#ifndef PUMPER_H
#define PUMPER_H
//=====================================
#include <Arduino.h>
#include <OneButton.h>
/* Enums */

// pumps status
typedef enum
{
  _WAITING,      // Wait for start of pumping
  _RUNNING,      // Pump is running
  _OUT_OF_WATER, // Out of water (cant operate)
  _STOP_PUMP,    // STOP mode
  _ERROR,        // Error (cant reach desired moisture level for some reason)
} EStatusOfPump;

// constants
// Capacitive Sensor RAW data
// Dry: (565 430]
// Wet: (430 350]
// Water: (350 205]
static constexpr int AirValue = 565;         // 100 % Calibration of sensors needed!
static constexpr int WaterValue = 205;       // 0% Calibration of sensors needed!
static constexpr uint8_t maxPumpCykles = 50; // Max count of pumping cykles to reach desired moisture level (for safety reasons)
//=====================================

#pragma pack(push, 1)
struct pumpSetting
{
  uint8_t minM;      // Min moisture to start watering (0-99)
  uint8_t maxM;      // Max moisture to stop watering (0-99)
  uint8_t pumpTime;  // Time of pumping in seconds (0-10)
  uint8_t pumpPause; // Time of pause between pump cykles in seconds (0-20)
}; // 4 bytes (32 bits)
#pragma pack(pop)

union tUnionSetting
{
  pumpSetting D;
  byte B[sizeof(pumpSetting)];
};

class PUMPER
{
private:
  uint8_t pumpNo; // self number of pump
  OneButton pumpBtn;
  int sensPinNo, pumpPinNo;
  uint8_t alarmPinNo, buttonPinNo;
  EStatusOfPump pumpStatus; // status
  tUnionSetting pumpSetup;  // pumpSetup.D.minM
  uint8_t currMoist;        // current moisture from capacitive sensor
  uint8_t runUpCounter;     // Runs up counter to avoid infinite pumping (for safety reasons)
  bool pumpPinState = LOW;
  unsigned long previousMillis; // For counting time in millis

  void readMoisture();                                                  // +Refresh current moisture from capacity sensor 0-100%
  void onePump();                                                       // + One time of shot pamping (for calibrating purposes)
  void pumpGo();                                                        // + Just start pumping to desired moisture level
  bool isStorageEmpty();                                                // +Check if water storage is empty (for resistive sensor)
  void readDataEPR();                                                   // Read pump settings from EEPROM
  void diasableEnablePump();                                            // Disable pump (for example, when leak is detected)
  void nonBlockingPumpRun(unsigned long onTime, unsigned long offTime); // Non-blocking pumping to desired moisture level (for normal operation)

  // Pump button functions
  void LongPressStart();      // Disable or enable pump (or reset error)
  void LongPressStop();       // not used
  void DuringLongPress();     // not used
  void ClickFunction();       // Start one time of shot pumping or reset out of water status
  void DoubleClickFunction(); // Start/Stop pumping to seted moisture level

public:
  /* Constructors */
  PUMPER();
  explicit PUMPER(const int i, const int sensPin, const int pumpPin, const uint8_t alarmPin, const uint8_t buttonPin);

  /* Methods */
  void init();                  //+
  void stopIt();                // +(Emergency) Stop of pumping
  void pumpIt();                // +handle Pump
  uint8_t getMoisture();        // +Return current moisture
  uint8_t getDesiredMoisture(); // +Return desired moisture level
  EStatusOfPump getStatus();    // +Return status

}; // class
//========================================
#endif /* PUMPER */