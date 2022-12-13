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

#define FRSKYR9_BIND rfState8_p2M
#define FLEX915MODE (g_model.rfSubType == 0)
#define FLEX868MODE (g_model.rfSubType == 1)
#define FLEXFCCMODE (g_model.rfSubType == 2)
#define FLEXEUMODE (g_model.rfSubType == 3)

#define XTELEMETRY  (g_model.rfOptionBool1)
#define X16MODE     (g_model.rfOptionBool2)// no telemetry if mode 16ch

#define DISP_FREQ_TABLE

#define FLEX_FREQ	29
#define FCC_FREQ	43
#define EU_FREQ		19

// Telemetry
//#define TELEMETRY_BUFFER_SIZE 32
//uint8_t telem_save_data_p2M[TELEMETRY_BUFFER_SIZE];//telemetry receiving packets
//uint8_t v_lipo1;
//uint8_t v_lipo2;
//uint8_t RX_RSSI;
//uint8_t TX_RSSI;
//uint8_t RX_LQI;
//uint8_t TX_LQI;
uint8_t  len;
uint16_t pps_counter;

const pm_char STR_SUBTYPE_FRSKYR9_SPI[] PROGMEM = " 915"" 868"" FCC""  EU";
const pm_char STR_X16MODE[] PROGMEM = INDENT "X16";//telemetry ne fonctionne pas en 16ch

const static RfOptionSettingsvar_t RfOpt_FRSKYR9_Ser[] PROGMEM = {
/*rfProtoNeed*/PROTO_NEED_SPI  | BOOL1USED | BOOL2USED, //can be PROTO_NEED_SPI | BOOL1USED | BOOL2USED | BOOL3USED
/*rfSubTypeMax*/3,//nb of sub protocols -1
/*rfOptionValue1Min*/0, // FREQFINE MIN
/*rfOptionValue1Max*/0,  // FREQFINE MAX
/*rfOptionValue2Min*/0,
/*rfOptionValue2Max*/0,
/*rfOptionValue3Max*/23,// rfpower: 0 -> 2dB, 15 -> 17dB
};


enum FRSKY_R9
{
	R9_915		= 0,//Flex
	R9_868		= 1,//Flex
	R9_915_8CH	= 2,//Flex
	R9_868_8CH	= 3,//Flex
	R9_FCC		= 4,//FCC
	R9_EU		= 5,
	R9_FCC_8CH	= 6,//FCC
	R9_EU_8CH	= 7,
};


enum {
	FRSKYR9_FREQ=0,
	FRSKYR9_DATA,
	FRSKYR9_RX1,
	FRSKYR9_RX2,
};

const uint16_t PROGMEM FrSkyX_CRC_Short[]={
	0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
	0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7 };
static uint16_t __attribute__((unused)) FrSkyX_CRCTable(uint8_t val)
{
	uint16_t word ;
	word = pgm_read_word(&FrSkyX_CRC_Short[val&0x0F]) ;
	val /= 16 ;
	return word ^ (0x1081 * val) ;
}

uint16_t FrSkyX_crc(uint8_t *data, uint8_t len, uint16_t init=0)
{
	uint16_t crc = init;
	for(uint8_t i=0; i < len; i++)
		crc = (crc<<8) ^ FrSkyX_CRCTable((uint8_t)(crc>>8) ^ *data++);
	return crc;
}

//uint8_t sub_protocol;//g_model.rfSubType
//uint32_t MProtocol_id;//tx id,
//uint32_t MProtocol_id_master;
//uint32_t blink=0,last_signal=0;
uint8_t FrSkyX_RX_Seq ;
uint8_t protocol_flags=0,protocol_flags2=0,protocol_flags3=0;

//Bind flag
//#define BIND_IN_PROGRESS	protocol_flags &= ~_BV(7)
//#define BIND_DONE			protocol_flags |= _BV(7)
//#define IS_BIND_DONE		( ( protocol_flags & _BV(7) ) !=0 )
//#define IS_BIND_IN_PROGRESS	( ( protocol_flags & _BV(7) ) ==0 )

//uint8_t send_seq_p2M;
//uint8_t FrSkyFormat=0;

