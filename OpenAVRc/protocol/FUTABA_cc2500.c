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


//#define USE_TUNE_FREQ

const static RfOptionSettingsvar_t RfOpt_SFHSS_Ser[] PROGMEM =
{
  /*rfProtoNeed*/PROTO_NEED_SPI | BOOL2USED, //can be PROTO_NEED_SPI | BOOL1USED | BOOL2USED | BOOL3USED
  /*rfSubTypeMax*/0,
  /*rfOptionValue1Min*/-128, // FREQFINE MIN
  /*rfOptionValue1Max*/127,  // FREQFINE MAX
  /*rfOptionValue2Min*/0,
  /*rfOptionValue2Max*/0,
  /*rfOptionValue3Max*/7,    // RF POWER
};


#define CHAN_MULTIPLIER  100
#define CHAN_MAX_VALUE (100 * CHAN_MULTIPLIER)
#define SFHSS_autobind g_model.rfOptionBool2

#define SFHSS_COARSE	0

#define SFHSS_PACKET_LEN 13
#define SFHSS_TX_ID_LEN   2


static uint8_t fhss_code=0; // 0-27
static uint16_t counter;

enum {
    SFHSS_START = 0x00,
    SFHSS_CAL   = 0x01,
    SFHSS_DATA1 = 0x02,
    SFHSS_DATA2 = 0x03,
    SFHSS_TUNE  = 0x04
};

#define SFHSS_FREQ0_VAL 0xC4

// Some important initialization parameters, all others are either default,
// or not important in the context of transmitter
// IOCFG2   2F - GDO2_INV=0 GDO2_CFG=2F - HW0
// IOCFG1   2E - GDO1_INV=0 GDO1_CFG=2E - High Impedance
// IOCFG0   2F - GDO0 same as GDO2, TEMP_SENSOR_ENABLE=off
// FIFOTHR  07 - 33 decimal TX threshold
// SYNC1    D3
// SYNC0    91
// PKTLEN   0D - Packet length, 0D bytes
// PKTCTRL1 04 - APPEND_STATUS on, all other are receive parameters - irrelevant
// PKTCTRL0 0C - No whitening, use FIFO, CC2400 compatibility on, use CRC, fixed packet length
// ADDR     29
// CHANNR   10
// FSCTRL1  06 - IF 152343.75Hz, see page 65
// FSCTRL0  00 - zero freq offset
// FREQ2    5C - synthesizer frequency 2399999633Hz for 26MHz crystal, ibid
// FREQ1    4E
// FREQ0    C4
// MDMCFG4  7C - CHANBW_E - 01, CHANBW_M - 03, DRATE_E - 0C. Filter bandwidth = 232142Hz
// MDMCFG3  43 - DRATE_M - 43. Data rate = 128143bps
// MDMCFG2  83 - disable DC blocking, 2-FSK, no Manchester code, 15/16 sync bits detected (irrelevant for TX)
// MDMCFG1  23 - no FEC, 4 preamble bytes, CHANSPC_E - 03
// MDMCFG0  3B - CHANSPC_M - 3B. temp_rfid_addr_p2M spacing = 249938Hz (each 6th temp_rfid_addr_p2M used, resulting in spacing of 1499628Hz)
// DEVIATN  44 - DEVIATION_E - 04, DEVIATION_M - 04. Deviation = 38085.9Hz
// MCSM2    07 - receive parameters, default, irrelevant
// MCSM1    0C - no CCA (transmit always), when packet received stay in RX, when sent go to IDLE
// MCSM0    08 - no autocalibration, PO_TIMEOUT - 64, no pin radio control, no forcing XTAL to stay in SLEEP
// FOCCFG   1D - not interesting, Frequency Offset Compensation
// FREND0   10 - PA_POWER = 0


