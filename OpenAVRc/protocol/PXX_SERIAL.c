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

#define PXX_SEND_BIND           0x01
#define PXX_SEND_FAILSAFE       (1 << 4)
#define PXX_SEND_RANGECHECK     (1 << 5)

#define START_STOP    0x7e
#define PXX_PKT_BYTES 18
#define PW_HIGH        8    //  8 microcseconds high, rest of pulse low
#define PW_ZERO       16    // 16 microsecond pulse is zero
#define PW_ONE        24    // 24 microsecond pulse is one

#define CHAN_MULTIPLIER  100
#define CHAN_MAX_VALUE (100 * CHAN_MULTIPLIER)
#define PXX_PERIOD 24000

/*
static const char * const pxx_opts[] = {
  _tr_noop("Failsafe"), "Hold", "NoPulse", "RX", NULL,
  _tr_noop("Country"), "US", "JP", "EU", NULL,
  _tr_noop("Rx PWM out"), "1-8", "9-16", NULL,
  _tr_noop("Rx Telem"), "On", "Off", NULL,
  NULL
};
*/

const pm_char STR_SUBTYPE_COUNTRY[] PROGMEM = "US""JP""EU";
#define TR_OUTPUT_PWM        INDENT "Output Pwm"
const pm_char STR_OUTPUT_PWM[] PROGMEM = TR_OUTPUT_PWM;

const static RfOptionSettingsvar_t RfOpt_PXX_Ser[] PROGMEM = {
  /*rfProtoNeed*/ BOOL1USED | BOOL2USED,//can be PROTO_NEED_SPI | BOOL1USED | BOOL2USED | BOOL3USED
  /*rfSubTypeMax*/2,//3 countries subtypes
  /*rfOptionValue1Min*/0,
  /*rfOptionValue1Max*/0,
  /*rfOptionValue2Min*/0,
  /*rfOptionValue2Max*/0,
  /*rfOptionValue3Max*/7,//Power ????
};

static void PXX_SERIAL_Reset()
{
  USART_DISABLE_TX(PXX_USART);
}
/*
enum {
    PROTO_OPTS_FAILSAFE,
    PROTO_OPTS_COUNTRY,
    PROTO_OPTS_RXPWM,
    PROTO_OPTS_RXTELEM,
    LAST_PROTO_OPT,
};
*/

#define FAILSAFE_HOLD    0
#define FAILSAFE_NOPULSE 1
#define FAILSAFE_RX      2

static uint16_t failsafe_count;
static uint8_t chan_offset;
static uint8_t FS_flag;
static uint8_t range_check;

enum XJTRFProtocols {
  RF_PROTO_OFF = -1,
  RF_PROTO_X16,
  RF_PROTO_D8,
  RF_PROTO_LR12,
  RF_PROTO_LAST = RF_PROTO_LR12
};

enum R9MSubTypes
{
  MODULE_SUBTYPE_R9M_FCC,
  MODULE_SUBTYPE_R9M_LBT,
};

