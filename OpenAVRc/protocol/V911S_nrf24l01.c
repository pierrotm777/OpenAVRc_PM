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

const pm_char STR_SUBTYPE_V911S[] PROGMEM = "V911S";

const static RfOptionSettingsvar_t RfOpt_V911S_Ser[] PROGMEM =
{
  /*rfProtoNeed*/PROTO_NEED_SPI | BOOL1USED | BOOL2USED, //can be PROTO_NEED_SPI | BOOL1USED | BOOL2USED | BOOL3USED
  /*rfSubTypeMax*/0,
  /*rfOptionValue1Min*/0, // FREQFINE MIN
  /*rfOptionValue1Max*/0,  // FREQFINE MAX
  /*rfOptionValue2Min*/0,
  /*rfOptionValue2Max*/0,
  /*rfOptionValue3Max*/3,    // RF POWER
};


#define V911S_BIND_COUNT      1000
#define V911S_PACKET_PERIOD   5000 // Timeout for callback in uSec
#define V911S_BIND_PACKET_PERIOD    3300
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
//#define dbgprintf if(0) printf

//#define V911S_ORIGINAL_ID

#define V911S_INITIAL_WAIT          500
#define V911S_PACKET_SIZE           16
#define V911S_RF_BIND_CHANNEL       35
#define V911S_NUM_RF_CHANNELS       8
#define NUM_OUT_CHANNELS            5//ajout PM

#define CHAN_MAX_VALUE              2000
#define CHAN_MIN_VALUE              1000


uint8_t tx_power = 3;// 0 to 3
#define V911S_RF_CH_NUM channel_offset_p2M
//static uint16_t bind_idx_p2M;
uint16_t packetV911s_period;
//extern volatile int32_t Channels[NUM_OUT_CHANNELS];

/*
enum V911S
{
 V911S	 = 0,
 E119	 = 1,
};
*/

enum{
    V911S_BIND,
    V911S_DATA
};

// flags going to packet[1]
#define V911S_FLAG_EXPERT   0x04
// flags going to packet[2]
#define V911S_FLAG_CALIB    0x01

// For code readability
enum {
    CHANNEL1 = 0,
    CHANNEL2,
    CHANNEL3,
    CHANNEL4,
    CHANNEL5,
};
#define CHANNEL_CALIB   CHANNEL5

// Bit vector from bit position
#define BV(bit) (1 << bit)

#define CHAN_RANGE (CHAN_MAX_VALUE - CHAN_MIN_VALUE)
static uint16_t scale_channel(uint8_t ch, uint16_t destMin, uint16_t destMax)
{
    int32_t chanval = FULL_CHANNEL_OUTPUTS(ch);
    int32_t range = (int32_t) destMax - (int32_t) destMin;

    if (chanval < CHAN_MIN_VALUE)
        chanval = CHAN_MIN_VALUE;
    else if (chanval > CHAN_MAX_VALUE)
        chanval = CHAN_MAX_VALUE;
    return (range * (chanval - CHAN_MIN_VALUE)) / CHAN_RANGE + destMin;
}

#define GET_FLAG(ch, mask) (FULL_CHANNEL_OUTPUTS(ch) > 0 ? mask : 0)
static void V911S_send_packet(uint8_t bind)
{
    uint8_t channel=channel_index_p2M;
    if(bind)
    {
        packet_p2M[0] = 0x42;
        packet_p2M[1] = 0x4E;
        packet_p2M[2] = 0x44;
        for(uint8_t i=0;i<5;i++)
            packet_p2M[i+3] = temp_rfid_addr_p2M[i];
        for(uint8_t i=0;i<8;i++)
            packet_p2M[i+8] = channel_used_p2M[i];
    }
    else
    {
        if(V911S_RF_CH_NUM&1)
        {
            if((channel_index_p2M&1)==0)
                channel+=8;
            channel>>=1;
        }
        if(V911S_RF_CH_NUM&2)
            channel=7-channel;
        packet_p2M[ 0]=(V911S_RF_CH_NUM<<3)|channel;
        packet_p2M[ 1]=V911S_FLAG_EXPERT; // short press on left button
        packet_p2M[ 2]=GET_FLAG(CHANNEL_CALIB,V911S_FLAG_CALIB); // long  press on right button
        memset(packet_p2M+3, 0x00, V911S_PACKET_SIZE - 3);
        //packet_p2M[3..6]=trims TAER signed
        uint16_t ch=scale_channel(CHANNEL3 ,0,0x7FF);// throttle
        packet_p2M[ 7] = ch;
        packet_p2M[ 8] = ch>>8;
        ch=scale_channel(CHANNEL1 ,0x7FF,0);    // aileron
        packet_p2M[ 8]|= ch<<3;
        packet_p2M[ 9] = ch>>5;
        ch=scale_channel(CHANNEL2,0,0x7FF);     // elevator
        packet_p2M[10] = ch;
        packet_p2M[11] = ch>>8;
        ch=scale_channel(CHANNEL4,0x7FF,0);      // rudder
        packet_p2M[11]|= ch<<3;
        packet_p2M[12] = ch>>5;
    }

    // Power on, TX mode, 2byte CRC
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    if (!bind)
    {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, channel_used_p2M[channel]);
        channel_index_p2M++;
        channel_index_p2M&=7;    // 8 RF channels
    }
    // clear packet_p2M status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    XN297_WritePayload(packet_p2M, V911S_PACKET_SIZE);


    //if (tx_power != Model.tx_power) {
        //Keep transmit power updated
    //    tx_power = Model.tx_power;
        NRF24L01_ManagePower();//NRF24L01_SetPower(tx_power);
    //}

}

