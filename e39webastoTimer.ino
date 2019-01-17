// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"
#include <OneWire.h>                                                                                          // Include OneWire for temperature sensor
#include <DallasTemperature.h>                                                                                // Include Dallas Temperature for temperature sensor
#include <Sim800l.h>
#include <SoftwareSerial.h> //is necesary for the library!! 

#define oneWireBus 2                                                                                          // Data wire is plugged into pin 2 on the Arduino 
#define forceStartPin 3                                                                                       // Output pin for relay for Webasto to turn on
#define webastoOutputPin 4                                                                                    // Force start pin.  Remote control?
#define forceOffPin 5                                                                                         // Manual shutdown switch
#define webastoOnLEDPin 6                                                                                     // Webasto ON LED Pin
#define GSMrx 10                                                                                              // GSM RX Pin
#define GSMtx 11                                                                                              // GSM RX Pin
#define battSense 3
//#define DEBUG                                                                                                 // If debug is defined, allow serial

#ifdef DEBUG
#define DEBUG_PRINT(x)     Serial.print (x)                                                                                                                  
#define DEBUG_PRINTDEC(x)  Serial.print (x, DEC)
#define DEBUG_PRINTLN(x)   Serial.println (x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTDEC(x)
#define DEBUG_PRINTLN(x)
#endif

OneWire oneWire(oneWireBus);
DallasTemperature temperatureSensor(&oneWire);                                                                // Pass our oneWire reference to Dallas Temperature.
RTC_DS1307 rtc;                                                                                               // Connect A4 to SDA. Connect A5 to SCL.
Sim800l GSM;                                                                                                  // Sim800L GSM Module

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};   // Define day "text"

String currentYear = ""; String currentMonth = ""; String currentDay = "";                                    // define currentMonth and currentDay as strings.  Will be combined later.
String currentHour = ""; String currentMinute = "";                                                           // to get the current hour as an int and the current minute as an int
String combinedDayMonthYear = ""; String combinedHourMinute;                                                  // define combinedDayMonth as strings.  This is the combined result
String smsRead;                                                                                               // sms read text

unsigned long runDuration = 1080000;                                                                          // the duration to run for once activated.  1500000 = 1000 * 60 * 15 (1000ms, 60 sec, 18 mins)
unsigned long previousMillis = 0;                                                                             // will store last time Millis was called (better than delay

bool allowStart = false;                                                                                      // is allowed to start?
bool webastoStatus = false;                                                                                   // webasto status.  Enabled or not?
bool isSpecialDate();                                                                                         // return for function isSpecialDate
bool smsError;                                                                                                // bool for smsError
bool commandClear;                                                                                            // understand SMS text?

String smsWrite; char* smsNumber;                                                                             // char for smsWrite (text to send) and smsNumber (number to send it to)

float currentTemperature = 0.00; float startTemperature = 8.00;                                               // minimum start temperture.  OEM is 5 degrees, 8 for comfort
float battVoltage = 0.0; float r1 = 300000.0; float r2 = 100000.0;                                            // battery sense and resistor calibration

int startCombinedStartTime1 = 655; int startCombinedStartTime2 = 1555;                                        // when do you start work?  06:30 Leave work? 15:45 (lose the leading 0!)

