// Settings for the application

#define VER_NUM   "v0.2"
#define VER_DATE  "2021-01-13"
#define VERSION   "TouchPadClock " VER_NUM " " VER_DATE
#define DISP_VER  VER_NUM "\n" VER_DATE

#define DEBUG
#define TRACE
#define SERIAL_BAUD  115200 

/* Change log

  v0.1  2020-03-19    Explore the addressing structures of the nRF module
  v0.2  2020-06-21    Clean up for auto-discovery coding
  
*/


// -------------------------------------------------------------------------------------------------
// Configuration of commonly changed items



// -------------------------------------------------------------------------------------------------
// Includes

#include <SPI.h>
#include <Wire.h>
#include <TM1637TinyDisplay.h>
#include "Adafruit_MPR121.h"


// -------------------------------------------------------------------------------------------------
// Hardware pin mapping

#define TM1637_CLK_PIN  2
#define TM1637_DATA_PIN  3
#define PIEZO_PIN  5


// -------------------------------------------------------------------------------------------------
// Application setup


// Clock
const unsigned long MILLI_ADJUST = 86400;  // How many millis pass before
const long ADJUST_BY = +10UL;      // we add/subtract this many millis
const unsigned long SUBTICK = 250;   // Process every millis

unsigned long previousMillis = 0;
int iMillis = 0;
unsigned long unaccountedTime = 0;

unsigned long iSeconds = 0;

byte seconds = 0;
byte minutes = 0;
byte hours = 0;

byte previousMinutes = 0;

TM1637TinyDisplay tm(TM1637_CLK_PIN, TM1637_DATA_PIN);  // Clk pin, Data pin


// Touch pad
Adafruit_MPR121 cap = Adafruit_MPR121();
#define _BV(bit) (1 << (bit)) 

// Keeps track of the last pins touched so we know when buttons are 'released'
uint16_t lastTouched = 0;
uint16_t currentTouched = 0;

#define TOUCH_MS_MIN 100      // Minimum duration of touch required to be a proper keypress
uint16_t touchedAt[12];      // When was it touched 
uint16_t released = 0;       // Which pads were released
uint8_t touchDuration[12];   // How long was it touched for in tenths of a second, therefore max of 25.6 seconds
uint16_t someMillis = 0;

// Phone pad format          0, 1, 2, 3, 4, 5, 6, 7,  8, 9, 10, 11
int8_t capPadToActual[] = { -1, 7, 4, 1, 0, 8, 5, 2, -2, 9,  6,  3 }; // -1 is * and -2 is #
// Why phone pad format? As its only got 12 pads, it can't be used as a calculator ...

// * short press to toggle alarm on/off
// * long press to set alarm, enter digits, * short to enter
// # short press for seconds, (then date)
// # long press to set time, enter digits, # short to enter
#define TOUCH_LONG 20      // Minimum duration in tenths of a second of touch required to be a long keypress, ie 30 = 3 seconds

int16_t digitsEntered = 0; // Digits typed thus far in entry mode
uint8_t alarmHours = 0;
uint8_t alarmMinutes = 0;


// General
uint8_t state = 0;        // Keep a state machine of current mode
#define AS_IDLE 0         // Normal clock mode, alarm off
#define AS_ALARM 1        // Normal clock mode, alarm on
#define AS_ALARMED 2      // Normal clock mode, alarm on, alarm sounding
#define AS_ALARM_ENTRY 3  // Entering alarm time
#define AS_TIME_ENTRY 4  // Entering clock time
#define AS_ALARM_ON 5     // Show On for display period
#define AS_ALARM_OFF 6    // Show Off for display period

#define DISPLAY_PERIOD  1000   // Display something else for in milliseconds
uint16_t  displayStartMillis = 0; // When did we start displaying something

#define PIEZO_TONE  2400
#define PIEZO_DURATION  500

// -------------------------------------------------------------------------------------------------
// Infrastructure

uint32_t theMillis = 0;

#define LOOP_DELAY 25 // ms
#define DEBOUNCE 50 // ms

#define BLINK101    10000 // How often
#define BLINK102    10    // How long for
#define BLINK103    1000  // How often when armed
uint8_t Blink = BLINK101;
uint32_t timeOfLastBlink = 0;




// #################################################################################################
// v1.3 descartes standard utilities

#include "src/printf.h"

void serialBegin(uint32_t baudrate){
  Serial.begin(baudrate);
  uint32_t timeout = millis();
  while (!Serial) if ((millis() - timeout) < 2000) delay(50); else break;
}

