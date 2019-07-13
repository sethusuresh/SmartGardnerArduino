#include <ESP8266WiFi.h>// Wifi library
#include<FirebaseArduino.h>// Firebase library
#include <DS1302.h>// RTC library for DS1302
#include <Ticker.h>//nodemcu timer library
#include <Wire.h>
#include "OLED.h"

//Initializing FIREBASE
#define FIREBASE_HOST "smart-gardener-bf768.firebaseio.com"                     //Your Firebase Project URL goes here without "http:" , "\" and "/"
#define FIREBASE_AUTH "CNuPIJwpGHzeFBeGbae7CsKvghZEiqMNjIthrOiB"       //Your Firebase Database Secret goes here

//Initializing WiFi
#define WIFI_SSID "suresh"                                               //your WiFi SSID for which yout NodeMCU connects
#define WIFI_PASSWORD "9447778829"                                      //Password of your wifi network 
//#define WIFI_SSID "Motosethu"  
//#define WIFI_PASSWORD "12345678"  

//Initializing OLED Display
const int _SDA = 4;//where 4, 5 correspond to NodeMCU D2 and D1
const int _SCL = 5;
OLED display(_SDA, _SCL);

// Initialzing the DS1302 RTC
const int RST = 15, DAT = 13, CLK = 12;
DS1302 rtc(RST, DAT, CLK);

// Initializing the motor
const int motorPositive = 14;//D5 of nodeMCU. motor negative is always grounded

//Initializing Timer
Ticker timer;
Ticker waterTimer;

//Initializing other global variables
String rxData = "";
String days = "";
String morningTime = "";
String eveningTime = "";
String data[8];
int sizeOfData = 0;
bool timeout = false;
bool stopWateringTimer = false;

void splitString(String rxData, String delimiter) {
  int i = 0;
  sizeOfData = 0;
  int m = 0;
  int n = 0;
  while (m < rxData.length()) {
    n = rxData.indexOf(delimiter, m + 1);
    if (n != -1) {
      data[i] = rxData.substring(m + 1, n);
      i++;
      sizeOfData = i;
      m = n;
    }
    else {
      break;
    }
  }
  return;
}

void initializeRTC() {
  // Set the clock to run-mode, and disable the write protection
  rtc.halt(false);
  rtc.writeProtect(false);
  // The following lines can be commented out to use the values already stored in the DS1302
  //rtc.setDOW(SATURDAY);        // Set Day-of-Week to FRIDAY
  //rtc.setTime(13, 45, 00);     // Set the time to 12:00:00 (24hr format)
  //rtc.setDate(13, 7, 2019);   // Set the date to August 6th, 2010
  //Display time in OLED
  Serial.println(rtc.getTimeStr());
  display.print(rtc.getTimeStr());
}

bool isMorning(String currentTime) {
  bool morning = true;
  if ((currentTime.substring(0, 2).toInt()) < 12) {
    morning = true;
  }
  else {
    morning = false;
  }
  return morning;
}

String convertTimeFormat(String currentTime) {
  String hr = String((currentTime.substring(0, 2).toInt()) - 12);
  String Hr24 = currentTime.substring(0, 2);
  currentTime.replace(Hr24, hr);
  return currentTime;
}

bool waterNow(String days, String morningTime, String eveningTime) {
  splitString(days, ",");
  bool startWater = false;
  if (sizeOfData > 0) {
     for (int i = 0; i < sizeOfData; i++) {
      if (data[i].equalsIgnoreCase(rtc.getDOWStr())) {
        if (morningTime.substring(0, 2).toInt() != 0 && isMorning(rtc.getTimeStr(FORMAT_SHORT))) {
          if (morningTime.indexOf(rtc.getTimeStr(FORMAT_SHORT)) > -1) {
            startWater = true;
          }
          else {
            startWater = false;
          }
        }
        else {
          if (eveningTime.substring(0, 2).toInt() != 0) {
            if (eveningTime.indexOf(convertTimeFormat(rtc.getTimeStr(FORMAT_SHORT))) > 0) {
              startWater = true;
            }
            else {
              startWater = false;
            }
          }
          else{
            startWater = false;
          }
        }
      }
    }
  }
  else {
    startWater = false;
  }
  return startWater;
}

void startWatering() {
  //print watering started in OLED display
  digitalWrite(motorPositive, HIGH);
  display.clear();
  display.print("WATERING STARTED", 4);
  initializeWateringTimer();
}

void stopWatering() {
  digitalWrite(motorPositive, LOW);
  Serial.println("Watering Completed.........");
  scrollText("WATERING COMPLETED");
  waterTimer.detach();
}

