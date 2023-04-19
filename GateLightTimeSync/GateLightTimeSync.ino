#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <time.h>
#include <ArduinoJson.h>

#ifndef APSSID
#define APSSID "ESPap"
#define APPSK "thereisnospoon"
#endif

int OnTimeHrs = 18, OnTimeMins = 30, OffTimeHrs = 21, OffTimeMins = 0;

unsigned long timeNow = 0;
unsigned long timeLast = 0;

int ledState = LOW;
unsigned long previousMillis = 0;
const long interval = 100;

bool timeSet = false;

StaticJsonDocument<200> doc;

//Time start Settings:
int startingHour = 12;
int seconds = 0;
int minutes = 0;
int hours = startingHour;
int days = 0;

char h[3], m[3], s[3], h1[3], m1[3];

//Accuracy settings
int dailyErrorFast = 0; // set the average number of milliseconds your microcontroller's time is fast on a daily basis
int dailyErrorBehind = 0; // set the average number of milliseconds your microcontroller's time is behind on a daily basis

int correctedToday = 1; // do not change this variable, one means that the time has already been corrected today for the error in your boards crystal. This is true for the first day because you just set the time when you uploaded the sketch.  

const char *ssid = APSSID;
const char *password = APPSK;

ESP8266WebServer server(80);

void handleRoot() {
  server.send(200, "text/html", String() + "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'><style type='text/css'>*{margin: 10pt;}</style></head><body><script>let t = new Date(); fetch('/date', { method: 'POST',  headers: {'Accept': 'application/json', 'Content-Type': 'application/json'},body: JSON.stringify({ 'hrs': t.getHours(), 'mins': t.getMinutes(), 'sec': t.getSeconds()})}).then(response => console.log(response))</script><h1><center>Gate Light</center></h1><br><label for='appt'>ON time  :</label><input type='time' id='on' name='appt' value='" + OnTimeHrs + ":" + OnTimeMins + "'><br><label for='appt'>OFF time :</label><input type='time' id='off' name='appt' value='" + OffTimeHrs + ":" + OffTimeMins + "'><br><button onclick='sendTimings()'>Update On/Off Timing</button><script>function sendTimings(){var on = document.getElementById('on').value.split(':');var off = document.getElementById('off').value.split(':');fetch('/setOnOffTiming', {method: 'POST',headers: {'Accept': 'application/json', 'Content-Type': 'application/json'},body: JSON.stringify({ 'OnTimeHrs': on[0],'OnTimeMins': on[1],'OffTimeHrs': off[0],'OffTimeMins': off[1]})}).then(response => console.log(response))}</script></body></html>");
}

void handleDate() {
  if (server.hasArg("plain")== false){
    server.send(200, "text/plain", "Body not received");
    return;
  }
  String date = server.arg("plain");
  server.send(200, "text/plain", date);
  Serial.println(date);
  deserializeJson(doc, date);
  //Set time
  seconds = doc["sec"];
  timeLast = timeNow - seconds;
  minutes = doc["mins"];
  hours = doc["hrs"];

  timeSet = true;
  digitalWrite(D0, HIGH);
}

void handleGetTime() {
  sprintf(h, "%02d", hours); //for '0' append
  sprintf(m, "%02d", minutes);
  sprintf(s, "%02d", seconds);
  server.send(200, "text/html", String(days) + "d " + h + ":" + m + ":" + s);
}

void handleSetOnOffTiming() {
  if (server.hasArg("plain")== false){
    server.send(200, "text/plain", "Body not received");
    return;
  }
  String OnOffTime = server.arg("plain");
  Serial.println(OnOffTime);
  deserializeJson(doc, OnOffTime);

  OnTimeHrs = doc["OnTimeHrs"];
  OnTimeMins = doc["OnTimeMins"];
  OffTimeHrs = doc["OffTimeHrs"];
  OffTimeMins = doc["OffTimeMins"];
  server.send(200, "text/plain", "OK");
}

void handleGetOnOffTiming() {
  sprintf(h, "%02d", OnTimeHrs); //for '0' append
  sprintf(m, "%02d", OnTimeMins);
  sprintf(h1, "%02d", OffTimeHrs);
  sprintf(m1, "%02d", OffTimeMins);
  server.send(200, "text/html", String(days) + "d<br>OnTime" + h + ":" + m + "<br>OffTime" + h1 + ":" + m1);
}

void setup() {
  pinMode(D0, OUTPUT);
  digitalWrite(D0,HIGH);
  pinMode(D4, OUTPUT);
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/date", handleDate);
  server.on("/getTime", handleGetTime);
  server.on("/setOnOffTiming", handleSetOnOffTiming);
  server.on("/getOnOffTiming", handleGetOnOffTiming);
  server.begin();
  Serial.println("HTTP server started");
  digitalWrite(D4,HIGH);
}

void loop() {
  server.handleClient();
  timeNow = millis()/1000; // the number of milliseconds that have passed since boot
  seconds = timeNow - timeLast;//the number of seconds that have passed since the last time 60 seconds was reached.

  if (seconds == 60) {
    timeLast = timeNow;
    minutes = minutes + 1;
  }//if one minute has passed, start counting milliseconds from zero again and add one minute to the clock.

  if (minutes == 60){ 
    minutes = 0;
    hours = hours + 1;
  }// if one hour has passed, start counting minutes from zero and add one hour to the clock

  if (hours == 24){
    hours = 0;
    days = days + 1;
  }//if 24 hours have passed , add one day

  if (hours ==(24 - startingHour) && correctedToday == 0){
    delay(dailyErrorFast*1000);
    seconds = seconds + dailyErrorBehind;
    correctedToday = 1;
  }
  //every time 24 hours have passed since the initial starting time and it has not been reset this day before, add milliseconds or delay the progran with some milliseconds. 
  //Change these varialbes according to the error of your board. 
  // The only way to find out how far off your boards internal clock is, is by uploading this sketch at exactly the same time as the real time, letting it run for a few days 
  // and then determine how many seconds slow/fast your boards internal clock is on a daily average. (24 hours).

  if (hours == 24 - startingHour + 2) { 
    correctedToday = 0;
  }
  //let the sketch know that a new day has started for what concerns correction, if this line was not here the arduiono
  // would continue to correct for an entire hour that is 24 - startingHour. 
  // Serial.print("The time is:           ");
  // Serial.print(days);
  // Serial.print(":");
  // Serial.print(hours);
  // Serial.print(":");
  // Serial.print(minutes);
  // Serial.print(":"); 
  // Serial.println(seconds); 

  if(!timeSet){
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;

      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }
    }
    // set the LED with the ledState of the variable:
    digitalWrite(D0, ledState);
  }

  if(hours == OnTimeHrs & minutes == OnTimeMins & seconds == 0)
  {
    digitalWrite(D4, LOW);
  }
  if(hours == OffTimeHrs & minutes == OffTimeMins & seconds == 0)
  {
    digitalWrite(D4, HIGH);
  }
}
