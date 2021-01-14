
#include "settings.h"

void setup() {
  SERIAL_BEGIN(SERIAL_BAUD)
  SERIAL_BANNER("\n\n", "\n\n")
  SERIAL_PRINTLN(VERSION)
  SERIAL_PRINTLN(__FILE__)
  SERIAL_SHOWRAM()

  SERIAL_PRINTF("Compiled ")
  SERIAL_PRINT(__DATE__)
  SERIAL_PRINTF(" ")
  SERIAL_PRINTLN(__TIME__)

  SERIAL_PRINTF("Arduino IDE ")
  SERIAL_PRINT(ARDUINO)
  SERIAL_PRINTF(" using GCC ")
  SERIAL_PRINTLN(__VERSION__)


  SERIAL_BANNER("\n", "\n\n")
  SERIAL_PRINTLN("Setup started\n")


  Serial.print(F("MILLI_ADJUST = "));
  Serial.print(MILLI_ADJUST);
  Serial.print(F(", ADJUST_BY = "));
  Serial.print(ADJUST_BY);
  Serial.print(F(", __TIME__ = "));
  Serial.print(__TIME__);
  Serial.println();

  if (!cap.begin(0x5A)) {
    Serial.println(F("MPR121 not found"));
    while (1);
  }

  tm.setBrightness(3);

  // Set the time based on the compile / upload time
  // Do this at least 7 seconds before the next minute change
  {
    String theTime = String(__TIME__);  // HH:MM:SS
    seconds = theTime.substring(6).toInt();
    minutes = theTime.substring(3).toInt();
    hours   = theTime.substring(0).toInt();
  }


  seconds += 6;   // Typically takes 5 seconds for an update compile & download
  previousMillis = millis();
  timeAdjust();

  //tm1637.colonOn();
  //tm.display(hours * 100 + minutes);

  Serial.println(F("\nSetup done\n"));

}


void loop() {

  theClock();
  
  displayCheck();

  checkKeyPad();
  doKeyPad();

}


void displayStart() {
  displayStartMillis = millis() & 0xFFFF;
  DEBUG_SX("displayStartMillis", displayStartMillis)
}

void displayCheck() {
  if ( state == AS_ALARM_ON || state == AS_ALARM_OFF ) { // If we are doing something that needs the display checking
    uint16_t tempMillis = millis() & 0xFFFF;
    uint16_t elapsed = tempMillis - displayStartMillis + ((tempMillis < displayStartMillis) * 65536);
    if (elapsed > DISPLAY_PERIOD) { // times up
      DEBUG_S("Display expired")
      if (state == AS_ALARM_ON) {         // showing alarm on
        state = AS_ALARM;             // set normal alarm state
      } else if (state == AS_ALARM_OFF) {  // showing alarm off
        state = AS_IDLE;                // set normal idle state
      }
    }
  }
}
