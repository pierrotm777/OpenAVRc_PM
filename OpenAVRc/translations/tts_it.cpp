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


#include "../OpenAVRc.h"

enum ItalianPrompts {
  IT_PROMPT_NUMBERS_BASE = 0,
  IT_PROMPT_ZERO = IT_PROMPT_NUMBERS_BASE+0,
  IT_PROMPT_CENT = IT_PROMPT_NUMBERS_BASE+100,
  IT_PROMPT_MILA = IT_PROMPT_NUMBERS_BASE+101,
  IT_PROMPT_MILLE = IT_PROMPT_NUMBERS_BASE+102,
  IT_PROMPT_VIRGOLA = 103,
  IT_PROMPT_UN,
  IT_PROMPT_E,
  IT_PROMPT_MENO,
  IT_PROMPT_ORA,
  IT_PROMPT_ORE,
  IT_PROMPT_MINUTO,
  IT_PROMPT_MINUTI,
  IT_PROMPT_SECONDO,
  IT_PROMPT_SECONDI,

  IT_PROMPT_UNITS_BASE = 113,
  IT_PROMPT_VOLTS = IT_PROMPT_UNITS_BASE+(UNIT_VOLTS*2),
  IT_PROMPT_AMPS = IT_PROMPT_UNITS_BASE+(UNIT_AMPS*2),
  IT_PROMPT_METERS_PER_SECOND = IT_PROMPT_UNITS_BASE+(UNIT_METERS_PER_SECOND*2),
  IT_PROMPT_SPARE1 = IT_PROMPT_UNITS_BASE+(UNIT_RAW*2),
  IT_PROMPT_KMH = IT_PROMPT_UNITS_BASE+(UNIT_SPEED*2),
  IT_PROMPT_METERS = IT_PROMPT_UNITS_BASE+(UNIT_DIST*2),
  IT_PROMPT_DEGREES = IT_PROMPT_UNITS_BASE+(UNIT_TEMPERATURE*2),
  IT_PROMPT_PERCENT = IT_PROMPT_UNITS_BASE+(UNIT_PERCENT*2),
  IT_PROMPT_MILLIAMPS = IT_PROMPT_UNITS_BASE+(UNIT_MILLIAMPS*2),
  IT_PROMPT_MAH = IT_PROMPT_UNITS_BASE+(UNIT_MAH*2),
  IT_PROMPT_WATTS = IT_PROMPT_UNITS_BASE+(UNIT_WATTS*2),
  IT_PROMPT_FEET = IT_PROMPT_UNITS_BASE+(UNIT_FEET*2),
  IT_PROMPT_KTS = IT_PROMPT_UNITS_BASE+(UNIT_KTS*2),

};

#if defined(VOICE)

