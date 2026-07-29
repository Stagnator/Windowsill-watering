#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

// Enable debug output by defining DEBUG_ENABLE before including this header,
// or uncomment the next line to enable it project-wide.

//#define DEBUG_ENABLE

#ifdef DEBUG_ENABLE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#endif // DEBUG_H
