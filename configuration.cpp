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
 *******************************************************************************/

#include <ESP8266WiFi.h>
#include "FS.h"
#include "configuration.h"
#include "string.h"
#include <ArduinoJson.h>

  char *Configuration::GetJson(char *buf, int len) {
    StaticJsonDocument<256> jsonBuffer;

    JsonObject root = jsonBuffer.to<JsonObject>();
    root["enable1"] = enable1;
    root["control1"] = (int) control1;
    root["ki1"] = (double) ki1;
    root["kd1"] = (double) kd1;
    root["kp1"] = (double) kp1;
    root["alpha1"] = (double) alpha1;

    root["enable2"] = enable2;
    root["control2"] = (int) control2;
    root["ki2"] = (double) ki2;
    root["kd2"] = (double) kd2;
    root["kp2"] = (double) kp2;
    root["alpha2"] = (double) alpha2;
        
    serializeJsonPretty(root,buf,len);
    Serial.printf("Configuration: GetJson returned %s\n",buf);    

    return buf;
  }




  // no checking on values is enforced, we are in an embedded system inside our lan so it should be safe...in the worst case it will set default values
  bool Configuration::SetJson(char *buf) {
    StaticJsonDocument<256> root;

    DeserializationError err = deserializeJson(root,buf);
    if (err!=DeserializationError::Ok) {
      Serial.printf("Configuration: SetJson error parsing %s\n",buf);
      return false;
    }
    enable1=root["enable1"];
    control1=(ControlType) root["control1"].as<int>();
    kp1=root["kp1"].as<double>();
    kd1=root["kd1"].as<double>();
    ki1=root["ki1"].as<double>();
    alpha1=root["alpha1"].as<double>();

    enable2=root["enable2"];
    control2=(ControlType) root["control2"].as<int>();
    kp2=root["kp2"].as<double>();
    kd2=root["kd2"].as<double>();
    ki2=root["ki2"].as<double>();
    alpha2=root["alpha2"].as<double>();

    Serial.printf("Configuration: SetJson successfully set %s\n",buf);    

    return true;
  }



  bool Configuration::Save() {
    char buf[256];

    File f = SPIFFS.open("/config.json", "w");
    f.print(GetJson(buf,256));      
    f.close();
  }

   
  bool Configuration::Load() {
    char *buf;
    
    File f = SPIFFS.open("/config.json", "r");
    if (!f) 
      return false;

    buf=(char *) f.readString().c_str();     
    bool ris=SetJson(buf);

    if (ris)
      Serial.printf("Configuration: Load successfully loaded configuration from flash (%s)\n",buf);
    else 
      Serial.printf("Configuration: Load unable to load configuration from flash (%s)\n",buf);

    return ris;
  }
