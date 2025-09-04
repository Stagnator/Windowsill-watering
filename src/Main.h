/*============================================================\
|MAIN SETTINGS                                                |
\============================================================*/
#ifndef  Main_h
#define  Main_h
//===================================== https://github.com/yuliskov/SmartTube/releases

#define NbOfPump 3  //Quantity of pump units
/*=============================================\
|One Unit include:                             |
| -Capacitive Moisture Sensor (analog input)   |
| -Emergency Resestive Sensor for leak control |
| -Pump ON/OFF reley                           |
| -Pump control button                         |
\=============================================*/

#define PumpingCycleTime 10  //Initial time in sec to one pumping attempt
#define PumpingPause 5       //Initial time in sec to pause betwine pump attemps
#define PumpingTimes 8       //Initial times of pumping attempt to reach desired moisture
#define MoistMax 60          //Initial max moisture of soil
#define MoistMin 20          //Initial min moisture of soil to start

#define overAttempts 3  //Quantity of additional attempts to water plant

//Capacitive Sensor RAW data
//Dry: (520 430]
//Wet: (430 350]
//Water: (350 260]
const int AirValue = 620;  //Calibration of sensors needed!
const int WaterValue = 310; //Calibration of sensors needed!

//Pins definitions
static constexpr int pinOfSensor[NbOfPump] = { A0, A1, A2 };        //Pins connected to capacity sensors 1-2-3
static constexpr int pinOfPump[NbOfPump] = { A3, A6, A7 };          //Pins connected to pump relays 1-2-3
static constexpr uint8_t pinOfAlarmSensor[NbOfPump] = { 7, 8, 9 };  //Pins connected to leak resestive sensors 1-2-3 in digital mode
static constexpr uint8_t pinOfCntrlButton[NbOfPump] = { 4, 5, 6 };  //Pins connected to control buttons 1-2-3
static constexpr uint8_t pinOfEncoder[3] = { 10, 11, 12 };          //Pins connected to encoder (last number is encbutton)
static constexpr uint8_t pinINT0StopButton = 2;                     //Pin of Emergency STOP button (and START too)
static constexpr uint8_t pinINT1AlarmSensors = 3;                   //Pin of Emergency STOP form Resestive Leak Sensors
static constexpr uint8_t pinAlarmLED = 13;                          //Pin of Alarm LED

/* Enums */

//machine status
typedef enum  {
  STOP,        // System halted
  RUN,         // Sysytem works
  SETUP_MODE,  // System in setup mode
  ALARM        // Leaking detected
} CURR_STATUS;

// Result of watering attempt
typedef enum  {
  PASS,        // Dont need watering
  DONE,        // Watering succesful
  CANT_REACH,  // Watering failed
  LEAK         // Leaking detected
} WATERING_RESULT;




//=====================================

//========================================
#endif /* Main */