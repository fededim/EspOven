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
#ifndef _PidAutotuneControl_h_
#define _PidAutotuneControl_h_

#include "IControl.h"
//#include <ESP8266WiFi.h>
#include <PID_v1.h>
#include "PID_AutoTune.h"
#include "PidControl.h"

enum PidAutotuneStatus { Init=0, Stabilization=1, Tuning=2, Done=3};

class PidAutotuneControl: public PidControl {

public:
  // set temperature must be fixed at a fixed value for all tuning process
  PidAutotuneControl(const char *_name, IControlAction *_action, double set, double *actual, double initialKp, double initialKi, double initialKd, int windowsize);
  ~PidAutotuneControl();

  virtual void Control(bool started) override;
  virtual ControlType GetControlType() override;
  void Reset();

  PidAutotuneStatus status;
  double tunedKp,tunedKi,tunedKd;
  
  protected:
    PID_ATune *pidtuning;
    double setTemp;
    int numStable;
};
 
#endif
