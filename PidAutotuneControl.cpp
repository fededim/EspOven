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
 
#include <stdlib.h>
#include "PidAutotuneControl.h"
#include "IControl.h"


#define DELTA_STABILIZATION 1.5

  void PidAutotuneControl::Reset() {
    pidtuning->Cancel();
    numStable=0;
    status=PidAutotuneStatus::Init;

    PidControl::Control(false);
  }

  
  PidAutotuneControl::PidAutotuneControl(const char *_name, IControlAction *_action, double *set, double *actual, double initialKp, double initialKi, double initialKd, int windowsize) :
      PidControl(_name,_action,set,actual,initialKp,initialKi,initialKd,windowsize)
  {    
    pidtuning=new PID_ATune(set,actual);
    pidtuning->SetControlType(1); // PID
    pidtuning->SetNoiseBand(0.5);  // half degree
    pidtuning->SetOutputStep(10);  // 10 degree
    pidtuning->SetLookbackSec(20); // 20 seconds
   
    Reset();
  }


  
  // Parent destructors are always called
  PidAutotuneControl::~PidAutotuneControl()  {
    delete pidtuning;
  }


  void PidAutotuneControl::Control(bool started) {
    if (status==PidAutotuneStatus::Done)
      return;
    
    if (!started) {
        Reset();
        return;
    }

    // tuning starts when we have reached the setTemp and it is stabilized.
    if (status==PidAutotuneStatus::Tuning) {
      // pidtuning->Runtime steps the stable output to understand how actual values change accordingly, this must be repeated till it returns a value!=0
      if (pidtuning->Runtime()!=0) {
        PidControl::Control(false);

        tunedKp=pidtuning->GetKp();
        tunedKi=pidtuning->GetKi();
        tunedKd=pidtuning->GetKd();

        status=PidAutotuneStatus::Done;
        
        Serial.printf("EspOven: PidAutotuneControl autotuning done, tuned Kp %f Ki %f Kd %d\n",tunedKp,tunedKi,tunedKd);
      }
      else {
        Serial.printf("EspOven: PidAutotuneControl autotuning %s set %f actual %f\n",name,*set,*actual); 

        PidControl::Control(true);
      }
    }
    else {
      if (status==PidAutotuneStatus::Init)
        status=PidAutotuneStatus::Stabilization;
      
      // Before the tuning starts, we need first to stabilize actual to set temp by using standard pid control (25 cycles)
      PidControl::Control(true);

      if (abs(*set-*actual)<DELTA_STABILIZATION) {
        if (numStable++>25) {
          status=PidAutotuneStatus::Tuning;
          numStable=0;
          Serial.printf("EspOven: PidAutotuneControl actual temp stabilized, moving to the tuning process\n");
        }
      }
      else {
        numStable=0;
      }

      Serial.printf("EspOven: PidAutotuneControl waiting for temp stabilization, set %f actual %f numStable %d\n",*set,*actual,numStable);   
    }
  }

ControlType PidAutotuneControl::GetControlType() {
    return ControlType::PIDAutotune;
  }
