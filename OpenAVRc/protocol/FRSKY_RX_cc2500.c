/*
**************************************************************************
*                                                                        *
*                 ____                ___ _   _____                      *
*                / __ \___  ___ ___  / _ | | / / _ \____                 *
*               / /_/ / _ \/ -_) _ \/ __ | |/ /, _/ __/                 *
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
*   (at your g_model.rfOptionValue1) any later version.                                  *
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

// Multiprotocol inspired. Thanks a lot !

#include "../OpenAVRc.h"

const static RfOptionSettingsvar_t RfOpt_Frsky_RX_Ser[] PROGMEM =
{
 /*rfProtoNeed*/PROTO_NEED_SPI | BOOL1USED | BOOL3USED, //can be PROTO_NEED_SPI | BOOL1USED | BOOL2USED | BOOL3USED
 /*rfSubTypeMax*/4,
 /*rfOptionValue1Min*/-128, // FREQFINE MIN
 /*rfOptionValue1Max*/127,  // FREQFINE MAX
 /*rfOptionValue2Min*/0,
 /*rfOptionValue2Max*/0,
 /*rfOptionValue3Max*/7,    // RF POWER
};

//**CRC**
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
//**CRC**

uint16_t rx_rc_chan[16];
uint8_t FrSkyFormat;
bool rx_data_started;
uint8_t telemetry_link=0;
void FRSKY_RX_initialize(uint8_t bind);
uint16_t State_Frsky_RX=0;
#define FRSKYRX_BIND rfState8_p2M
#define FRSKY_RX_EEPROM_OFFSET	178		// (1) format + (3) TX ID + (1) freq_tune + (47) channels, 52 bytes, end is 178+52=230
#define FRSKYD_CLONE_EEPROM_OFFSET	771	// (1) format + (3) TX ID + (47) channels, 51 bytes, end is 822
#define FRSKYX_CLONE_EEPROM_OFFSET	822	// (1) format + (3) TX ID + (47) channels, 51 bytes, end is 873
#define FRSKYX2_CLONE_EEPROM_OFFSET	873	// (1) format + (3) TX ID, 4 bytes, end is 877
#define min(a,b) ((a)<(b)?(a):(b))

//#define XTELEMETRY     (g_model.rfOptionBool1)
//#define V2MODE         (g_model.rfOptionBool2)
#define rx_disable_lna (g_model.rfOptionBool3)
const pm_char STR_RX_DIS_LNA[] PROGMEM = INDENT "LNA Off";

#define FRSKY_RX_D16FCC_LENGTH	0x1D+1
#define FRSKY_RX_D16LBT_LENGTH	0x20+1
#define FRSKY_RX_D16v2_LENGTH	0x1D+1
#define FRSKY_RX_D8_LENGTH		0x11+1
#define FRSKY_RX_FORMATS		5

static int8_t  FRSKY_RX_finetune;

#define FRSKY_RX_format g_model.rfSubType // g_model.rfSubType is used to distinguish the kind of RX Proto
const pm_char STR_SUBTYPE_FRSKY_RX[] PROGMEM = "D8  ""D16 "" LBT""FCC2""LBT2";
enum
{
	FRSKY_RX_D8			=0,
	FRSKY_RX_D16FCC		=1,
	FRSKY_RX_D16LBT		=2,
	FRSKY_RX_D16v2FCC	=3,
	FRSKY_RX_D16v2LBT	=4,
};

enum FRSKY_RX
{
	FRSKY_RX	= 0,
	FRSKY_CLONE	= 1,
	FRSKY_ERASE	= 2,
	FRSKY_CPPM  = 3,
};

enum {
	FRSKY_RX_TUNE_START,
	FRSKY_RX_TUNE_LOW,
	FRSKY_RX_TUNE_HIGH,
	FRSKY_RX_BIND,
	FRSKY_RX_DATA,
};

