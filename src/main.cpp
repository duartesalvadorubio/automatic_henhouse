#include <Arduino.h>
#include "functionsLibrary.h"

unsigned long prevMainMillis;

void setup()
{
  initializeInputOutput();
  Serial.begin(115200);
  readStoredTime();
  manageLCD();
}

void loop()
{
  if (millis() - prevMainMillis > 50) // Ensure Loop Cycle each 50 ms
  {
    updateButtonStates();
    getCurrentHour();
    FSM();
    manageInterface();
    storeTime();
    manageActuators();
    manageAlarms();
    manageLCDBacklight();
    timeFlags();

    prevMainMillis = millis();
  }
}
