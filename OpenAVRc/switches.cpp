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


#include "OpenAVRc.h"

#define CS_LAST_VALUE_INIT -32768

int16_t lsLastValue[NUM_LOGICAL_SWITCH];

GETSWITCH_RECURSIVE_TYPE s_last_switch_used = 0;
GETSWITCH_RECURSIVE_TYPE s_last_switch_value = 0;

PACK(typedef struct {
  uint8_t state;
  uint8_t last;
}) ls_sticky_struct;

uint8_t getLogicalSwitch(uint8_t idx)
{
  LogicalSwitchData * ls = lswAddress(idx);
  uint8_t result;

  uint8_t s = (ls->andsw);

  if (ls->func == LS_FUNC_NONE || (s && !getSwitch(s))) {
    if (ls->func != LS_FUNC_STICKY) {
      // AND switch must not affect STICKY and EDGE processing
      lsLastValue[idx] = CS_LAST_VALUE_INIT;
    }
    result = false;
  } else if ((s=lswFamily(ls->func)) == LS_FAMILY_BOOL) {
    uint8_t res1 = getSwitch(ls->v1);
    uint8_t res2 = getSwitch(ls->v2);
    switch (ls->func) {
    case LS_FUNC_AND:
      result = (res1 && res2);
      break;
    case LS_FUNC_OR:
      result = (res1 || res2);
      break;
    // case LS_FUNC_XOR:
    default:
      result = (res1 ^ res2);
      break;
    }
  } else if (s == LS_FAMILY_TIMER) {
    result = (lsLastValue[idx] <= 0);
  } else if (s == LS_FAMILY_STICKY) {
    result = (lsLastValue[idx] & (1<<0));
  } else {
    getvalue_t x = getValue(ls->v1);
    getvalue_t y;
    if (s == LS_FAMILY_COMP) {
      y = getValue(ls->v2);

      switch (ls->func) {
      case LS_FUNC_EQUAL:
        result = (x==y);
        break;
      case LS_FUNC_GREATER:
        result = (x>y);
        break;
      default:
        result = (x<y);
        break;
      }
    } else {
      mixsrc_t v1 = ls->v1;
#if defined(FRSKY)
      // Telemetry
      if (v1 >= MIXSRC_FIRST_TELEM) {
        if ((!TELEMETRY_STREAMING() && v1 >= MIXSRC_FIRST_TELEM+TELEM_FIRST_STREAMED_VALUE-1) || IS_FAI_FORBIDDEN(v1-1)) {
          result = false;
          goto DurationAndDelayProcessing;
        }

        y = convertLswTelemValue(ls);

        // Fill the telemetry bars threshold array
        if (s == LS_FAMILY_OFS) {
          uint8_t idx = v1-MIXSRC_FIRST_TELEM+1-TELEM_ALT;
          if (idx < THLD_MAX) {
            FILL_THRESHOLD(idx, ls->v2);
          }
        }

      } else if (v1 >= MIXSRC_GVAR1) {
        y = ls->v2;
      } else {
        y = calc100toRESX(ls->v2);
      }
#else
      if (v1 >= MIXSRC_FIRST_TELEM) {
        //y = (int16_t)3 * (128+ls->v2); // it's a Timer
        y = convertLswTelemValue(ls); // Add by Mentero -> under test (todo)
      } else if (v1 >= MIXSRC_GVAR1) {
        y = ls->v2; // it's a GVAR
      } else {
        y = calc100toRESX(ls->v2);
      }
#endif

      switch (ls->func) {

      case LS_FUNC_VEQUAL:// v==offset
        result = (x==y);
        break;

      case LS_FUNC_VALMOSTEQUAL:
#if defined(GVARS)
        if (v1 >= MIXSRC_GVAR1 && v1 <= MIXSRC_LAST_GVAR)
          result = (x==y);
        else
#endif
          result = (abs(x-y) < (1024 / STICK_TOLERANCE));
        break;
      case LS_FUNC_VPOS:
        result = (x>y);
        break;
      case LS_FUNC_VNEG:
        result = (x<y);
        break;
      case LS_FUNC_APOS:
        result = (abs(x)>y);
        break;
      case LS_FUNC_ANEG:
        result = (abs(x)<y);
        break;
      default: {
        if (lsLastValue[idx] == CS_LAST_VALUE_INIT) {
          lsLastValue[idx] = x;
        }
        int16_t diff = x - lsLastValue[idx];
        uint8_t update = false;
        if (ls->func == LS_FUNC_DIFFEGREATER) {
          if (y >= 0) {
            result = (diff >= y);
            if (diff < 0)
              update = true;
          } else {
            result = (diff <= y);
            if (diff > 0)
              update = true;
          }
        } else {
          result = (abs(diff) >= y);
        }
        if (result || update) {
          lsLastValue[idx] = x;
        }
        break;
      }
      }
    }
  }

#if defined(FRSKY)
DurationAndDelayProcessing:
#endif

  return result;
}