const PROGMEM uint8_t FRSKY_RX_common_reg[][2] = {
	{CC2500_02_IOCFG0, 0x01},
	{CC2500_18_MCSM0, 0x18},
	{CC2500_07_PKTCTRL1, 0x05},
	{CC2500_3E_PATABLE, 0xFF},
	{CC2500_0C_FSCTRL0, 0},
	{CC2500_0D_FREQ2, 0x5C},
	{CC2500_13_MDMCFG1, 0x23},
	{CC2500_14_MDMCFG0, 0x7A},
	{CC2500_19_FOCCFG, 0x16},
	{CC2500_1A_BSCFG, 0x6C},
	{CC2500_1B_AGCCTRL2, 0x03},
	{CC2500_1C_AGCCTRL1, 0x40},
	{CC2500_1D_AGCCTRL0, 0x91},
	{CC2500_21_FREND1, 0x56},
	{CC2500_22_FREND0, 0x10},
	{CC2500_23_FSCAL3, 0xA9},
	{CC2500_24_FSCAL2, 0x0A},
	{CC2500_25_FSCAL1, 0x00},
	{CC2500_26_FSCAL0, 0x11},
	{CC2500_29_FSTEST, 0x59},
	{CC2500_2C_TEST2, 0x88},
	{CC2500_2D_TEST1, 0x31},
	{CC2500_2E_TEST0, 0x0B},
	{CC2500_03_FIFOTHR, 0x07},
	{CC2500_09_ADDR, 0x03},
};

const PROGMEM uint8_t FRSKY_RX_d16fcc_reg[][2] = {
	{CC2500_17_MCSM1, 0x0C},
	{CC2500_0E_FREQ1, 0x76},
	{CC2500_0F_FREQ0, 0x27},
	{CC2500_06_PKTLEN, 0x1E},
	{CC2500_08_PKTCTRL0, 0x01},
	{CC2500_0B_FSCTRL1, 0x0A},
	{CC2500_10_MDMCFG4, 0x7B},
	{CC2500_11_MDMCFG3, 0x61},
	{CC2500_12_MDMCFG2, 0x13},
	{CC2500_15_DEVIATN, 0x51},
};

const PROGMEM uint8_t FRSKY_RX_d16lbt_reg[][2] = {
	{CC2500_17_MCSM1, 0x0E},
	{CC2500_0E_FREQ1, 0x80},
	{CC2500_0F_FREQ0, 0x00},
	{CC2500_06_PKTLEN, 0x23},
	{CC2500_08_PKTCTRL0, 0x01},
	{CC2500_0B_FSCTRL1, 0x08},
	{CC2500_10_MDMCFG4, 0x7B},
	{CC2500_11_MDMCFG3, 0xF8},
	{CC2500_12_MDMCFG2, 0x03},
	{CC2500_15_DEVIATN, 0x53},
};

const PROGMEM uint8_t FRSKY_RX_d8_reg[][2] = {
	{CC2500_17_MCSM1,    0x0C},
	{CC2500_0E_FREQ1,    0x76},
	{CC2500_0F_FREQ0,    0x27},
	{CC2500_06_PKTLEN,   0x19},
	{CC2500_08_PKTCTRL0, 0x05},
	{CC2500_0B_FSCTRL1,  0x08},
	{CC2500_10_MDMCFG4,  0xAA},
	{CC2500_11_MDMCFG3,  0x39},
	{CC2500_12_MDMCFG2,  0x11},
	{CC2500_15_DEVIATN,  0x42},
};

static void FRSKY_RX_strobe_rx()
{
	 CC2500_Strobe(CC2500_SIDLE);
	 CC2500_Strobe(CC2500_SFRX);
	 CC2500_Strobe(CC2500_SRX);
}

