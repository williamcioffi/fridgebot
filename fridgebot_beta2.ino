#include "Adafruit_FONA.h"
#include <SoftwareSerial.h>
#include "Adafruit_MCP9808.h"

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
Adafruit_MCP9808 tempsens = Adafruit_MCP9808();

uint8_t type;
String getrealtime();

String realtimeformatted_;

void setup() {
  while (!Serial);

  Serial.begin(115200);

  if(!tempsens.begin()) {
    Serial.println("Couldn't find MCP9808!");
    while (1);
  }

  Serial.println(F("FONA basic test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  fonaSerial->begin(4800);
  if (!fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  type = fona.type();
  Serial.println(F("FONA is OK"));
  Serial.print(F("Found "));
  switch (type) {
    case FONA800L:
      Serial.println(F("FONA 800L")); break;
    case FONA800H:
      Serial.println(F("FONA 800H")); break;
    case FONA808_V1:
      Serial.println(F("FONA 808 (v1)")); break;
    case FONA808_V2:
      Serial.println(F("FONA 808 (v2)")); break;
    case FONA3G_A:
      Serial.println(F("FONA 3G (American)")); break;
    case FONA3G_E:
      Serial.println(F("FONA 3G (European)")); break;
    default: 
      Serial.println(F("???")); break;
  }
  
  // Print module IMEI number.
  char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("Module IMEI: "); Serial.println(imei);
  }
  //enable RTC
  //int success;
  //success = fona.enableRTC(1);
  //Serial.println(success);

  //set up sensors

  //initialize the time
  realtimeformatted_ = getrealtime();
}

void writereadings(int lightread, float tempread, String realtimeformatted);
void tweet();
void alert();
uint8_t getminute();
uint8_t getday();
uint8_t getmonth();

int doorcounts_ = 0;
int lightpin_ = 0;
boolean dooropen_ = 0;

uint8_t year_   = 0,
    month_  = 0,
    day_    = 0,
    hour_   = 0,
    minute_ = 0,
    sec_    = 0;

unsigned long prevtimelight_ = 0;
unsigned long prevtime_ = 0;
unsigned long interval_ = 60000;

float tempsum_ = 0.0;
float tempmax_ = 0.0;
float tempmin_ = 100.0;
int tempnum_ = 0;

int textnum_ = 0;

char TWITTER_[6] = "40404";
// char CIOFFI_[11] = "0000000000"; // this would be my phone number
// String tweetatthisperson_ = "@KCBierlich";

#define MAXTEXTSPERMONTH 35

void loop() {
  unsigned long curtime = millis();

  //eventually millis() will roll over (after about 50 days) and so the expression 
  //will be negative. in this case you'll have to reset prevtime or it'll never fulfill
  //the next condition.
  if(curtime - prevtime_ < 0) {
    prevtime_ = curtime;
  }

  int lightread = analogRead(lightpin_);

  if(curtime - prevtimelight_ > 1000) {
    prevtimelight_ = curtime;
    boolean dooropen = 0;

    if(lightread > 0) {
      dooropen = 1;
    } else if(lightread == 0) {
      dooropen = 0;
    }

    if(dooropen != dooropen_) {
      dooropen_ = dooropen;
      if(dooropen) {
        doorcounts_++;
      }
    }
  }
  
  if(curtime - prevtime_ > interval_) {
    prevtime_ = curtime;
    uint8_t day = getday();
    boolean newday = 0;
    boolean newmonth = 0;
    
    if(day_ != day) {
      day_ = day;
      newday = 1;

      if(month_ != getmonth()) {
        newmonth = 1;
      }
    }

    realtimeformatted_ = getrealtime();

    float tempread = tempsens.readTempC();
    tempsum_ += tempread;
    tempnum_++;
    if(tempread > tempmax_) {
      tempmax_ = tempread;
    }
    if(tempread < tempmin_) {
      tempmin_ = tempread;
    }
    
    writereadings(lightread, tempread, realtimeformatted_);

    if(newday) {
      tweet();
      tempnum_ = 0;
      tempsum_ = 0.0;
      tempmax_ = 0.0;
      tempmin_ = 100.0;
      doorcounts_ = 0;

      if(newmonth) {
        textnum_ = 0;
      }
    }
  }
}



//functions
String getrealtime() {
  String cyear = "",
         cmonth = "",
         cday = "",
         chour = "",
         cminute = "",
         csec = "",
         formatted_date = "";
       
  char tbuffer[23];
  fona.getTime(tbuffer, 23);
  
  cyear += tbuffer[1];
  cyear += tbuffer[2];
  cmonth += tbuffer[4];
  cmonth += tbuffer[5];
  cday += tbuffer[7];
  cday += tbuffer[8];
  chour += tbuffer[10];
  chour += tbuffer[11];
  cminute += tbuffer[13];
  cminute += tbuffer[14];
  csec += tbuffer[16];
  csec += tbuffer[17];

  formatted_date = "20" + cyear + "-" + cmonth + "-" + cday + " " + chour + ":" + cminute + ":" + csec;
  year_ = cyear.toInt();
  month_ = cmonth.toInt();
  day_ = cday.toInt();
  hour_ = chour.toInt();
  minute_ = cminute.toInt();
  sec_ = csec.toInt();

  return(formatted_date);
}

void writereadings(int lightread, float tempread, String realtimeformatted) {
  String output = "light:";
  output += lightread; 
  output += " temp:"; 
  output += tempread;
  output += " time:";
  output += realtimeformatted;
  output += " door opens:";
  output += doorcounts_;
  
  Serial.println(output);
}

void tweet() {
  String message = "hi";
  float mean_temp = 0;
  
  mean_temp = tempsum_ / (tempnum_);
  
  message += ", ";
  //message += tweetatthisperson_;
  //message += ",";
  message += " it's me, fridgebot.";
  message += " my mean temp today was ";
  message += mean_temp;
  message += " degC";
  message += " [";
  message += tempmin_;
  message += ", ";
  message += tempmax_;
  if(doorcounts_ == 0) {
    message += "]. nobody opened me today : /";
  } else {
    message += "]. i was opened ";
    message += doorcounts_;
    message += " time";
    if(doorcounts_ == 1) {
      message += ".";
    } else {
      message += "s.";
    }
  }
  
  if(textnum_ < MAXTEXTSPERMONTH) {
    char sendbuffer[144];
    message.toCharArray(sendbuffer, message.length() + 1);
    fona.sendSMS(TWITTER_, sendbuffer);
//    fona.sendSMS(CIOFFI_, sendbuffer);
    textnum_++; 
  }
}

void alert() {
  textnum_++;
}

uint8_t getminute() {
  String temp = "";
  char tbuffer[23];
  uint8_t out = 99;

  fona.getTime(tbuffer, 23);
  temp = String(tbuffer);
  temp = temp.substring(13, 15);
  out = temp.toInt();
  
  return(out);
}

uint8_t getday() {
  String temp = "";
  char tbuffer[23];
  uint8_t out = 99;

  fona.getTime(tbuffer, 23);
  temp = String(tbuffer);
  temp = temp.substring(7, 9);
  out = temp.toInt();
  
  return(out);
}

uint8_t getmonth() {
  String temp = "";
  char tbuffer[23];
  uint8_t out = 99;

  fona.getTime(tbuffer, 23);
  temp = String(tbuffer);
  temp = temp.substring(4, 6);
  out = temp.toInt();
  
  return(out);
}
