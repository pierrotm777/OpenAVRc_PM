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

#include "GRAUPNER_PCM1024.h"

#define TR_PCMFRAME            "Trame PCM"
const pm_char STR_PCMFRAME[] PROGMEM = TR_PCMFRAME;

#define TR_PCMFAILSAFE            "Failsafe"
const pm_char STR_PCMFAILSAFE[] PROGMEM = TR_PCMFAILSAFE;
const pm_char STR_SUBTYPEFAILSAFE_GRAPCM1K[] PROGMEM = "  NO""HOLD""SAVE";

#define GRA_PCM1024_FRAME_PERIOD_US 44000U
#define GRA_PCM1024_PROP_CH_NB      8

static uint8_t GRAPCM1K_Failsafe_Value()
{
	return g_model.rfSubType;
}

const static RfOptionSettingsvar_t RfOpt_GRAPCM1K_Ser[] PROGMEM =
{
 /*rfProtoNeed*/0, //can be PROTO_NEED_SPI | BOOL1USED | BOOL2USED | BOOL3USED
 /*rfSubTypeMax*/2,
 /*rfOptionValue1Min*/0, // FREQFINE MIN
 /*rfOptionValue1Max*/0,  // FREQFINE MAX
 /*rfOptionValue2Min*/0,
 /*rfOptionValue2Max*/0,
 /*rfOptionValue3Max*/0,    // RF POWER
};

void PROTO_GRAPCM1K_cb1() // Needs to be renamed PROTO_FUTABA_PCM1024_cb1()
{

}


static void PROTO_GRAPCM1K_reset() // Needs to be renamed PROTO_FUTABA_PCM1024_reset()
{

}

static void PROTO_GRAPCM1K_initialize() // Needs to be renamed PROTO_FUTABA_PCM1024_initialize()
{

}

const void * PROTO_GRAPCM1K_Cmds(enum ProtoCmds cmd) // Needs to be renamed PROTO_FUTABA_PCM1024_Cmds()
{
  switch(cmd) {
    case PROTOCMD_INIT: PROTO_GRAPCM1K_initialize();
    return 0;
    case PROTOCMD_RESET:
      PROTO_GRAPCM1K_reset();
    return 0;
  case PROTOCMD_GETOPTIONS:
   SetRfOptionSettings(pgm_get_far_address(RfOpt_GRAPCM1K_Ser),
                       STR_SUBTYPEFAILSAFE_GRAPCM1K, //Failsafe modes
                       STR_DUMMY,      //Option 1 (int)
                       STR_DUMMY,      //Option 2 (int)
                       STR_DUMMY,      //Option 3 (uint 0 to 31)
                       STR_DUMMY,      //OptionBool 1
                       STR_DUMMY,      //OptionBool 2
                       STR_DUMMY       //OptionBool 3
                      );
   return 0;
  default: break;
  }
  return 0;
}