static void FRSKY_RX_initialise_cc2500() {
	const uint8_t FRSKY_RX_length[] = { FRSKY_RX_D8_LENGTH, FRSKY_RX_D16FCC_LENGTH, FRSKY_RX_D16LBT_LENGTH, FRSKY_RX_D16v2_LENGTH, FRSKY_RX_D16v2_LENGTH };
	packetSize_p2M = FRSKY_RX_length[FRSKY_RX_format];
	CC2500_Reset();
	CC2500_Strobe(CC2500_SIDLE);
	for (uint8_t i = 0; i < sizeof(FRSKY_RX_common_reg) / 2; i++)
		CC2500_WriteReg(pgm_read_byte_near(&FRSKY_RX_common_reg[i][0]), pgm_read_byte_near(&FRSKY_RX_common_reg[i][1]));

	switch (FRSKY_RX_format)
	{
		case FRSKY_RX_D16v2FCC:
		case FRSKY_RX_D16FCC:
			for (uint8_t i = 0; i < sizeof(FRSKY_RX_d16fcc_reg) / 2; i++)
				CC2500_WriteReg(pgm_read_byte_near(&FRSKY_RX_d16fcc_reg[i][0]), pgm_read_byte_near(&FRSKY_RX_d16fcc_reg[i][1]));
			if(FRSKY_RX_format==FRSKY_RX_D16v2FCC)
			{
				CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x05);	// Enable CRC
				CC2500_WriteReg(CC2500_17_MCSM1, 0x0E);		// Go/Stay in RX mode
				CC2500_WriteReg(CC2500_11_MDMCFG3, 0x84);	// bitrate 70K->77K
			}
			break;
		case FRSKY_RX_D16v2LBT:
		case FRSKY_RX_D16LBT:
			for (uint8_t i = 0; i < sizeof(FRSKY_RX_d16lbt_reg) / 2; i++)
				CC2500_WriteReg(pgm_read_byte_near(&FRSKY_RX_d16lbt_reg[i][0]), pgm_read_byte_near(&FRSKY_RX_d16lbt_reg[i][1]));
			if(FRSKY_RX_format==FRSKY_RX_D16v2LBT)
				CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x05);	// Enable CRC
			break;
		case FRSKY_RX_D8:
			for (uint8_t i = 0; i < sizeof(FRSKY_RX_d8_reg) / 2; i++)
				CC2500_WriteReg(pgm_read_byte_near(&FRSKY_RX_d8_reg[i][0]), pgm_read_byte_near(&FRSKY_RX_d8_reg[i][1]));
			CC2500_WriteReg(CC2500_23_FSCAL3, 0x89);
			break;
	}
	CC2500_WriteReg(CC2500_0A_CHANNR, 0);  // bind channel
	//rx_disable_lna = IS_POWER_FLAG_on;
	CC2500_SetTxRxMode(rx_disable_lna ? TXRX_OFF : RX_EN); // lna disable / enable
	FRSKY_RX_strobe_rx();
	_delay_us(1000);// wait for RX to activate
}

static void FRSKY_RX_set_channel(uint8_t channel)
{
	CC2500_WriteReg(CC2500_0A_CHANNR, channel_used_p2M[channel]);
	if(FRSKY_RX_format == FRSKY_RX_D8)
		CC2500_WriteReg(CC2500_23_FSCAL3, 0x89);
	CC2500_WriteReg(CC2500_25_FSCAL1, calData[channel]);
	FRSKY_RX_strobe_rx();
}

static void FRSKY_RX_calibrate()
{
	FRSKY_RX_strobe_rx();
	for (unsigned c = 0; c < 47; c++)
	{
		CC2500_Strobe(CC2500_SIDLE);
		CC2500_WriteReg(CC2500_0A_CHANNR, channel_used_p2M[c]);
		CC2500_Strobe(CC2500_SCAL);
		_delay_us(900);
		calData[c] = CC2500_ReadReg(CC2500_25_FSCAL1);
	}
}

