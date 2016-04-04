/*
******************************************************************************************************************************
No need for the solenoid valve if we use the pump on the bottom drain hole of the tank.
When the pump is off, the water will naturally drain back through it, so I've removed the valve code.

This code has a built in reset configurable in the schedule.txt file. 
The Arduino resets once a day at the time specified.
I think there is a memory leak in one of the libraries that causes it to fail after a few weeks. 
This way it will never get large enough to become an issue.

pins: 

hygro
  Has 2 output pins DC and AC
  AC - 5v dry 0v wet
  DC - uses trim pot, goes high when moisture reaches a certain level
        A0
  Vcc   5v  Red
  Gnd   Gnd Black

ldr
        A3
  Aref  5v
    
lcd serial  
  SDA   A4  Yellow
  SCL   A5  Green
  Vcc   5v  Red
  Gnd   Gnd Black
    
rtc
  SDA   A4  Yellow
  SCL   A5  Green
  Vcc   5v  Red
  Gnd   Gnd Black
    
temp
        D3
relays
  Vcc   5v  Red
  Gnd   Gnd Black             
  RL1   D5  Pump
  RL3   D7  Fan1
  RL4   D8  Fan2
  RL5   D9  Heater
  RL6   D10 Light
  RL7   D11
  RL8   D12
              
******************************************************************************************************************************  
*/
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <Wire.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#define ONE_WIRE_BUS 3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress Thermometer = { 0x28, 0xF4, 0xBC, 0x65, 0x05, 0x00, 0x00, 0xFA };
String strTemperature;
int intTemperature;
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
int relayPump=5;
int relayFan1=7;
int relayFan2=8;
int relayHeater=9;
int relayLight=10;
int relayFan3=11;
int relayFan4=12;
int soil=0;
String strHygro;
int intHygro;
int ldrValue = 0;
int ldr;
String strLight;
int intLight;
String strHour, strMinute, strSecond, strTime;
int intMinutes;
RTC_DS1307 RTC;
File myFile;
const int chipSelect = 10;
char val[20];
float v;
int i=0;
String strLightsOnH, strLightsOnM, strLightsOnS, strLightsOffH, strLightsOffM, strLightsOffS;
String strResetH, strResetM, strResetS;
String strFanSpeed;
String strPumpOnH, strPumpOnM, strPumpOnS, strPumpOffH, strPumpOffM, strPumpOffS;
int intLightsOnH, intLightsOnM, intLightsOnS, intLightsOffH, intLightsOffM, intLightsOffS;
int intResetH, intResetM, intResetS;
int intFanSpeed;
int intPumpOnH[40];
int intPumpOnM[40];
int intPumpOnS[40];
int intPumpOffH[40];
int intPumpOffM[40];
int intPumpOffS[40];
String strPumpStatus = "off";
String strFan1Status = "off";
String strFan2Status = "off";
bool boolSetFlag = 0;
bool boolLightOn = 0;
bool boolFanStatusOn = 0;
bool boolFanStatusOff = 0;
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  if (! RTC.isrunning()) 
  {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // uncomment it & upload to set the time, date and start run the RTC!
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  DateTime now = RTC.now();
  // OK so this next bit of code only works for the next 985 years. I'll change it near the time.
  setTime(now.hour(),now.minute(),now.second(),now.month(),now.day(),now.year()-2000);
  //////////////////////////////////////////////////////////////////////////////////
  Serial.print("Initializing SD card...");
  pinMode(SS, OUTPUT);
  if (!SD.begin(10, 11, 12, 13)) 
  {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  
  // open the file for reading:
  myFile = SD.open("schedule.txt");
  if (myFile) 
  {
    // read from the schedule.txt file on the SD card and put the values into the schedule:
    while (myFile.available()) 
    {
      if (myFile) 
      {
        while (myFile.available()) 
        {
          String line = myFile.readStringUntil('\n');
          if (String(line).substring(0,5) == "reset") // reset alarm called
          {
            strResetH = (String(line).substring(7,9));
            strResetM = (String(line).substring(10,12));
            strResetS = (String(line).substring(13,15));
            intResetH = strResetH.toInt();
            intResetM = strResetM.toInt();
            intResetS = strResetS.toInt();
            Serial.print("reset:\t\t"); Serial.print(intResetH); Serial.print(','); Serial.print(intResetM); Serial.print(','); Serial.println(intResetS);
            Alarm.alarmRepeat(intResetH,intResetM,intResetS,resetArduino);
          }
          if (String(line).substring(0,8) == "fanSpeed") // reset alarm called
          {
            strFanSpeed = (String(line).substring(9,10));
            intFanSpeed = strFanSpeed.toInt();
            Serial.print("fanSpeed:\t"); Serial.println(intFanSpeed);
          }
          if (String(line).substring(0,8) == "lightsOn")
          {
            strLightsOnH = (String(line).substring(9,11));
            strLightsOnM = (String(line).substring(12,14));
            strLightsOnS = (String(line).substring(15,17));
            intLightsOnH = strLightsOnH.toInt();
            intLightsOnM = strLightsOnM.toInt();
            intLightsOnS = strLightsOnS.toInt();
            Serial.print("lightsOn:\t"); Serial.print(intLightsOnH); Serial.print(','); Serial.print(intLightsOnM); Serial.print(','); Serial.println(intLightsOnS);
          }
          if (String(line).substring(0,9) == "lightsOff") // Lights off alarm called will reset Arduino
          {
            strLightsOffH = (String(line).substring(10,12));
            strLightsOffM = (String(line).substring(13,15));
            strLightsOffS = (String(line).substring(16,18));
            intLightsOffH = strLightsOffH.toInt();
            intLightsOffM = strLightsOffM.toInt();
            intLightsOffS = strLightsOffS.toInt();
            Serial.print("lightsOff:\t"); Serial.print(intLightsOffH); Serial.print(','); Serial.print(intLightsOffM); Serial.print(','); Serial.println(intLightsOffS);
          }
          if (String(line).substring(0,6) == "pumpOn")
          {
            strPumpOnH = (String(line).substring(8,10));
            strPumpOnM = (String(line).substring(11,13));
            strPumpOnS = (String(line).substring(14,16));
            intPumpOnH[i] = strPumpOnH.toInt();
            intPumpOnM[i] = strPumpOnM.toInt();
            intPumpOnS[i] = strPumpOnS.toInt();
            Serial.print("pumpOn:\t\t"); Serial.print(intPumpOnH[i]); Serial.print(','); Serial.print(intPumpOnM[i]); Serial.print(','); Serial.println(intPumpOnS[i]);
            Alarm.alarmRepeat(intPumpOnH[i],intPumpOnM[i],intPumpOnS[i],pumpOn);
            i++;
          }
          if (String(line).substring(0,7) == "pumpOff")
          {
            strPumpOffH = (String(line).substring(9,11));
            strPumpOffM = (String(line).substring(12,14));
            strPumpOffS = (String(line).substring(15,17));
            intPumpOffH[i] = strPumpOffH.toInt();
            intPumpOffM[i] = strPumpOffM.toInt();
            intPumpOffS[i] = strPumpOffS.toInt();
            Serial.print("pumpOff:\t"); Serial.print(intPumpOffH[i]); Serial.print(','); Serial.print(intPumpOffM[i]); Serial.print(','); Serial.println(intPumpOffS[i]);
            Alarm.alarmRepeat(intPumpOffH[i],intPumpOffM[i],intPumpOffS[i],pumpOff);
            i++;       
          }
        }
      }
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening schedule.txt");
  }
  //////////////////////////////////////////////////////////////////////////////////
  lcd.init(); 
  lcd.init();
  lcd.backlight();
  lcd.clear(); //Clears the LCD screen and positions the cursor in the upper-left corner
  lcd.setCursor(0,0); lcd.print("EcoSys initializing");
  delay(3000);
  lcd.clear();
  pinMode(relayPump, OUTPUT);
  pinMode(relayHeater, OUTPUT);
  pinMode(relayLight, OUTPUT);
  pinMode(relayFan1, OUTPUT);
  pinMode(relayFan2, OUTPUT);
  pinMode(relayFan3, OUTPUT);
  pinMode(relayFan4, OUTPUT);
  pinMode(A0, INPUT); 
  pinMode(A3, INPUT); 
  digitalWrite(relayPump, HIGH); // The relay board runs the logic back to front so HIGH is off and LOW on
  digitalWrite(relayHeater, HIGH);
  digitalWrite(relayLight, HIGH);
  digitalWrite(relayFan1, HIGH);
  digitalWrite(relayFan2, HIGH);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void(* resetFunc) (void) = 0; //declare reset function @ address 0
////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void  loop()
{  
  currentTime();
  lcd.setCursor(0,0); lcd.print(strTime);

  lcd.setCursor(16,0); lcd.print("    ");  // clears any stray digits if the current intMinutes is less than 4 digits long
  lcd.setCursor(16,0); lcd.print(String(intMinutes));

  //temperature
  sensors.requestTemperatures();
  getTemperature(Thermometer);
  lcd.setCursor(0,1); lcd.print("Temp:" + strTemperature + char(223) + "C");

  //hygrometer
  readHygro();
  lcd.setCursor(11,1); lcd.print("H:" + strHygro + "%  ");
  
  //light
  readLight();
  lcd.setCursor(0,2); lcd.print("Light:" + strLight + "%  ");

  //lights schedule
  lights();
  //fans
  fanVent();
  fanMov();
  Alarm.delay(900); // wait one second between clock display

  DateTime now = RTC.now();
  if (String(now.second(), DEC) == "0") writeToLog();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
// functions to be called when an alarm triggers:
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void currentTime()
{
  if(hour() < 10)
  {
    strHour = '0' + String(hour());
  }
  else
  {
    strHour = String(hour());
  }
  
  if(minute() < 10)
  {
    strMinute = '0' + String(minute());
  }
  else
  {
    strMinute = String(minute());
  } 
   
  if(second() < 10)
  {
    strSecond = '0' + String(second());
  }
  else
  {
    strSecond = String(second());
  }
  strTime = strHour + ':' + strMinute + ':' + strSecond;
  intMinutes = (hour()*60) + minute();
  //Serial.print(String(intMinutes));
  //Serial.println(strTime);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void getTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  if (tempC == -127.00) 
  {
    strTemperature = String("ERR");
  } 
  else 
  {
  strTemperature = String(tempC,0);
  intTemperature = tempC;
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void maintainTemperature()
{
  if (intTemperature < 10)      // Try to keep the temperature above 10 degrees C - switches heater relay on
  {
    digitalWrite(relayHeater, LOW);
    Serial.print("\tHeater on");
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void lights()
{
  if (intMinutes >= (intLightsOffH*60) + intLightsOffM)
  {
    //current time greater than lights off time
    if (intMinutes >= ((intLightsOnH*60) + intLightsOnM))
    {
      //and current time greater than lights on time
      //Lights on
      digitalWrite(relayLight, LOW);
      lcd.setBacklight(128);  // lcd backlight on
      lcd.setCursor(11,2); lcd.print("Light:on ");
      boolLightOn = 1;
      //Serial.print("Lights on\t"); Serial.println(boolLightOn);
    }
    else
    {
      //Lights off
      digitalWrite(relayLight, HIGH);
      lcd.setBacklight(0);  // lcd backlight off - we don't want any light present during the lights out phase
      lcd.setCursor(11,2); lcd.print("Light:off");
       boolLightOn = 0;
     //Serial.print("Lights off\t"); Serial.println(boolLightOn);
    }
  }
  else
  {
    // current time less than lights off time
    //Lights on
    digitalWrite(relayLight, LOW);
    lcd.setBacklight(128);  // lcd backlight on
    lcd.setCursor(11,2); lcd.print("Light:on ");
    boolLightOn = 1;
    //Serial.print("Lights on\t"); Serial.println(boolLightOn);
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void readHygro() 
{
  int intHygroValue = analogRead(A0);
  //Serial.print("Hygrometer value: ");Serial.println(String(intHygroValue));
  intHygroValue = constrain(intHygroValue, 485, 1023);
  soil = map(intHygroValue, 485, 1023, 100, 0);
  strHygro = String(soil);
  intHygro = soil;
} 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void readLight() 
{
   ldrValue = analogRead(A1);
   //Serial.print("LDR value: "); Serial.println(String(ldrValue));
   constrain(ldrValue, 10, 1024);
   ldr = map(ldrValue, 10, 1024, 0, 100);
   constrain(ldr, 0, 100);
   strLight = String(ldr);
   intLight = ldr;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void pumpOn()
{
  digitalWrite(relayPump, LOW);
  currentTime();
  //Serial.println("Pump ON\t");
  lcd.setCursor(0,3); lcd.print("Pump on ");
  strPumpStatus="on";
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void pumpOff()
{
  digitalWrite(relayPump, HIGH);
  currentTime();
  //Serial.println("Pump OFF\t");
  lcd.setCursor(0,3); lcd.print("Pump off");
  strPumpStatus="off";
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void resetArduino()
{
  // There appears to be a memory leak. I suspect the alarm or the SD library, but to counter it, I've set the Arduino to reset daily.
  resetFunc();  //call reset
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void fanVent()
{
  // The fans are not for ventilation purposes, just to create a breeze so the stems grow strong
  // Fan1 will run for one minute when intMinutes/5=0 - 12 times an hour 
  // Fan2 will run for one minute when intMinutes/6=0 - 10 times an hour
  int intModulo5 = intMinutes % 5;
////fan1///////////////////////////////////////////////////////////////
  // Only want the ventilation fans working while the lights are on
  if (boolLightOn == 1)
  {
    if (intModulo5 == 0)
    {
      digitalWrite(relayFan1, LOW);
      lcd.setCursor(11,3); lcd.print("F1:1");
      strFan1Status="on";
    } 
    else
    {
      digitalWrite(relayFan1,HIGH);
      lcd.setCursor(11,3); lcd.print("F1:0");  
      strFan1Status="off";       
    }
  }
  else
  {
    digitalWrite(relayFan1,HIGH);
    lcd.setCursor(11,3); lcd.print("F1:0");  
    strFan1Status="off";
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void fanMov()
{
  // This fan is not for ventilation purposes, just to create a breeze so the stems grow strong
  // To turn it on the fan speed button must be pressed and the rotate button
  // The fan speed has three settings hence the for loop
  // Off is another button
  // The booleans are flags to check to see if it has already run
  int intModulo2 = intMinutes % 2;
  if (intModulo2 == 0)
  {
    if (boolFanStatusOn == 0)
    {
      lcd.setCursor(11,3); lcd.print("F2:1");
      strFan2Status="on";   
      for(int n = 0; n < intFanSpeed; n++)  // intFanSpeed is read from the schedule.txt file and can be a value from 1-3
      {
        digitalWrite(relayFan2, LOW);
        Alarm.delay(300);
        digitalWrite(relayFan2, HIGH);
        Alarm.delay(300);
      }
      digitalWrite(relayFan3, LOW);   // Rotate
      Alarm.delay(300);
      digitalWrite(relayFan3, HIGH);
      boolFanStatusOn = 1;
      boolFanStatusOff = 0;
    }
  } 
  else
  {
    if (boolFanStatusOff == 0)
    {
      lcd.setCursor(16,3); lcd.print("F2:0");  
      strFan2Status="off";     
      digitalWrite(relayFan4,LOW);    // Off
      Alarm.delay(1000);
      digitalWrite(relayFan4, HIGH);
      boolFanStatusOff = 1;
    }
    boolFanStatusOn = 0;
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void writeToLog()
{
  DateTime now = RTC.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print("\t");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print("\t");
  Serial.print("Temp:" + strTemperature + "C");
  Serial.print("\t");
  Serial.print("Hygrometer: " + strHygro + "%  ");
  Serial.print("\t");
  Serial.print("Light: " + strLight + "%  ");
  Serial.print("\t");
  Serial.print("Pump: " + strPumpStatus);
  Serial.print("\t");
  Serial.print("Fan1: " + strFan1Status);
  Serial.print("\t");
  Serial.print("Fan2: " + strFan2Status);
  Serial.print("\t");
  if (boolLightOn == 1) Serial.println("Light: on");
  if (boolLightOn == 0)Serial.println("Light: off");
// writing to the log file causes the relays to chatter so I've disabled it for now
//  myFile = SD.open("log.txt", FILE_WRITE);
//  
//  // if the file opened okay, write to it:
//  if (myFile) {
//    myFile.print(now.year(), DEC);
//    myFile.print('/');
//    myFile.print(now.month(), DEC);
//    myFile.print('/');
//    myFile.print(now.day(), DEC);
//    myFile.print("\t");
//    myFile.print(now.hour(), DEC);
//    myFile.print(':');
//    myFile.print(now.minute(), DEC);
//    myFile.print("\t");
//    myFile.print("Temp:" + strTemperature + "C");
//    myFile.print("\t");
//    myFile.print("Hygrometer: " + strHygro + "%  ");
//    myFile.print("\t");
//    myFile.print("Light: " + strLight + "%  ");
//    myFile.print("\t");
//    myFile.print("Pump: " + strPumpStatus);
//    myFile.print("\t");
//    myFile.print("Fan1: " + strFan1Status);
//    myFile.print("\t");
//    myFile.print("Fan2: " + strFan2Status);
//    myFile.print("\t");
//    if (boolLightOn == 1) myFile.println("Light: on");
//    if (boolLightOn == 0)myFile.println("Light: off");
//    myFile.close();
//  } else {
//    // if the file didn't open, print an error:
//    Serial.println("error opening log.txt");
//  }
}


