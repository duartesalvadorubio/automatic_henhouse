// *** Libraries ***

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <RTClib.h>
#include <EEPROM.h>

#include <functionsLibrary.h>

// *** Variables ***

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

// I/O Pins
const int adjustButtonPin = 2;
const int openButtonPin = 3;
const int closeButtonPin = 4;
const int auxButtonPin = 5;
const int hbridge_A = 7;
const int hbridge_B = 8;

const int greenLedPin = 11;
const int redLedPin = 12;
const int alarmPin = 13;

// Interface
bool flagOpenButton = false;
bool flagCloseButton = false;
bool flagAdjustButton = false;
bool flagAuxButton = false;

bool prevUpButton = true;
bool prevDownButton = true;
bool prevAdjustButton = true;
bool prevAuxButton = true;

unsigned long prevMillisButtonStates = 0;

bool enableAutoOpening = true;
bool enableAutoClosing = true;
bool enableBacklight = true;

int prevInterfaceState = 0;

// FSM
int state = 0;
// int prevState = 0;
bool openButton = false;
bool closeButton = false;
bool adjustButton = false;
bool auxButton = false;
bool enableChangeState1 = false;
bool enableChangeState2 = false;
bool enableChangeState5 = false;
bool firstTimeState2 = true;
bool firstTimeState1 = true;

int secondsPressedBottonUp = 0;
int secondsPressedBottonDown = 0;
int secondsPressedBottonAdjust = 0;
const int secondsToChangeState = 2;

unsigned long secondsToOpen = 55;
unsigned long secondsToClose = 55;
unsigned long prevMillisClosing = 0;
unsigned long prevMillisOpening = 0;

unsigned long secondsBacklight = 15 * 1000;
unsigned long prevMillisBacklight = 0;

// Time
int openingHour = 8;
int openingMinute = 00;
int closingHour = 22;
int closingMinute = 00;

int setupHour = 0;
int setupMinute = 0;

byte openingHourAddress = 0;
byte openingMinuteAddress = 1;
byte closingHourAddress = 2;
byte closingMinuteAddress = 3;

int currentYear = 0;
int currentMonth = 0;
int currentDay = 0;
int currentHour = 0;
int currentMinute = 0;
int currentSecond = 0;

bool flag250ms = false;
bool flag500ms = false;
bool flag1s = false;

unsigned long prevMillis250flag = 0;
unsigned long prevMillis500flag = 0;
unsigned long prevMillis1sflag = 0;

// Alarms
bool alarmFlag = false;
bool alarmStatus = false;
int alarmCounter = 1;

// Actuation

bool doorStatus = false; // 0 = Open, 1 = Close

// *** Functions ***

// Initialize sensors and actuators
void initializeInputOutput()
{
  // Buttons
  pinMode(openButtonPin, INPUT_PULLUP);
  pinMode(closeButtonPin, INPUT_PULLUP);
  pinMode(adjustButtonPin, INPUT_PULLUP);
  pinMode(auxButtonPin, INPUT_PULLUP);

  pinMode(hbridge_A, OUTPUT);
  pinMode(hbridge_B, OUTPUT);

  pinMode(alarmPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);

  // LCD
  lcd.init();
  lcd.backlight();

  // RTC Clock
  rtc.begin();
  currentHour = (rtc.now().hour());
  currentMinute = (rtc.now().minute());
  setupHour = openingHour;
  setupMinute = currentMinute;
}

// In case we need to update RTC time
void setupTimeDS3231()
{
  rtc.begin();

  if (!rtc.begin())
  {
    Serial.println(F("Couldn't find RTC"));
    while (1)
      ;
  }

  // Si se ha perdido la corriente, fijar fecha y hora
  if (rtc.lostPower())
  {
    // Fijar a fecha y hora de compilacion
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    // Fijar a fecha y hora específica. En el ejemplo, 21 de Enero de 2016 a las 03:00:00
    // rtc.adjust(DateTime(2016, 1, 21, 3, 0, 0));
  }
}
// Get hour from clock
void getCurrentHour()
{
  currentYear = int(rtc.now().year());
  currentMonth = int(rtc.now().month());
  currentDay = int(rtc.now().day());
  currentHour = int(rtc.now().hour());
  currentMinute = int(rtc.now().minute());
  currentSecond = int(rtc.now().second());
}

