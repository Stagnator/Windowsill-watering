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

/* Enums */

//machine status
typedef enum CURR_STATUS {
  STOP,        // System halted
  RUN,         // Sysytem works
  SETUP_MODE,  // System in setup mode
  ALARM        // Leaking detected
};

// Result of watering attempt
typedef enum WATERING_RESULT {
  PASS,        // Dont need watering
  DONE,        // Watering succesful
  CANT_REACH,  // Watering failed
  LEAK         // Leaking detected
};




//=====================================

//========================================
#endif /* Main */