static uint8_t frskyx_rx_check_crc_id(bool bind,bool init)
{
	/*debugln("RX");
	for(uint8_t i=0; i<packetSize_p2M;i++)
		//debug(" %02X",packet_p2M[i]);
	debugln("");*/

	if(bind && packet_p2M[0]!=packetSize_p2M-1 && packet_p2M[1] !=0x03 && packet_p2M[2] != 0x01)
		return false;
	uint8_t offset=bind?3:1;

	// Check D8 checksum
	if (FRSKY_RX_format == FRSKY_RX_D8)
	{
		if((packet_p2M[packetSize_p2M+1] & 0x80) != 0x80)	// Check CRC_OK flag in status byte 2
			return false; 								// Bad CRC
		if(init)
		{//Save TXID
			temp_rfid_addr_p2M[3] = packet_p2M[3];
			temp_rfid_addr_p2M[2] = packet_p2M[4];
			temp_rfid_addr_p2M[1] = packet_p2M[17];
		}
		else
			if(temp_rfid_addr_p2M[3] != packet_p2M[offset] || temp_rfid_addr_p2M[2] != packet_p2M[offset+1] || temp_rfid_addr_p2M[1] != packet_p2M[bind?17:5])
				return false;							// Bad address
		return true;									// Full match
	}

	// Check D16v2 checksum
	if (FRSKY_RX_format == FRSKY_RX_D16v2LBT || FRSKY_RX_format == FRSKY_RX_D16v2FCC)
		if((packet_p2M[packetSize_p2M+1] & 0x80) != 0x80)	// Check CRC_OK flag in status byte 2
			return false;
	//debugln("HW Checksum ok");

	// Check D16 checksum
	uint16_t lcrc = FrSkyX_crc(&packet_p2M[3], packetSize_p2M - 5);		// Compute crc
	uint16_t rcrc = (packet_p2M[packetSize_p2M-2] << 8) | (packet_p2M[packetSize_p2M-1] & 0xff);	// Received crc
	if(lcrc != rcrc)
		return false; 									// Bad CRC
	//debugln("Checksum ok");

	if (bind && (FRSKY_RX_format == FRSKY_RX_D16v2LBT || FRSKY_RX_format == FRSKY_RX_D16v2FCC))
		for(uint8_t i=3; i<packetSize_p2M-2; i++)		//unXOR bind packet
			packet_p2M[i] ^= 0xA7;

	uint8_t offset2=0;
	if (bind && (FRSKY_RX_format == FRSKY_RX_D16LBT || FRSKY_RX_format == FRSKY_RX_D16FCC))
		offset2=6;
	if(init)
	{//Save TXID
		temp_rfid_addr_p2M[3] = packet_p2M[3];
		temp_rfid_addr_p2M[2] = packet_p2M[4];
		temp_rfid_addr_p2M[1] = packet_p2M[5+offset2];
		temp_rfid_addr_p2M[0] = packet_p2M[6+offset2];				// RXnum
	}
	else
		if(temp_rfid_addr_p2M[3] != packet_p2M[offset] || temp_rfid_addr_p2M[2] != packet_p2M[offset+1] || temp_rfid_addr_p2M[1] != packet_p2M[offset+2+offset2])
			return false;								// Bad address
	//debugln("Address ok");

	if(!bind && temp_rfid_addr_p2M[0] != packet_p2M[6])
		return false;									// Bad RX num

	//debugln("Match");
	return true;										// Full match
}

static void FRSKY_RX_build_telemetry_packet()
{
	uint16_t raw_channel[8];
	uint32_t bits = 0;
	uint8_t bitsavailable = 0;
	uint8_t idx = 0;

	uint8_t i;

	if (FRSKY_RX_format == FRSKY_RX_D8)
	{// decode D8 channels
		raw_channel[0] = ((packet_p2M[10] & 0x0F) << 8 | packet_p2M[6]);
		raw_channel[1] = ((packet_p2M[10] & 0xF0) << 4 | packet_p2M[7]);
		raw_channel[2] = ((packet_p2M[11] & 0x0F) << 8 | packet_p2M[8]);
		raw_channel[3] = ((packet_p2M[11] & 0xF0) << 4 | packet_p2M[9]);
		raw_channel[4] = ((packet_p2M[16] & 0x0F) << 8 | packet_p2M[12]);
		raw_channel[5] = ((packet_p2M[16] & 0xF0) << 4 | packet_p2M[13]);
		raw_channel[6] = ((packet_p2M[17] & 0x0F) << 8 | packet_p2M[14]);
		raw_channel[7] = ((packet_p2M[17] & 0xF0) << 4 | packet_p2M[15]);
		for (i = 0; i < 8; i++) {
			if (raw_channel[i] < 1290)
				raw_channel[i] = 1290;
			rx_rc_chan[i] = min(((raw_channel[i] - 1290) << 4) / 15, 2047);
		}
	}
	else
	{// decode D16 channels
		raw_channel[0] = ((packet_p2M[10] << 8) & 0xF00) | packet_p2M[9];
		raw_channel[1] = ((packet_p2M[11] << 4) & 0xFF0) | (packet_p2M[10] >> 4);
		raw_channel[2] = ((packet_p2M[13] << 8) & 0xF00) | packet_p2M[12];
		raw_channel[3] = ((packet_p2M[14] << 4) & 0xFF0) | (packet_p2M[13] >> 4);
		raw_channel[4] = ((packet_p2M[16] << 8) & 0xF00) | packet_p2M[15];
		raw_channel[5] = ((packet_p2M[17] << 4) & 0xFF0) | (packet_p2M[16] >> 4);
		raw_channel[6] = ((packet_p2M[19] << 8) & 0xF00) | packet_p2M[18];
		raw_channel[7] = ((packet_p2M[20] << 4) & 0xFF0) | (packet_p2M[19] >> 4);
		for (i = 0; i < 8; i++) {
			// ignore failsafe channels
			if(packet_p2M[7] != 0x10+(i<<1)) {
				uint8_t shifted = (raw_channel[i] & 0x800)>0;
				uint16_t channel_value = raw_channel[i] & 0x7FF;
				if (channel_value < 64)
					rx_rc_chan[shifted ? i + 8 : i] = 0;
				else
					rx_rc_chan[shifted ? i + 8 : i] = min(((channel_value - 64) << 4) / 15, 2047);
			}
		}
	}


	// buid telemetry packet
	telem_save_data_p2M[idx++] = 0;//RX_LQI; (à modifier , PM)
	telem_save_data_p2M[idx++] = 0;//RX_RSSI; (à modifier, PM)
	telem_save_data_p2M[idx++] = 0;  // start channel
	telem_save_data_p2M[idx++] = FRSKY_RX_format == FRSKY_RX_D8 ? 8 : 16; // number of channels in packet

	// pack channels
	for (i = 0; i < telem_save_data_p2M[3]; i++) {
		bits |= ((uint32_t)rx_rc_chan[i]) << bitsavailable;
		bitsavailable += 11;
		while (bitsavailable >= 8) {
			telem_save_data_p2M[idx++] = bits & 0xff;
			bits >>= 8;
			bitsavailable -= 8;
		}
	}
}

