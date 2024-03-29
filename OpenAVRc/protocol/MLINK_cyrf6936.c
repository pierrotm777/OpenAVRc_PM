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

#define MLINK_BIND_P2M           BYTE_P2M(1)
#define MLINK_SEND_SEQ_P2M       BYTE_P2M(2)
#define MLINK_CH_IDX_P2M         BYTE_P2M(3)
#define MLINK_BIND_IDX_P2M       BYTE_P2M(4)

//#define MLINK_FORCE_ID
#define MLINK_BIND_COUNT	696	// around 20s
#define MLINK_NUM_FREQ		78
#define MLINK_BIND_CHANNEL	0x01
#define MLINK_PACKET_SIZE	8

#define MLINK_NUM_WAIT_LOOPS (200 / 5)   //each loop is ~5us.  Do not wait more than 200us TODO measure

uint8_t  crc8;
uint8_t packet_count_p2M;
uint32_t MProtocol_id;

//Channel MIN MAX values
#define CHANNEL_MAX_100	1844	//	+100%
#define CHANNEL_MIN_100	204		//	-100%

const static RfOptionSettingsvar_t RfOpt_MLINK_Ser[] PROGMEM = {
/*rfProtoNeed*/PROTO_NEED_SPI | BOOL1USED, //can be PROTO_NEED_SPI | BOOL1USED | BOOL2USED | BOOL3USED
/*rfSubTypeMax*/0,
/*rfOptionValue1Min*/0,
/*rfOptionValue1Max*/0,
/*rfOptionValue2Min*/0,
/*rfOptionValue2Max*/0,
/*rfOptionValue3Max*/7
};


static void crc8_update(uint8_t byte)
{
	crc8 = crc8 ^ byte;
	for ( uint8_t j = 0; j < 8; j++ )
		if ( crc8 & 0x80 )
			crc8 = (crc8<<1) ^ 0x31;//0x31 = Default CRC crc8_polynomial
		else
			crc8 <<= 1;
}

