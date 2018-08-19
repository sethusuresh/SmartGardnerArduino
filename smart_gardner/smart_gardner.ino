#include <LiquidCrystal.h>// include the library code:
#include <DS1302.h>// RTC library for DS1302

// initialize the library by associating any needed LCD interface pin with the arduino pin number it is connected to
int Contrast = 100;
int backLight = 50;
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
// Initialzing the DS1302
const int CE = 2, IO = 3, SCLK = 4;
DS1302 rtc(CE, IO, SCLK);
// Initializing the motor
const int motorPositive = 9;
//motor negative is always grounded
//other global variables
String rxData = "";
String days = "";
String morningTime = "";
String eveningTime = "";
String data[8];
int sizeOfData = 0;
bool dataReceived = false;

void scrollInFromRight(int line, char str1[]) {
  int i = strlen(str1);
  for (int j = 16; j >= 0; j--) {
    lcd.setCursor(0, line);
    for (int k = 0; k <= 15; k++) {
      lcd.print(" "); // Clear line
    }
    lcd.setCursor(j, line);
    lcd.print(str1);
    delay(500);
  }
  if (i > 16) {
    char str[50];
    for (int x = 0; x < i - 16; x++) {
      int a = 0;
      for (int y = x; y < i; y++) {
        str[a] = str1[y + 1];
        a++;
      }
      for (int k = 0; k <= 15; k++) {
        lcd.print(" "); // Clear line
      }
      lcd.setCursor(0, line);
      lcd.print(str);
      delay(500);
    }
  }
}

void initializeLCD() {
  //set up LCD's contrast
  analogWrite(6, Contrast);
  //Turn on LCD's backlight
  analogWrite(9, backLight);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
}

void startBT() {
  //turn on BT
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("Turning On BT");
  delay(3000);
  Serial.begin(9600);//starting BT in communication mode
  //ready for connection
  dataReceived = false;
  lcd.setCursor(0, 1);
  lcd.print("                ");
  scrollInFromRight(1, "Waiting For Connection");
  delay(3000);
}

void splitString(String rxData, String delimiter) {
  int i = 0;
  int sizeOfData = 0;
  int m = 0;
  int n = 0;
  while (m < rxData.length()) {
    n = rxData.indexOf("|", m + 1);
    if (n != -1) {
      data[i] = rxData.substring(m, n);
      sizeOfData = i;
      i++;
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
  rtc.setDOW(FRIDAY);        // Set Day-of-Week to FRIDAY
  rtc.setTime(12, 0, 0);     // Set the time to 12:00:00 (24hr format)
  rtc.setDate(6, 8, 2010);   // Set the date to August 6th, 2010
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
          if (morningTime.indexOf(rtc.getTimeStr(FORMAT_SHORT)) > 0) {
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
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("watering started");
  digitalWrite(motorPositive, HIGH);
  delay(15000);//change this time based on real water speed
  digitalWrite(motorPositive, LOW);
  //end watering
  lcd.setCursor(0, 1);
  lcd.print("                ");
  scrollInFromRight(1, "Watering completed");
}

void initializeMotor() {
  pinMode(motorPositive, OUTPUT);
}

void setup() {
  initializeLCD();
  initializeRTC();
  initializeMotor();
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("Powering Up!!");
  delay(3000);
  startBT();
}

void loop() {
  //display time in LCD first row
  lcd.setCursor(4, 0);
  lcd.print("                ");
  lcd.print(rtc.getTimeStr());

  //check transceiver module
  //lcd.print(Transceiver Modules connected");

//  //post connection
//  lcd.setCursor(0, 1);
//  lcd.print("                ");
//  scrollInFromRight(1, "BT Connected to Device");

  if (Serial.available() > 0) {
    rxData = Serial.readString();
    if(rxData.length() > 0){
      splitString(rxData, "|");
      String days = data[0];
      String morningTime = data[1];
      String eveningTime = data[2];
      dataReceived = true;
    }
  }


  //check selected option
  if (dataReceived && waterNow(days, morningTime, eveningTime)) {
    //send mode to other arduinos
  lcd.setCursor(0, 1);
  lcd.print("                ");
  scrollInFromRight(1, "Mode Broadcasting");
  delay(3000);

  //wait for ack from all arduinos
  lcd.setCursor(0, 1);
  lcd.print("                ");
  scrollInFromRight(1, "Received Ack from subscibers");
  delay(3000);
    startWatering();
  }

}