void playNumber(getvalue_t number, uint8_t unit, uint8_t att)
{
  /*  if digit >= 1000000000:
        temp_digit, digit = divmod(digit, 1000000000)
        prompts.extend(self.getNumberPrompt(temp_digit))
        prompts.append(Prompt(GUIDE_00_BILLION, dir=2))
    if digit >= 1000000:
        temp_digit, digit = divmod(digit, 1000000)
        prompts.extend(self.getNumberPrompt(temp_digit))
        prompts.append(Prompt(GUIDE_00_MILLION, dir=2))
  */
  getvalue_t orignumber;
  if (number < 0) {
    PUSH_NUMBER_PROMPT(IT_PROMPT_MENO);
    number = -number;
  }
  orignumber=number;
  if (unit) {
    unit--;
    convertUnit(number, unit);
    if (IS_IMPERIAL_ENABLE()) {
      if (unit == UNIT_DIST) {
        unit = UNIT_FEET;
      }
      if (unit == UNIT_SPEED) {
        unit = UNIT_KTS;
      }
    }
    unit++;
  }

  int8_t mode = MODE(att);
  if (mode > 0) {
    // we assume that we are PREC1
    div_t qr = div(number, 10);
    if (qr.rem > 0) {
      playNumber(qr.quot, 0, 0);
      PUSH_NUMBER_PROMPT(IT_PROMPT_VIRGOLA);
      if (mode==2 && qr.rem < 10)
        PUSH_NUMBER_PROMPT(IT_PROMPT_ZERO);
      playNumber(qr.rem, 0, 0);
    } else {
      if (qr.quot==1) {
        PUSH_NUMBER_PROMPT(IT_PROMPT_UN);
        if (unit) {
          PUSH_NUMBER_PROMPT(IT_PROMPT_UNITS_BASE+(unit*2));
        }
        return;
      } else {
        playNumber(qr.quot, 0, 0);
      }
    }
  } else {
    if (orignumber == 1 && unit) {
      PUSH_NUMBER_PROMPT(IT_PROMPT_UN);
    } else {
      if (number >= 1000) {
        if (number >= 2000) {
          playNumber(number / 1000, 0, 0);
          PUSH_NUMBER_PROMPT(IT_PROMPT_MILA);
        } else {
          PUSH_NUMBER_PROMPT(IT_PROMPT_MILLE);
        }
        number %= 1000;
        if (number == 0)
          number = -1;
      }
      if (number >= 100) {
        if (number >= 200)
          PUSH_NUMBER_PROMPT(IT_PROMPT_ZERO + number/100);
        PUSH_NUMBER_PROMPT(IT_PROMPT_CENT);
        number %= 100;
        if (number == 0)
          number = -1;
      }
      PUSH_NUMBER_PROMPT(IT_PROMPT_ZERO+number);
    }
  }
  if (unit) {
    if (orignumber == 1) {
      PUSH_NUMBER_PROMPT(IT_PROMPT_UNITS_BASE+(unit*2));
    } else {
      PUSH_NUMBER_PROMPT(IT_PROMPT_UNITS_BASE+(unit*2)+1);
    }
  }
}

void playDuration(int16_t seconds )
{
  if (seconds < 0) {
    PUSH_NUMBER_PROMPT(IT_PROMPT_MENO);
    seconds = -seconds;
  }

  uint8_t ore = 0;
  uint8_t tmp = seconds / 3600;
  seconds %= 3600;
  if (tmp > 0) {
    ore=tmp;
    if (tmp > 1 || IS_PLAY_TIME()) {
      playNumber(tmp, 0, 0);
      PUSH_NUMBER_PROMPT(IT_PROMPT_ORE);
    } else {
      PUSH_NUMBER_PROMPT(IT_PROMPT_UN);
      PUSH_NUMBER_PROMPT(IT_PROMPT_ORA);
    }
  }
  if (seconds>0) {
    tmp = seconds / 60;
    seconds %= 60;
    if (tmp>0 && seconds==0 && ore>0) {
      PUSH_NUMBER_PROMPT(IT_PROMPT_E);
    }
    if (tmp > 0) {
      if (tmp != 1) {
        playNumber(tmp, 0, 0);
        PUSH_NUMBER_PROMPT(IT_PROMPT_MINUTI);
      } else {
        PUSH_NUMBER_PROMPT(IT_PROMPT_UN);
        PUSH_NUMBER_PROMPT(IT_PROMPT_MINUTO);
      }
    }
    if ((tmp>0 || ore>0) && seconds>0) {
      PUSH_NUMBER_PROMPT(IT_PROMPT_E);
    }
  }
  if (seconds != 0 || (ore==0 && tmp==0)) {
    if (seconds != 1) {
      playNumber(seconds, 0, 0);
      PUSH_NUMBER_PROMPT(IT_PROMPT_SECONDI);
    } else {
      PUSH_NUMBER_PROMPT(IT_PROMPT_UN);
      PUSH_NUMBER_PROMPT(IT_PROMPT_SECONDO);
    }
  }
}

LANGUAGE_PACK_DECLARE(it, "Italiano");

#endif