// Channel value for FrSky (PPM is multiplied by 1.5)
uint16_t convert_channel_frsky(uint8_t num)
{
	uint16_t val=FULL_CHANNEL_OUTPUTS(num);
	return ((val*15)>>4)+1290;
}

// 0-2047, 0 = 817, 1024 = 1500, 2047 = 2182
//64=860,1024=1500,1984=2140//Taranis 125%
static uint16_t  FrSkyX_scaleForPXX( uint8_t i, uint8_t num_chan=8)
{	//mapped 860,2140(125%) range to 64,1984(PXX values);
	uint16_t chan_val=convert_channel_frsky(i)-1226;
	if(i>=num_chan) chan_val|=2048;   // upper channels offset
	return chan_val;
}

// Convert 32b id to temp_rfid_addr_p2M
static void set_temp_rfid_addr_p2M(uint32_t id)
{ // Used by almost all protocols
	temp_rfid_addr_p2M[0] = (id >> 24) & 0xFF;
	temp_rfid_addr_p2M[1] = (id >> 16) & 0xFF;
	temp_rfid_addr_p2M[2] = (id >>  8) & 0xFF;
	temp_rfid_addr_p2M[3] = (id >>  0) & 0xFF;
	temp_rfid_addr_p2M[4] = (temp_rfid_addr_p2M[2]&0xF0)|(temp_rfid_addr_p2M[3]&0x0F);
}


static void FrSkyX_channels(uint8_t offset)
{
	static uint8_t chan_start=0;

	//packet_p2M[7] = FLAGS 00 - standard packet
	//10, 12, 14, 16, 18, 1A, 1C, 1E - failsafe packet
	//20 - range check packet
/*	#ifdef FAILSAFE_ENABLE
		#define FRSKYX_FAILSAFE_TIMEOUT 1032
		static uint16_t failsafe_count=0;
		static uint8_t FS_flag=0,failsafe_chan=0;
		if (FS_flag == 0  &&  failsafe_count > FRSKYX_FAILSAFE_TIMEOUT  &&  chan_start == 0  &&  IS_FAILSAFE_VALUES_on)
		{
			FS_flag = 0x10;
			failsafe_chan = 0;
		} else if (FS_flag & 0x10 && failsafe_chan < (FrSkyFormat & 0x01 ? 8-1:16-1))
		{
			FS_flag = 0x10 | ((FS_flag + 2) & 0x0F);					//10, 12, 14, 16, 18, 1A, 1C, 1E - failsafe packet
			failsafe_chan ++;
		} else if (FS_flag & 0x10)
		{
			FS_flag = 0;
			failsafe_count = 0;
			FAILSAFE_VALUES_off;
		}
		failsafe_count++;
		if(protocol==PROTO_FRSKY_R9)
			failsafe_count++;					// R9 is 20ms, X is 9ms
		packet_p2M[offset] = FS_flag;
	#else*/
		packet_p2M[offset] = 0;
//	#endif
	//
	packet_p2M[offset+1] = 0;						//??
	//
	uint8_t chan_index = chan_start;
	uint16_t ch1,ch2;
	for(uint8_t i = offset+2; i < 12+offset+2 ; i+=3)
	{//12 bytes of channel data
/*		#ifdef FAILSAFE_ENABLE
			if( (FS_flag & 0x10) && ((failsafe_chan & 0x07) == (chan_index & 0x07)) )
				ch1 = FrSkyX_scaleForPXX_FS(failsafe_chan);
			else
		#endif*/
				ch1 = FrSkyX_scaleForPXX(chan_index);
		chan_index++;
		//
/*		#ifdef FAILSAFE_ENABLE
			if( (FS_flag & 0x10) && ((failsafe_chan & 0x07) == (chan_index & 0x07)) )
				ch2 = FrSkyX_scaleForPXX_FS(failsafe_chan);
			else
		#endif*/
				ch2 = FrSkyX_scaleForPXX(chan_index);
		chan_index++;
		//3 bytes per channel
		packet_p2M[i] = ch1;
		packet_p2M[i+1]=(((ch1>>8) & 0x0F)|(ch2 << 4));
		packet_p2M[i+2]=ch2>>4;
	}
	if(!X16MODE)//if(FrSkyFormat & 0x01)											//In X8 mode send only 8ch every 9ms
		chan_start = 0 ;
	else
		chan_start^=0x08;
}