// Save opening-closing time moment in EEPROM
int storeTimePrevState = 0;
void storeTime()
{
  if (storeTimePrevState == 4 && state == 0)
  {
    EEPROM.write(openingHourAddress, (byte)openingHour);
    EEPROM.write(openingMinuteAddress, (byte)openingMinute);
    EEPROM.write(closingHourAddress, (byte)closingHour);
    EEPROM.write(closingMinuteAddress, (byte)closingMinute);
  }

  if (storeTimePrevState == 5 && state == 6){
    setupMinute = currentMinute;
    setupHour = currentHour;
  }

  if (storeTimePrevState == 6 && state == 0)
  {
    rtc.adjust(DateTime(rtc.now().year(), rtc.now().month(), rtc.now().day(), setupHour, setupMinute, 0));

  }
  storeTimePrevState = state;
}

// Read opening-closing time moment from EEPROM
void readStoredTime()
{
  byte byteVar = 0;
  EEPROM.get(openingHourAddress, byteVar);
  openingHour = (int)byteVar;
  EEPROM.get(openingMinuteAddress, byteVar);
  openingMinute = (int)byteVar;
  EEPROM.get(closingHourAddress, byteVar);
  closingHour = (int)byteVar;
  EEPROM.get(closingMinuteAddress, byteVar);
  closingMinute = (int)byteVar;
}

// Digital read of button states
void updateButtonStates()
{
  // When button pushed, digitalRead gives 0, negative logic

  bool currentUpButton = digitalRead(openButtonPin);
  bool currentDownButton = digitalRead(closeButtonPin);
  bool currentAdjustButton = digitalRead(adjustButtonPin);
  bool currentAuxButton = digitalRead(auxButtonPin);

  if (currentUpButton == false && prevUpButton == true)
  {
    openButton = true;
  }
  else
  {
    openButton = false;
  }

  if (currentDownButton == false && prevDownButton == true)
  {
    closeButton = true;
  }
  else
  {
    closeButton = false;
  }

  if (currentAdjustButton == false && prevAdjustButton == true)
  {
    adjustButton = true;
  }
  else
  {
    adjustButton = false;
  }

  if (currentAuxButton == false && prevAuxButton == true)
  {
    auxButton = true;
  }
  else
  {
    auxButton = false;
  }

  prevUpButton = currentUpButton;
  prevDownButton = currentDownButton;
  prevAdjustButton = currentAdjustButton;
  prevAuxButton = currentAuxButton;

  prevMillisButtonStates = millis();
}

// FSM
void FSM()
{
  switch (state)
  {
  case 0:

    if (openButton && doorStatus == 1)
    {
      enableChangeState1 = true;
      secondsPressedBottonUp = 0;
    }
    if (enableChangeState1)
    {
      if (!digitalRead(openButtonPin))
      {
        if (flag1s)
        {
          secondsPressedBottonUp += 1;
        }
        if (secondsPressedBottonUp >= secondsToChangeState)
        {
          state = 1;
          secondsPressedBottonUp = 0;
          enableChangeState1 = false;
        }
      }

      else
      {
        enableChangeState1 = false;
        secondsPressedBottonUp = 0;
      }
    }

    if (closeButton && doorStatus == 0)
    {
      enableChangeState2 = true;
      secondsPressedBottonDown = 0;
    }
    if (enableChangeState2)
    {
      if (!digitalRead(closeButtonPin))
      {
        if (flag1s)
        {
          secondsPressedBottonDown += 1;
        }
        if (secondsPressedBottonDown >= secondsToChangeState)
        {
          state = 2;
          secondsPressedBottonDown = 0;
          enableChangeState2 = false;
        }
      }

      else
      {
        enableChangeState2 = false;
        secondsPressedBottonDown = 0;
      }
    }

    if (adjustButton)
    {
      enableChangeState5 = true;
      secondsPressedBottonAdjust = 0;
    }
    if (enableChangeState5)
    {
      if (!digitalRead(adjustButtonPin))
      {
        if (flag1s)
        {
          secondsPressedBottonAdjust += 1;
        }
        if (secondsPressedBottonAdjust >= secondsToChangeState)
        {
          state = 5;
          secondsPressedBottonAdjust = 0;
          enableChangeState5 = false;
        }
      }

      else
      {
        enableChangeState5 = false;
        secondsPressedBottonAdjust = 0;
      }
    }

    if (enableAutoOpening && (doorStatus == 1))
    {
      if (currentHour == openingHour && currentMinute == openingMinute)
      {
        state = 1;
      }
    }

    if (enableAutoClosing && (doorStatus == 0))
    {
      if (currentHour == closingHour && currentMinute == closingMinute)
      {
        state = 2;
      }
    }
    break;

  case 1: // Opening

    if (doorStatus == 1)
    {

      if (firstTimeState1)
      {
        firstTimeState1 = false;
        prevMillisOpening = millis();
      }
      if ((millis() - prevMillisOpening) > secondsToOpen * 1000)
      {
        prevMillisOpening = 0;
        doorStatus = false;
        firstTimeState1 = true;
        state = 0;
      }
    }
    else
    {
      state = 0;
    }
    break;

  case 2: // Closing

    if (doorStatus == 0)
    {
      if (firstTimeState2)
      {
        firstTimeState2 = false;
        prevMillisClosing = millis();
      }
      if ((millis() - prevMillisClosing) > secondsToClose * 1000)
      {
        prevMillisClosing = 0;
        doorStatus = 1;
        firstTimeState2 = true;
        state = 0;
      }
    }
    else
    {
      state = 0;
    }
    break;

  case 5: //Auto mode on-off setup

    if (adjustButton)
    {
      state = 3;
    }
    break;

  case 3: //Opening time setup

    if (adjustButton)
    {
      state = 4;
    }
    break;

  case 4: //Closing time setup

    if (adjustButton)
    {
      // state = 0;
      state = 6;
    }
    break;
  
  case 6: //Clock time setup

    if (adjustButton)
    {
      state = 0;
    }
    break;

  default:
    break;
  }
}