static void FRSKY_RX_data()
{
	//uint16_t temp = FRSKY_RX_EEPROM_OFFSET;
	//FRSKY_RX_format = eeprom_read_byte((EE_ADDR)temp++) % FRSKY_RX_FORMATS;
	//temp_rfid_addr_p2M[3] = eeprom_read_byte((EE_ADDR)temp++);
	//temp_rfid_addr_p2M[2] = eeprom_read_byte((EE_ADDR)temp++);
	//temp_rfid_addr_p2M[1] = eeprom_read_byte((EE_ADDR)temp++);
	//temp_rfid_addr_p2M[0] = RXNUM;
	//FRSKY_RX_finetune = eeprom_read_byte((EE_ADDR)temp++);
	//debug("format=%d, ", FRSKY_RX_format);
	//debug("addr[3]=%02X, ", temp_rfid_addr_p2M[3]);
	//debug("addr[2]=%02X, ", temp_rfid_addr_p2M[2]);
	//debug("addr[1]=%02X, ", temp_rfid_addr_p2M[1]);
	//debug("rx_num=%02X, ",  temp_rfid_addr_p2M[0]);
	//debugln("tune=%d", (int8_t)FRSKY_RX_finetune);
	if(FRSKY_RX_format != FRSKY_RX_D16v2LBT && FRSKY_RX_format != FRSKY_RX_D16v2FCC)
	{//D8 & D16v1
		//for (uint8_t ch = 0; ch < 47; ch++)
		//	channel_used_p2M[ch] = eeprom_read_byte((EE_ADDR)temp++);
	}
	else
	{
		FrSkyFormat=FRSKY_RX_format == FRSKY_RX_D16v2FCC?0:2;
		FRSKY_generate_channels();//FrSkyX2_init_hop();
	}
	//debug("ch:");
	//for (uint8_t ch = 0; ch < 47; ch++)
		//debug(" %02X", channel_used_p2M[ch]);
	//debugln("");

	FRSKY_RX_initialise_cc2500();
	FRSKY_RX_calibrate();
	CC2500_WriteReg(CC2500_18_MCSM0, 0x08); // FS_AUTOCAL = manual
	CC2500_WriteReg(CC2500_09_ADDR, temp_rfid_addr_p2M[3]); // set address
	CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x05); // check address
	if (g_model.rfOptionValue1 == 0)//freq_fine_mem_p2M != g_model.rfOptionValue1
		CC2500_WriteReg(CC2500_0C_FSCTRL0, FRSKY_RX_finetune);
	else
		CC2500_WriteReg(CC2500_0C_FSCTRL0, g_model.rfOptionValue1);
	FRSKY_RX_set_channel(channel_index_p2M);
	send_seq_p2M = FRSKY_RX_DATA;
}

