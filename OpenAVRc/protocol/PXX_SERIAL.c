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

#define TLM_PXX_SERIAL TLM_USART0 // usart 0


// PXX control bits
#define PXX_CHANS                          16
//#define FRANCE_BIT                         0x10
//#define DSMX_BIT                           0x08
//#define BAD_DATA                           0x47
#define PXX_SEND_BIND                     0x80
#define PXX_SEND_RANGECHECK               0x20

bool pxxBind = 0;
bool pxxRange = 0;

int16_t channels[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
bool prepare = true;

/*
enum SubProtoDsm {
  Sub_LP45 = 0,
  Sub_DSM2,
  Sub_DSMX
};
*/

const static RfOptionSettingsvar_t RfOpt_Pxx_Ser[] PROGMEM = {
  /*rfProtoNeed*/0,
  /*rfSubTypeMax*/0,
  /*rfOptionValue1Min*/0,
  /*rfOptionValue1Max*/0,
  /*rfOptionValue2Min*/0,
  /*rfOptionValue2Max*/0,
  /*rfOptionValue3Max*/0,
};

static void PXX_SERIAL_Reset()
{
  Usart0DisableTx();
  Usart0DisableRx();
}


static uint16_t PXX_SERIAL_cb()
{
  // Schedule next Mixer calculations.
  SCHEDULE_MIXER_END_IN_US(22000);

  Usart0TxBufferCount = 0;

  uint8_t pxxTxBufferCount = 64;

  /*
  uint8_t pxx_header;

  if(g_model.rfSubType == Sub_LP45)
    pxx_header = 0x00;
  else if(g_model.rfSubType == Sub_DSM2)
    pxx_header = 0x10;
  else pxx_header = 0x10 | DSMX_BIT; // PROTO_DSM2_DSMX

  if(pxxBind)
    pxx_header |= PXX_SEND_BIND;
  else if(pxxRange)
    pxx_header |= PXX_SEND_RANGECHECK;

  Usart0TxBuffer[--pxxTxBufferCount] = pxx_header;

  Usart0TxBuffer[--pxxTxBufferCount] = g_model.modelId; // DSM2 Header. Second byte for model match.
  */
  /*
#if defined(X_ANY)
  Xany_scheduleTx_AllInstance();
#endif
*/
  pxxBind = 0;
  if(prepare)
  {
    for (uint8_t i = 0; i < PXX_CHANS; i++) {
      uint16_t pulse = PXX_limit(0, ((FULL_CHANNEL_OUTPUTS(i)*63)>>5)+512,1023);
      channels[i] = pulse;
      Usart0TxBuffer[--pxxTxBufferCount] = (i<<2) | ((pulse>>8)&0x03); // Encoded channel + upper 2 bits pulse width.
      Usart0TxBuffer[--pxxTxBufferCount] = pulse & 0xff; // Low byte
    }
    Usart0TxBufferCount = 64; // Indicates data to transmit.
  }

  else
  {
    PXX_send();
  }
  prepare = !prepare;
  delay(2);

/*
#if !defined(SIMU)
  Usart0TransmitBuffer();
#endif
*/
  heartbeat |= HEART_TIMER_PULSES;
  CALCULATE_LAT_JIT(); // Calculate latency and jitter.
  return 22000U *2; // 22 mSec Frame.
}


static void PXX_SERIAL_initialize()
{
// 125K 8N1
  Usart0Set125000BAUDS();
  Usart0Set8N1();
  Usart0EnableTx();
  Usart0TxBufferCount = 0;
  PROTO_Start_Callback( PXX_SERIAL_cb);
}


const void *PXX_SERIAL_Cmds(enum ProtoCmds cmd)
{
  switch(cmd) {
  case PROTOCMD_INIT:
    pxxBind = 0;
    PXX_SERIAL_initialize();
    return 0;
  case PROTOCMD_BIND:
    pxxBind = 1;
    DSM_SERIAL_initialize();
    return 0;
  case PROTOCMD_RESET:
    PROTO_Stop_Callback();
    PXX_SERIAL_Reset();
    return 0;
  case PROTOCMD_GETOPTIONS:
    SetRfOptionSettings(pgm_get_far_address(RfOpt_Pxx_Ser),
                        STR_PXX_PROTOCOLS,
                        STR_DUMMY,
                        STR_DUMMY,
                        STR_DUMMY,
                        STR_DUMMY,
                        STR_DUMMY,
                        STR_DUMMY);
    return 0;
  default:
    break;
  }
  return 0;
}
