/*============================================================\
| PUMPER                                                      |
\============================================================*/
//=====================================
#ifndef PUMPER_H
#define PUMPER_H
//=====================================
#include <Arduino.h>
#include <OneButton.h>

#define ON  LOW
#define OFF HIGH

/* Enums */

// pumps status
typedef enum
{
  _WAITING,      // Wait for start of pumping
  _RUNNING,      // Pump is running
  _OUT_OF_WATER, // Out of water (cant operate)
  _STOP_PUMP,    // STOP mode
  _ONE_TIME,    // One time of shot pumping (for calibrating purposes)
  _ERROR,        // Error (cant reach desired moisture level for some reason)
} EStatusOfPump;

// constants
static constexpr uint8_t maxPumpCykles = 60; // Max count of pumping cykles to reach desired moisture level (for safety reasons)
static constexpr uint32_t SensorSampleIntervalMs = 30000;      // Delay between sensor readings to stabilize the analog input
static constexpr uint32_t K = 32; //  Constant for EMA filter (for more stable readings and avoid false triggering of pump)
//=====================================

#pragma pack(push, 1)
struct pumpSetting
{
  uint8_t minM;      // Min moisture to start watering (0-99)
  uint8_t maxM;      // Max moisture to stop watering (0-99)
  uint8_t pumpTime;  // Time of pumping in seconds (0-10)
  uint8_t pumpPause; // Time of pause between pump cykles in seconds (0-20)
  uint16_t sensAirValue;   // Calibration value of sensor for 0% moisture (air) ~ 600-800, needs to be set for each sensor
  uint16_t sensWaterValue; // Calibration value of sensor for 100% moisture (water) ~ 200-600, needs to be set for each sensor
}; // 8 bytes (64 bits)
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
  uint8_t sensPinNo, pumpPinNo;
  uint8_t alarmPinNo, buttonPinNo;  
  EStatusOfPump pumpStatus=_WAITING; // status
  tUnionSetting pumpSetup;  // pumpSetup.D.minM
  uint8_t currMoist=99;        // current moisture 0-99%
  uint32_t rawCurrMoist=0;     // 0-1023 raw moisture reading from sensor (10 bit) (for more precise calculations and EMA filter)
  uint8_t runUpCounter=0;     // Runs up counter to avoid infinite pumping (for safety reasons)
  bool pumpPinState=OFF; // Pump pin state (ON/OFF)
  uint32_t prevMillisPump; // For counting time in millis
  uint32_t prevMillisSens; // For counting time in millis for sensor readings (to stabilize the analog input)

  void readMoisture();                                                  // +Refresh current moisture from capacity sensor 0-100%
  void pumpOnOff(bool on);                                              // +Turn pump ON or OFF
  void onePump();                                                       // + One time of shot pamping (for calibrating purposes)
  void pumpGo();                                                        // + Just start pumping to desired moisture level
  bool isStorageEmpty();                                                // +Check if water storage is empty (for resistive sensor)
  void readDataEPR();                                                   // Read pump settings from EEPROM
  void diasableEnablePump();                                            // Disable pump (for example, when leak is detected)
  void nonBlockingPumpRun(uint32_t onTime, uint32_t offTime); // Non-blocking pumping to desired moisture level (for normal operation)

  static void staticClickHandler(void *scope);
  static void staticDoubleClickHandler(void *scope);
  static void staticLongPressStartHandler(void *scope);
  static void staticDuringLongPressHandler(void *scope);
  static void staticLongPressStopHandler(void *scope);

   // Pump button functions
  void LongPressStart();      // Disable or enable pump (or reset error)
  void LongPressStop();       // not used
  void DuringLongPress();     // not used
  void ClickFunction();       // Start one time of shot pumping or reset out of water status
  void DoubleClickFunction(); // Start/Stop pumping to seted moisture level

public:
  /* Constructors */
  PUMPER();
  explicit PUMPER(const uint8_t i, const uint8_t sensPin, const uint8_t pumpPin, const uint8_t alarmPin, const uint8_t buttonPin);

  /* Methods */
  void init();                  //
  void stopIt();                //(Emergency) Stop of pumping
  void handlePump();            //handle Pump
  uint8_t getMoisture();        //Return current moisture
  uint8_t getDesiredMoisture(); //Return desired moisture level
  EStatusOfPump getStatus();    //Return status
  
}; // class
//========================================
#endif /* PUMPER */