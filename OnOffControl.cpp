#include <ESP8266WiFi.h>
#include "OnOffControl.h"

  OnOffControl::OnOffControl(const char *_name, IControlAction *_action, double * _set, double * _actual, double _delta ) : IControl (_name,_action) {
    set=_set;
    actual=_actual;
    delta=_delta;
  }


  void OnOffControl::Control(bool started) { 
    bool active=action->Active();
    
    Serial.printf("EspOven: OnOffControl %s (%d) heating %d von %d voff %d\n",name,started,active,action->von,action->voff);
  
    if (started) {
      if (active && (*actual)>((1.0+delta)*(*set))) {
        Serial.printf("EspOven: turning off %s heater set %f actual %f\n", name,*set, *actual);
        action->Off();
        active=false;
      }
      else if (!active && (*actual)<((1.0-delta)*(*set))) {
        Serial.printf("EspOven: turning on %s heater set %f actual %f\n", name,*set, *actual);
        action->On();
        active=true;
      }
    }
    else
    {
        action->Off();
    }
  }


  ControlType OnOffControl::GetControlType() {
    return ControlType::OnOff;
  }
  
  