const PROGMEM uint8_t SFHSS_init_values[] = {
  /* 00 */ 0x2F, 0x2E, 0x2F, 0x07, 0xD3, 0x91, 0x0D, 0x04,
  /* 08 */ 0x0C, 0x29, 0x10, 0x06, 0x00, 0x5C, 0x4E, SFHSS_FREQ0_VAL + SFHSS_COARSE,
  /* 10 */ 0x7C, 0x43, 0x83, 0x23, 0x3B, 0x44, 0x07, 0x0C,
  /* 18 */ 0x08, 0x1D, 0x1C, 0x43, 0x40, 0x91, 0x57, 0x6B,
  /* 20 */ 0xF8, 0xB6, 0x10, 0xEA, 0x0A, 0x11, 0x11
};

static void SFHSS_rf_init()
{
	CC2500_Strobe(CC2500_SIDLE);

	for (uint8_t i = 0; i < 39; ++i)
		CC2500_WriteReg(i, pgm_read_byte_near(&SFHSS_init_values[i]));

	CC2500_WriteReg(CC2500_0C_FSCTRL0, g_model.rfOptionValue1);

	CC2500_SetTxRxMode(TX_EN);
	CC2500_ManagePower();//CC2500_SetPower();
}

static void SFHSS_tune_chan()
{
    CC2500_Strobe(CC2500_SIDLE);
    CC2500_WriteReg(CC2500_0A_CHANNR, channel_index_p2M*6+16);
    CC2500_Strobe(CC2500_SCAL);
}

static void SFHSS_tune_chan_fast()
{
    CC2500_Strobe(CC2500_SIDLE);
    CC2500_WriteReg(CC2500_0A_CHANNR, channel_index_p2M*6+16);
    CC2500_WriteReg(CC2500_25_FSCAL1, calData[channel_index_p2M]);
    //_delay_us(6);
}

static void SFHSS_tune_freq()
{
	if ( freq_fine_mem_p2M != g_model.rfOptionValue1)//prev_option != option
	{
		CC2500_WriteReg(CC2500_0C_FSCTRL0, g_model.rfOptionValue1);
		CC2500_WriteReg(CC2500_0F_FREQ0, SFHSS_FREQ0_VAL + SFHSS_COARSE);
		freq_fine_mem_p2M = g_model.rfOptionValue1 ;
		rfState16_p2M = SFHSS_START;								// Restart the tune process if option is changed to get good tuned values
	}
}

static void SFHSS_calc_next_chan()
{
    channel_index_p2M += fhss_code + 2;
    if (channel_index_p2M > 29) {
        if (channel_index_p2M < 31)
            channel_index_p2M += fhss_code + 2;
        channel_index_p2M -= 31;
    }
}


// Channel values are 12-bit values between 1020 and 2020, 1520 is the middle
// Futaba @140% is 2070...1520...970
// Values grow down and to the right, exact opposite to Deviation, so
// we just revert every channel
static uint16_t convert_channel(uint8_t num)
{
    uint16_t value = 1520 - (int32_t)FULL_CHANNEL_OUTPUTS(num) * 410 / CHAN_MAX_VALUE;
    if (value > 2135)     //-150%
        value = 2135;
    else if (value < 905) //+150%
        value = 905;
    return value;
}

