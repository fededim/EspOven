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
#include "PidControl.h"
#include <PID_v1.h>

  // for digital relay control (not SSR)
  PidControl::PidControl(const char *_name, IControlAction *_action, double *_set, double *_actual, double Kp, double Ki, double Kd, int _windowsize) : IControl(_name,_action) {
    set=_set;
    actual=_actual;
    windowsize=_windowsize;
    oldstarted=false;
    output=0;
    
    pid=new PID(actual, &output, set,Kp,Ki,Kd, DIRECT); 
    //turn the PID off
    pid->SetMode(MANUAL);
    
    //tell the PID to range between 0 and the full window size
    pid->SetOutputLimits(0, windowsize);
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
    
    if (started) {
      //started is true, turn the PID on if there is a transition from false to true
      if (!oldstarted)
        pid->SetMode(AUTOMATIC);

      // output will contain the number of ms of the window (0,windowsize) that the heater must be on
      pid->Compute();

      unsigned long now = millis()%windowsize;  // get where we are in windowsize

      Serial.printf("EspOven: PidControl %s (%d) set %f actual %f output %f now %d\n",name,started,*set,*actual,output, now);

      if (now<=output) {
        if (!action->Active()) {
          Serial.printf("EspOven: PidControl turning on %s heater temperature set %f actual %f\n", name,*set,*actual);
          action->On();
        }
      }
      else {
        if (action->Active()) {
          Serial.printf("EspOven: PidControl turning off %s heater temperature set %f actual %f\n", name,*set,*actual);
          action->Off();
        }
      }
    }
    // started is false and there has been a transition from true to false.
    else if (oldstarted) {
      //turn the PID off
      pid->SetMode(MANUAL);
        
      action->Off();
    } 

    oldstarted=started;
  }


  ControlType PidControl::GetControlType() {
    return ControlType::PID;
  }
  
