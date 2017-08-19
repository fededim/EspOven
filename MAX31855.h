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
 *                                                                             *
 *******************************************************************************/
#ifndef _MAX31855_h_
#define _MAX31855_h_

#include <SPI.h> // Have to include this in the main sketch too... (Using SPI)

class MAX31855
{
public:
  // Temperature unit type
  enum unitType { F, C, K, R };  // Fahrenheit, Celsius, Kelvin, Rankine
 
  enum probeStatus { OK=1, VCCSHORT=2, GNDSHORT=3, OC=4};
  
  // Reads temperatures and status from probe
  void sampleProbe(void);
  // Converts temperature to the specified unit
  double ConvertTemp(double temp, MAX31855::unitType u);
  // Returns the probe temperature
  double readTemp(MAX31855::unitType u=C);
  // Returns the cold junction temperature
  double readCJTemp(MAX31855::unitType u=C);
  // Checks probe faults
  probeStatus checkStatus(void);

  // Handy function types
  inline double readTempF() { return readTemp(MAX31855::F); }
  inline double readTempR() { return readTemp(MAX31855::R); }
  inline double readTempK() { return readTemp(MAX31855::K); }
  inline double readCJTempF() { return readCJTemp(MAX31855::F); }
  inline double readCJTempR() { return readCJTemp(MAX31855::R); }
  inline double readCJTempK() { return readCJTemp(MAX31855::K); }

  MAX31855(const uint8_t);
protected:
  union { // Union makes conversion from 4 bytes to an unsigned 32-bit int easy
    uint8_t bytes[4];
    uint32_t uint32;
  } status;
  uint8_t cs;
};
 
#endif
