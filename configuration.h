/********************************** max31855k.h ********************************
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

#ifndef _Configuration_h_
#define _Configuration_h_

#include "IControl.h"

enum class GPIO05Sel { Relay=1, nInt=2 };
enum class GPIO16Sel { Buzzer=1, Scl=2 };

class Configuration {
  public:
    bool enable1,autopid1;
    ControlType control1;
    double kp1,kd1,ki1,alpha1;  // parameters for pid / on off control

    bool enable2,autopid2;
    ControlType control2;
    double kp2,kd2,ki2,alpha2;  // parameters for pid / on off control

    GPIO05Sel io5Sel;
    GPIO16Sel io16Sel;

    char *GetJson(char *buf, int len);
    bool SetJson(char *buf);
    bool Load();    
    bool Save();
};

#endif


