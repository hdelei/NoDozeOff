//  ###### NO-DOZE-OFF ######

/*  Prevent doorman/watchman from dozing off while working
 *  Light and buzzer signals and trigger for alarm systems
 *  author: Vanderlei Mendes
 */


/* buzzer/led pin = 4
 * button pin = 3 
 * relay pin = 6 
 * PB2 and PB0 = I2C   
 */

//#include <Wire.h> //#include <TiniWireS.h>
//#include <RTClib.h> //change for tinyrtclib.h
#include <TinyWireM.h>
#include <TinyRTClib.h>

#include <TinyDebugSerial.h>

TinyDebugSerial mySerial = TinyDebugSerial();


RTC_DS1307 rtc;

byte button = PB3;//button = PB3;
byte led = PB1;//led = PB4;
byte relay = PB4; //relay = PB1 - arduino uno I/O 6

byte prevSeconds = 0; //debug only


byte startPeriod[] = {8, 00}; //unmutable: after first set
byte stopPeriod[] = {5, 50}; //unmutable: after first set
byte currentTime[] = {15, 4}; //time now
byte interval = 5; //unmutable: after first set
byte warning = 3; //unmutable: after first set

//blink variables
byte signalState = LOW;
unsigned long previousMillis = 0;
unsigned long blinkInterval = 100;
byte blinkCount = 0;
bool blinkFlag = true;
bool failFlag = true;

//Beep variables and constants
bool silenceTime = false;
const byte B_WARN = 1;
const byte B_FAIL = 2;
const byte B_OK = 3;
const byte B_MUTE = 4;
unsigned long beepPause;

void updateCurrentTime(byte * curr) { //updates arduino variable, not RTC time
  DateTime timeNow = rtc.now();
  curr[0] = timeNow.hour();
  curr[1] = timeNow.minute();
  
  if(prevSeconds != timeNow.second()){

    prevSeconds = timeNow.second();
    mySerial.print(curr[0]);
    mySerial.print(":");
    mySerial.print(currentTime[1]);    
    mySerial.print(":");
    mySerial.println(prevSeconds);
    }  
}

int timeToInt(byte hh, byte mm)
{
  int tCheck = hh * 60 + mm;
  return tCheck;
}

void setup()
{
  pinMode(led, OUTPUT); //Buzzer or LED
  pinMode(relay, OUTPUT); //Trigger for alarm system
  pinMode(button, INPUT_PULLUP); //Push button

  mySerial.begin(9600);

  //Serial.begin(9600);
  
  TinyWireM.begin();
  rtc.begin();

  if (!rtc.isrunning()) {      // Uncomment lines below first use of clock to set time
    rtc.adjust(DateTime(__DATE__, __TIME__));    
  }
  delay(2000);  
  mySerial.println("Started");

}

void loop()
{ 
  updateCurrentTime(currentTime);
  //currentTime[0] = 15;
  //currentTime[1] = 4;
  unsigned int curr = timeToInt(currentTime[0], currentTime[1]);
  bool blinkTime = checkPoint(startPeriod, stopPeriod, curr, interval, warning);

  if(blinkTime == true){
    beep(B_WARN);
    
    if(digitalRead(button) == LOW){
      beep(B_OK);
      blinkFlag = false;
      failFlag = false;
      //Serial.println("*** Shift OK ***");
    }    
  }    
  
  if(blinkTime == false && blinkFlag == true){
    if(failFlag){
      beep(B_FAIL);
      failFlag = false;
      //Serial.println("### Shift fail! ###");
      }   
    }

  if(!blinkTime){
    blinkFlag = true;
    }
   
  if(blinkTime == true && blinkFlag == true){
    failFlag = true;    
    }   

  //setRTCTime(); //debug only. Good for testing different times
}

bool checkPoint(byte* startT, byte* stopT, unsigned int curr, byte interval, byte warn) {
  unsigned int startI = timeToInt(startT[0], startT[1]);
  unsigned int stopI = timeToInt(stopT[0], stopT[1]);
  unsigned int aux = startI;

  unsigned int maxIter = myRound(1440 / interval) + 1;

  if (startI < stopI) {
    for (int i = 0; i <= maxIter; i++) {
      aux += interval;
      if (curr >= aux - warn && curr < aux) {
        return true;
      }

      if (aux > stopI) {
        break;
      }
    }
  }

  if (startI > stopI) {
    aux = 0;

    for (int i = 0; i <= maxIter; i++) {
      aux += interval;
      if (aux > stopI) {
        break;
      }
      if (curr >= aux - warn && curr < aux) {
        return true;
      }
    }

    aux = startI;
    for (int i = 0; i <= maxIter; i++) {
      aux += interval;
      if (aux > 2359) {
        break;
      }
      if (curr >= aux - warn && curr < aux) {
        return true;
      }
    }
  }
  return false;
}

int myRound(float value)
{
  return value < 0 ? value - 0.5 : value + 0.5;
}

void beep(byte beepType)
{
  if (beepType == B_MUTE) //TODO: check if useless
    return;

  if (beepType == B_WARN)
  {
    checkSilenceTime(silenceTime);
    if (silenceTime)
    {
      return;
    }
  }

  if (beepType == B_WARN)
  {
    multipleBlinks(blinkFlag);    
  }
  else if (beepType == B_FAIL)
  {
    digitalWrite(led, HIGH);
    delay(700);
    digitalWrite(led, LOW);
    delay(100);
    digitalWrite(led, HIGH);
    delay(700);
    digitalWrite(led, LOW);
    delay(100);
    digitalWrite(led, HIGH);
    digitalWrite(relay, HIGH);
    delay(1200);
    digitalWrite(led, LOW);
    digitalWrite(relay, LOW);
    delay(3000);
    return;
  }
  else if (beepType == B_OK)
  {
    digitalWrite(led, HIGH);
    delay(1200);
    digitalWrite(led, LOW);
    delay(3000);
    return;
  }
}

void checkSilenceTime(bool &silenceTime)
{
  silenceTime = true;
  if (millis() >= beepPause)
  {
    silenceTime = false;
  }
}

void multipleBlinks(bool blinkFlag)
{
  if (!blinkFlag)
    return;

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= blinkInterval)
  {
    previousMillis = currentMillis;

    if (signalState == LOW)
    {
      //Serial.print(".");
      blinkInterval = 75;
      signalState = HIGH;
      blinkCount++;
    }
    else
    {
      if (blinkCount >= 4)
      {
        //Serial.println("-");
        blinkInterval = 3000;
        blinkCount = 0;
      }
      signalState = LOW;
    }
    digitalWrite(led, signalState);
  }
}

//Debug only -- can be removed on final project
/*void setRTCTime() {
  if (Serial.available() > 0) {
    String hhmm = Serial.readString();
    byte hh = hhmm.substring(0, 2).toInt();
    byte mm = hhmm.substring(2, 4).toInt();
    rtc.adjust(DateTime(1980, 12, 29, hh, mm, 0));
    Serial.println("New time set");
  }
}*/
