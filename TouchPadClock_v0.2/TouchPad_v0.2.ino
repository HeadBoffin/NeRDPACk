void checkKeyPad() {

  uint32_t actualMillis = millis();
  someMillis = (uint16_t) (actualMillis & 0x0000FFFF);
  //Serial.print(F("someMillis="));
  //Serial.print(someMillis);
  //Serial.print(F(", actualMillis="));
  //Serial.println(actualMillis);
  
  currentTouched = cap.touched();   // Get the currently touched pads
  released = 0;                     // Clear the released pad flags
  
  for (uint8_t i=0; i<12; i++) {
    
    // if it is touched and wasn't touched before
    if ((currentTouched & _BV(i)) && !(lastTouched & _BV(i)) ) {
      touchedAt[i] = someMillis;
    }
    
    // if it was touched and now isn't
    if (!(currentTouched & _BV(i)) && (lastTouched & _BV(i)) ) {
      Serial.print(i);
      Serial.print(F(": touchedAt="));
      Serial.print(touchedAt[i]);
      
      
      uint16_t someDuration = 0;
      if (touchedAt[i] < someMillis) {  // Manage rollover
        someDuration = someMillis - touchedAt[i];
      } else {
        someDuration = 65536 - touchedAt[i] + someMillis;
      }

      Serial.print(F(", someDuration="));
      Serial.print(someDuration);

      if (someDuration >= TOUCH_MS_MIN) {   // If touch > than ms, then definitely touched
        touchDuration[i] = (uint8_t) (someDuration / 100);
        released = released | _BV(i);   // Set the pad flag
        Serial.print(F(", touchDuration="));
        Serial.print(touchDuration[i]);
      }
      
      Serial.println();

    }
    
  }
//
//  if (released) {
//    Serial.print(F("released="));
//    Serial.println(released);
//  }

  lastTouched = currentTouched;
  
} // checkKeyPad


void doKeyPad() {

  for (uint8_t i=0; i<12; i++) {
    if (released & _BV(i)) {
      int8_t key = capPadToActual[i];

      Serial.print(i);
      Serial.print(F(": key="));
      Serial.print(key);
      

      // ---------- * for alarm ----------
      if (key == -1) {

        if (state == AS_ALARMED) {        // alarm sounding
          state = AS_ALARM;               // silence alarm
          
        } else {
          
          if (touchDuration[i] < TOUCH_LONG) {     // short press
            if (state == AS_IDLE) {         // alarm off
              state = AS_ALARM_ON;             // turn on
              tm.showString("On");
              displayStart();
              
            } else if (state == AS_ALARM) {  // alarm on or in alarm condition
              state = AS_ALARM_OFF;                // turn off
              tm.showString("Off");
              displayStart();
              
            } else if (state == AS_ALARM_ENTRY) {   // alarm entry mode, so save
              // Validate / store alarm
              alarmHours = digitsEntered / 100;
              alarmMinutes = (uint8_t) (digitsEntered - ((uint16_t)alarmHours * 100));

              Serial.print(F(", alarm time="));
              Serial.print(alarmHours);
              Serial.print(F(":"));
              Serial.print(alarmMinutes);

              state = AS_ALARM_ON;             // turn on
              tm.showString("On");
              displayStart();
              
            }
            
          } else {                          // long press
            if (state != AS_ALARM_ENTRY) {  // alarm entry not in progress
              state = AS_ALARM_ENTRY;       // so go for entry mode
              digitsEntered = -1;           // clearing the entry buffer
              tm.clear();
            }
          }
        }
        

      // ---------- # for clock ----------
      } else if (key == -2) {  
                    
        if (state == AS_ALARMED) {        // alarm sounding
          state = AS_ALARM;               // silence alarm
          
        } else {
          
          if (touchDuration[i] < TOUCH_LONG) {     // short press
            if (state == AS_TIME_ENTRY) {   // alarm entry mode, so save
              // Validate / store alarm
              hours = digitsEntered / 100;
              minutes = (uint8_t) (digitsEntered - ((uint16_t)hours * 100));
              seconds = 0;

              Serial.print(F(", new time="));
              Serial.print(hours);
              Serial.print(F(":"));
              Serial.print(minutes);

              state = AS_IDLE;
              
            }
            
          } else {                          // long press
            if (state != AS_TIME_ENTRY) {  // alarm entry not in progress
              state = AS_TIME_ENTRY;       // so go for entry mode
              digitsEntered = -1;           // clearing the entry buffer
              tm.clear();
            }
          }
        }
      
      
      // ---------- It's a digit ----------
      } else {
                                    
        if (state == AS_ALARMED) {        // alarm sounding
          state = AS_ALARM;               // silence alarm
          
        } else {
          
          if ( (state == AS_ALARM_ENTRY) | (state == AS_TIME_ENTRY) ) {    // we are in entry mode
            
            if (digitsEntered == -1) {    // nothing entered so far
              digitsEntered = key;        // so start with the key pressed
              
            } else {                      // already got at least one digit
              digitsEntered = digitsEntered * 10 + key;   // so shift left and include this
              
              if (digitsEntered > 2359) {  // Typed in too much stuff or too big an hour
                digitsEntered = -1;        // so clear
              }
              
              if (digitsEntered > 999) {  // Typed in enough to check the minutes
                uint8_t mins = (uint8_t) (digitsEntered - ((digitsEntered / 100 ) * 100));
                if (mins > 59) {        // Typed in silly minutes,
                  digitsEntered = -1;   // so clear
                }
              }
            }
            if (digitsEntered == -1) {  // Cleared the entry, clear the display
              tm.clear();
            } else {
              tm.showNumberDec(digitsEntered, 0b00000000, true);
            }
            
          }
        }
      }

      Serial.print(F(", new state="));
      Serial.print(state);
      Serial.print(F(", digitsEntered="));
      Serial.println(digitsEntered);
      
    } //released
  } // for each key
  
} // doKeyPad
