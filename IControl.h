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
 
#ifndef _IControl_h_
#define _IControl_h_

#include <ESP8266WiFi.h>


enum class ControlType { OnOff=1, PID=2, PIDAutotune=3 };


// Simple interface to implement the on/off actions called by IControl interface
class IControlAction {
   public:
      int von,voff;
      
      IControlAction (int _pin, bool reverse) {
        pin=_pin;

        if (reverse) {
          von=LOW;
          voff=HIGH;
        }
        else {
          von=HIGH;
          voff=LOW;
        }
      }
      
      void SetPin(int _pin) {
        pin=_pin;
      }

    virtual bool Active()=0;
    virtual void On ()=0;
    virtual void Off()=0;

  protected:
    int pin;
};
    


// interface for control types (OnOff, PID, etc.)
class IControl
{      

public:      
      IControl(const char *_name, IControlAction *_action) {
        Serial.printf("IControl: name %s\n",_name);
        name=_name;
        action=_action;
      }
      
      virtual void Control(bool started)=0;
      virtual ControlType GetControlType()=0;

protected:
      const char *name;  // useful for debugging multiple controls of the same type
      IControlAction *action;
};

#endif
