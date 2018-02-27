/*                                  University of San Carlos
 *                    Department of Electronics and Electrical Engineering 
 *        Smart Countertop Aquaponics System (SCAS): An Internet of Things (IoT) Integration 
 *                            Abrico | Paloma | Timtim | Villamor
 */

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include <DallasTemperature.h>
#include <FirebaseArduino.h>
#include <Servo.h>
#include "DHT.h"
#include <NTPtimeESP.h> 

//SynchTime
#define DEBUG_ON

//FIREBASE
#define FIREBASE_HOST "my-very-first-project-539ed.firebaseio.com"
#define FIREBASE_AUTH "6dxQlX9X3z0Qfkxn1EA3m2wZbgL5vFhzNFwB2Qru"

//GrowLights
#define REDPIN D3
#define GREENPIN D7
#define BLUEPIN D8 

// Water Temperature Sensor
#define ONEWIRE D1

// pH Sensor
#define SensorPin A0
#define Offset 0.12
#define samplingInterval 20
#define printInterval 200
#define ArrayLength 40

// Servo Motor
#define servoInterval 120000
#define servoPin D0

// DHT11
#define DHTPIN D2
#define DHTTYPE DHT11

// Water Level
#define conlength 12
#define TrigPin D5
#define EchoPin D6
#define lvlArrayLength 50

// Water Temperature Global Variables
OneWire oneWire(ONEWIRE);
DallasTemperature DS18B20(&oneWire);
float tempCelsius, tempFahrenheit, humidityData, celData, fehrData;

// pH Sensor Global Variables
int pHArray[ArrayLength];
int pHArrayIndex=0;

// Servo Global Variables
Servo myservo;
int pos=0, repeat=0;

// DHT22 Global Variables
DHT dht(DHTPIN, DHTTYPE);

// Water Level Global Variables
static float duration = 0;
static float distancecm = 0, distancein = 0;
int lvlArray[lvlArrayLength];
int lvlArrayIndex = 0;

//NTP
NTPtime NTPch("ph.pool.ntp.org");
strDateTime dateTime;
byte actualHour = 0, actualMinute = 0, actualsecond = 0, actualMonth = 0, actualday = 0;
int actualyear = 0;


void setup() 
{

  Serial.begin(115200);

  dht.begin();
  
  myservo.attach(servoPin); 
 
  pinMode(TrigPin, OUTPUT);
  pinMode(EchoPin, INPUT);

  //GrowLights Initialization
  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);