// Channel value -125%<->125% is scaled to 16bit value with no limit
int16_t convert_channel_16b_nolimit(uint8_t num, int16_t min, int16_t max, bool failsafe)
{
	int32_t val;
	#ifdef FAILSAFE_ENABLE
	if(failsafe)
		val=Failsafe_data[num];				// 0<->2047
	else
	#endif
		val=FULL_CHANNEL_OUTPUTS(num);				// 0<->2047
	val=(val-CHANNEL_MIN_100)*(max-min)/(CHANNEL_MAX_100-CHANNEL_MIN_100)+min;
	return (uint16_t)val;
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

enum {
	MLINK_BIND_TX=0,
	MLINK_BIND_PREP_RX,
	MLINK_BIND_RX,
	MLINK_PREP_DATA,
	MLINK_SEND1,
	MLINK_SEND2,
	MLINK_SEND3,
	MLINK_CHECK3,
	MLINK_RX,
	MLINK_BUILD4,
};

uint8_t MLINK_Data_Code[16], MLINK_CRC_Init, MLINK_Unk_6_2;

const uint8_t PROGMEM MLINK_init_vals[][2] = {
	//Init from dump
	{ CYRF_01_TX_LENGTH,	0x08 },	  // length of packet
	{ CYRF_02_TX_CTRL,		0x40 },	  // Clear TX Buffer
	{ CYRF_03_TX_CFG,		0x3C },     //0x3E in normal mode, 0x3C in bind mode: SDR 64 chip codes (=8 bytes data code used)
	{ CYRF_05_RX_CTRL,		0x00 },
	{ CYRF_06_RX_CFG,		0x93 },	    // AGC enabled, overwrite enable, valid flag enable
	{ CYRF_0B_PWR_CTRL,		0x00 },
	//{ CYRF_0C_XTAL_CTRL,	0x00 },	// Set to GPIO on reset
	//{ CYRF_0D_IO_CFG,		0x00 },	  // Set to GPIO on reset
	//{ CYRF_0E_GPIO_CTRL,	0x00 }, // Set by the CYRF_SetTxRxMode function
	{ CYRF_0F_XACT_CFG,		0x04 },	  // end state idle
	{ CYRF_10_FRAMING_CFG,	0x00 },	// SOP disabled
	{ CYRF_11_DATA32_THOLD,	0x05 }, // not used???
	{ CYRF_12_DATA64_THOLD,	0x0F }, // 64 Chip Data PN Code Correlator Threshold
	{ CYRF_14_EOP_CTRL,		0x05 },   // 5 consecutive noncorrelations symbol for EOP
	{ CYRF_15_CRC_SEED_LSB,	0x00 }, // not used???
	{ CYRF_16_CRC_SEED_MSB,	0x00 }, // not used???
	{ CYRF_1B_TX_OFFSET_LSB,0x00 },
	{ CYRF_1C_TX_OFFSET_MSB,0x00 },
	{ CYRF_1D_MODE_OVERRIDE,0x00 },
	{ CYRF_1E_RX_OVERRIDE,	0x14 },	// RX CRC16 is disabled and Force Receive Data Rate
	{ CYRF_1F_TX_OVERRIDE,	0x04 }, // TX CRC16 is disabled
	{ CYRF_26_XTAL_CFG,		0x08 },
	{ CYRF_29_RX_ABORT,		0x00 },
	{ CYRF_32_AUTO_CAL_TIME,0x3C },
	{ CYRF_35_AUTOCAL_OFFSET,0x14 },
	{ CYRF_39_ANALOG_CTRL,	0x03 },	// Receive invert and all slow
};

static void MLINK_cyrf_config()
{
	for(uint8_t i = 0; i < sizeof(MLINK_init_vals) / 2; i++)
		CYRF_WriteRegister(pgm_read_byte_near(&MLINK_init_vals[i][0]), pgm_read_byte_near(&MLINK_init_vals[i][1]));
	CYRF_WritePreamble(0x333304);
	CYRF_SetTxRxMode(TX_EN);
}

static void MLINK_send_bind_packet()
{
	uint8_t p_c=packet_count_p2M>>1;

	memset(packet_p2M, p_c<0x16?0x00:0xFF, MLINK_PACKET_SIZE-1);
	packet_p2M[0]=0x0F;	// bind
	packet_p2M[1]=p_c;
	switch(p_c)
	{
		case 0x00:
			packet_p2M[2]=0x40;			//unknown but seems constant
			packet_p2M[4]=0x01;			//unknown but seems constant
			packet_p2M[5]=0x03;			//unknown but seems constant
			packet_p2M[6]=0xE3;			//unknown but seems constant
			break;
		case 0x05:
			packet_p2M[6]=MLINK_CRC_Init;	//CRC init value
			break;
		case 0x06:
			packet_p2M[2]=MLINK_Unk_6_2;	//unknown and different
			//Start of hopping frequencies
			for(uint8_t i=0;i<4;i++)
				packet_p2M[i+3]=channel_used_p2M[i];
			break;
		case 0x15:
			packet_p2M[6]=0x51;			//unknown but seems constant
			break;
		case 0x16:
			packet_p2M[2]=0x51;			//unknown but seems constant
			packet_p2M[3]=0xEC;			//unknown but seems constant
			packet_p2M[4]=0x05;			//unknown but seems constant
			break;
		case 0x1A:
			packet_p2M[1]=0xFF;
			memset(&packet_p2M[2],0x00,5);
			break;
	}
	if(p_c>=0x01 && p_c<=0x04)
	{//DATA_CODE
		uint8_t p_c_5=(p_c-1)*5;
		for(uint8_t i=0;i<5;i++)
			if(i+p_c_5<16)
				packet_p2M[i+2]=MLINK_Data_Code[i+p_c_5];
	}
	else
		if(p_c>=0x07 && p_c<=0x15)
		{//Hopping frequencies
			uint8_t p_c_5=5*(p_c-6)-1;
			for(uint8_t i=0;i<5;i++)
				if(i+p_c_5<MLINK_NUM_FREQ)
					packet_p2M[i+2]=channel_used_p2M[i+p_c_5];
		}
		else
			if(p_c>0x19)
			{
				packet_p2M[1]=0xFF;
				memset(&packet_p2M[2], 0x00, MLINK_PACKET_SIZE-3);
			}

	//Calculate CRC
	crc8=0xFF;	// Init = 0xFF
	for(uint8_t i=0;i<MLINK_PACKET_SIZE-1;i++)
		crc8_update(bit_reverse(packet_p2M[i]));
	packet_p2M[7] = bit_reverse(crc8);			// CRC reflected out

	//Debug
	//#if 0
		//debug("P(%02d):",p_c);
		//for(uint8_t i=0;i<8;i++)
			//debug(" %02X",packet_p2M[i]);
		//debugln("");
	//#endif

	//Send packet
	CYRF_WriteDataPacketLen(packet_p2M, MLINK_PACKET_SIZE);
}

static void MLINK_send_data_packet()
{
	static uint8_t tog=0;
	uint8_t start;

#ifdef FAILSAFE_ENABLE
	static uint8_t fs=0;
	if(IS_FAILSAFE_VALUES_on && MLINK_SEND_SEQ_P2M==MLINK_SEND1)
	{
		fs=10;	// Original radio is sending 70 packets
		FAILSAFE_VALUES_off;
	}
	if(fs)
	{// Failsafe packets
		switch(MLINK_SEND_SEQ_P2M)
		{
			case MLINK_SEND2:
				packet_p2M[0]=0x06;
				start=17;
				break;
			case MLINK_SEND3:
				packet_p2M[0]=0x84;
				start=5;
				fs--;
				break;
			default: //MLINK_SEND1:
				packet_p2M[0]=0x05;
				start=11;
				break;
		}
		//Pack 6 channels per packet
		for(uint8_t i=0;i<6;i++)
		{
			uint8_t val=start<16 ? convert_channel_16b_nolimit(start,426 >> 4,3448 >> 4,true) : 0x00;
			start--;	// switch to next channel
			packet_p2M[i+1]=val;
		}
	}
	else
#endif
	{// Normal packets
		if(MLINK_CH_IDX_P2M==0)
			tog=1;
		//Channels to be sent
		if(MLINK_SEND_SEQ_P2M==MLINK_SEND1 || ((MLINK_CH_IDX_P2M%5==0) && (MLINK_SEND_SEQ_P2M==MLINK_SEND2)))
		{
			if((MLINK_CH_IDX_P2M&1)==0)
				packet_p2M[0] = 0x09;	//10,8,6
			else
				packet_p2M[0] = 0x01;	//11,9,7
		}
		else
			if(MLINK_SEND_SEQ_P2M==MLINK_SEND2)
			{
				if(tog)
					packet_p2M[0] = 0x02;	//x,15,13
				else
					packet_p2M[0] = 0x0A;	//x,14,12
				tog^=1;
			}
			else
			{//MLINK_SEND_SEQ_P2M==MLINK_SEND3
				if((MLINK_CH_IDX_P2M&1)==0)
					packet_p2M[0] = 0x88;	//4,2,0
				else
					packet_p2M[0] = 0x80;	//5,3,1
			}

		//Start channel
		start=4+6*(packet_p2M[0]&3);
		if((packet_p2M[0]&0x08)==0)
			start++;

		//Channels 426..1937..3448
		for(uint8_t i=0;i<3;i++)
		{
			uint16_t val=start<16 ? convert_channel_16b_nolimit(start,426,3448,false) : 0x0000;
			start-=2;	// switch to next channel
			packet_p2M[i*2+1]=val>>8;
			packet_p2M[i*2+2]=val;
		}
	}

	//Calculate CRC
	crc8=bit_reverse(MLINK_CH_IDX_P2M + MLINK_CRC_Init);	// Init = relected freq index + offset
	for(uint8_t i=0;i<MLINK_PACKET_SIZE-1;i++)
		crc8_update(bit_reverse(packet_p2M[i]));
	packet_p2M[7] = bit_reverse(crc8);			// CRC reflected out

	//Send
	CYRF_WriteDataPacketLen(packet_p2M, MLINK_PACKET_SIZE);

	//Debug
	//#if 0
		//debug("P(%02d):",MLINK_CH_IDX_P2M);
		//for(uint8_t i=0;i<8;i++)
			//debug(" %02X",packet_p2M[i]);
		//debugln("");
	//#endif
}

#ifdef MLINK_HUB_TELEMETRY
	static void MLINK_Send_Telemetry()
	{ // not sure how MLINK telemetry works, the 2 RXs I have are sending something completly different...
		telemetry_counter += 2;				// TX LQI counter
		telemetry_link = 1;

		if(telem_save_data_buff[0]==0x13)
		{ // RX-9-DR : 13 1A C8 00 01 64 00
			uint8_t id;
			for(uint8_t i=1; i<5; i+=3)
			{//2 sensors per packet
				id=0x00;
				switch(telem_save_data_buff[i]&0x0F)
				{
					case 1: //voltage
						if((telem_save_data_buff[i]&0xF0) == 0x00)
							//v_lipo1 = telem_save_data_buff[i+1];		// Rx_Batt*20
							telemetryData.analog[TELEM_ANA_A1].set(telem_save_data_buff[i+1], g_model.telemetry.channels[TELEM_ANA_A1].type);
						else
							//v_lipo2 = telem_save_data_buff[i+1];
							telemetryData.analog[TELEM_ANA_A2].set(telem_save_data_buff[i+1], g_model.telemetry.channels[TELEM_ANA_A2].type);
						break;
					case 2: //current
						id = 0x28;
						break;
					case 3: //vario
						id = 0x30;
						break;
					case 5: //rpm
						id = 0x03;
						break;
					case 6: //temp
						id = 0x02;
						break;
					case 10: //lqi
						RX_RSSI=RX_LQI=telem_save_data_buff[i+1]>>1;
						break;
				}
				#if defined HUB_TELEMETRY
					if(id)
					{
						uint16_t val=((telem_save_data_buff[i+2]&0x80)<<8)|((telem_save_data_buff[i+2]&0x7F)<<7)|(telem_save_data_buff[i+1]>>1);	//remove the alarm LSB bit, move the sign bit to MSB
						frsky_send_user_frame(id, val, val>>8);
					}
				#endif
			}
		}
		else
			if(telem_save_data_buff[0]==0x03)
			{ // RX-5 :    03 15 23 00 00 01 02
				//Incoming packet values
				RX_RSSI = telem_save_data_buff[2]<<1;	// Looks to be the RX RSSI value
				RX_LQI  = telem_save_data_buff[5];		// Looks to be connection lost
			}
			else
				RX_RSSI = TX_LQI;

		// Read TX RSSI
		TX_RSSI = CYRF_ReadRegister(CYRF_13_RSSI)&0x1F;

		if(telemetry_lost)
		{
			telemetry_lost = 0;
			packet_count_p2M = 50;
			telemetry_counter = 100;
		}
	}
#endif

#ifdef MLINK_FW_TELEMETRY
	static void MLINK_Send_Telemetry()
	{
		telemetry_counter += 2;				// TX LQI counter
		telemetry_link = 4;

		// Read TX RSSI
		TX_RSSI = CYRF_ReadRegister(CYRF_13_RSSI)&0x1F;

		if(telemetry_lost)
		{
			telemetry_lost = 0;
			packet_count_p2M = 50;
			telemetry_counter = 100;
		}
	}
#endif

uint16_t MLINK_callback()
{
	uint8_t MLINK_Status, len;
	uint16_t start;

	switch(MLINK_SEND_SEQ_P2M)
	{
		case MLINK_BIND_RX:
			////debugln("RX");
			MLINK_Status=CYRF_ReadRegister(CYRF_05_RX_CTRL);
			if( (MLINK_Status&0x80) == 0 )
			{//Packet received
				len=CYRF_ReadRegister(CYRF_09_RX_COUNT);
				//debugln("L=%02X",len)
				if( len==8 )
				{
					CYRF_ReadDataPacketLen(packet_p2M, len*2);
					//debug("RX=");
					//for(uint8_t i=0;i<8;i++)
						//debug(" %02X",packet_p2M[i*2]);
					//debugln("");
					//Check CRC
					crc8=0xFF;	// Init = 0xFF
					for(uint8_t i=0;i<MLINK_PACKET_SIZE-1;i++)
						crc8_update(bit_reverse(packet_p2M[i<<1]));
					if(packet_p2M[14] == bit_reverse(crc8))
					{// CRC is ok
						//debugln("CRC ok");
						if(packet_p2M[0]==0x7F)
							packet_count_p2M=3;					// Start sending bind payload
						else if(packet_count_p2M > 0x19*2)
						{
							if(packet_p2M[0] == 0x8F)
								packet_count_p2M++;
							else if(packet_p2M[0] == 0x9F)
								packet_count_p2M=0x80;			// End bind
							else
								packet_count_p2M=0;				// Restart bind...
						}
					}
				}
			}
			else
				packet_count_p2M=0;
			CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);		// Enable RX abort
			CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24);		// Force end state
			CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);		// Disable RX abort
			MLINK_SEND_SEQ_P2M=MLINK_BIND_TX;							// Retry sending bind packet
			CYRF_SetTxRxMode(TX_EN);						// Transmit mode
			if(packet_count_p2M)
				return 18136*2;
		case MLINK_BIND_TX:
			if(--MLINK_BIND_IDX_P2M==0 || packet_count_p2M>=0x1B*2)
			{ // Switch to normal mode
				MLINK_BIND_P2M = 0;//BIND_DONE;
				MLINK_SEND_SEQ_P2M=MLINK_PREP_DATA;
				return 22720*2;
			}
			MLINK_send_bind_packet();
			if(packet_count_p2M == 0 || packet_count_p2M > 0x19*2)
			{
				MLINK_SEND_SEQ_P2M++;		// MLINK_BIND_PREP_RX
				return 4700*2;	// Original is 4900
			}
			packet_count_p2M++;
			if(packet_count_p2M&1)
				return 6000*2;
			return 22720*2;
		case MLINK_BIND_PREP_RX:
		  {
			uint8_t i = MLINK_NUM_WAIT_LOOPS;
			while(i--)// Wait max 200µs for TX
			{
			  if((CYRF_ReadRegister(CYRF_02_TX_CTRL) & 0x80) == 0x00)
			  {
				break;// Packet transmission complete
			  }
			}
		  }
			CYRF_SetTxRxMode(RX_EN);							// Receive mode
			CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x82);			// Prepare to receive
			MLINK_SEND_SEQ_P2M++;	//MLINK_BIND_RX
			if(packet_count_p2M > 0x19*2)
				return 28712*2;									// Give more time to the RX to confirm that the bind is ok...
			return (28712-4700)*2;
		case MLINK_PREP_DATA:
			CYRF_ConfigDataCode(MLINK_Data_Code,16);
			MLINK_CRC_Init += 0xED;
			MLINK_CH_IDX_P2M = 0x00;
			CYRF_ConfigRFChannel(channel_used_p2M[MLINK_CH_IDX_P2M]);
			CYRF_WriteRegister(CYRF_03_TX_CFG,0x38);//CYRF_SetPower(0x38);
			#if defined(MLINK_HUB_TELEMETRY) || defined(MLINK_FW_TELEMETRY)
				packet_count_p2M = 0;
				telemetry_lost = 1;
			#endif
			MLINK_SEND_SEQ_P2M++;
		case MLINK_SEND1:
			MLINK_send_data_packet();
			MLINK_SEND_SEQ_P2M++;
			return (4880+1111)*2;
		case MLINK_SEND2:
		  SCHEDULE_MIXER_END_IN_US((4617+1422)*2);
			MLINK_send_data_packet();
			MLINK_SEND_SEQ_P2M++;
			if(MLINK_CH_IDX_P2M%5==0)
				return (4617+1017)*2;
			return (4617+1422)*2;
		case MLINK_SEND3:
			MLINK_send_data_packet();
			MLINK_SEND_SEQ_P2M++;
			return 4611*2;
		case MLINK_CHECK3:
			//Switch to next channel
			MLINK_CH_IDX_P2M++;
			if(MLINK_CH_IDX_P2M>=MLINK_NUM_FREQ)
				MLINK_CH_IDX_P2M=0;
			CYRF_ConfigRFChannel(channel_used_p2M[MLINK_CH_IDX_P2M]);

			//Receive telemetry
			if(MLINK_CH_IDX_P2M%5==0)
			{//Receive telemetry
				CYRF_SetTxRxMode(RX_EN);							// Receive mode
				CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x82);			// Prepare to receive
				MLINK_SEND_SEQ_P2M++;	//MLINK_RX
				return (8038+2434+410-1000)*2;
			}
			else
				CYRF_WriteRegister(CYRF_03_TX_CFG,0x38);//CYRF_SetPower(0x38);
			MLINK_SEND_SEQ_P2M=MLINK_SEND1;
			return 4470*2;
		case MLINK_RX:
			#if defined(MLINK_HUB_TELEMETRY) || defined(MLINK_FW_TELEMETRY)
				//TX LQI calculation
				packet_count_p2M++;
				if(packet_count_p2M>=50)
				{
					packet_count_p2M=0;
					TX_LQI=telemetry_counter;
					if(telemetry_counter==0)
						telemetry_lost = 1;
					telemetry_counter = 0;
				}
			#endif
			MLINK_Status=CYRF_ReadRegister(CYRF_05_RX_CTRL);
			//debug("T(%02X):",MLINK_Status);
			if( (MLINK_Status&0x80) == 0 )
			{//Packet received
				len=CYRF_ReadRegister(CYRF_09_RX_COUNT);
				//debug("(%X)",len)
				if( len && len <= MLINK_PACKET_SIZE )
				{
					CYRF_ReadDataPacketLen(telem_save_data_buff, len*2);
					#if defined(MLINK_HUB_TELEMETRY) || defined(MLINK_FW_TELEMETRY)
						if(len==MLINK_PACKET_SIZE)
						{
							for(uint8_t i=0;i<8;i++)
							//Check CRC
							crc8=bit_reverse(MLINK_CRC_Init);
							for(uint8_t i=0;i<MLINK_PACKET_SIZE-1;i++)
							{
								telem_save_data_buff[i]=telem_save_data_buff[i<<1];
								crc8_update(bit_reverse(telem_save_data_buff[i]));
								//debug(" %02X",telem_save_data_buff[i]);
							}
							if(telem_save_data_buff[14] == bit_reverse(crc8))	// Packet CRC is ok
								MLINK_Send_Telemetry();
							//else
								//debug(" NOK");
						}
					#endif
				}
			}
			//debugln("");
			CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);		// Enable RX abort
			CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24);		// Force end state
			CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);		// Disable RX abort
			CYRF_SetTxRxMode(TX_EN);						// Transmit mode
			MLINK_SEND_SEQ_P2M=MLINK_SEND2;
			heartbeat |= HEART_TIMER_PULSES;
      CALCULATE_LAT_JIT(); // Calculate latency and jitter.
			return 1000*2;
	}
	return 1000;
}