uint16_t FRSKY_RX_callback()
{
	static int8_t read_retry = 0;
	static int8_t tune_low, tune_high;
	uint8_t len, ch;

	if(FRSKY_RX_format == FRSKY_ERASE)
	{
		if(packet_count_p2M)
			packet_count_p2M--;
		else
			FRSKYRX_BIND = 0;//BIND_DONE
		return 10000*2;	//  Nothing to do...
	}

  if (FRSKYRX_BIND == 0)//IS_BIND_DONE
  {
    if(send_seq_p2M != FRSKY_RX_DATA)
      FRSKY_RX_initialize(0);	// Abort bind
  }

	if ((freq_fine_mem_p2M != g_model.rfOptionValue1) && (send_seq_p2M >= FRSKY_RX_DATA))
	{
		if (g_model.rfOptionValue1 == 0)
			CC2500_WriteReg(CC2500_0C_FSCTRL0, FRSKY_RX_finetune);
		else
			CC2500_WriteReg(CC2500_0C_FSCTRL0, g_model.rfOptionValue1);
		freq_fine_mem_p2M = g_model.rfOptionValue1;
	}

	if (rx_disable_lna != g_model.rfOptionValue3)
	{
		rx_disable_lna = g_model.rfOptionValue3;
		CC2500_SetTxRxMode(rx_disable_lna ? TXRX_OFF : RX_EN);
	}

	len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;
	switch(send_seq_p2M)
	{
		case FRSKY_RX_TUNE_START:
		  SCHEDULE_MIXER_END_IN_US(18000); // Schedule next Mixer calculations.
			if (len == packetSize_p2M + 2) //+2=RSSI+LQI+CRC
			{
				CC2500_ReadData(packet_p2M, len);
				if(frskyx_rx_check_crc_id(true,true))
				{
					FRSKY_RX_finetune = -127;
					CC2500_WriteReg(CC2500_0C_FSCTRL0, FRSKY_RX_finetune);
					send_seq_p2M = FRSKY_RX_TUNE_LOW;
					//debugln("FRSKY_RX_TUNE_LOW");
					FRSKY_RX_strobe_rx();
					State_Frsky_RX = 0;
					return 1000*2;
				}
			}
			FRSKY_RX_format = (FRSKY_RX_format + 1) % FRSKY_RX_FORMATS; // switch to next format (D8, D16FCC, D16LBT, D16v2FCC, D16v2LBT)
			FRSKY_RX_initialise_cc2500();
			FRSKY_RX_finetune += 10;
			CC2500_WriteReg(CC2500_0C_FSCTRL0, FRSKY_RX_finetune);
			FRSKY_RX_strobe_rx();
			return 18000;

		case FRSKY_RX_TUNE_LOW:
			if (len == packetSize_p2M + 2) //+2=RSSI+LQI+CRC
			{
				CC2500_ReadData(packet_p2M, len);
				if(frskyx_rx_check_crc_id(true,false)) {
					tune_low = FRSKY_RX_finetune;
					FRSKY_RX_finetune = 127;
					CC2500_WriteReg(CC2500_0C_FSCTRL0, FRSKY_RX_finetune);
					send_seq_p2M = FRSKY_RX_TUNE_HIGH;
					//debugln("FRSKY_RX_TUNE_HIGH");
					FRSKY_RX_strobe_rx();
					return 1000*2;
				}
			}
			FRSKY_RX_finetune += 1;
			CC2500_WriteReg(CC2500_0C_FSCTRL0, FRSKY_RX_finetune);
			FRSKY_RX_strobe_rx();
			return 18000;

		case FRSKY_RX_TUNE_HIGH:
			if (len == packetSize_p2M + 2) //+2=RSSI+LQI+CRC
			{
				CC2500_ReadData(packet_p2M, len);
				if(frskyx_rx_check_crc_id(true,false)) {
					tune_high = FRSKY_RX_finetune;
					FRSKY_RX_finetune = (tune_low + tune_high) / 2;
					CC2500_WriteReg(CC2500_0C_FSCTRL0, (int8_t)FRSKY_RX_finetune);
					if(tune_low < tune_high)
					{
						send_seq_p2M = FRSKY_RX_BIND;
						//debugln("FRSKY_RX_TUNE_HIGH");
					}
					else
					{
						send_seq_p2M = FRSKY_RX_TUNE_START;
						//debugln("FRSKY_RX_TUNE_START");
					}
					FRSKY_RX_strobe_rx();
					return 1000*2;
				}
			}
			FRSKY_RX_finetune -= 1;
			CC2500_WriteReg(CC2500_0C_FSCTRL0, FRSKY_RX_finetune);
			FRSKY_RX_strobe_rx();
			return 18000;

		case FRSKY_RX_BIND:
		  SCHEDULE_MIXER_END_IN_US(1000);
			if (len == packetSize_p2M + 2) //+2=RSSI+LQI+CRC
			{
				CC2500_ReadData(packet_p2M, len);
				if(frskyx_rx_check_crc_id(true,false)) {
					if(FRSKY_RX_format != FRSKY_RX_D16v2LBT && FRSKY_RX_format != FRSKY_RX_D16v2FCC)
					{// D8 & D16v1
						if(packet_p2M[5] <= 0x2D)
						{
							for (ch = 0; ch < 5; ch++)
								channel_used_p2M[packet_p2M[5]+ch] = packet_p2M[6+ch];
							State_Frsky_RX |= 1 << (packet_p2M[5] / 5);
						}
					}
					else
						State_Frsky_RX = 0x3FF; //No hop table for D16v2
					if (State_Frsky_RX == 0x3FF)
					{
						//debugln("Bind complete");
						FRSKYRX_BIND = 0;
						// store format, finetune setting, txid, channel list

						uint16_t temp = FRSKY_RX_EEPROM_OFFSET;
						if(FRSKY_RX_format==FRSKY_CLONE)
						{
							if(FRSKY_RX_format==FRSKY_RX_D8)
								temp=FRSKYD_CLONE_EEPROM_OFFSET;
							else if(FRSKY_RX_format == FRSKY_RX_D16FCC || FRSKY_RX_format == FRSKY_RX_D16LBT)
								temp=FRSKYX_CLONE_EEPROM_OFFSET;
							else
								temp=FRSKYX2_CLONE_EEPROM_OFFSET;
						}

						//eeprom_write_byte((EE_ADDR)temp++, FRSKY_RX_format);
						//eeprom_write_byte((EE_ADDR)temp++, temp_rfid_addr_p2M[3]);
						//eeprom_write_byte((EE_ADDR)temp++, temp_rfid_addr_p2M[2]);
						//eeprom_write_byte((EE_ADDR)temp++, temp_rfid_addr_p2M[1]);
						//if(FRSKY_RX_format == FRSKY_RX || FRSKY_RX_format == FRSKY_CPPM)	// FRSKY_RX, FRSKY_CPPM
						//	eeprom_write_byte((EE_ADDR)temp++, FRSKY_RX_finetune);
						if(FRSKY_RX_format != FRSKY_RX_D16v2FCC && FRSKY_RX_format != FRSKY_RX_D16v2LBT)
						//	for (ch = 0; ch < 47; ch++)
						//		eeprom_write_byte((EE_ADDR)temp++, channel_used_p2M[ch]);
						FRSKY_RX_data();
						//debugln("FRSKY_RX_DATA");
					}
				}
				FRSKY_RX_strobe_rx();
			}
			return 1000*2;

		case FRSKY_RX_DATA://telemetry ?
		  SCHEDULE_MIXER_END_IN_US(1000);
/*			if (len == packetSize_p2M + 2) //+2=RSSI+LQI+CRC
			{
				CC2500_ReadData(packet_p2M, len);
				if(frskyx_rx_check_crc_id(false,false))
				{
					RX_RSSI = packet_p2M[len-2];
					if(RX_RSSI >= 128)
						RX_RSSI -= 128;
					else
						RX_RSSI += 128;
					bool chanskip_valid=true;
					// hop to next channel
					if (FRSKY_RX_format != FRSKY_RX_D8)
					{//D16v1 & D16v2
						if(rx_data_started)
						{
							if(channel_skip_p2M != (((packet_p2M[4] & 0xC0) >> 6) | ((packet_p2M[5] & 0x3F) << 2)))
							{
								chanskip_valid=false;	// chanskip value has changed which surely indicates a bad frame
								packet_count_p2M++;
								if(packet_count_p2M>5)		// the TX must have changed chanskip...
									channel_skip_p2M = ((packet_p2M[4] & 0xC0) >> 6) | ((packet_p2M[5] & 0x3F) << 2);	// chanskip init
							}
							else
								packet_count_p2M=0;
						}
						else
							channel_skip_p2M = ((packet_p2M[4] & 0xC0) >> 6) | ((packet_p2M[5] & 0x3F) << 2);	// chanskip init
					}
					channel_index_p2M = (channel_index_p2M + channel_skip_p2M) % 47;
					FRSKY_RX_set_channel(channel_index_p2M);
					if(chanskip_valid)
					{
						if ((telemetry_link & 0x7F) == 0)
						{ // send channels to TX
							FRSKY_RX_build_telemetry_packet();
							telemetry_link = 1;
							#ifdef SEND_CPPM
								if(g_model.rfSubType == FRSKY_CPPM)
									telemetry_link |= 0x80;		// Disable telemetry output
							#endif
						}
						pps_counter++;
					}
					rx_data_started = true;
					read_retry = 0;
				}
			}

			// packets per second
			if (millis() - pps_timer >= 1000) {
				pps_timer = millis();
				//debugln("%d pps", pps_counter);
				RX_LQI = pps_counter;
				if(pps_counter==0)	// no packets for 1 sec or more...
				{// restart the search
					rx_data_started=false;
					packet_count_p2M=0;
				}
				pps_counter = 0;
			}
*/

			// skip channel if no packet received in time
			if (read_retry++ >= 9) {
				channel_index_p2M = (channel_index_p2M + channel_skip_p2M) % 47;
				FRSKY_RX_set_channel(channel_index_p2M);
				if(rx_data_started)
					read_retry = 0;
				else
					read_retry = -50; // retry longer until first packet is catched
			}
			break;
	}
	return 1000*2;
}