int freeRam(){
  extern uint16_t __heap_start, *__brkval; 
  uint16_t v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
  
#define SERIAL_SHOWRAM()    \
    Serial.print(F("Free RAM = "));   \
    Serial.println(freeRam());

void(* resetMe) (void) = 0; // declare reset function at address 0



// Standard trace & debug definitions
// Turn on & off at the top of this file with the TRACE and DEBUG

// These items are always on so we get a message on startup to identify code base details

#define SERIAL_BEGIN(baudRate)  serialBegin(baudRate);

#define SERIAL_BANNER(before, after)  \
    Serial.print(F(before));      \
    Serial.print(F("########################################"));  \
    Serial.print(F(after));

#define SERIAL_PRINT(x)     Serial.print(x);
#define SERIAL_PRINTLN(x)   Serial.println(x);
#define SERIAL_PRINTF(x)     Serial.print(F(x));
#define SERIAL_PRINTLNF(x)   Serial.println(F(x));




// This is where we turn things on and off
    
#if defined(TRACE) || defined(DEBUG)
  
  #define SERIAL_TIME(type)   \
    Serial.print(type);     \
    Serial.print(F(" "));   \
    Serial.print(F("millis: "));\
    Serial.println(millis());
  
  #define SERIAL_THETIME()    \
    Serial.print(F("theTime: ")); \
    Serial.println(millis());
  
  #define SERIAL_PRINTSTART(type, x)  \
    Serial.print(type);     \
    Serial.print(F(" "));   \
    Serial.print(__FUNCTION__); \
    Serial.print(F(":"));   \
    Serial.print(__LINE__);     \
    Serial.print(F(" "));   \
    Serial.print(x);
  #define SERIAL_PRINTEND(x)    Serial.println(x);
    
  #define SERIAL_PRINTLINE(type, x) \
    Serial.print(type);       \
    Serial.print(F(" "));     \
    Serial.println(x);
    
  #define SERIAL_S(type, str)   \
    Serial.print(type);     \
    Serial.print(F(" "));   \
    Serial.print(__FUNCTION__); \
    Serial.print(F(":"));   \
    Serial.print(__LINE__);     \
    Serial.print(F(" "));   \
    Serial.println(F(str));
  
  #define SERIAL_SX(type, str, val)    \
    Serial.print(type);     \
    Serial.print(F(" "));   \
    Serial.print(__FUNCTION__); \
    Serial.print((":"));    \
    Serial.print(__LINE__);     \
    Serial.print(F(" "));   \
    Serial.print(F(str));   \
    Serial.print(F(" = "));   \
    Serial.println(val);

    
  #define PRINTBIN(Num) for (uint32_t t = (1UL<< ((sizeof(Num)*8)-1)); t; t >>= 1) Serial.write(Num & t ? '1' : '0'); // Prints a binary number with leading zeros (Automatic Handling) 
    
#else

  #define SERIAL_TIME()
  #define SERIAL_PRINTSTART(type, x)
  #define SERIAL_PRINTEND(x)
  #define SERIAL_PRINTLINE(type, x)
  #define SERIAL_S(str)
  #define SERIAL_SX(str, val)

  #define PRINTBIN(Num)

#endif

#ifdef TRACE
  #define TRACE_TIME()    SERIAL_TIME(F("T"))
  #define TRACE_PRINTSTART(x) SERIAL_PRINTSTART(F("T"), x)
  #define TRACE_PRINT(x)    SERIAL_PRINT(x)
  #define TRACE_PRINTEND(x) SERIAL_PRINTEND(x)
  #define TRACE_PRINTLN(x)    SERIAL_PRINTLN(x)
  #define TRACE_PRINTLINE(x)  SERIAL_PRINTLINE(F("T"), x)
  #define TRACE_S(str)    SERIAL_S(F("T"), str) 
  #define TRACE_SX(str, val)  SERIAL_SX(F("T"), str, val)
#else
  #define TRACE_TIME() 
  #define TRACE_PRINTSTART(x)
  #define TRACE_PRINT(x)
  #define TRACE_PRINTEND(x)
  #define TRACE_PRINTLN(x)
  #define TRACE_PRINTLINE(x)
  #define TRACE_S(str)
  #define TRACE_SX(str, val)
#endif

#ifdef DEBUG
  #define DEBUG_TIME()    SERIAL_TIME(F("D"))
  #define DEBUG_PRINTSTART(x) SERIAL_PRINTSTART(F("T"), x)
  #define DEBUG_PRINT(x)    SERIAL_PRINT(x)
  #define DEBUG_PRINTEND(x) SERIAL_PRINTEND(x)
  #define DEBUG_PRINTLN(x)    SERIAL_PRINTLN(x)
  #define DEBUG_PRINTLINE(x)  SERIAL_PRINTLINE(F("T"), x)
  #define DEBUG_S(str)    SERIAL_S(F("D"), str) 
  #define DEBUG_SX(str, val)  SERIAL_SX(F("D"), str, val)
#else
  #define DEBUG_TIME() 
  #define DEBUG_PRINTSTART(x)
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTEND(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTLINE(x)
  #define DEBUG_S(str)
  #define DEBUG_SX(str, val)
#endif