static void MLINK_shuffle_freqs(uint32_t seed, uint8_t *hop)
{
	srandom(seed);//randomSeed(seed);

	for(uint8_t i=0; i < MLINK_NUM_FREQ/2; i++)
	{
	  srandom(0xfefefefe);
		uint8_t r   = random() % (MLINK_NUM_FREQ/2);
		uint8_t tmp = hop[r];
		hop[r] = hop[i];
		hop[i] = tmp;
	}
}

void MLINK_init()
{
	MLINK_cyrf_config();

	//Init ID and RF freqs
	for(uint8_t i=0; i < MLINK_NUM_FREQ/2; i++)
	{
		channel_used_p2M[i                 ] = (i<<1) + 3;
		channel_used_p2M[i+MLINK_NUM_FREQ/2] = (i<<1) + 3;
	}

  // Load temp_rfid_addr_p2M + Model match. (DEVO)
  MProtocol_id = temp_rfid_addr_p2M[0];
  MProtocol_id ^= (RXNUM*3);


	// part1
	memcpy(MLINK_Data_Code  ,temp_rfid_addr_p2M,4);
	MLINK_shuffle_freqs(MProtocol_id, channel_used_p2M);

		// part2
	MProtocol_id ^= 0x6FBE3201;
	set_temp_rfid_addr_p2M(MProtocol_id);
	memcpy(MLINK_Data_Code+4,temp_rfid_addr_p2M,4);
	MLINK_shuffle_freqs(MProtocol_id, &channel_used_p2M[MLINK_NUM_FREQ/2]);

	// part3
	MLINK_CRC_Init = temp_rfid_addr_p2M[3];		//value sent during bind then used to init the CRC
	MLINK_Unk_6_2  = 0x3A;		//unknown value sent during bind but doesn't seem to matter

	#ifdef MLINK_FORCE_ID
		if(RXNUM)
		{
			//Cockpit SX
			memcpy(MLINK_Data_Code,"\x4C\x97\x9D\xBF\xB8\x3D\xB5\xBE",8);
			memcpy(channel_used_p2M,"\x0D\x41\x09\x43\x17\x2D\x05\x31\x13\x3B\x1B\x3D\x0B\x41\x11\x45\x09\x2B\x17\x4D\x19\x3F\x03\x3F\x0F\x37\x1F\x47\x1B\x49\x07\x35\x27\x2F\x15\x33\x23\x39\x1F\x33\x19\x45\x0D\x2D\x11\x35\x0B\x47\x25\x3D\x21\x37\x1D\x3B\x05\x2F\x21\x39\x23\x4B\x03\x31\x25\x29\x07\x4F\x1D\x4B\x15\x4D\x13\x4F\x0F\x49\x29\x2B\x27\x43",MLINK_NUM_FREQ);
			MLINK_Unk_6_2  = 0x3A;		//unknown value sent during bind but doesn't seem to matter
			MLINK_CRC_Init = 0x07;		//value sent during bind then used to init the CRC
		}
		else
		{
			//HFM3
			memcpy(MLINK_Data_Code,"\xC0\x90\x8F\xBB\x7C\x8E\x2B\x8E",8);
			memcpy(channel_used_p2M,"\x05\x41\x27\x4B\x17\x33\x11\x39\x0F\x3F\x05\x2F\x13\x2D\x25\x31\x1F\x2D\x25\x35\x03\x41\x1B\x43\x09\x3D\x1F\x29\x1D\x35\x0D\x3B\x19\x49\x23\x3B\x17\x47\x1D\x2B\x13\x37\x0B\x31\x23\x33\x29\x3F\x07\x37\x07\x43\x11\x2B\x1B\x39\x0B\x4B\x03\x4F\x21\x47\x0F\x4D\x15\x45\x21\x4F\x09\x3D\x19\x2F\x15\x45\x0D\x49\x27\x4D",MLINK_NUM_FREQ);
			MLINK_Unk_6_2  = 0x02;		//unknown value but doesn't seem to matter
			MLINK_CRC_Init = 0x3E;		//value sent during bind then used to init the CRC
		}
		//Other TX
		//MLINK_Unk_6_2  = 0x7e;		//unknown value but doesn't seem to matter
		//MLINK_CRC_Init = 0xA2;		//value sent during bind then used to init the CRC
	#endif

	for(uint8_t i=0;i<8;i++)
		MLINK_Data_Code[i+8]=MLINK_Data_Code[7-i];

	//debug("ID:")
	//for(uint8_t i=0;i<16;i++)
		//debug(" %02X", MLINK_Data_Code[i]);
	//debugln("");

	//debugln("CRC init: %02X", MLINK_CRC_Init)

	//debug("RF:")
	//for(uint8_t i=0;i<MLINK_NUM_FREQ;i++)
		//debug(" %02X", channel_used_p2M[i]);
	//debugln("");

	if(MLINK_BIND_P2M)//if(IS_BIND_IN_PROGRESS)
	{
		packet_count_p2M = 0;
		MLINK_BIND_IDX_P2M = MLINK_BIND_COUNT;
		CYRF_ConfigDataCode((uint8_t*)"\x6F\xBE\x32\x01\xDB\xF1\x2B\x01\xE3\x5C\xFA\x02\x97\x93\xF9\x02",16); //Bind data code
		CYRF_ConfigRFChannel(MLINK_BIND_CHANNEL);
		MLINK_SEND_SEQ_P2M = MLINK_BIND_TX;
	}
	else
		MLINK_SEND_SEQ_P2M = MLINK_PREP_DATA;
}

