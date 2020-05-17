/*******************************************************************************
 * Copyright (c) 2017 Federico Di Marco <fededim@gmail.com>                    *
 *                                                                             *
 * Permission is hereby granted, free of charge, to any person obtaining a     *
 * copy of this software and associated documentation files (the "Software"),  *
 * to deal in the Software without restriction, including without limitation   *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
 * and/or sell copies of the Software, and to permit persons to whom the       *
 * Software is furnished to do so, subject to the following conditions:        *
 *                                                                             *
 * The above copyright notice and this permission notice shall be included in  *
 * all copies or substantial portions of the Software.                         *
 *                                                                             *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
 * DEALINGS IN THE SOFTWARE.                                                   *
 *                                                                             *
 *                                                                             *
 *******************************************************************************/
 
#include <ESP8266WiFi.h>
#include <mem.h>
#include "FS.h"
#include <NTPClient.h>
#include "WiFiUdp.h"
#include <string.h>
#include <ArduinoJson.h>

#include "MAX31855.h"
#include "PidAutotuneControl.h"
#include "PidControl.h"
#include "OnOffControl.h"
#include "configuration.h"
#include "LowPassFilter.h"

#include "pitches.h"


#define SSID  "<SSID>"         // wifi ssid to connect
#define PASSWORD "<PASSWORD>"  // ssid password

#define PID_WINDOW_SIZE 5000  // PID window size in ms

#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "europe.pool.ntp.org"

#define DELTA 1  // OnOff delta abs value


// ESP 8266
// The I2C Bus signals SCL and SDA have been assigned to D1 and D2 (GPIO5 & GPIO4) while the four
// SPI Bus signals (SCK, MISO, MOSI & SS) have been assigned to GPIO pins 14, 12, 13 and 15, respectively)
  

#define GPIO16_RELAY
#define GPIO05_BUZZER

#define PIN_RELAY1          D8  
#define PIN_THERMO1         D3
#define PIN_THERMO2         D4

#ifdef  GPIO16_RELAY
  #define PIN_RELAY2        D0
#else
  #define PIN_NINT          D0
  #define PIN_RELAY2        114 // on the sx1509 > 100
#endif


#ifdef  GPIO05_BUZZER
  #define PIN_BUZZER        D1
#else
  // I2C SCL is by default D1 (GPIO05)
  #define PIN_BUZZER        115 // on the sx1509 >100
#endif


// Board configuration (edit accordingly)

#define PIN_RELAY_CHAMBER   PIN_RELAY1
#define PIN_RELAY_STONE     PIN_RELAY2

MAX31855 probeChamber(PIN_THERMO1);
MAX31855 probeStone(PIN_THERMO2);


// State variables
double tempChamber, tempStone, setChamber = 100, setStone=200,cjChamber=0, cjStone=0;
int timer, starttimer, chamberStatus, stoneStatus;
bool started=false;

unsigned long start; // test 
int ste=0;  // test



#define stricmp strcasecmp

WiFiServer server(80);

// For ntp sync
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

Configuration *conf;

IControl *chamberControl,*stoneControl;
LowPassFilter *chamberTempFilter,*stoneTempFilter;


class RelayAction: public IControlAction {

public:    
    RelayAction (int pin, bool reverse) : IControlAction (pin,reverse) { }
    
    virtual bool Active() override {
      return (digitalRead(pin)==von)?true:false;
    }
    
    virtual void On () override {
      digitalWrite(pin,von);  
    }

    virtual void Off() override {
      digitalWrite(pin,voff);        
    }
};


