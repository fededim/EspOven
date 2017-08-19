#ifndef _IControl_h_
#define _IControl_h_

#include <ESP8266WiFi.h>


enum class ControlType { OnOff=1, PID=2};


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