static uint16_t FRSKY_RX_cb()
{
 uint16_t time = FRSKY_RX_callback();
 heartbeat |= HEART_TIMER_PULSES;
 CALCULATE_LAT_JIT(); // Calculate latency and jitter.
 return time;
}

void FRSKY_RX_initialize(uint8_t bind)
{
	if(FRSKY_RX_format == FRSKY_ERASE)
	{
	  FRSKYRX_BIND = bind;
		if(bind)
		{// Clear all cloned addresses
		  /*
			uint16_t addr[]={ FRSKYD_CLONE_EEPROM_OFFSET+1, FRSKYX_CLONE_EEPROM_OFFSET+1, FRSKYX2_CLONE_EEPROM_OFFSET+1 };
			for(uint8_t i=0; i<3;i++)
				for(uint8_t j=0; j<3;j++)
					eeprom_write_byte((EE_ADDR)(addr[i]+j), 0xFF);
      */
			packet_count_p2M = 100;
		}
	}
	else
	{
		channel_skip_p2M = 1;
		channel_index_p2M = 0;
		rx_data_started = false;
		FRSKY_RX_finetune = 0;
		telemetry_link = 0;
		packet_count_p2M = 0;
		if (bind)
		{
			FRSKY_RX_format = FRSKY_RX_D8;
			FRSKY_RX_initialise_cc2500();
			send_seq_p2M = FRSKY_RX_TUNE_START;
			//debugln("FRSKY_RX_TUNE_START");
		}
		else
			FRSKY_RX_data();
	}
	PROTO_Start_Callback(FRSKY_RX_cb);
}

const void *FRSKY_RX_Cmds(enum ProtoCmds cmd)
{
 switch(cmd)
  {
  case PROTOCMD_INIT:
   FRSKY_RX_initialize(0);
   return 0;
  case PROTOCMD_BIND:
   FRSKY_RX_initialize(1);
   return 0;
  case PROTOCMD_RESET:
   PROTO_Stop_Callback();
   CC2500_Reset();
   return 0;
  case PROTOCMD_GETOPTIONS:
   SetRfOptionSettings(pgm_get_far_address(RfOpt_Frsky_RX_Ser),
                       STR_SUBTYPE_FRSKY_RX,  //Sub proto
                       STR_RFTUNEFINE,        //Option 1 (int)
                       STR_DUMMY,             //Option 2 (int)
                       STR_RFPOWER,           //Option 3 (uint 0 to 31)
                       STR_TELEMETRY,         //OptionBool 1
                       STR_DUMMY,             //OptionBool 2
                       STR_RX_DIS_LNA         //OptionBool 3
                      );
   return 0;
  default:
   break;
  }
 return 0;
}