static const uint16_t CRCTable[] = {
  0x0000,0x1189,0x2312,0x329b,0x4624,0x57ad,0x6536,0x74bf,
  0x8c48,0x9dc1,0xaf5a,0xbed3,0xca6c,0xdbe5,0xe97e,0xf8f7,
  0x1081,0x0108,0x3393,0x221a,0x56a5,0x472c,0x75b7,0x643e,
  0x9cc9,0x8d40,0xbfdb,0xae52,0xdaed,0xcb64,0xf9ff,0xe876,
  0x2102,0x308b,0x0210,0x1399,0x6726,0x76af,0x4434,0x55bd,
  0xad4a,0xbcc3,0x8e58,0x9fd1,0xeb6e,0xfae7,0xc87c,0xd9f5,
  0x3183,0x200a,0x1291,0x0318,0x77a7,0x662e,0x54b5,0x453c,
  0xbdcb,0xac42,0x9ed9,0x8f50,0xfbef,0xea66,0xd8fd,0xc974,
  0x4204,0x538d,0x6116,0x709f,0x0420,0x15a9,0x2732,0x36bb,
  0xce4c,0xdfc5,0xed5e,0xfcd7,0x8868,0x99e1,0xab7a,0xbaf3,
  0x5285,0x430c,0x7197,0x601e,0x14a1,0x0528,0x37b3,0x263a,
  0xdecd,0xcf44,0xfddf,0xec56,0x98e9,0x8960,0xbbfb,0xaa72,
  0x6306,0x728f,0x4014,0x519d,0x2522,0x34ab,0x0630,0x17b9,
  0xef4e,0xfec7,0xcc5c,0xddd5,0xa96a,0xb8e3,0x8a78,0x9bf1,
  0x7387,0x620e,0x5095,0x411c,0x35a3,0x242a,0x16b1,0x0738,
  0xffcf,0xee46,0xdcdd,0xcd54,0xb9eb,0xa862,0x9af9,0x8b70,
  0x8408,0x9581,0xa71a,0xb693,0xc22c,0xd3a5,0xe13e,0xf0b7,
  0x0840,0x19c9,0x2b52,0x3adb,0x4e64,0x5fed,0x6d76,0x7cff,
  0x9489,0x8500,0xb79b,0xa612,0xd2ad,0xc324,0xf1bf,0xe036,
  0x18c1,0x0948,0x3bd3,0x2a5a,0x5ee5,0x4f6c,0x7df7,0x6c7e,
  0xa50a,0xb483,0x8618,0x9791,0xe32e,0xf2a7,0xc03c,0xd1b5,
  0x2942,0x38cb,0x0a50,0x1bd9,0x6f66,0x7eef,0x4c74,0x5dfd,
  0xb58b,0xa402,0x9699,0x8710,0xf3af,0xe226,0xd0bd,0xc134,
  0x39c3,0x284a,0x1ad1,0x0b58,0x7fe7,0x6e6e,0x5cf5,0x4d7c,
  0xc60c,0xd785,0xe51e,0xf497,0x8028,0x91a1,0xa33a,0xb2b3,
  0x4a44,0x5bcd,0x6956,0x78df,0x0c60,0x1de9,0x2f72,0x3efb,
  0xd68d,0xc704,0xf59f,0xe416,0x90a9,0x8120,0xb3bb,0xa232,
  0x5ac5,0x4b4c,0x79d7,0x685e,0x1ce1,0x0d68,0x3ff3,0x2e7a,
  0xe70e,0xf687,0xc41c,0xd595,0xa12a,0xb0a3,0x8238,0x93b1,
  0x6b46,0x7acf,0x4854,0x59dd,0x2d62,0x3ceb,0x0e70,0x1ff9,
  0xf78f,0xe606,0xd49d,0xc514,0xb1ab,0xa022,0x92b9,0x8330,
  0x7bc7,0x6a4e,0x58d5,0x495c,0x3de3,0x2c6a,0x1ef1,0x0f78
};

static uint16_t crc(uint8_t *data, uint8_t len) {
  uint16_t crc = 0;
  for(int i=0; i < len; i++)
      crc = (crc<<8) ^ CRCTable[((uint8_t)(crc>>8) ^ *data++) & 0xFF];
  return crc;
}

//#define STICK_SCALE    819  // full scale at +-125
#define STICK_SCALE    751  // +/-100 gives 2000/1000 us pwm
static uint16_t scaleForPXX(uint8_t chan, uint8_t failsafe)
{ //mapped 860,2140(125%) range to 64,1984(PXX values);
//  return (uint16_t)(((Servo_data[i]-PPM_MIN)*3)>>1)+64;
// 0-2047, 0 = 817, 1024 = 1500, 2047 = 2182
    int32_t chan_val;

    if (chan >= 16) //if (chan >= Model.num_channels)
    {
        if (chan > 7 && !failsafe)
            return scaleForPXX(chan-8, 0);
        return (chan < 8) ? 1024 : 3072;   // center values
    }
/*
    if (failsafe)
        if (Model.limits[chan].flags & CH_FAILSAFE_EN)
            chan_val = Model.limits[chan].failsafe * CHAN_MULTIPLIER;
        else if (Model.proto_opts[PROTO_OPTS_FAILSAFE] == FAILSAFE_HOLD)
            return (chan < 8) ? 2047 : 4095;    // Hold
        else
            return (chan < 8) ? 0 : 2048;       // No Pulses
    else
*/
        chan_val = FULL_CHANNEL_OUTPUTS(chan);//chan_val = Channels[chan];

#if 0
    if (Model.proto_opts[PROTO_OPTS_RSSICHAN] && (chan == Model.num_channels - 1) && !failsafe)
        chan_val = Telemetry.value[TELEM_FRSKY_RSSI] * 21;      // Max RSSI value seems to be 99, scale it to around 2000
    else
#endif
        chan_val = chan_val * STICK_SCALE / CHAN_MAX_VALUE + 1024;

    if (chan_val > 2046)   chan_val = 2046;
    else if (chan_val < 1) chan_val = 1;

    if (chan > 7) chan_val += 2048;   // upper channels offset

    return chan_val;
}