int prevInterfaceMinute = 0;
void manageInterface()
{
  if (state != prevInterfaceState)
  {
    manageLCD();
  }

  switch (state)
  {
  case 0:
    if (currentMinute != prevInterfaceMinute)
    {
      manageLCD();
    }
    break;

  case 3:
    if (closeButton)
    {
      openingMinute += 15;

      if (openingMinute >= 60)
      {
        openingHour++;
        openingMinute = openingMinute - 60;

        if (openingHour >= 24)
        {
          openingHour = 0;
        }
      }
      manageLCD();
    }

    if (openButton)
    {
      openingMinute -= 15;
      if (openingMinute < 0)
      {
        openingMinute += 60;
        openingHour--;
        if (openingHour < 0)
        {
          openingHour = 23;
        }
      }
      manageLCD();
    }

    break;

  case 4:
    if (closeButton)
    {
      closingMinute += 15;

      if (closingMinute >= 60)
      {
        closingHour++;
        closingMinute = closingMinute - 60;

        if (closingHour >= 24)
        {
          closingHour = 0;
        }
      }
      manageLCD();
    }

    if (openButton)
    {
      closingMinute -= 15;
      if (closingMinute < 0)
      {
        closingMinute += 60;
        closingHour--;
        if (closingHour < 0)
        {
          closingHour = 23;
        }
      }
      manageLCD();
    }

    break;

  case 5:
    if (openButton)
    {
      enableAutoOpening = !enableAutoOpening;
      manageLCD();
    }

    if (closeButton)
    {
      enableAutoClosing = !enableAutoClosing;
      manageLCD();
    }
    break;

  case 6: //Setup clock time
    // if (closeButton)
    while (!digitalRead(closeButtonPin))
    {
      setupMinute += 1;

      if (setupMinute >= 60)
      {
        setupHour++;
        setupMinute = setupMinute - 60;

        if (setupHour >= 24)
        {
          setupHour = 0;
        }
      }
      manageLCD();
    }

    // if (openButton)
    while(!digitalRead(openButtonPin))
    {
      setupMinute -= 1;
      if (setupMinute < 0)
      {
        setupMinute += 60;
        setupHour--;
        if (setupHour < 0)
        {
          setupHour = 23;
        }
      }
      manageLCD();
    }

    break;

  default:
    break;
  }

  if (auxButton)
  {
    // Do async stuff;
  }

  prevInterfaceState = state;
  prevInterfaceMinute = currentMinute;
}

void manageActuators()
{
  switch (state)
  {
  case 1:
    digitalWrite(hbridge_A, LOW);
    digitalWrite(hbridge_B, HIGH);
    break;

  case 2:
    digitalWrite(hbridge_A, HIGH);
    digitalWrite(hbridge_B, LOW);
    break;

  default:
    digitalWrite(hbridge_A, HIGH);
    digitalWrite(hbridge_B, HIGH);
    break;
  }
}