void setup () {
  while (!Serial); // for Leonardo/Micro/Zero
  pinMode(webastoOutputPin, OUTPUT); pinMode(forceStartPin, INPUT_PULLUP);
  pinMode(forceOffPin, INPUT); pinMode(webastoOnLEDPin, OUTPUT);                                                // set pins for working (relay output, force start, force off
  attachInterrupt(digitalPinToInterrupt(forceStartPin), webastoStart, RISING);

  digitalWrite(webastoOutputPin, LOW); digitalWrite(webastoOnLEDPin, LOW);
  webastoStatus = true;

  Serial.begin(57600);                                                                                        // begin serial
  temperatureSensor.begin();                                                                                  // prepare temperature module
  smsNumber = "+447866178122";                                                                                  // set number to text
  DEBUG_PRINTLN(F("Preparing GSM Module"));
  GSM.begin(); GSM.delAllSms();                                                                               // prepare GSM module

  rtc.begin();
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop ()
{
  DateTime nowTime = rtc.now();
  unsigned long currentMillis = millis();

  currentYear = String(nowTime.year(), DEC); currentMonth = String(nowTime.month(), DEC); currentDay = String(nowTime.day(), DEC);
  combinedDayMonthYear = currentDay + currentMonth + currentYear;

  currentHour = String(nowTime.hour(), DEC); currentMinute = String(nowTime.minute(), DEC);
  combinedHourMinute = currentHour + currentMinute;

  temperatureSensor.requestTemperatures();                              // Send the command to get temperature readings
  currentTemperature = temperatureSensor.getTempCByIndex(0);            // You can have more than one DS18B20 on the same bus.  0 refers to the first IC on the wire

  battVoltage = ((analogRead(battSense) * 5.0) / 1024.0) / (r2 / (r1 + r2));
  
  DEBUG_PRINTLN("Batt. voltage: " + String(battVoltage));
  //DEBUG_PRINTLN("Signal Strength: " + String(GSM.signalQuality()));

  DEBUG_PRINTLN("CurrentMonth:" + currentMonth);
  DEBUG_PRINTLN("currentDay:" + currentDay);
  //DEBUG_PRINTLN("combinedDayMonth:" + combinedDayMonth);
  DEBUG_PRINTLN("currentHour:" + currentHour);
  DEBUG_PRINTLN("combinedHourMinute:" + combinedHourMinute);
  //DEBUG_PRINTLN("currentDayText:" + daysOfTheWeek[nowTime.dayOfTheWeek()]);
  DEBUG_PRINTLN(combinedHourMinute.toInt());

  DEBUG_PRINTLN("Current Temperature = " + String(currentTemperature));

  if ((daysOfTheWeek[nowTime.dayOfTheWeek()] != "Saturday" or daysOfTheWeek[nowTime.dayOfTheWeek()] != "Sunday") && (currentTemperature <= startTemperature) && (allowStart == false) && (webastoStatus == true)) // check to see the current day.  If weekday and cold...
  {
    allowStart = isSpecialDate();
  }

  if (digitalRead(forceOffPin) == LOW) {
    DEBUG_PRINTLN(F("Shut down switch is LOW, ignore all signals and reset allowStart."));
    allowStart = false;
  }

  if ((GSM.getNumberSms(1).toInt() > 0)) {
    DEBUG_PRINTLN("Reading SMS");
    smsRead = GSM.readSms(1); smsRead.toUpperCase(); commandClear = false;
    DEBUG_PRINTLN(smsRead);
    delay(800);
  }
  if (smsRead.indexOf("HELP") >= 0 && smsRead.indexOf(smsNumber) >= 0) {
    //asking for help
    smsError = GSM.sendSms(smsNumber, "'Webasto On', 'Webasto Enable', 'Webasto Disable', 'Status', 'Help'");
    GSM.delAllSms(); commandClear = true;
  }
  if (smsRead.indexOf("WEBASTO ON") >= 0 && smsRead.indexOf(smsNumber) >= 0) {
    //turn the webasto on
    if (webastoStatus == false) {
      smsError = GSM.sendSms(smsNumber, "Webasto will not be turned on because it is currently disabled.");
      GSM.delAllSms(); commandClear = true;
      allowStart = false;
    } else {
      smsError = GSM.sendSms(smsNumber, "Webasto turned on.");
      GSM.delAllSms(); commandClear = true;
      allowStart = true;
    }
  }
  if (smsRead.indexOf("WEBASTO ENABLE") >= 0 && smsRead.indexOf(smsNumber) >= 0) {
    webastoStatus = true;
    smsError = GSM.sendSms(smsNumber, "Webasto enabled.");
    GSM.delAllSms(); commandClear = true;
  }
  if (smsRead.indexOf("WEBASTO DISABLE") >= 0 && smsRead.indexOf(smsNumber) >= 0) {
    webastoStatus = false;
    smsError = GSM.sendSms(smsNumber, "Webasto disabled.");
    GSM.delAllSms(); commandClear = true;
  }
  if (smsRead.indexOf("STATUS") >= 0 && smsRead.indexOf(smsNumber) >= 0) {
    if (webastoStatus == true) {
      if (allowStart == true) {
        DEBUG_PRINTLN("Enabled:Y. Running:Y. Temp: " + String(currentTemperature) + "C. Batt: " + String(battVoltage) + "V");
        GSM.sendSms(smsNumber, "Enabled:Y. Running:Y. Temp: " + String(currentTemperature) + "C. Batt: " + String(battVoltage) + "V");
      } else {
        DEBUG_PRINTLN("Enabled:Y. Running:N. Temp: " + String(currentTemperature) + "C. Batt: " + String(battVoltage) + "V");
        GSM.sendSms(smsNumber, "Enabled:Y. Running:N. Temp: " + String(currentTemperature) + "C. Batt: " + String(battVoltage) + "V");
      }
    } else {
      DEBUG_PRINTLN("Enabled:N. Running:N. Temp: " + String(currentTemperature) + "C. Batt: " + String(battVoltage) + "V");
      GSM.sendSms(smsNumber, "Enabled:N. Running:N. Temp: " + String(currentTemperature) + "C. Batt: " + String(battVoltage) + "V");
    }
    GSM.delAllSms(); commandClear = true;
  }
  if (smsRead.length() > 0 && smsRead.indexOf(smsNumber) < 0) {
    DEBUG_PRINTLN("Number not found");
    GSM.sendSms(smsNumber, smsRead); GSM.delAllSms();
  }
  if (smsRead.length() > 0 && commandClear == false && smsRead.indexOf(smsNumber) >= 0) {
    GSM.sendSms(smsNumber, "Command not clear."); GSM.delAllSms();
  }

  if ((allowStart == true) && (webastoStatus == true)) {
    DEBUG_PRINTLN("Webasto turned on.  Waiting: " + String(runDuration / 1000 / 60) + " minutes");
    digitalWrite(webastoOutputPin, HIGH); digitalWrite(webastoOnLEDPin, HIGH);
    
    if (currentMillis - previousMillis >= runDuration) {
      previousMillis = currentMillis;
      
      allowStart = false;
      digitalWrite(webastoOutputPin, LOW); digitalWrite(webastoOnLEDPin, LOW);
      DEBUG_PRINTLN(F("Webasto turned off"));
    }
  }
}

void webastoStart() {
  allowStart = true;
  return;
}