static void SFHSS_send_packet()//build_data_packet()
{
    uint16_t channel[4];
    // command.bit0 is the packet number indicator: =0 -> SFHSS_DATA1, =1 -> SFHSS_DATA2
    // command.bit1 is unknown but seems to be linked to the payload[0].bit0 but more dumps are needed: payload[0]=0x82 -> =0, payload[0]=0x81 -> =1
    // command.bit2 is the failsafe transmission indicator: =0 -> normal data, =1->failsafe data
    // command.bit3 is the temp_rfid_addr_p2Ms indicator: =0 -> CH1-4, =1 -> CH5-8
    uint8_t command = (rfState16_p2M == SFHSS_DATA1) ? 0 : 1; // Building packet for Data1 or Data2
    counter+=command;
    /*// Failsafe
    if( (counter&0x3FC) == 0x3FC )
    {	// Transmit failsafe data twice every 7s
        if( ((counter&1)^(command&1)) == 0 )
            command|=0x04;
    }// Failsafe
    else
    */
        command|=0x02; // Assuming packet[0] == 0x81
    counter&=0x3FF; // Reset failsafe counter
    if(counter&1) command|=0x08; // Transmit lower and upper temp_rfid_addr_p2Ms twice in a row

    uint8_t ch_offset = (command&0x08) >> 1;
    if(!(command & 0x04)) { // regular channels
        channel[0] = convert_channel(ch_offset + 0);
        channel[1] = convert_channel(ch_offset + 1);
        channel[2] = convert_channel(ch_offset + 2);
        channel[3] = convert_channel(ch_offset + 3);
    }
    /*else { // failsafe channels
        //Failsafe data are:
        // 0 to 1023 -> no output on channel
        // 1024-2047 -> hold output on channel
        // 2048-4095 -> channel_output=(data&0x3FF)*5/4+880 in 탎
        // Notes:
        //    2048-2559 -> does not look valid since it only covers the range from 1520탎 to 2160탎
        //    2560-3583 -> valid for any channel values from 880탎 to 2160탎
        //    3584-4095 -> looks to be used for the throttle channel with values ranging from 880탎 to 1520탎
        for(ch=0; ch<4; ch++) {
            if(ch+ch_offset<Model.num_channels && (Model.limits[ch+ch_offset].flags & CH_FAILSAFE_EN)) {
                channel[ch] = 0xc00 - ((s32)Model.limits[ch+ch_offset].failsafe * 4096/1000);
                if(channel[ch] > 0xdff)
                    channel[ch] = 0xdff;
            }
            else
                channel[ch] = 0x400; // hold ?
        }
        if((command&0x08)==0)
            channel[2]|=0x800; // special flag for throttle
    }*/

	// XK		[0]=0x81 [3]=0x00 [4]=0x00
	// T8J		[0]=0x81 [3]=0x42 [4]=0x07
	// T10J		[0]=0x81 [3]=0x0F [4]=0x09
	// TM-FH	[0]=0x82 [3]=0x9A [4]=0x06
    packet_p2M[0]  = 0x81; // can be 80, 81, 81 for Orange, only 81 for XK
    packet_p2M[1]  = temp_rfid_addr_p2M[0];
    packet_p2M[2]  = temp_rfid_addr_p2M[1];
    packet_p2M[3]  = 0;// unknown but prevents some receivers to bind if not 0
    packet_p2M[4]  = 0;// unknown but prevents some receivers to bind if not 0
    packet_p2M[5]  = (channel_index_p2M << 3) | ((channel[0] >> 9) & 0x07);
    packet_p2M[6]  = (channel[0] >> 1);
    packet_p2M[7]  = (channel[0] << 7) | ((channel[1] >> 5) & 0x7F);
    packet_p2M[8]  = (channel[1] << 3) | ((channel[2] >> 9) & 0x07);
    packet_p2M[9]  = (channel[2] >> 1);
    packet_p2M[10] = (channel[2] << 7) | ((channel[3] >> 5) & 0x7F);
    packet_p2M[11] = (channel[3] << 3) | ((fhss_code >> 2) & 0x07);
    packet_p2M[12] = (fhss_code << 6) | command;

    CC2500_WriteData(packet_p2M, sizeof(packet_p2M));
}