RelayAction actChamber(PIN_RELAY_CHAMBER,false); // arduino relay module has inverse logic LOW --> ON, HIGH --> OFF
RelayAction actStone(PIN_RELAY_STONE,false); // arduino relay module has inverse logic LOW --> ON, HIGH --> OFF

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  Serial.println("\n\nEspOven: V1.0\n");
  
  pinMode(PIN_RELAY_CHAMBER, OUTPUT);
  pinMode(PIN_RELAY_STONE, OUTPUT);


  actChamber.Off();
  actStone.Off();
  
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER,LOW);  // turn off the speaker

  Serial.printf("\n\nEspOven: Chamber %d Stone %d\n",digitalRead(PIN_RELAY_CHAMBER),digitalRead(PIN_RELAY_STONE)); 
  
  if (!SPIFFS.begin())
    Serial.printf("EspOven: Error mounting SPIFFS\n");

  CheckConnectWifi();
  //server.onNotFound(handleNotFound);

  timeClient.begin();
  timeClient.update();

  server.begin();
  Serial.printf("EspOven: Web server started, open %s in a web browser port 80\n", WiFi.localIP().toString().c_str());

  conf=new Configuration();
  if (!conf->Load()) {
    // Default configuration if flash memory is uninitialised
    conf->enable1=false;    // chamber heater and probe present
    conf->enable2=true;     // stone heater and probe present
    conf->kp1=100;          // PID KP constant for chamber heater control
    conf->ki1=5;            // PID KI constant for chamber heater control
    conf->kd1=1;            // PID KD constant for chamber heater control
    conf->kp2=100;          // PID KP constant for stone heater control
    conf->ki2=5;            // PID KI constant for stone heater control
    conf->kd2=1;            // PID KD constant for stone heater control
    conf->alpha1=0.3;       // Lowpass filter alpha value for chamber temperature
    conf->alpha2=0.3;       // Lowpass filter alpha value for stone temperature
    conf->control1=ControlType::OnOff;  // Control Type for chamber heater
    conf->control2=ControlType::OnOff;  // Control Type for stone heater
  }
  
  UpdateParams();
  
  start=millis();
}



void UpdateParams() {  
  // Low pass filters
  if (chamberTempFilter!=NULL)
    delete chamberTempFilter;

  if (stoneTempFilter!=NULL)
    delete stoneTempFilter;

  chamberTempFilter=new LowPassFilter(conf->alpha1);
  stoneTempFilter=new LowPassFilter(conf->alpha2);

  // Chamber control
  if (chamberControl!=NULL)
    delete chamberControl;

  if (conf->control1==ControlType::OnOff)
    chamberControl=new OnOffControl("chamber",&actChamber,&setChamber,&tempChamber,DELTA);
  else if (conf->control1==ControlType::PID)
    chamberControl=new PidControl("chamber",&actChamber,&setChamber,&tempChamber,conf->kp1,conf->ki1,conf->kd1,PID_WINDOW_SIZE);
  else
    chamberControl=new PidAutotuneControl("chamber",&actChamber,&setChamber,&tempChamber,conf->kp1,conf->ki1,conf->kd1,PID_WINDOW_SIZE);

  
  // Stone control
  if (stoneControl!=NULL)
    delete stoneControl;
  
  if (conf->control2==ControlType::OnOff)
    stoneControl=new OnOffControl("stone",&actStone,&setChamber,&tempChamber,DELTA);
  else if (conf->control2==ControlType::PID)
    stoneControl=new PidControl("stone",&actStone,&setChamber,&tempChamber,conf->kp2,conf->ki2,conf->kd2,PID_WINDOW_SIZE);  
  else
    stoneControl=new PidAutotuneControl("stone",&actStone,&setChamber,&tempChamber,conf->kp2,conf->ki2,conf->kd2,PID_WINDOW_SIZE);  
    
}



void CheckConnectWifi()
{
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.hostname("espoven");
    WiFi.begin(SSID, PASSWORD);
  
    Serial.printf("EspOven: Connecting to %s", SSID);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.printf("\nEspOven: Connected, IP address: %s Hostname %s\n", WiFi.localIP().toString().c_str(),WiFi.hostname().c_str());
  }
}