void FrSkyR9_set_frequency()
{
	uint8_t data[3];
	uint16_t num=0;
	channel_index_p2M += channel_skip_p2M;
	switch(g_model.rfSubType)//switch(sub_protocol & 0xFD)
	{
		case 1://R9_868://g_model.rfSubType=1
			if(!FRSKYR9_BIND)//IS_BIND_DONE							// if bind is in progress use R9_915 instead
			{
				channel_index_p2M %= FLEX_FREQ;
				num=channel_index_p2M;
				if(channel_index_p2M>=FLEX_FREQ-2)
					num+=channel_skip_p2M-FLEX_FREQ+2;	// the last 2 values are channel_skip_p2M and channel_skip_p2M+1
				num <<= 5;
				num += 0xD700;
				break;
			}//else use R9_915
		case 0://R9_915://g_model.rfSubType=0
			channel_index_p2M %= FLEX_FREQ;
			num=channel_index_p2M;
			if(channel_index_p2M>=FLEX_FREQ-2)
				num+=channel_skip_p2M-FLEX_FREQ+2;		// the last 2 values are channel_skip_p2M and channel_skip_p2M+1
			num <<= 5;
			num += 0xE4C0;
			break;
		case 2://R9_FCC://g_model.rfSubType=2
			channel_index_p2M %= FCC_FREQ;
			num=channel_index_p2M;
			num <<= 5;
			num += 0xE200;
			break;
		case 3://R9_EU://g_model.rfSubType=3
			channel_index_p2M %= EU_FREQ;
			num=channel_index_p2M;
			num <<= 4;
			num += 0xD7D0;
			break;
	}
	data[0] = num>>8;
	data[1] = num&0xFF;
	data[2] = 0x00;
/*
	#ifdef DISP_FREQ_TABLE
		if(send_seq_p2M==0xFF)
			debugln("F%d=%02X%02X%02X=%lu", channel_index_p2M, data[0], data[1], data[2], (uint32_t)((data[0]<<16)+(data[1]<<8)+data[2])*61);
	#endif
*/
	SX1276_WriteRegisterMulti(SX1276_06_FRFMSB, data, 3);
}

static void FrSkyR9_build_packet()
{
	//ID
	packet_p2M[0] = temp_rfid_addr_p2M[1];
	packet_p2M[1] = temp_rfid_addr_p2M[2];
	packet_p2M[2] = temp_rfid_addr_p2M[3];
	//Hopping
	packet_p2M[3] = channel_index_p2M;	// current channel index
	packet_p2M[4] = channel_skip_p2M;		// step size and last 2 channels start index
	//RX number
	packet_p2M[5] = RXNUM;//RX_num;					// receiver number from OpenTX
	//Channels
	FrSkyX_channels(6);					// Set packet_p2M[6]=failsafe, packet_p2M[7]=0?? and packet_p2M[8..19]=channels data
	//Bind
	if(FRSKYR9_BIND)// bind in progress
	{/* 915 0x01=CH1-8_TELEM_ON
	        0x41=CH1-8_TELEM_OFF
	        0xC1=CH9-16_TELEM_OFF
	        0x81=CH9-16_TELEM_ON
    */
		packet_p2M[6] = 0x01;				// bind indicator
		if(g_model.rfSubType&1) //if(sub_protocol & 1)
			packet_p2M[6] |= 0x20;			// 868
		if(binding_idx_p2M&0x01)
			packet_p2M[6] |= 0x40;			// telem OFF
		if(binding_idx_p2M&0x02)
			packet_p2M[6] |= 0x80;			// ch9-16
	}
	//Sequence and send SPort
	//FrSkyX_seq_sport(20,23);			//20=RX|TXseq, 21=bytes count, 22&23=data

	//CRC
	uint16_t crc = FrSkyX_crc(packet_p2M, 24);
	packet_p2M[24] = crc;					// low byte
	packet_p2M[25] = crc >> 8;				// high byte
}

static uint8_t FrSkyR9_CRC8(uint8_t *p, uint8_t l)
{
	uint8_t crc = 0xFF;
	for (uint8_t i = 0; i < l; i++)
    {
		crc = crc ^ p[i];
		for ( uint8_t j = 0; j < 8; j++ )
			if ( crc & 0x80 )
			{
				crc <<= 1;
				crc ^= 0x07;
			}
			else
				crc <<= 1;
	}
	return crc;
}