uint8_t getSwitch(swsrc_t swtch)
{
  uint8_t result;

  if (swtch == SWSRC_NONE)
    return true;

  uint8_t cs_idx = abs(swtch);

  if (cs_idx == SWSRC_ONE) {
    result = !systemBolls.s_mixer_first_run_done;
  } else if (cs_idx == SWSRC_ON) {
    result = true;
  } else if (cs_idx <= SWSRC_LAST_SWITCH) {
    result = switchState((EnumKeys)(SW_BASE+cs_idx-SWSRC_FIRST_SWITCH));
  } else if (cs_idx <= SWSRC_LAST_TRIM) {
    uint8_t idx = cs_idx - SWSRC_FIRST_TRIM;
    idx = (CONVERT_MODE(idx/2) << 1) + (idx & 1);
    result = trimDown(idx);
  }
  else if (cs_idx == SWSRC_REA) {
    result = REA_DOWN();
  }
  else if (cs_idx == SWSRC_REB) {
    result = REB_DOWN();
  }
  else if (cs_idx == SWSRC_REN) {
    result = !(REA_DOWN() || REB_DOWN());
  }
  else if (cs_idx == SWSRC_XD0) {
    result = (calibratedStick[NUM_STICKS+EXTRA_3POS-1] > 0x200); // > 512
  }
  else if (cs_idx == SWSRC_XD1) {
    result = IS_IN_RANGE(calibratedStick[NUM_STICKS+EXTRA_3POS-1],-(0x200),0x200); // -512 < X < 512
  }
  else if (cs_idx == SWSRC_XD2) {
    result = (calibratedStick[NUM_STICKS+EXTRA_3POS-1] < -(0x200)); // < -512
  }
  else {
    cs_idx -= SWSRC_FIRST_LOGICAL_SWITCH;
    GETSWITCH_RECURSIVE_TYPE mask = ((GETSWITCH_RECURSIVE_TYPE) (1 << cs_idx));
    if (s_last_switch_used & mask) {
      result = (s_last_switch_value & mask)? 1:0;
    } else {
      s_last_switch_used |= mask;
      result = getLogicalSwitch(cs_idx);
      if (result) {
        s_last_switch_value |= mask;
      } else {
        s_last_switch_value &= ~mask;
      }
    }
  }

  return swtch > 0 ? result : !result;
}

swarnstate_t switches_states = 0;

swsrc_t getMovedSwitch()
{
  static tmr10ms_t s_move_last_time = 0;
  swsrc_t result = 0;

  // return delivers 1 to 3 for ID1 to ID3
  // 4..8 for all other switches if changed to true
  // -4..-8 for all other switches if changed to false
  // 9 for Trainer switch if changed to true; Change to false is ignored
  swarnstate_t mask = 0x80;
  for (uint8_t i=NUM_PSWITCH; i>1; i--) {
    bool prev = (switches_states & mask);
    // don't use getSwitch here to always get the proper value, even getSwitch manipulates
    bool next = switchState((EnumKeys)(SW_BASE+i-1));
    if (prev != next) {
      if (((i<NUM_PSWITCH) && (i>3)) || next==true)
        result = next ? i : -i;
      if (i<=3 && result==0) result = 1;
      switches_states ^= mask;
    }
    mask >>= 1;
  }

  if ((tmr10ms_t)(getTmr10ms() - s_move_last_time) > 10)
    result = 0;

  s_move_last_time = getTmr10ms();
  return result;
}