// very simple
void GetMimeTypeFromFile(char *path, char *mime) {
  char *dot = strrchr(path, '.') + 1;

  //Serial.printf("EspOven: GetMimeTypeFromFile starts %s %s\n",path,dot);

  if (dot != NULL) {
    if (stricmp(dot, "html") == 0 || stricmp(dot, "htm") == 0)
      strcpy(mime, "text/html;; charset=iso-8859-1");
    else if (stricmp(dot, "jpg") == 0)
      strcpy(mime, "image/jpeg");
    else if (stricmp(dot, "png") == 0)
      strcpy(mime, "image/png");
    else if (stricmp(dot, "gif") == 0)
      strcpy(mime, "image/gif");
    else if (stricmp(dot, "txt") == 0)
      strcpy(mime, "text/plain; charset=iso-8859-1");
    else if (stricmp(dot, "js") == 0)
      strcpy(mime, "text/javascript; charset=iso-8859-1");
    else if (stricmp(dot, "css") == 0)
      strcpy(mime, "text/css; charset=iso-8859-1");
    else
      strcpy(mime, "application/octet-stream");
  }
  else
    strcpy(mime, "application/octet-stream");

  //Serial.printf("EspOven: GetMimeTypeFromFile ends %s\n",mime);
}



char tstamp[32];
char *getTimestamp() {
  unsigned long rawTime = timeClient.getEpochTime();

  sprintf(tstamp, "%02d:%02d:%02d.%03d", (rawTime % 86400L) / 3600, (rawTime % 3600) / 60, rawTime % 60, millis() % 1000);

  return tstamp;
}


void parseQueryString(char *querystring, JsonObject ht) {
  char *paramstk, *paramsend, *key, *value, *paramend;

  // split parameters by &
  paramstk = strtok_r(querystring, "&", &paramsend);

  while ( paramstk != NULL )
  {
    //Serial.printf("EspOven: Analyzing %s\n", paramstk);

    // we should urldecode the data, but we keep things "simple", no unallowed chars are passed, only integers
    // extract key = val
    key = strtok_r(paramstk, "=", &paramend);
    value = strtok_r(NULL, "=", &paramend);
    if (value == NULL)
      value = "";

    ht[key]=value;
    Serial.printf("EspOven: Parsed key %s value %s\n", key,value);

    paramstk = strtok_r(NULL, "&", &paramsend);
  }
}



bool HandleCGI(char *path, char *resp, char *body) {
  char buf[256];
  bool handled = true;

  StaticJsonDocument<128> jsonBuffer;
  JsonObject ht = jsonBuffer.to<JsonObject>();

  if (strncmp(path, "/getsensordata.cgi", 18) == 0) {
    // JSON object
    sprintf(buf, "{ \"tempChamber\":%f, \"tempStone\":%f,\"timer\":%d,\"chamberStatus\":%d, \"stoneStatus\":%d, \"started\":%d }", tempChamber, tempStone, (timer!=0)?(timer-(millis()-starttimer)/1000):0, chamberStatus,stoneStatus, started);

    sprintf(resp, "HTTP/1.1 200 OK\nServer: EspOven (Esp8266)\nContent-Length: %d\nConnection: Closed\nCache-Control: no-cache, no-store, must-revalidate\nContent-Type: application/json\n\n%s", strlen(buf), buf);
  }
  else if (strncmp(path, "/getconf.cgi", 12) == 0) {
    conf->GetJson(buf,256);
    
    sprintf(resp, "HTTP/1.1 200 OK\nServer: EspOven (Esp8266)\nContent-Length: %d\nConnection: Closed\nCache-Control: no-cache, no-store, must-revalidate\nContent-Type: application/json\n\n%s", strlen(buf), buf);
  }
  else if (strncmp(path, "/setconf.cgi", 12) == 0) {
    if (!started) {
      bool res=conf->SetJson(body);
      if (res) {
        conf->Save();
        UpdateParams();
      }
      
      const char *result=res?"true":"false";   
      sprintf(resp, "HTTP/1.1 200 OK\nServer: EspOven (Esp8266)\nContent-Length: %d\nConnection: Closed\nCache-Control: no-cache, no-store, must-revalidate\nContent-Type: application/json\n\n%s",strlen(result),result);
    }
    else
      sprintf(resp,"HTTP/1.1 405 Not Allowed\nConfiguration can only be changed when oven is turned off\nConnection: Closed\n\n");           
  }
  else if (strncmp(path, "/setparams.cgi", 14) == 0) {
    path += 15; // we skip url
    parseQueryString(path, ht);

    int c, s;

    c = ht["setChamber"].as<int>();
    s = ht["setStone"].as<int>();

    Serial.printf("EspOven: setparams.cgi parsed setChamber %d setStone %d timer %d started %d\n", c,s, ht["timer"].as<int>(), ht["started"].as<int>());

    // basic checks on temperature
    if (c > 20 && c < 380)
      setChamber = c;

    if (s > 20 && s < 380)
      setStone = s;

    // if oven is turned on we can update the temperatures, but not the timer, we check on previous value
    if (!started) {
      timer=ht["timer"].as<int>();

      if (timer!=0) {
        starttimer=millis();
        Serial.printf("EspOven: Timer set %ds starttimer %d\n",timer,starttimer);
      }
    }
    
    started = ht["started"].as<int>();
    if (started==0)
      timer=0;

    sprintf(resp, "HTTP/1.1 200 OK\nServer: EspOven (Esp8266)\nContent-Length: 0\nCache-Control: no-cache, no-store, must-revalidate\nConnection: Closed\n\n");
  }
  else
    handled = false;

  return handled;
}