static void FrSkyR9_build_EU_packet()
{
	//ID
	packet_p2M[0] = temp_rfid_addr_p2M[1];
	packet_p2M[1] = temp_rfid_addr_p2M[2];
	packet_p2M[2] = temp_rfid_addr_p2M[3];
	//Hopping
	packet_p2M[3] = channel_skip_p2M;		// step size and last 2 channels start index
	//RX number
	packet_p2M[4] = RXNUM;//RX_num;					// receiver number from OpenTX
	//Channels
	//TODO FrSkyX_channels(5,4);			// Set packet_p2M[5]=failsafe and packet_p2M[6..11]=4 channels data

	//Bind
	if(FRSKYR9_BIND)// bind in progress
	{
		packet_p2M[5] = 0x01;				// bind indicator
		if(!X8MODE)//if((sub_protocol & 2) == 0)
			packet_p2M[5] |= 0x10;			// 16CH
		// if(sub_protocol & 1)
			// packet_p2M[5] |= 0x20;			// 868
		if(binding_idx_p2M&0x01)
			packet_p2M[5] |= 0x40;			// telem OFF
		if(binding_idx_p2M&0x02)
			packet_p2M[5] |= 0x80;			// ch9-16
	}

	//Sequence and send SPort
	packet_p2M[12] = (FrSkyX_RX_Seq << 4)|0x08;	//TX=8 at startup

	//CRC
	packet_p2M[13] = FrSkyR9_CRC8(packet_p2M, 13);
}

static void FRSKYR9_init()
{
	//Check frequencies
	#ifdef DISP_FREQ_TABLE
		send_seq_p2M=0xFF;
		channel_skip_p2M=1;
		channel_index_p2M=0xFF;
		for(uint8_t i=0;i<FCC_FREQ;i++)
			FrSkyR9_set_frequency();
	#endif

	//Reset ID
	set_temp_rfid_addr_p2M(RXNUM);

	//channel_skip_p2M
	//channel_skip_p2M = 1 + (random(0xfefefefe) % 24);
	srandom(0xfefefefe);
	uint8_t idx = random() % 24;
	channel_skip_p2M = 1 + idx;
	//debugln("chanskip=%d", channel_skip_p2M);


	//Set FrSkyFormat
/*	if(X16MODE)//if((sub_protocol & 0x02) == 0)
		FrSkyFormat=0;											// 16 channels
	else
		FrSkyFormat=1;											// 8 channels
	//debugln("%dCH", FrSkyFormat&1 ? 8:16);
*/

	//EU packet length
	if(FLEXEUMODE)//if( (sub_protocol & 0xFD) == R9_EU )
		packetSize_p2M=14;
	else
		packetSize_p2M=26;

	//SX1276 Init
	SX1276_SetMode(true, false, SX1276_OPMODE_SLEEP);
	SX1276_SetMode(true, false, SX1276_OPMODE_STDBY);

	// uint8_t buffer[2];
	// buffer[0] = 0x00;
	// buffer[1] = 0x00;
	// SX1276_WriteRegisterMulti(SX1276_40_DIOMAPPING1, buffer, 2);

	SX1276_SetDetectOptimize(true, SX1276_DETECT_OPTIMIZE_SF6);
	SX1276_ConfigModem1(SX1276_MODEM_CONFIG1_BW_500KHZ, SX1276_MODEM_CONFIG1_CODING_RATE_4_5, true);
	SX1276_ConfigModem2(6, false, false);
	SX1276_ConfigModem3(false, false);
	SX1276_SetPreambleLength(9);
	SX1276_SetDetectionThreshold(SX1276_MODEM_DETECTION_THRESHOLD_SF6);
	SX1276_SetLna(1, true);
	SX1276_SetHopPeriod(0);										// 0 = disabled, we hop frequencies manually
	//RF Power
	SX1276_SetPaDac(false);										// Disable 20dBm mode
	SX1276_SetPaConfig(true, 7, g_model.rfOptionValue3);			// Use PA_HP on PA_BOOST, power=17-(15-option) dBm with option equal or lower to 15
	//SX1276_ManagePower();
	SX1276_SetOcp(true,27);										// Set OCP to max 240mA
	SX1276_SetTxRxMode(TX_EN);								// Set RF switch to TX
	//Enable all IRQ flags
	SX1276_WriteReg(SX1276_11_IRQFLAGSMASK,0x00);
	//FrSkyX_telem_init();TODO

	channel_index_p2M=0;
	send_seq_p2M=FRSKYR9_FREQ;
	//return 20000;												// Start calling FrSkyR9_callback in 20 milliseconds
}

