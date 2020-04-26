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
 * Library to read temperature from a MAX31855 type K thermocouple digitizer  *
 *                                                                             *
 ******************************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include "MAX31855.h"

////////////////////////////////////////////////////////////////////////////////
// Description  : This constructor does the required setup
// Input        : uint8_t _cs: The Arduino pin number of the chip select line
//                for this instance
// Return       : Instance of this class with pins configured
// Usage        : MAX31855 <name>(<pinNumber>);
////////////////////////////////////////////////////////////////////////////////
MAX31855::MAX31855(const uint8_t _cs)
{
  cs=_cs;
  
  // Redundant with SPI library if using default SS
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);

  // SCK & MOSI set in SPI library, MISO autoconfigures  
  SPI.begin();

  //SPI.setBitOrder(MSBFIRST);
  //SPI.setDataMode(SPI_MODE1);
  //SPI.setClockDivider(SPI_CLOCK_DIV4);  
}

////////////////////////////////////////////////////////////////////////////////
// Deconstructor does nothing.  It's up to the user to re-assign
// chip select pin if they want to use it for something else.  We don't call
// SPI.end() in case there is another SPI device we don't want to kill.
////////////////////////////////////////////////////////////////////////////////

 

////////////////////////////////////////////////////////////////////////////////
// Description  : This function reads MAX31855 data
// Input        : None
// Output       : Loads 32bit status from the IC into class variable "status"
// Return:      : None
// Usage        : <objectName>.sampleProbe();
// MAX31855 32bit dataemory Map:
//  D[31:18] Signed 14-bit thermocouple temperature data
//  D17      Reserved: Always reads 0
//  D16      Fault: 1 when any of the SCV, SCG, or OC faults are active, else 0
//  D[15:4]  Signed 12-bit internal temperature
//  D3       Reserved: Always reads 0
//  D2       SCV fault: Reads 1 when thermocouple is shorted to V_CC, else 0
//  D1       SCG fault: Reads 1 when thermocouple is shorted to gnd, else 0
//  D0       OC  fault: Reads 1 when thermocouple is open-circuit, else 0
////////////////////////////////////////////////////////////////////////////////
void MAX31855::sampleProbe(void)
{
  digitalWrite(cs, LOW);
  delayMicroseconds(1);
  
  SPI.beginTransaction(SPISettings(SPI_CLOCK_DIV4, MSBFIRST, SPI_MODE0));  // Defaults 
  status.bytes[3] = SPI.transfer(0x00);
  status.bytes[2] = SPI.transfer(0x00);
  status.bytes[1] = SPI.transfer(0x00);
  status.bytes[0] = SPI.transfer(0x00);  
  SPI.endTransaction();

  digitalWrite(cs, HIGH);
  
  Serial.printf("MAX31855(%d): sampleProbe new status %x bytes[3] %x\n",cs,status.uint32,status.bytes[3]);

  return;
}


double MAX31855::ConvertTemp(double temp, MAX31855::unitType u) {
  switch (u) {
	  case F:
		temp = (temp * 9.0 / 5.0) + 32.0; 
		break;
	  case K:
		temp += 273.15;
		break;
	  case R:
		temp = (temp + 273.15) * 9.0 / 5.0;
	  case C:
	  default:
		break;
  }
  
  return temp;
}


////////////////////////////////////////////////////////////////////////////////
// Description  : This function reads the current temperature f
// Input        : MAX31855::unitType u: The unitType of temperature to return. C (default), K, R, or F
// Return:      : double: The temperature in requested unitType or NaN in case of error.
// Usage        : double tempC = <objectName>.readTemp();
//                double tempC = <objectName>.readTemp(MAX31855::C);
//                double tempF = <objectName>.readTemp(MAX31855::F);
//                double tempK = <objectName>.readTemp(MAX31855::K);
//                double tempR = <objectName>.readTemp(MAX31855::R);
////////////////////////////////////////////////////////////////////////////////
double MAX31855::readTemp(MAX31855::unitType u)
{
  int16_t value;
  double temp;
  
  if (checkStatus()!=OK) {
    return NAN;
  }

  // Bits D[31:18] are the signed 14-bit thermocouple temperature value
  value=(status.uint32 >> 18) & 0x3FFF;  // skip first 18 bits and mask 14 bit of temperature data

  value|=((value&0x2000)?0xC000:0); // sign extend to 16bit 14bit value

  temp=value*0.25; // 0.25 LSB

  Serial.printf("MAX31855(%d): readTemp status %x value %hx temp %f C\n",cs,status.uint32,value,temp);
    
  return ConvertTemp(temp,u);
}



////////////////////////////////////////////////////////////////////////////////
// Description  : This function reads the cold junction temperature
// Input        : None
// Output       : None
// Return:      : double: The cold junction temperature 
// Usage        : double tempC = <objectName>.readCJTemp();
////////////////////////////////////////////////////////////////////////////////
double MAX31855::readCJTemp(MAX31855::unitType u)
{
  int16_t value;
  double temp;
  bool neg;
  
  // we can read internal temperature even without probe (12bit signed)
  value=(status.uint32 >> 4) & 0xFFF;  // skip first 4 bits and mask 12 bit of temperature data

  value|=((value&0x800)?0xF000:0); // sign extend to 16bit 12bit value

  temp=value*0.0625; // 0.0625 LSB
  
  Serial.printf("MAX31855(%d): readCJTemp status %x value %hx temp %f C\n",cs,status.uint32,value,temp);

  return ConvertTemp(temp,u);
}


 
////////////////////////////////////////////////////////////////////////////////
// Description  : This function checks the fault bits from the MAX31855 status
// Input        : None
// Return:      : NONE, VCCSHORT, GNDSHORT OR OC
// Usage        : checkStatus();
////////////////////////////////////////////////////////////////////////////////
MAX31855::probeStatus MAX31855::checkStatus(void)
{
  probeStatus ft=OK;

  //Serial.printf("MAX31855(%d): checkStatus %x\n",cs,status.uint32);

  // Bit D16 is high => fault
  if ((status.uint32 & (1<<16))!=0) {

    if ((status.uint32 & 1)!=0) {
	    Serial.printf("MAX31855(%d): checkStatus (%x) OC Fault: No Probe\n",cs,status.uint32);
      ft=OC;
    }
	  
	  if ((status.uint32 & 2)!=0) {
      Serial.printf("MAX31855(%d): checkStatus (%x) Fault: Thermocouple is shorted to GND\n",cs,status.uint32);
      ft=GNDSHORT;
    }
	  
	  if ((status.uint32 & 4)!=0) {
      Serial.printf("MAX31855(%d): checkStatus (%x) Fault: Thermocouple is shorted to VCC\n",cs,status.uint32);
      ft=VCCSHORT;
    }
  }

  return ft;
}