void handleHttpRequests() {
  int i, len, k;
  char *meth, *path, *ver;
  char *buf, *resp, *mime,*body;
  unsigned long start, elapsed;
  double speed;
  File f;

  // check for a client (web browser)
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  Serial.printf("\nEspOven: Client connected from %s:%d (freeheap %d)\n", client.remoteIP().toString().c_str(), client.remotePort(), ESP.getFreeHeap());

  if (client.connected()) {
    // Wait 30ms to allow the client to send some data
    // there are no threads so we can't block and we have to poll and disconnect idling clients
    i = millis();
    while (!client.available()) {
      yield();
      if ((millis() - i) > 30) {
        Serial.printf("EspOven: client %s timeout, disconnecting client\n", client.remoteIP().toString().c_str());
        client.stop();
        return;
      }
    }

    // read first line of request by client
    String req = client.readStringUntil('\r'); // readString();
    Serial.printf("EspOven: received request:\n%s\n", &req[0]);

    meth = strtok(&req[0], " ");
    path = strtok(NULL, " ");
    ver = strtok(NULL, " ");

    printf("EspOven: method %s path %s ver %s (freeheap %d)\n", meth, path, ver, ESP.getFreeHeap());

    // AllocMemory

    resp = (char *) os_malloc(512);
    mime = (char *) os_malloc(64);

    if (stricmp(meth, "GET") == 0) {
      if (HandleCGI(path, resp, NULL)) {
        client.print(resp);
      }
      else if (!SPIFFS.exists(path)) {
        Serial.printf("EspOven: path %s not found\n", path);
        sprintf(resp, "HTTP/1.1 404 Not Found\nServer: EspOven (Esp8266)\nContent-Length: %d\nConnection: Closed\nContent-Type: text/html; charset=iso-8859-1\n\n<!DOCTYPE html>\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\n<p>The requested URL %s was not found on this server.</p>\n</body></html>", 167 + strlen(path), path);
        client.print(resp);
      }
      else {
        // since there is no preemptive multitasking and we need to send all file to the client we hope that the transfer won't take too much
        // otherwise all other functions (temperature control) will get stucked :( with short webpages and images it shouldn't be an issue.
        f = SPIFFS.open(path, "r");
        GetMimeTypeFromFile(path, mime);

        int len = f.size();

        Serial.printf("EspOven (%s): path %s found, file len %d mime %s\n", getTimestamp(), path, len, mime);
        start = millis();

        sprintf(resp, "HTTP/1.1 200 OK\nServer: EspOven (Esp8266)\nContent-Length: %d\nConnection: Closed\nContent-Type: %s\n\n", len, mime);
        client.print(resp);

        buf = (char *) os_malloc(8192);

        k = 0;
        while (f.available() > 0) {
          i = f.readBytes(buf, 8192);

          client.write((const uint8_t *) buf, i);
        }

        elapsed = millis() - start;
        speed = len / elapsed;
        Serial.printf("EspOven (%s): transferred %d bytes to client, elapsed %dms speed %.2f KB/s\n", getTimestamp(), len, elapsed, speed);

        os_free(buf);

        f.close();
      }
    }
    if (stricmp(meth, "POST") == 0) {
        char *header,*value;
        
        // Process headers
        while (true) {
          String req = client.readStringUntil('\r');
          header = strtok(&req[0], ":");
          value = strtok(NULL, "\r");

          Serial.printf("Read header %s value %s\n",header,value);
          
          if (strlen(header)==0)
            break;
          else if (stricmp(header,"Content-Type")==0 && stricmp(value,"application/json")!=0) {
            client.print("HTTP/1.1 405 Not Allowed\nAllow: GET, POST (only with application/json)\nConnection: Closed\n\n");           
            goto end;
          }
        }

        body = (char *) os_malloc(512);
        client.readBytes(body,512);
        
        if (HandleCGI(path, resp, body)) {
          client.print(resp);
        }

        os_free(body);
    }
    else {
      client.print("HTTP/1.1 405 Not Allowed\nAllow: GET, POST (only with application/json)\nConnection: Closed\n\n");
    }

end:
    os_free(resp);
    os_free(mime);
  }

  // close the connection:
  Serial.printf("EspOven: Client %s:%d disconnecting (freeheap %d)\n", client.remoteIP().toString().c_str(), client.remotePort(), ESP.getFreeHeap());
  client.stop();
}



