#include <ESP8266WiFi.h>
#include "PidControl.h"
#include <PID_v1.h>

  PidControl::PidControl(const char *_name, IControlAction *_action, double *_set, double *_actual, double Kp, double Ki, double Kd, int _windowsize) : IControl(_name,_action) {
    set=_set;
    actual=_actual;
    windowsize=_windowsize;
    oldstarted=false;
    output=0;
    
    pid=new PID(actual, &output, set,Kp,Ki,Kd, DIRECT); 
    
    //tell the PID to range between 0 and the full window size
    pid->SetOutputLimits(0, windowsize);
    
    windowStartTime = millis();
  }


  PidControl::~PidControl() {
    delete pid;
  }


/********************************************************
   PID RelayOutput Example (https://playground.arduino.cc/Code/PIDLibraryRelayOutputExample)
   Same as basic example, except that this time, the output
   is going to a digital pin which (we presume) is controlling
   a relay.  The pid is designed to output an analog value,
   but the relay can only be On/Off.

     To connect them together we use "time proportioning
   control"  Tt's essentially a really slow version of PWM.
   First we decide on a window size (5000mS say.) We then
   set the pid to adjust its output between 0 and that window
   size.  Lastly, we add some logic that translates the PID
   output into "Relay On Time" with the remainder of the
   window being "Relay Off Time"
 ********************************************************/
void PidControl::Control(bool started) {
    active=action->Active();
  
    unsigned long now = millis();

    //time to shift the Relay Window (could skip window if too much time has passed)
    if ((now - windowStartTime) > windowsize)
      windowStartTime += windowsize;
   
    Serial.printf("EspOven: PidControl %s (%d) set %f actual %f output %f now %d windowStartTime %d (now - windowStartTime) %d\n",name,started,*set,*actual,output, now, windowStartTime, (now - windowStartTime));

    if (started) {
      //turn the PID on
      if (!oldstarted)
        pid->SetMode(AUTOMATIC);

      pid->Compute();

      if (output > (now - windowStartTime)) {
        Serial.printf("EspOven: PidControl turning on %s heater temperature set %f actual %f\n", name,*set,*actual);
        action->On();
      }
      else {
        Serial.printf("EspOven: PidControl turning off %s heater temperature set %f actual %f\n", name,*set,*actual);
        action->Off();
      }  
    }
    else if (oldstarted) {
      //turn the PID off
      pid->SetMode(MANUAL);
        
      action->Off();
    } 

    oldstarted=started;
  }


  ControlType PidControl::GetControlType() {
    return ControlType::OnOff;
  }
  
