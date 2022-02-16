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

enum EnglishPrompts {
  EN_PROMPT_NUMBERS_BASE = 0,
  EN_PROMPT_ZERO = EN_PROMPT_NUMBERS_BASE+0,       //02-99
  EN_PROMPT_HUNDRED = EN_PROMPT_NUMBERS_BASE+100,  //100,200 .. 900
  EN_PROMPT_THOUSAND = EN_PROMPT_NUMBERS_BASE+109, //1000
  EN_PROMPT_AND = EN_PROMPT_NUMBERS_BASE+110,
  EN_PROMPT_MINUS = EN_PROMPT_NUMBERS_BASE+111,
  EN_PROMPT_POINT = EN_PROMPT_NUMBERS_BASE+112,
  EN_PROMPT_UNITS_BASE = 113,
  EN_PROMPT_POINT_BASE = 165, //.0 - .9
};

#if defined(VOICE)

#define EN_PUSH_UNIT_PROMPT(p, u) pushUnitPrompt((p), (u))

void pushUnitPrompt(int16_t number, uint8_t unitprompt)
{
  if (number == 1)
    PUSH_NUMBER_PROMPT(unitprompt);
  else
    PUSH_NUMBER_PROMPT(unitprompt+1);
}

void playNumber(getvalue_t number, uint8_t unit, uint8_t att)
{
  if (number < 0) {
    PUSH_NUMBER_PROMPT(EN_PROMPT_MINUS);
    number = -number;
  }

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
    if (qr.rem) {
      playNumber(qr.quot, 0, 0);
      PUSH_NUMBER_PROMPT(EN_PROMPT_POINT_BASE + qr.rem);
      number = -1;
    } else {
      number = qr.quot;
    }
  }

  int16_t tmp = number;

  if (number >= 1000) {
    playNumber(number / 1000, 0, 0);
    PUSH_NUMBER_PROMPT(EN_PROMPT_THOUSAND);
    number %= 1000;
    if (number == 0)
      number = -1;
  }
  if (number >= 100) {
    PUSH_NUMBER_PROMPT(EN_PROMPT_HUNDRED + (number/100)-1);
    number %= 100;
    if (number == 0)
      number = -1;
  }
  if (number >= 0) {
    PUSH_NUMBER_PROMPT(EN_PROMPT_ZERO + number);
  }

  if (unit) {
    EN_PUSH_UNIT_PROMPT(tmp, EN_PROMPT_UNITS_BASE + unit*2);
  }
}

void playDuration(int16_t seconds )
{
  if (seconds == 0) {
    playNumber(seconds, 0, 0);
    return;
  }

  if (seconds < 0) {
    PUSH_NUMBER_PROMPT(EN_PROMPT_MINUS);
    seconds = -seconds;
  }

  uint8_t tmp = seconds / 3600;
  seconds %= 3600;
  if (tmp > 0 || IS_PLAY_TIME()) {
    playNumber(tmp, UNIT_HOURS, 0);
  }

  tmp = seconds / 60;
  seconds %= 60;
  if (tmp > 0) {
    playNumber(tmp, UNIT_MINUTES, 0);
    if (seconds > 0)
      PUSH_NUMBER_PROMPT(EN_PROMPT_AND);
  }

  if (seconds > 0) {
    playNumber(seconds, UNIT_SECONDS, 0);
  }
}

LANGUAGE_PACK_DECLARE_DEFAULT(en, "English");

#endif
