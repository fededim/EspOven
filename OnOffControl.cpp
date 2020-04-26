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

#include "OnOffControl.h"


  //Keeps the temperature within set value +- DELTA
  OnOffControl::OnOffControl(const char *_name, IControlAction *_action, double * _set, double * _actual, int _delta ) : IControl (_name,_action) {
    set=_set;
    actual=_actual;
    delta=_delta;
  }


  void OnOffControl::Control(bool started) { 
    bool active=action->Active();
    
    Serial.printf("EspOven: OnOffControl %s (%d) heating %d von %d voff %d\n",name,started,active,action->von,action->voff);
  
    if (started) {
      if (active && (*actual)>((*set)+delta)) {
        Serial.printf("EspOven: turning off %s heater set %f actual %f\n", name,*set, *actual);
        action->Off();
      }
      else if (!active && (*actual)<((*set)-delta)) {
        Serial.printf("EspOven: turning on %s heater set %f actual %f\n", name,*set, *actual);
        action->On();
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
  
  
