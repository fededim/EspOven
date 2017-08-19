#include <ESP8266WiFi.h>
#include <mem.h>
#include "FS.h"
#include <NTPClient.h>
#include "WiFiUdp.h"
#include <string.h>
#include <ArduinoJson.h>

#include "MAX31855.h"
#include "PidControl.h"
#include "OnOffControl.h"
#include "configuration.h"
#include "LowPassFilter.h"

#include "pitches.h"

//const int doPin = 13;
//const int clPin = 14;

// ESP 8266
// The I2C Bus signals SCL and SDA have been assigned to D1 and D2 (GPIO5 & GPIO4) while the four
// SPI Bus signals (SCK, MISO, MOSI & SS) have been assigned to GPIO pins 14, 12, 13 and 15, respectively)

// State variables
double tempChamber, tempStone, setChamber = 100, setStone=200,cjChamber=0, cjStone=0;
int timer, starttimer, chamberStatus, stoneStatus;
bool started=false;

unsigned long start; // test 
int ste=0;  // test

#define GPIO16_RELAY
#define GPIO05_BUZZER

#define WINDOW_SIZE 15000  // 15s

#define PIN_RELAY_CHAMBER D8

#ifdef  GPIO16_RELAY
  #define PIN_RELAY_STONE   D0
#else
  #define PIN_NINT   D0
  #define PIN_RELAY_STONE   114 // on the sx1509 > 100
#endif


#ifdef  GPIO05_BUZZER
  #define PIN_BUZZER        D1
#else
  // I2C SCL is by default D1 (GPIO05)
  #define PIN_BUZZER        115 // on the sx1509 >100
#endif

MAX31855 probeChamber(D3);
MAX31855 probeStone(D4);

#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "europe.pool.ntp.org"

#define DELTA 0.02  // 2%

#define stricmp strcasecmp

const char* ssid = "Telecom-2G";
const char* password = "!provolone!";

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
  actStone.On();
  
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
  conf->Load();

  conf->enable1=true;
  conf->enable2=true;
  conf->kp2=100;
  conf->ki2=5;
  conf->kd2=1;
  conf->alpha1=1;
  conf->alpha2=1;
  conf->control1=ControlType::OnOff;

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
  else
    chamberControl=new PidControl("chamber",&actChamber,&setChamber,&tempChamber,conf->kp1,conf->ki1,conf->kd1,WINDOW_SIZE);

  
  // Stone control
  if (stoneControl!=NULL)
    delete stoneControl;
  
  if (conf->control2==ControlType::OnOff)
    stoneControl=new OnOffControl("stone",&actStone,&setChamber,&tempChamber,DELTA);
  else
    stoneControl=new PidControl("stone",&actStone,&setChamber,&tempChamber,conf->kp2,conf->ki2,conf->kd2,WINDOW_SIZE);  
    
}



void CheckConnectWifi()
{
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
  
    Serial.printf("EspOven: Connecting to %s", ssid);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.printf("\nEspOven: Connected, IP address: %s\n", WiFi.localIP().toString().c_str());
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


void parseQueryString(char *querystring, JsonObject& ht) {
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

    ht.set(key, value);
    Serial.printf("EspOven: Parsed key %s value %s\n", key,value);

    paramstk = strtok_r(NULL, "&", &paramsend);
  }
}



bool HandleCGI(char *path, char *resp) {
  char buf[256];
  bool handled = true;

  StaticJsonBuffer<128> jsonBuffer;
  JsonObject& ht = jsonBuffer.createObject();

  if (strncmp(path, "/getsensordata.cgi", 18) == 0) {
    // JSON object
    sprintf(buf, "{ \"tempChamber\":%f, \"tempStone\":%f,\"timer\":%d,\"chamberStatus\":%d, \"stoneStatus\":%d, \"started\":%d }", tempChamber, tempStone, (timer!=0)?(timer-(millis()-starttimer)/1000):0, chamberStatus,stoneStatus, started);

    sprintf(resp, "HTTP/1.1 200 OK\nServer: EspOven (Esp8266)\nContent-Length: %d\nConnection: Closed\nCache-Control: no-cache, no-store, must-revalidate\nContent-Type: application/json\n\n%s", strlen(buf), buf);
  }
  else if (strncmp(path, "/setparams.cgi", 14) == 0) {
    path += 15; // we skip url
    parseQueryString(path, ht);

    int c, s;

    c = ht.get<int>("setChamber");
    s = ht.get<int>("setStone");

    Serial.printf("EspOven: setparams.cgi parsed setChamber %d setStone %d timer %d started %d\n", c,s, ht.get<int>("timer"), ht.get<int>("started"));

    // basic checks on temperature
    if (c > 20 && c < 380)
      setChamber = c;

    if (s > 20 && s < 380)
      setStone = s;

    // if oven is turned on we can update the temperatures, but not the timer, we check on previous value
    if (!started) {
      timer=ht.get<int>("timer");

      if (timer!=0) {
        starttimer=millis();
        Serial.printf("EspOven: Timer set %ds starttimer %d\n",timer,starttimer);
      }
    }
    
    started = ht.get<int>("started");
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
  char *buf, *resp, *mime;
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

    buf = (char *) os_malloc(8192);
    resp = (char *) os_malloc(512);
    mime = (char *) os_malloc(64);

    if (stricmp(meth, "GET") == 0) {
      if (HandleCGI(path, resp)) {
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

        k = 0;
        while (f.available() > 0) {
          i = f.readBytes(buf, 8192);
          //Serial.printf("EspOven (%s): read %d bytes to buffer (%d): %02x %02x %02x %02x %02x %02x %02x %02x (freeheap %d)\n",getTimestamp(),i,k,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],ESP.getFreeHeap());

          client.write((const uint8_t *) buf, i);
          //Serial.printf("EspOven (%s): written %d bytes to client (%d) (freeheap %d)f\n",getTimestamp(),i,k++,ESP.getFreeHeap());
        }

        elapsed = millis() - start;
        speed = len / elapsed;
        Serial.printf("EspOven (%s): transferred %d bytes to client, elapsed %dms speed %.2f KB/s\n", getTimestamp(), len, elapsed, speed);

        f.close();
      }
    }
    else {
      client.print("HTTP/1.1 405 Not Allowed\nAllow: GET\nConnection: Closed\n\n");
    }

    os_free(buf);
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