void checkSwitches()
{
  swarnstate_t last_bad_switches = 0xff;
  swarnstate_t states = g_model.switchWarningState;

  while (1) {

#ifdef GETADC_COUNT
    for (uint8_t i=0; i<GETADC_COUNT; i++) {
      getADC();
    }
#endif

    getMovedSwitch();

    uint8_t warn = false;
    for (uint8_t i=0; i<NUM_SWITCHES-1; i++) {
      if (!(g_model.switchWarningEnable & (1<<i)))
        {
          if (i == 0)
            {
              if ((states & 0x03) != (switches_states & 0x03))
                {
                  warn = true;
                }
            }
          else if ((states & (1<<(i+1))) != (switches_states & (1<<(i+1))))
            {
              warn = true;
            }
        }
    }

    if (!warn) {
      return;
    }

    // first - display warning
    if (last_bad_switches != switches_states) {
      MESSAGE(STR_SWITCHWARN, NULL, STR_PRESSANYKEYTOSKIP, last_bad_switches == 0xff ? AU_SWITCH_ALERT : AU_NONE);
      uint8_t x = 2;
      for (uint8_t i=0; i<NUM_SWITCHES-1; i++) {
        uint8_t attr;
        if (i == 0)
          attr = ((states & 0x03) != (switches_states & 0x03)) ? INVERS : 0;
        else
          attr = (states & (1 << (i+1))) == (switches_states & (1 << (i+1))) ? 0 : INVERS;
        if (!(g_model.switchWarningEnable & (1<<i)))
          lcdPutsSwitches(x, 5*FH, (i>0?(i+3):(states&0x3)+1), attr);
        x += 3*FW+FW/2;
      }
      lcdRefresh();
      last_bad_switches = switches_states;
    }


    if (keyDown()) {
      return;
    }

    if (!systemBolls.pwrCheck) {
      return;
    }

    checkBacklight();

    MYWDT_RESET();
  }
  }

  void logicalSwitchesTimerTick()
  {
    for (uint8_t i=0; i<NUM_LOGICAL_SWITCH; i++) {
      LogicalSwitchData * ls = lswAddress(i);
      if (ls->func == LS_FUNC_TIMER) {
        int16_t *lastValue = &lsLastValue[i];
        if (*lastValue == 0 || *lastValue == CS_LAST_VALUE_INIT) {
          *lastValue = -lswTimerValue(ls->v1);
        } else if (*lastValue < 0) {
          if (++(*lastValue) == 0)
            *lastValue = lswTimerValue(ls->v2);
        } else { // if (*lastValue > 0)
          *lastValue -= 1;
        }
      } else if (ls->func == LS_FUNC_STICKY) {
        ls_sticky_struct & lastValue = (ls_sticky_struct &)lsLastValue[i];
        uint8_t before = lastValue.last & 0x01;
        if (lastValue.state) {
          uint8_t now = getSwitch(ls->v2);
          if (now != before) {
            lastValue.last ^= 1;
            if (!before) {
              lastValue.state = 0;
            }
          }
        } else {
          uint8_t now = getSwitch(ls->v1);
          if (before != now) {
            lastValue.last ^= 1;
            if (!before) {
              lastValue.state = 1;
            }
          }
        }
      }
    }
  }

  LogicalSwitchData * lswAddress(uint8_t idx)
  {
    return &g_model.logicalSw[idx];
  }

  uint8_t lswFamily(uint8_t func)
  {
    if (func <= LS_FUNC_ANEG)
      return LS_FAMILY_OFS;
    else if (func <= LS_FUNC_XOR)
      return LS_FAMILY_BOOL;
    else if (func <= LS_FUNC_LESS)
      return LS_FAMILY_COMP;
    else if (func <= LS_FUNC_ADIFFEGREATER)
      return LS_FAMILY_DIFF;
    else
      return LS_FAMILY_TIMER+func-LS_FUNC_TIMER;
  }

  int16_t lswTimerValue(delayval_t val)
  {
    return (val < -109 ? 129+val : (val < 7 ? (113+val)*5 : (53+val)*10));
  }

  void logicalSwitchesReset()
  {
    s_last_switch_value = 0;

    for (uint8_t i=0; i<NUM_LOGICAL_SWITCH; i++) {
      lsLastValue[i] = CS_LAST_VALUE_INIT;
    }
  }

  getvalue_t convertLswTelemValue(LogicalSwitchData * ls)
  {
    getvalue_t val;
    if (lswFamily(ls->func)==LS_FAMILY_OFS)
      val = convert8bitsTelemValue(ls->v1 - MIXSRC_FIRST_TELEM + 1, 128+ls->v2);
    else
      val = convert8bitsTelemValue(ls->v1 - MIXSRC_FIRST_TELEM + 1, 128+ls->v2) - convert8bitsTelemValue(ls->v1 - MIXSRC_FIRST_TELEM + 1, 128);
    return val;
  }