int currentLedPin = 0;
void manageAlarms()
{
  switch (state)
  {
  case 0:
    if (doorStatus == true) // Door closed
    {
      digitalWrite(greenLedPin, LOW);
      digitalWrite(redLedPin, HIGH);
    }
    else // Door open
    {
      digitalWrite(greenLedPin, HIGH);
      digitalWrite(redLedPin, LOW);
    }
    alarmFlag = false;
    alarmStatus = false;
    break;
  case 1:
    alarmFlag = true;
    currentLedPin = greenLedPin;
    break;

  case 2:
    alarmFlag = true;
    currentLedPin = redLedPin;
    break;

  default:
    alarmFlag = false;
    alarmStatus = false;
    break;
  }

  if (alarmFlag && flag250ms)
  {
    alarmCounter = alarmCounter - 1;
    if (alarmCounter == 0 && alarmStatus == false)
    {
      alarmStatus = true;
      alarmCounter = 1;
    }
    if (alarmCounter == 0 && alarmStatus == true)
    {
      alarmStatus = false;
      alarmCounter = 3;
    }
  }

  digitalWrite(alarmPin, alarmStatus);
  if (state != 0) // Because if not in state 0 its always trying to put it down
  {
    digitalWrite(currentLedPin, alarmStatus);
  }
}

// Adds a 0 before single cifre to print Dates and Times correctly with LCD
String twoCifresValue(int value)
{
  String stringNumber = String(value);
  if (stringNumber.length() == 1)
  {
    stringNumber = "0" + stringNumber;
  }
  return stringNumber;
}

void manageLCD()
{
  switch (state)
  {
  case 0:
    lcd.clear();
    lcd.print(twoCifresValue(currentDay));
    lcd.print("/");
    lcd.print(twoCifresValue(currentMonth));
    lcd.print("/");
    lcd.print(currentYear);
    lcd.print(" ");
    lcd.print(twoCifresValue(currentHour));
    lcd.print(":");
    lcd.print(twoCifresValue(currentMinute));
    // lcd.print(":");
    // lcd.print(currentSecond);
    lcd.setCursor(0, 1);
    lcd.print("A>");
    if (enableAutoOpening)
    {
      lcd.print(twoCifresValue(openingHour));
      lcd.print(":");
      lcd.print(twoCifresValue(openingMinute));
    }
    else
    {
      lcd.print("NO");
    }
    lcd.print("  ");
    lcd.print("C>");
    if (enableAutoClosing)
    {
      lcd.print(twoCifresValue(closingHour));
      lcd.print(":");
      lcd.print(twoCifresValue(closingMinute));
    }
    else
    {
      lcd.print("NO");
    }
    break;

  case 1:
    lcd.clear();
    lcd.print("Motor activo");
    lcd.setCursor(0, 1);
    lcd.print("Abriendo -> ");
    break;

  case 2:
    lcd.clear();
    lcd.print("Motor activo");
    lcd.setCursor(0, 1);
    lcd.print("Cerrando ->");
    break;

  case 3:
    lcd.clear();
    lcd.print("Hora abrir");
    lcd.setCursor(0, 1);
    lcd.print("A>");
    lcd.print(twoCifresValue(openingHour));
    lcd.print(":");
    lcd.print(twoCifresValue(openingMinute));
    break;

  case 4:
    lcd.clear();
    lcd.print("Hora cerrar");
    lcd.setCursor(0, 1);
    lcd.print("         C>");
    lcd.print(twoCifresValue(closingHour));
    lcd.print(":");
    lcd.print(twoCifresValue(closingMinute));
    break;

  case 5:
    lcd.clear();
    lcd.print("Modo automatico");
    lcd.setCursor(0, 1);
    lcd.print("A>");
    if (enableAutoOpening)
    {
      lcd.print("Si");
    }
    else
    {
      lcd.print("No");
    }

    lcd.print("     C>");
    if (enableAutoClosing)
    {
      lcd.print("Si");
    }
    else
    {
      lcd.print("No");
    }
    break;

  case 6:
    lcd.clear();
    lcd.print("Hora Reloj");
    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(twoCifresValue(setupHour));
    lcd.print(":");
    lcd.print(twoCifresValue(setupMinute));
    break;

  default:
    break;
  }
}

void manageLCDBacklight()
{
  if (openButton || closeButton || adjustButton || auxButton)
  {
    lcd.backlight();
    prevMillisBacklight = millis();
  }
  else if ((millis() - prevMillisBacklight) > secondsBacklight)
  {
    lcd.noBacklight();
  }
}

void timeFlags()
{
  flag250ms = false;
  flag500ms = false;
  flag1s = false;

  if ((millis() - prevMillis250flag >= 250))
  {
    flag250ms = true;
    prevMillis250flag = millis();
  }
  if ((millis() - prevMillis500flag >= 500))
  {
    flag500ms = true;
    prevMillis500flag = millis();
  }
  if ((millis() - prevMillis1sflag >= 1000))
  {
    flag1s = true;
    prevMillis1sflag = millis();
  }
}