#define FAILSAFE_TIMEOUT 1032
static void build_data_pkt(uint8_t bind)
{
    uint16_t chan_0;
    uint16_t chan_1;
    uint8_t startChan = chan_offset;

    Usart0TxBufferCount = PXX_PKT_BYTES; // Indicates data to transmit.

    uint16_t * channelsSbus = &pulses2MHz.pword[CHANNEL_USED_OFFSET/2]; // re use channel_used_p2M memory
    uint8_t PxxTxBufferCount = Usart0TxBufferCount;

    // data frames sent every 8ms; failsafe every 8 seconds
    /*
    if (FS_flag == 0 && failsafe_count > FAILSAFE_TIMEOUT && chan_offset == 0 &&  Model.proto_opts[PROTO_OPTS_FAILSAFE] != FAILSAFE_RX) {
        FS_flag = 0x10;
    } else if (FS_flag & 0x10 && chan_offset == 0) {
        FS_flag = 0;
        failsafe_count = 0;
    }
    failsafe_count += 1;
    */

    // only need packet contents here. Start and end flags added by pxx_enable
//  packet_p2M[0] = set in initialize() to tx id

    // FLAG1,
    // b0: BIND / set Rx number -> if this bit is set, every rx listening
    // should change it's rx number to the one described in byte 2
    // b4: if 1, set failsafe positions for all channels
    // b5: rangecheck if set
    // b6..b7: rf protocol
    packet_p2M[PXX_PKT_BYTES - 1] = RF_PROTO_X16 << 6;
    if (bind) {
        // if b0, then b1..b2 = country code (us 0, japan 1, eu 2 ?)
        packet_p2M[PXX_PKT_BYTES - 1] |= PXX_SEND_BIND | (g_model.rfOptionValue1/*Model.proto_opts[PROTO_OPTS_COUNTRY]*/ << 1);
    } else if (range_check) {
        packet_p2M[PXX_PKT_BYTES - 1] |= PXX_SEND_RANGECHECK;
    } else {
        packet_p2M[PXX_PKT_BYTES - 1] |= FS_flag;
    }

    packet_p2M[PXX_PKT_BYTES - 2] = 0;  // FLAG2, Reserved for future use, must be “0” in this version.

    for(uint8_t i = 0; i < 12 ; i += 3) {    // 12 bytes of channel data
        chan_0 = scaleForPXX(startChan++, FS_flag == 0x10 ? 1 : 0);
        chan_1 = scaleForPXX(startChan++, FS_flag == 0x10 ? 1 : 0);

        packet_p2M[PXX_PKT_BYTES - 3+i]   = chan_0;
        packet_p2M[PXX_PKT_BYTES - 3+i+1] = (((chan_0 >> 8) & 0x0F) | (chan_1 << 4));
        packet_p2M[PXX_PKT_BYTES - 3+i+2] = chan_1 >> 4;
    }

    // extra_flags byte definitions pulled from openTX
    // b0: antenna selection on Horus and Xlite
    // b1: turn off telemetry at receiver
    // b2: set receiver PWM output to channels 9-16
    // b3-4: RF power setting
    // b5: set to disable R9M S.Port output
    packet_p2M[PXX_PKT_BYTES - 15] = (g_model.rfOptionValue3/*Model.tx_power*/ << 3)
               | (g_model.rfOptionBool1 /*Model.proto_opts[PROTO_OPTS_RXTELEM]*/ << 1)
               | (g_model.rfOptionBool2/*Model.proto_opts[PROTO_OPTS_RXPWM]*/ << 2);

    uint16_t lcrc = crc(packet_p2M, PXX_PKT_BYTES-2);
    packet_p2M[PXX_PKT_BYTES - 16] = lcrc >> 8;
    packet_p2M[PXX_PKT_BYTES - 17] = lcrc;

    chan_offset ^= 0x08;

    for (uint8_t i = 0; i < 16; i++) {
      uint16_t pulse = FULL_CHANNEL_OUTPUTS(i);
      Usart0TxBuffer_p2M[--PxxTxBufferCount] = (i<<2) | ((pulse>>8)&0x03); // Encoded channel + upper 2 bits pulse width.
      Usart0TxBuffer_p2M[--PxxTxBufferCount] = pulse & 0xff; // Low byte
    }

#if !defined(SIMU)
    USART_TRANSMIT_BUFFER(PXX_USART);
#endif
}