// Over-the-Air Update & WiFi Manager
  WiFiManager wifiManager;
  wifiManager.setBreakAfterConfig(true);
  wifiManager.resetSettings();
  
  if (!wifiManager.autoConnect("AquaponicsSystem", "CBELS2018!")) 
  {
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  Serial.println("CONNECTED!");
  Serial.print("LOCAL IP: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //Initialize Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  
  
  Firebase.setInt("RedLed",0);
  Firebase.setInt("GreenLed",0);
  Firebase.setInt("BlueLed",0);
 
}

void loop() 
{

  ArduinoOTA.handle();

  //Get Time
  dateTime = NTPch.getNTPtime(8, 0);

  if(dateTime.valid)
  {
    actualHour = dateTime.hour;
    actualMinute = dateTime.minute;
    actualsecond = dateTime.second;
    actualyear = dateTime.year;
    actualMonth = dateTime.month;
    actualday = dateTime.day;
  }
  
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static unsigned long servoTime = millis();
  static float pHValue,voltage;

  digitalWrite(TrigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(TrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(TrigPin, LOW);
  duration = pulseIn(EchoPin, HIGH);
  lvlArray[lvlArrayIndex++]= duration;
  if(lvlArrayIndex==lvlArrayLength)lvlArrayIndex=0;
  distancecm = lvlavergearray(lvlArray, lvlArrayLength)/58.00;
  //distancecm = conlength - distancecm;
  distancein = distancecm*0.393701;


  if(millis()-samplingTime > samplingInterval)
  {
      pHArray[pHArrayIndex++]=analogRead(SensorPin);
      if(pHArrayIndex==ArrayLength)pHArrayIndex=0;
      voltage = avergearray(pHArray, ArrayLength)*5.0/1024;
      pHValue = 3.5*voltage-Offset;

      DS18B20.requestTemperatures();
      tempCelsius = DS18B20.getTempCByIndex(0);
      tempFahrenheit = DS18B20.getTempFByIndex(0);

      humidityData = dht.readHumidity();
      celData = dht.readTemperature();
      fehrData = dht.readTemperature(true);

      samplingTime=millis();
  }
  
  if(millis() - printTime > printInterval)
  {
    Serial.println();

    Serial.print("Current Data & Time: ");
    Serial.print(actualHour);
    Serial.print(":");
    Serial.print(actualMinute);
    Serial.print(":");
    Serial.print(actualsecond);
    Serial.print("  ");
    Serial.print(actualMonth);
    Serial.print("/");
    Serial.print(actualday);
    Serial.print("/");
    Serial.println(actualyear);
    
    Serial.print("Voltage ");
    Serial.print("pH Value  ");
    Serial.print("Water TempC ");
    Serial.print("Water TempF ");
    Serial.print("Air TempC ");
    Serial.print("Air TempF ");
    Serial.print(" Humidity ");
    Serial.println("Water Level");
    Serial.print("  ");
    Serial.print(voltage,2);
    Serial.print("    ");
    Serial.print(pHValue,2);
    Serial.print("       ");
    Serial.print(tempCelsius);
    Serial.print("       ");
    Serial.print(tempFahrenheit);
    
    if (isnan(humidityData) || isnan(celData) || isnan(fehrData))
      {
        Serial.print("        ");
        Serial.print("DHT Sensor Failed!");
        Serial.print("     ");
        //Serial.println();
      }
    else 
      {  
        Serial.print("      ");
        Serial.print(celData);
        Serial.print("     ");
        Serial.print(fehrData);
        Serial.print("     ");
        Serial.print(humidityData);
        Serial.print("        ");
        //Serial.println();
      }

    Serial.print(distancecm);
    Serial.println();
 
    printTime=millis();
  }

  if(((actualHour == 18) && (actualMinute == 30)) || ((actualHour == 18) && (actualMinute == 32)) || ((actualHour == 18) && (actualMinute == 33)))
  {
    while(repeat < 9){
      for (pos = 90; pos <= 180; pos += 5) { 
        myservo.write(pos);             
        delay(20);                      
      }

      for (pos = 180; pos >= 90; pos -= 5) { 
        myservo.write(pos);              
        delay(20);                       
      }

      for (pos = 90; pos >= 0; pos -= 5) { 
        myservo.write(pos);              
        delay(20);                      
      }

      for (pos = 0; pos <= 90; pos += 5) { 
        myservo.write(pos);              
        delay(20);                       
      }

    delay(1000);
    repeat++;
  }
}

  //Send to Firebase
  Firebase.push("Sensors/pH Value",pHValue);
  Firebase.push("Sensors/Water Temperature (C)",tempCelsius);
  Firebase.push("Sensors/Air Temperature (C)",celData);
  Firebase.push("Sensors/Humidity",humidityData);
  Firebase.push("Sensors/Water Level",distancecm);

  //GrowLights Loop
  if((actualHour >= 6) && (actualHour <= 15)) 
  {
    analogWrite(REDPIN,Firebase.getInt("RedLed"));
    analogWrite(GREENPIN,Firebase.getInt("GreenLed"));
    analogWrite(BLUEPIN,Firebase.getInt("BlueLed"));
  }
  else
  {
    analogWrite(REDPIN,0);
    analogWrite(GREENPIN,0);
    analogWrite(BLUEPIN,0);  
  }
  
}

double avergearray(int* arr, int number)
{
  int i;
  int max,min;
  double avg;
  long amount=0;
  
  if(number<=0)
  {
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  
  if(number<5)
  {   //less than 5, calculated directly statistics
    for(i=0;i<number;i++)
    {
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }
  
  else
  {
    if(arr[0]<arr[1])
    {
      min = arr[0];max = arr[1];
    }
    
    else
    {
      min=arr[1];max=arr[0];
    }
    
    for(i=2;i<number;i++)
    {
      if(arr[i]<min)
      {
        amount+=min;        //arr<min
        min=arr[i];
      }
      else 
      {
        if(arr[i]>max)
        {
          amount+=max;    //arr>max
          max=arr[i];
        }
        else
        {
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}

double lvlavergearray(int* arr, int number)
{
  int i;
  int max,min;
  double avg;
  long amount=0;
  
  if(number<=0)
  {
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  
  if(number<5)
  {   //less than 5, calculated directly statistics
    for(i=0;i<number;i++)
    {
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }
  
  else
  {
    if(arr[0]<arr[1])
    {
      min = arr[0];max = arr[1];
    }
    
    else
    {
      min=arr[1];max=arr[0];
    }
    
    for(i=2;i<number;i++)
    {
      if(arr[i]<min)
      {
        amount+=min;        //arr<min
        min=arr[i];
      }
      else 
      {
        if(arr[i]>max)
        {
          amount+=max;    //arr>max
          max=arr[i];
        }
        else
        {
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}