static uint16_t MLINK_cb()
{
 uint16_t time = MLINK_callback();
 heartbeat |= HEART_TIMER_PULSES;
 CALCULATE_LAT_JIT(); // Calculate latency and jitter.
 return time;
}

static void MLINK_initialize(uint8_t bind)
{
  CYRF_Reset();
  MLINK_BIND_P2M = bind;
  MLINK_init();
  PROTO_Start_Callback( MLINK_cb);
}

const void *MLINK_Cmds(enum ProtoCmds cmd)
{
  switch(cmd)
    {
    case PROTOCMD_INIT:
      MLINK_initialize(0);
      return 0;
    case PROTOCMD_RESET:
      PROTO_Stop_Callback();
      CYRF_Reset();
      CYRF_SetTxRxMode(TXRX_OFF);
      return 0;
    case PROTOCMD_BIND:
      MLINK_initialize(1);
      return 0;
    case PROTOCMD_GETOPTIONS:
      SetRfOptionSettings(pgm_get_far_address(RfOpt_MLINK_Ser),
                          STR_DUMMY,//Sub proto
                          STR_DUMMY,         //Num channels STR_CH
                          STR_DUMMY,         //Option 2 (int)
                          STR_RFPOWER,       //Rf power
                          STR_TELEMETRY,      //OptionBool 1
                          STR_DUMMY,         //OptionBool 2
                          STR_DUMMY          //OptionBool 3
                         );
      return 0;
    default:
      break;
    }
  return 0;
}