static void V911S_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    XN297_SetTXAddr((uint8_t *)"\x4B\x4E\x42\x4E\x44", 5);          // Bind address
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, V911S_RF_BIND_CHANNEL);    // Bind channel
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);    // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);     // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01); // Enable data pipe 0 only
    NRF24L01_SetBitrate(NRF24L01_BR_250K);          // 250Kbps
    NRF24L01_ManagePower();//NRF24L01_SetPower(tx_power);

}

static void V911S_initialize_txid()
{
/*
    uint32_t lfsr = 0xb2c54a2ful;
    uint8_t i,j;

#ifndef USE_FIXED_MFGID
    uint8_t var[12];
    MCU_SerialNumber(var, 12);
    dbgprintf("Manufacturer id: ");
    for (i = 0; i < 12; ++i) {
        dbgprintf("%02X", var[i]);
        rand32_r(&lfsr, var[i]);
    }
    dbgprintf("\r\n");
#endif
*/
	//channels
	uint8_t offset=temp_rfid_addr_p2M[3]%5;				// 0-4
	for(uint8_t i=0;i<V911S_NUM_RF_CHANNELS;i++)
		channel_used_p2M[i]=0x10+i*5+offset;
	if(!offset) channel_used_p2M[0]++;

	// channels order
	//V911S_RF_CH_NUM=random(0xfefefefe)&0x03;			// 0-3
	srandom(g_eeGeneral.fixed_ID.ID_32 & 0xfefefefe);
}

static uint16_t V911S_callback()
{
    switch(rfState8_p2M) {
        case V911S_BIND:
            SCHEDULE_MIXER_END_IN_US(packetV911s_period); // Schedule next Mixer calculations.
            if (bind_idx_p2M == 0)
            {
                PROTOCOL_SetBindState(0);
                XN297_SetTXAddr(temp_rfid_addr_p2M, 5);
                packetV911s_period=V911S_PACKET_PERIOD;
                rfState8_p2M = V911S_DATA;
            }
            else
            {
                V911S_send_packet(1);
                bind_idx_p2M--;
                if(bind_idx_p2M==100)       // same as original TX...
                    packetV911s_period=V911S_BIND_PACKET_PERIOD*3;
            }
            break;
        case V911S_DATA:
            SCHEDULE_MIXER_END_IN_US(packetV911s_period); // Schedule next Mixer calculations.
            V911S_send_packet(0);
            break;
    }
    return packetV911s_period*2;
}

static uint16_t V911S_cb()
{
 uint16_t time = V911S_callback();
 heartbeat |= HEART_TIMER_PULSES;
 CALCULATE_LAT_JIT(); // Calculate latency and jitter.
 return time;
}

static void V911S_initialize(uint8_t bind)
{
    //CLOCK_StopTimer();
    V911S_initialize_txid();
    NRF24L01_ManagePower();//tx_power = Model.tx_power;

    #ifdef V911S_ORIGINAL_ID
        temp_rfid_addr_p2M[0]=0xA5;
        temp_rfid_addr_p2M[1]=0xFF;
        temp_rfid_addr_p2M[2]=0x70;
        temp_rfid_addr_p2M[3]=0x8D;
        temp_rfid_addr_p2M[4]=0x76;
        for(uint8_t i=0;i<V911S_NUM_RF_CHANNELS;i++)
            channel_used_p2M[i]=0x10+i*5;
        channel_used_p2M[0]++;
        V911S_RF_CH_NUM=0;
    #endif

    V911S_init();

    if(bind)
    {
        bind_idx_p2M = V911S_BIND_COUNT;
        packetV911s_period= V911S_BIND_PACKET_PERIOD;
        PROTOCOL_SetBindState(((V911S_BIND_COUNT-100) * V911S_BIND_PACKET_PERIOD + V911S_BIND_PACKET_PERIOD*300) / 1000);
        rfState8_p2M = V911S_BIND;
    }
    else
    {
        XN297_SetTXAddr(temp_rfid_addr_p2M, 5);
        packetV911s_period= V911S_PACKET_PERIOD;
        rfState8_p2M = V911S_DATA;
    }
    channel_index_p2M=0;
    PROTO_Start_Callback( V911S_cb);
}

const void *V911S_Cmds(enum ProtoCmds cmd)
{
  switch(cmd)
    {
    case PROTOCMD_INIT:
      V911S_initialize(0);
      return 0;
    case PROTOCMD_BIND:
      bind_idx_p2M = 0;
      V911S_initialize(1);
      return 0;
    case PROTOCMD_RESET:
      PROTO_Stop_Callback();
      NRF24L01_Reset();
      return 0;
    case PROTOCMD_GETOPTIONS:
      SetRfOptionSettings(pgm_get_far_address(RfOpt_V911S_Ser),
                          STR_SUBTYPE_V911S,       //Sub proto
                          STR_DUMMY,      //Option 1 (int)
                          STR_DUMMY,      //Option 2 (int)
                          STR_RFPOWER,    //Option 3 (uint 0 to 31)
                          STR_DUMMY,      //OptionBool 1
                          STR_AUTOBIND,      //OptionBool 2
                          STR_DUMMY       //OptionBool 3
                         );
      return 0;
    default:
      break;
    }
  return 0;
}

