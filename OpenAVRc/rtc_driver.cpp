/*
**************************************************************************
*                                                                        *
*                 ____                ___ _   _____                      *
*                / __ \___  ___ ___  / _ | | / / _ \____                 *
*               / /_/ / _ \/ -_) _ \/ __ | |/ / , _/ __/                 *
*               \____/ .__/\__/_//_/_/ |_|___/_/|_|\__/                  *
*                   /_/                                                  *
*                                                                        *
*              This file is part of the OpenAVRc project.                *
*                                                                        *
*                         Based on code(s) named :                       *
*             OpenTx - https://github.com/opentx/opentx                  *
*             Deviation - https://www.deviationtx.com/                   *
*                                                                        *
*                Only AVR code here for visibility ;-)                   *
*                                                                        *
*   OpenAVRc is free software: you can redistribute it and/or modify     *
*   it under the terms of the GNU General Public License as published by *
*   the Free Software Foundation, either version 2 of the License, or    *
*   (at your option) any later version.                                  *
*                                                                        *
*   OpenAVRc is distributed in the hope that it will be useful,          *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of       *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
*   GNU General Public License for more details.                         *
*                                                                        *
*       License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html          *
*                                                                        *
**************************************************************************
*/

#include "i2c_master.h"
#include "OpenAVRc.h"

#define RTC_ADRESS  (0x68 << 1) // DS3231

uint8_t bin2bcd(int8_t bin)
{
  return (uint8_t)(bin + 6 * (bin / 10));
}

int8_t bcd2bin(uint8_t bcd)
{
  return (int8_t)(bcd - 6 * (bcd >> 4));
}

/*-------------------------------------------------*/
/* RTC functions                                   */

void rtcSetTime(struct tm * t)
{
  g_ms100 = 0; // start of next second begins now
  //g_rtcTime = MKTIME(t); needed ?

  uint8_t buf[7];

  buf[0] = bin2bcd(t->tm_sec);
  buf[1] = bin2bcd(t->tm_min);
  buf[2] = bin2bcd(t->tm_hour);
  buf[3] = bin2bcd(t->tm_wday + 1);
  buf[4] = bin2bcd(t->tm_mday);
  buf[5] = bin2bcd(t->tm_mon + 1);
  buf[6] = bin2bcd(t->tm_year - 100);

  i2c_writeReg(RTC_ADRESS, 0x00, buf, 7);
}

void rtcInit()
{
  /* Read RTC registers */
  uint8_t powerlost;
  uint8_t buf[7];	/* RTC R/W buffer */

  if (i2c_readReg(RTC_ADRESS, 0x0F, &powerlost, 1))
    {
      return;	/* Check data corruption and return on IIC error */
    }

  if (powerlost >> 7)  	/* When data has been volatiled, set default time */
    {
      /* Reset time to  16 08 2018 (Now he he :-). Reg[0..7] */
      buf[0] = 0x00;
      buf[1] = 0x00;
      buf[2] = 0x00;
      buf[3] = 0x04;
      buf[4] = 0x16;
      buf[5] = 0x08;
      buf[6] = 0x18;
      i2c_writeReg(RTC_ADRESS, 0x00, buf, 7);

      uint8_t state;
      i2c_readReg(RTC_ADRESS, 0x0F, &state, 1);
      state &= ~0x80; // flip OSF bit
      i2c_writeReg(RTC_ADRESS, 0x0F, &state, 1);
    }

  if (i2c_readReg(RTC_ADRESS, 0x00, buf, 7))
    {
      return;
    }

  struct tm utm;

  utm.tm_year = bcd2bin(buf[6]) + 100;       // 1900 to 2000 offset
  utm.tm_mon =  bcd2bin((buf[5] & 0x1F) - 1);// 12 month
  utm.tm_mday = bcd2bin(buf[4] & 0x3F);      // 31 days
  utm.tm_hour = bcd2bin(buf[2]);             // 23 hours
  utm.tm_min =  bcd2bin(buf[1]);             // 59 min
  utm.tm_sec =  bcd2bin(buf[0]);             // 59 sec
  utm.tm_wday = bcd2bin((buf[3] & 0x03) - 1);// 7 week days

  g_rtcTime = MKTIME(&utm);
#if defined(SIMU)
  g_rtcTime = time(NULL);
#endif
}

uint8_t rtcReadTemp(int16_t * temp)
{
  uint8_t buf[2];

  if (i2c_readReg(RTC_ADRESS, 0x11, buf, 2))
    // reg 0x11 & 0x12 send temp value but only the 2Msb on 0x12 -> we div by 64
    return 1;

  int16_t calc = (buf[0] << 8) | buf[1];
  calc /= 64;
  calc *= 25;                             // Value X 0.25�C
#if defined(SIMU)
  calc = 2000;                           // 20.00�C in Simu
#endif
#if defined(IMPERIAL_UNITS)
  // f=1.8*c+32 (All is X 100)
  int32_t fcalc = calc * 18;
  fcalc += 32000;
  calc = fcalc / 10;
#endif
  *temp = calc;
  return 0;
}
