void theClock() {
  uint32_t currentMillis = millis();

  /* Beware of millis rollover every ~50 days.
   * Unsigned Long = 0 to 4,294,967,295 (2^32 - 1)
   * 
   * Normally, previousMillis will be less than currentMillis
   * so elapsed is current - previous
   * 
   * But at some point, previousMillis could be, for example, 4294966500
   * and currentMillis could be 205, so if previous > current,
   * then the calculation is current - previous + 4,294,967,295
   */
 
  uint32_t elapsedTime = currentMillis - previousMillis + ((currentMillis < previousMillis) * 4294967295);

  if ( elapsedTime >= SUBTICK ) {  // Update period passed? Update the timers

    iMillis += elapsedTime;
    unaccountedTime += elapsedTime;

    if (unaccountedTime >= MILLI_ADJUST) {
      unaccountedTime -= MILLI_ADJUST;
      iMillis += ADJUST_BY;
      Serial.print(F("* "));
    }

    previousMillis = currentMillis;

    if ( iMillis >= 1000 ) {  // We've done 1000 or more millis?
      seconds++;
      iMillis -= 1000;
      timeAdjust();  // Adjust the whole seconds, minutes, hours chain
      timeSerialOutput();
      Serial.println();
      if (state == AS_IDLE || state == AS_ALARM || state == AS_ALARMED) {  // Normal display modes
        if ( seconds % 2 ) {          // Colon flash 
          tm.showNumberDec(hours * 100 + minutes, 0b01000000, true);
        } else {
          if (state == AS_ALARMED) {
            tm.showString("Beep");
            tone(PIEZO_PIN, PIEZO_TONE, PIEZO_DURATION);
          } else {
            tm.showNumberDec(hours * 100 + minutes, 0b00000000, true);
          }
        }
      }
    }

    if ( minutes != previousMinutes ) {
      previousMinutes = minutes;

      // Do things related to a minute change over
      checkAlarm();

    }
    
  } // Update period passed?

} // theClock()



void timeAdjust() {

  if (seconds == 60) {
    seconds = 0;
    minutes++;
    
    if (minutes == 60) {
      minutes = 0;
      hours++;

      if (hours == 24) {
        hours = 0;
      }
    }
  }
  
} // timeAdjust()


void checkAlarm() {

    if (hours == alarmHours && minutes == alarmMinutes) {
      TRACE_S("Alarm triggered")
      state = AS_ALARMED; 
      displayStart();
    }

}



void timeSerialOutput() {
  char timeString[8];  // HH:MM:SS
  sprintf_P(timeString, PSTR("%02d:%02d:%02d, %d"), hours, minutes, seconds, state);
  Serial.print(timeString);
} // timeSerialOutput()


void internalSerialOutput(unsigned long currentMillis) {
    Serial.print(F("  - ms = "));
    Serial.print(currentMillis);

    Serial.print(F(" iMS = "));
    Serial.print(iMillis);

    Serial.print(F(" Un = "));
    Serial.print(unaccountedTime);
    
    Serial.println();
} // internalSerialOutput()