void handleOvenHeating() {
  //Serial.printf("EspOven: handleOvenHeating\n");

  if (conf->enable1) {
    probeChamber.sampleProbe();
    chamberStatus = probeChamber.checkStatus();

    if (chamberStatus == MAX31855::OK) {
      tempChamber = probeChamber.readTemp();
    }
      
    cjChamber = probeChamber.readCJTemp();

    double tempChamberSmoothed=chamberTempFilter->GetFilteredValue(tempChamber);

    Serial.printf("EspOven: handleOvenHeating CHAMBER actual %f (smoothed %f) set %f cj %f status %d\n",tempChamber,tempChamberSmoothed,setChamber,cjChamber,chamberStatus);

    tempChamber=tempChamberSmoothed;

    chamberControl->Control(started);
  }

  
  if (conf->enable2) {
     probeStone.sampleProbe();
  
     stoneStatus = probeStone.checkStatus();

     if (stoneStatus == MAX31855::OK) {
        tempStone = probeStone.readTemp();
     }

     cjStone = probeStone.readCJTemp();

     double tempStoneSmoothed=chamberTempFilter->GetFilteredValue(tempStone);

     Serial.printf("EspOven: handleOvenHeating STONE actual %f (smoothed %f) set %f cj %f status %d\n",tempStone,tempStoneSmoothed,setStone,cjStone,stoneStatus);

     tempStone=tempStoneSmoothed;

     stoneControl->Control(started);
  }
}



int numNote=0;

int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

void PlayNote() {
    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[numNote];
    tone(PIN_BUZZER, melody[numNote], noteDuration);

    // stop the tone playing:
    noTone(8);

    numNote=(++numNote)%8;
  }

bool flag=true;


void loop() {
  CheckConnectWifi();
  
  handleHttpRequests();

  // stop timer
  if (timer!=0 && timer<((millis()-starttimer)/1000)) {
    Serial.printf("\nEspOven: timer %d elapsed %d turning off oven\n\n",timer,millis()-starttimer);
    started=0;
    timer=0;
  }

  handleOvenHeating();

  //digitalWrite(PIN_MAX31855_CHAMBER,flag);
  //flag=!flag;
  
  //PlayNote();
  
  Serial.println(" ");

  delay(100);
}
