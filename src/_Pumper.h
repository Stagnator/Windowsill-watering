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
// Capacitive Sensor RAW data
// Dry: (565 430]
// Wet: (430 350]
// Water: (350 205]
static constexpr uint16_t AirValue = 565;         // 0 % Calibration of sensors needed!
static constexpr uint16_t WaterValue = 205;       // 100% Calibration of sensors needed!
static constexpr uint8_t maxPumpCykles = 60; // Max count of pumping cykles to reach desired moisture level (for safety reasons)
static constexpr uint8_t SensorSampleDelayMs = 2;      // Delay between sensor readings to stabilize the analog input
static constexpr float alfaConst = 0.2; //alfa for EMA filter for moisture readings (to stabilize the readings and avoid false triggering of pump)
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
  uint8_t sensPinNo, pumpPinNo;
  uint8_t alarmPinNo, buttonPinNo;
  EStatusOfPump pumpStatus; // status
  tUnionSetting pumpSetup;  // pumpSetup.D.minM
  uint8_t currMoist;        // current moisture from capacitive sensor
  uint8_t runUpCounter;     // Runs up counter to avoid infinite pumping (for safety reasons)
  bool pumpPinState = OFF;
  uint64_t previousMillis; // For counting time in millis

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