static enum {
  PXX_BIND,
  PXX_BIND_DONE = 600,
  PXX_DATA1,
  PXX_DATA2,
} state;

static uint16_t mixer_runtime;

#if defined (TODO_FRSKY)// HAS_EXTENDED_TELEMETRY
// Support S.Port telemetry on RX pin
// couple defines to avoid errors from include file
static void serial_echo(uint8_t *packet) {(void)packet;}
#define PROTO_OPTS_AD2GAIN 0
#include "frsky_d_telem._c"
#include "frsky_s_telem._c"

static void frsky_parse_sport_stream_crc(uint8_t data) {
    frsky_parse_sport_stream(data, SPORT_CRC);
}
#endif  // HAS_EXTENDED_TELEMETRY


#define STD_DELAY   9000

static uint16_t PXX_SERIAL_cb()
{
    switch (state) {
    default: // binding
        build_data_pkt(1);
        state=PXX_BIND_DONE;
        return STD_DELAY;
    case PXX_BIND_DONE:
        PROTOCOL_SetBindState(0);
        state=PXX_DATA1;
        // intentional fall-through
    case PXX_DATA1:
        SCHEDULE_MIXER_END_IN_US(STD_DELAY);
        state = PXX_DATA2;
    case PXX_DATA2:
        build_data_pkt(0);
        state = PXX_DATA1;
         heartbeat |= HEART_TIMER_PULSES;
        CALCULATE_LAT_JIT(); // Calculate latency and jitter.
        return STD_DELAY *2;
    }
}

static void PXX_init()
{
// 125K 8N1
  USART_SET_BAUD_125K(PXX_USART);
  USART_SET_MODE_8N1(PXX_USART);
  USART_ENABLE_TX(PXX_USART);
  Usart0TxBufferCount = 0;
}

static void PXX_SERIAL_initialize(uint8_t bind)
{
    PXX_init();
#if defined(TODO_FRSKY)//HAS_EXTENDED_TELEMETRY
    USART_ENABLE_RX(PXX_USART);
    //SSER_Initialize(); // soft serial receiver
    //SSER_StartReceive(frsky_parse_sport_stream_crc);
#endif
    failsafe_count = 0;
    chan_offset = 0;
    FS_flag = 0;
    range_check = 0;
    packet_p2M[0] = (uint8_t) g_eeGeneral.fixed_ID.ID_32 & 0x3f;  // limit to valid range - 6 bits

    if (bind) {
        state = PXX_BIND;
        PROTOCOL_SetBindState(5000);
    } else {
        state = PXX_DATA1;
    }
    PROTO_Start_Callback(PXX_SERIAL_cb);
}


const void *PXX_Cmds(enum ProtoCmds cmd)
{
  switch(cmd) {
  case PROTOCMD_INIT:
    PXX_SERIAL_initialize(0);
    return 0;
  case PROTOCMD_BIND:
    PXX_SERIAL_initialize(1);
    return 0;
  case PROTOCMD_RESET:
    PROTO_Stop_Callback();
    PXX_SERIAL_Reset();
    return 0;
  case PROTOCMD_GETOPTIONS:
    SetRfOptionSettings(pgm_get_far_address(RfOpt_PXX_Ser),
                       STR_SUBTYPE_COUNTRY,      //Sub proto
                       STR_DUMMY,      //Option 1 (int)
                       STR_DUMMY,      //Option 2 (int)
                       STR_RFPOWER,      //Option 3 (uint 0 to 31)
                       STR_TELEMETRY,      //OptionBool 1
                       STR_OUTPUT_PWM,      //OptionBool 2
                       STR_DUMMY       //OptionBool 3
                      );
    return 0;
  default:
    break;
  }
  return 0;
}