void initializeMotor() {
  pinMode(motorPositive, OUTPUT);
}

void initializeFirebase() {
  if(WiFi.status() == WL_CONNECTED){
    Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
    int attempt = 0;
    while(Firebase.failed() && attempt < 5){
      Serial.print("Firebase Connection failed. Reconnecting...");
      Serial.println(Firebase.error());
      display.clear();
      scrollText("FIREBASE CONNECTION FAILED. RECONNECTING...");
      Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
      attempt++;
    }
    if(!Firebase.failed()){
      Serial.println("Firebase connection successfull.......");
      display.clear();
      scrollText("CONNECTED TO FIREBASE");
      delay(3000);
    }
    else{
      display.clear();
      scrollText("FIREBASE CONNECTION FAILED");
      delay(3000);
    }
  }
  else{
    display.clear();
    scrollText("FIREBASE CONNECTION FAILED. WIFI NOT CONNECTED");
    delay(3000);
  }
}

void ICACHE_RAM_ATTR onTimerISR(){
   timeout = true;
}

void initializeTimer() {
  //Initialize Ticker every 1 min
    timer.attach(60, onTimerISR);  
}

void ICACHE_RAM_ATTR onWaterTimerISR(){
   stopWateringTimer = true;
}

void initializeWateringTimer() {
  waterTimer.attach(20, onWaterTimerISR);
}

void initializeWifi() {
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  int attempt = 0;
  while (WiFi.status()!=WL_CONNECTED && attempt<5){
    Serial.print(".");
    delay(500);
    attempt++;
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("connected:");
    Serial.print(WiFi.localIP());
  }else{
    Serial.println("not connected...");
  }
}

void scrollText(String text) {
  char copy[50];
  if(text.length() < 16){
    text.toCharArray(copy, 50);
    display.print(copy, 4);
  }
  else{
    int diff = text.length() - 16;
    int i=0;
    while(i <= diff){
      display.print(rtc.getTimeStr(), 0, 4); 
      String newText = text.substring(i, 16+i);
      newText.toCharArray(copy, 50);
      display.print(copy, 4);
      i++;
      delay(1000);
    }
  }
}

void initializeDisplay() {
  display.begin();
  display.print("POWERING ON", 4);
  delay(3000);
  display.clear();
  if(WiFi.status() == WL_CONNECTED){
    scrollText("CONNECTED TO WiFi");
  }
  else{
    scrollText("NOT CONNECTED TO WiFi");
  }
  delay(3000); 
}

void setup() {
  Serial.begin(115200);
  initializeWifi();
  initializeDisplay();
  initializeRTC();
  initializeMotor();
  initializeFirebase();
  initializeTimer();
}

void loop() {
  Serial.println(rtc.getTimeStr());
  display.print(rtc.getTimeStr(), 0, 4);  
  if(timeout){
    timeout = false;
    if(WiFi.status() != WL_CONNECTED){
      scrollText("WiFi DISCONNECTED");
      initializeWifi();
      if(WiFi.status() != WL_CONNECTED){
        scrollText("NOT CONNECTED TO WiFi");
      }
      else{
        scrollText("CONNECTED TO WiFi");
        if (Firebase.failed()){
          initializeFirebase();
        }
      }
    }
    else{
      scrollText("CONNECTED TO WiFi");
      if (Firebase.failed()){
        initializeFirebase();
      }
    }
    String waterNowVal = Firebase.get("/FwJmI8JmppMITms75sTBiUA6cKD2").getString("waterNow"); 
    Serial.println("waterNow: "+ waterNowVal);
    String days = Firebase.get("/FwJmI8JmppMITms75sTBiUA6cKD2").getString("daysOfWeek"); 
    Serial.println("days: "+ days);
    String morningTime = Firebase.get("/FwJmI8JmppMITms75sTBiUA6cKD2").getString("morningTime"); 
    Serial.println("morningTime: "+ morningTime);
    String eveningTime = Firebase.get("/FwJmI8JmppMITms75sTBiUA6cKD2").getString("eveningTime"); 
    Serial.println("eveningTime: "+ eveningTime);
    if (waterNowVal.equalsIgnoreCase("true") || waterNow(days, morningTime, eveningTime)) {
      startWatering();
    }
  }

  if(stopWateringTimer){
    stopWateringTimer = false;
    stopWatering();
    Firebase.setString("/FwJmI8JmppMITms75sTBiUA6cKD2/waterNow","false");//test this in detail.
  }
  delay (1000);
}
