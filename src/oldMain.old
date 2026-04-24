/*============================================================\
|MAIN SETTINGS                                                |
\============================================================*/
#ifndef  MAIN_H
#define  MAIN_H
//===================================== https://github.com/yuliskov/SmartTube/releases

#define NB_OF_PUMPS 3  //Quantity of pump units
/*=============================================\
|One Unit include:                             |
| -Capacitive Moisture Sensor (analog input)   |
| -Emergency Resestive Sensor for leak control |
| -Pump ON/OFF reley                           |
| -Pump control button                         |
\=============================================*/


//Capacitive Sensor RAW data
//Dry: (520 430]
//Wet: (430 350]
//Water: (350 260]
static constexpr int AirValue = 620;  //Calibration of sensors needed!
static constexpr int WaterValue = 310; //Calibration of sensors needed!

//Pins definitions
static constexpr int pinOfSensor[NB_OF_PUMPS] = { A3, A6, A7 };        //Pins connected to capacity sensors 1-2-3
static constexpr int pinOfPump[NB_OF_PUMPS] = { A0, A1, A2 };          //Pins connected to pump relays 1-2-3
static constexpr uint8_t pinOfAlarmSensor[NB_OF_PUMPS] = { 7, 8, 9 };  //Pins connected to leak resestive sensors 1-2-3 in digital mode
static constexpr uint8_t pinOfCntrlButton[NB_OF_PUMPS] = { 4, 5, 6 };  //Pins connected to control buttons 1-2-3
static constexpr uint8_t pinOfEncoder[3] = { 10, 11, 12 };          //Pins connected to encoder (A, B, last number is encbutton)
static constexpr uint8_t pinINT0StopButton = 2;                     //Pin of Emergency STOP button (and START too)
static constexpr uint8_t pinINT1AlarmSensors = 3;                   //Pin of Emergency STOP form Resestive Leak Sensors
static constexpr uint8_t pinAlarmLED = 13;                          //Pin of Alarm LED
// A4 - SDA, A5 - SCL, LCD con  

/* Enums */

//machine status
typedef enum  {
  _STOP,        // System halted
  _RUN,         // Sysytem works
  _SETUP_MODE,  // System in setup mode
  _ALARM        // Leaking detected
} ECurrStatus;

// Result of watering attempt
typedef enum  {
  _PASS,        // Dont need watering
  _DONE,        // Watering succesful
  _CANT_REACH,  // Watering failed
  _LEAK         // Leaking detected
} EWateringResult;




//=====================================

//========================================
#endif /* Main */