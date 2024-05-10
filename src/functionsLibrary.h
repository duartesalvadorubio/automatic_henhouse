#ifndef FUNCTIONS_LIBRARY
#define FUNCTIONS_LIBRARY

// *** Functions ***

void manageLCD();
void initializeInputOutput();
void setupTimeDS3231();
void getCurrentHour();
void storeTime();
void readStoredTime();
void updateButtonStates();
void FSM();
void manageInterface();
void manageActuators();
void manageAlarms();
void manageLCDBacklight();
void manageLCD();
void timeFlags();

#endif // MAIN_H