static uint16_t FrSkyR9_callback()
{
	switch (send_seq_p2M)
	{
		case FRSKYR9_FREQ:
		  SCHEDULE_MIXER_END_IN_US(460*2); // Schedule next Mixer calculations.
			//Force standby
			SX1276_SetMode(true, false, SX1276_OPMODE_STDBY);
			//Set frequency
			FrSkyR9_set_frequency(); 							// Set current center frequency

			//Set power
			// max power: 15dBm (10.8 + 0.6 * MaxPower [dBm])
			// output_power: 2 dBm (17-(15-OutputPower) (if pa_boost_pin == true))
			//SX1276_SetPaConfig(true, 7, 0);						// Lowest power for the T18
      //remplacer SX1276_SetPaConfig par SX1276_ManagePower() dans menu_model_setup.cpp
      SX1276_ManagePower();//pour test

			//Build packet
			if( packetSize_p2M == 26 )
				FrSkyR9_build_packet();
			else
				FrSkyR9_build_EU_packet();
			send_seq_p2M++;
			return 460*2;											// Frequency settle time
		case FRSKYR9_DATA:
			//Set RF switch to TX
			SX1276_SetTxRxMode(TX_EN);
			//Send packet
			SX1276_WritePayloadToFifo(packet_p2M, packetSize_p2M);
			SX1276_SetMode(true, false, SX1276_OPMODE_TX);
#if not defined (FRSKY)
			send_seq_p2M=FRSKYR9_FREQ;
			return (20000-460)*2;
#else

			send_seq_p2M++;
			return 11140*2;										// Packet send time
		case FRSKYR9_RX1:
			//Force standby
			SX1276_SetMode(true, false, SX1276_OPMODE_STDBY);
			//RX packet size is 13
			SX1276_WriteReg(SX1276_22_PAYLOAD_LENGTH, 13);
			//Reset pointer
			SX1276_WriteReg(SX1276_0D_FIFOADDRPTR, 0x00);
			//Set RF switch to RX
			SX1276_SetTxRxMode(RX_EN);
			//Clear all IRQ flags
			SX1276_WriteReg(SX1276_12_REGIRQFLAGS,0xFF);
			//Switch to RX
			SX1276_WriteReg(SX1276_01_OPMODE, 0x85);
			send_seq_p2M++;
			return 7400*2;

		case FRSKYR9_RX2:

			if( (SX1276_ReadReg(SX1276_12_REGIRQFLAGS)&0xF0) == (_BV(SX1276_REGIRQFLAGS_RXDONE) | _BV(SX1276_REGIRQFLAGS_VALIDHEADER)) )
			{
				if(SX1276_ReadReg(SX1276_13_REGRXNBBYTES)==13)
				{
					SX1276_ReadRegisterMulti(SX1276_00_FIFO,telem_save_data_p2M,13);
#if defined(FRSKY)
          if (XTELEMETRY) // telemetry on?
            {
              frskyX_check_telemetry(telem_save_data_p2M, 13);
            }
#endif

					if( telem_save_data_p2M[9]==temp_rfid_addr_p2M[1] && telem_save_data_p2M[10]==temp_rfid_addr_p2M[2] && FrSkyX_crc(telem_save_data_p2M, 11, temp_rfid_addr_p2M[1]+(temp_rfid_addr_p2M[2]<<8))==(telem_save_data_p2M[11]+(telem_save_data_p2M[12]<<8)) )
					{
						if(telem_save_data_p2M[0]&0x80)
							telemetryData.rssi[0].set(telem_save_data_p2M[0]<<1);//RX_RSSI=telem_save_data_p2M[0]<<1;
						else
							telemetryData.analog[TELEM_ANA_A1].set((telem_save_data_p2M[0]<<1)+1);//v_lipo1=(telem_save_data_p2M[0]<<1)+1;
						//TX_LQI=~(SX1276_ReadReg(SX1276_19_PACKETSNR)>>2)+1;
						telemetryData.rssi[1].set(SX1276_ReadReg(SX1276_1A_PACKETRSSI)-157);//TX_RSSI=SX1276_ReadReg(SX1276_1A_PACKETRSSI)-157;
						for(uint8_t i=0;i<9;i++)
							packet_p2M[4+i]=telem_save_data_p2M[i];			// Adjust buffer to match FrSkyX
						//frsky_process_telemetry(packet_p2M,len);	// Process telemetry packet
						frskyStreaming = frskyStreaming ? FRSKY_TIMEOUT10ms : FRSKY_TIMEOUT_FIRST;
						pps_counter++;
						if(TX_LQI==0)
							TX_LQI++;							// Recover telemetry right away
					}

				}
			}
/*
			if (millis() - pps_timer >= 1000)
			{//1 packet every 20ms
				pps_timer = millis();
				debugln("%d pps", pps_counter);
				TX_LQI = pps_counter<<1;						// Max=100%
				pps_counter = 0;
			}

			if(TX_LQI==0)
      {
 				//FrSkyX_telem_init();							// Reset telemetry
      }

			else
      {
        telemetry_link=1;								// Send telemetry out anyway
      }
*/
			send_seq_p2M=FRSKYR9_FREQ;
      CC2500_Strobe(CC2500_SFRX);	// Flush the RX FIFO buffer
      send_seq_p2M = HITEC_PREP;
      heartbeat |= HEART_TIMER_PULSES;
      CALCULATE_LAT_JIT(); // Calculate latency and jitter.
			break;
#endif

	}
	return 1000;

}
/*
static uint16_t FRSKYR9_bind_cb()
{
  SCHEDULE_MIXER_END_IN_US(18000); // Schedule next Mixer calculations.
  bindR9mode = 1;
  FrSkyR9_callback();
  //FRSKYR9_send_bind_packet();
  heartbeat |= HEART_TIMER_PULSES;
  CALCULATE_LAT_JIT(); // Calculate latency and jitter.
  return 18000U *2;
}

static uint16_t FRSKYR9_cb()
{
  SCHEDULE_MIXER_END_IN_US(12000); // Schedule next Mixer calculations.
  bindR9mode = 0;
  uint16_t time = FrSkyR9_callback();
  //FRSKYR9_send_data_packet();
  heartbeat |= HEART_TIMER_PULSES;
  CALCULATE_LAT_JIT(); // Calculate latency and jitter.
  //return 12000U *2;
  return time * 2;
}
*/
static uint16_t FRSKYR9_cb()
{
 uint16_t time = FrSkyR9_callback();
 heartbeat |= HEART_TIMER_PULSES;
 CALCULATE_LAT_JIT(); // Calculate latency and jitter.
 return time;
}



static void FRSKYR9_initialize(uint8_t bind)
{
  FRSKYR9_init();
  FRSKYR9_BIND = bind; // store bind state
  PROTO_Start_Callback( FRSKYR9_cb);
}

const void *FRSKYR9_Cmds(enum ProtoCmds cmd)
{
  switch(cmd) {
  case PROTOCMD_INIT:
    FRSKYR9_initialize(0);
    return 0;
  case PROTOCMD_RESET:
    PROTO_Stop_Callback();
    return 0;
  case PROTOCMD_BIND:
    FRSKYR9_initialize(1);
    return 0;
  case PROTOCMD_GETOPTIONS:
     SetRfOptionSettings(pgm_get_far_address(RfOpt_FRSKYR9_Ser),
                          STR_SUBTYPE_FRSKYR9_SPI, //Sub proto
                          STR_DUMMY, //Option 1 (int)
                          STR_DUMMY,      //Option 2 (int)
                          STR_RFPOWER,     //Option 3 (uint 0 to 15)
                          STR_TELEMETRY,   //OptionBool 1
                          STR_X16MODE,   //OptionBool 2
                          STR_DUMMY       //OptionBool 3
                         );

    return 0;
  default:
    break;
  }
  return 0;
}

