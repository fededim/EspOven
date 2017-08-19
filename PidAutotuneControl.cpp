#include <stdlib.h>
#include "PidAutotuneControl.h"
#include "IControl.h"


#define DELTA_STABILIZATION 1.5

  void PidAutotuneControl::Reset() {
    pidtuning->Cancel();
    tuning=false;
    numStable=0;
    tuningDone=false;    
    
    action->Off();
  }

  
  PidAutotuneControl::PidAutotuneControl(const char *_name, IControlAction *_action, double set, double *actual, double initialKp, double initialKi, double initialKd, int windowsize) :
      PidControl(_name,_action,&setTemp,actual,initialKp,initialKi,initialKd,windowsize)
  {    
    setTemp=set;
    pidtuning=new PID_ATune(&setTemp,actual);
    pidtuning->SetControlType(1); // PID
    Reset();
  }


  
  // Parent destructors are always called
  PidAutotuneControl::~PidAutotuneControl()  {
    delete pidtuning;
  }


  void PidAutotuneControl::Control(bool started) {
    unsigned long now = millis();

    //time to shift the Relay Window (could skip window if too much time has passed)
    if ((now - windowStartTime) > windowsize)
      windowStartTime += windowsize;

    if (!started) {
        Reset();
        return;
    }

    if (tuning) {
      Serial.printf("EspOven: PidAutotuneControl before autotuning output %f\n",output);
      // pidtuning->Runtime steps the stable output to understand how actual values change accordingly, this must be repeated around 10 times
      if (pidtuning->Runtime()!=0) {
        tunedKp=pidtuning->GetKp();
        tunedKi=pidtuning->GetKi();
        tunedKd=pidtuning->GetKd();

        action->Off();
        tuning=false;
        tuningDone=true;

        Serial.printf("EspOven: PidAutotuneControl autotuning done, tuned Kp %f Ki %f Kd %d\n",tunedKp,tunedKi,tunedKd);
      }
      else {
        Serial.printf("EspOven: PidAutotuneControl after autotuning %s set %f actual %f output %f now %d windowStartTime %d (now - windowStartTime) %d\n",name,setTemp,*actual,output, now, windowStartTime, (now - windowStartTime)); 

        if (output > (now - windowStartTime)) {
          action->On();
        }
        else {
          action->Off();
        }  
      }
    }
    else {
      // We need first to stabilize actual to set temp by using standard pid control (25 cycles)
      PidControl::Control(true);

      if (!tuning && (setTemp-*actual)<DELTA_STABILIZATION) {
        if (numStable++>25) {
          tuning=true;
          numStable=0;
          Serial.printf("EspOven: PidAutotuneControl set and actual temp stabilized, turning on the autotuning process\n");
        }
      }
      else {
        numStable=0;
      }
    }
  }