//static uint16_t mixer_runtime;
static uint16_t SFHSS_callback()
{
    switch(rfState16_p2M)
    {
    case SFHSS_START:
        channel_index_p2M = 0;
        SFHSS_tune_chan();
        rfState16_p2M = SFHSS_CAL;
        return 2000*2;
    case SFHSS_CAL:
        //SCHEDULE_MIXER_END_IN_US(2000*2); // Schedule next Mixer calculations.
        calData[channel_index_p2M]=CC2500_ReadReg(CC2500_25_FSCAL1);
        if (++channel_index_p2M < 30) {
            SFHSS_tune_chan();
        } else {
            channel_index_p2M = 0;
            counter = 0;
            rfState16_p2M = SFHSS_DATA1;
        }
        return 2000*2;
    /* Work cycle, 6.8ms, second packet 1.65ms after first */
    #define SFHSS_PACKET_PERIOD	6800U
    #define SFHSS_DATA2_TIMING	1630	// Adjust this value between 1600 and 1650 if your RX(s) are not operating properly
    case SFHSS_DATA1:
        SCHEDULE_MIXER_END_IN_US(SFHSS_PACKET_PERIOD); // Schedule next Mixer calculations.
        SFHSS_send_packet();
        rfState16_p2M = SFHSS_DATA2;
        return SFHSS_DATA2_TIMING * 2;
    case SFHSS_DATA2:
        SFHSS_send_packet();
        SFHSS_calc_next_chan();
        rfState16_p2M = SFHSS_TUNE;
        return 2020 * 2;	// original 2000
    case SFHSS_TUNE:
			rfState16_p2M = SFHSS_DATA1;
			SFHSS_tune_freq();
			SFHSS_tune_chan_fast();
      CC2500_ManagePower();//CC2500_SetPower(TXPOWER_1);
      heartbeat |= HEART_TIMER_PULSES;
      CALCULATE_LAT_JIT(); // Calculate latency and jitter.
      return 2000*2;

    }

    return 0;
}


// Generate internal id
static void SFHSS_get_tx_id()
{
	// Some receivers (Orange) behaves better if they tuned to id that has
	//  no more than 6 consecutive zeros and ones
	uint32_t fixed_id;
	uint8_t run_count = 0;
	// add guard for bit count
	fixed_id = 1 ^ (g_eeGeneral.fixed_ID.ID_32 & 1);
	for (uint8_t i = 0; i < 16; ++i)
	{
		fixed_id = (fixed_id << 1) | (g_eeGeneral.fixed_ID.ID_32 & 1);
		g_eeGeneral.fixed_ID.ID_32 >>= 1;
		// If two LS bits are the same
		if ((fixed_id & 3) == 0 || (fixed_id & 3) == 3)
		{
			if (++run_count > 6)
			{
				fixed_id ^= 1;
				run_count = 0;
			}
		}
		else
			run_count = 0;
	}
	//    fixed_id = 0xBC11;
	temp_rfid_addr_p2M[0] = fixed_id >> 8;
	temp_rfid_addr_p2M[1] = fixed_id >> 0;
}


static uint16_t SFHSS_cb()
{
 uint16_t time = SFHSS_callback();
 heartbeat |= HEART_TIMER_PULSES;
 CALCULATE_LAT_JIT(); // Calculate latency and jitter.
 return time;
}

static void SFHSS_initialize()
{
  //BIND_DONE;	// Not a TX bind protocol
  SFHSS_get_tx_id();

  //Normal hopping
  uint32_t rnd;
  memcpy(&rnd, temp_rfid_addr_p2M, 4); // load id
  fhss_code = rnd % 28; // Initialize it to random 0-27 inclusive

  //loadrfidaddr_rxnum(3);
  //CC2500_Reset();
  SFHSS_rf_init();
  rfState16_p2M = SFHSS_START;
  /*
  if (bind || SFHSS_autobind)
  {
    if (SFHSS_autobind)
    {
      PROTOCOL_SetBindState(1000);
    }
  }*/
  PROTO_Start_Callback(SFHSS_cb);
}

const void *SFHSS_Cmds(enum ProtoCmds cmd)
{
  switch(cmd)
    {
    case PROTOCMD_INIT:
      SFHSS_initialize();
      return 0;
    case PROTOCMD_BIND:
      SFHSS_initialize();
      return 0;
    case PROTOCMD_RESET:
      PROTO_Stop_Callback();
      CC2500_Reset();
      return 0;
    case PROTOCMD_GETOPTIONS:
      SetRfOptionSettings(pgm_get_far_address(RfOpt_SFHSS_Ser),
                          STR_DUMMY,       //Sub proto
                          STR_RFTUNEFINE,      //Option 1 (int)
                          STR_DUMMY,       //Option 2 (int)
                          STR_RFPOWER,     //Option 3 (uint 0 to 31)
                          STR_DUMMY,   //OptionBool 1
                          STR_AUTOBIND,      //OptionBool 2
                          STR_DUMMY        //OptionBool 3
                         );
      return 0;
    default:
      break;
    }
  return 0;
}

