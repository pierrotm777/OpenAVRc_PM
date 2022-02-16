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



#ifdef PROTO_HAS_SX1276
#include "../OpenAVRc.h"


#define REG_PA_CONFIG          0x09
#define PA_BOOST               0x80

bool SX1276_Mode_LoRa=false;


void SX1276_WriteReg(uint8_t address, uint8_t data)
{
	RF_CS_CYRF6936_ACTIVE();
	RF_SPI_xfer(address | 0x80); // MSB 1 = write
//	NOP();
	RF_SPI_xfer(data);
	RF_CS_CYRF6936_INACTIVE();
}

uint8_t SX1276_ReadReg(uint8_t address)
{
	RF_CS_CYRF6936_ACTIVE();
	RF_SPI_xfer(address & 0x7F);
	uint8_t result = RF_SPI_xfer(0);
	RF_CS_CYRF6936_INACTIVE();
	return result;
}

static void SX1276_WriteRegisterMulti(uint8_t address, const uint8_t* data, uint8_t length)
{
	RF_CS_CYRF6936_ACTIVE();
	RF_SPI_xfer(address | 0x80); // MSB 1 = write

	for(uint8_t i = 0; i < length; i++){
		RF_SPI_xfer(data[i]);
  }
	RF_CS_CYRF6936_INACTIVE();
}


static void SX1276_ReadRegisterMulti(uint8_t address, uint8_t* data, uint8_t length)
{
	RF_CS_CYRF6936_ACTIVE();
	RF_SPI_xfer(address & 0x7F);

	for(uint8_t i = 0; i < length; i++)
		data[i]=RF_SPI_xfer(0);

	RF_CS_CYRF6936_INACTIVE();
}


uint8_t SX1276_Reset()
{
	//TODO when pin is not wired
	#ifdef SX1276_RST_pin
		SX1276_RST_off;
		delayMicroseconds(200);
		SX1276_RST_on;
	#endif
    return 0;
}

bool SX1276_DetectChip() //to be called after reset, verfies the chip has been detected
{
	#define SX1276_Detect_MaxAttempts 5
	uint8_t i = 0;
	bool chipFound = false;
	while ((i < SX1276_Detect_MaxAttempts) && !chipFound)
	{
		uint8_t ChipVersion = SX1276_ReadReg(SX1276_42_VERSION);
		if (ChipVersion == 0x12)
		{
//			debugln("SX1276 reg version=%d", ChipVersion);
			chipFound = true;
		}
		else
		{
//			debug("SX1276 not found! attempts: %d", i);
//			debug(" of ");
//			debugln("%d SX1276 reg version=%d", SX1276_Detect_MaxAttempts, ChipVersion);
			i++;
		}
	}
	if (!chipFound)
	{
//		debugln("SX1276 not detected!!!");
		return false;
	}
	else
	{
//		debugln("Found SX1276 Device!");
		return true;
	}
}

void SX1276_SetTxRxMode(uint8_t mode)
{
	#ifdef SX1276_TXEN_pin
		if(mode == TX_EN)
			SX1276_TXEN_on;
		else
			SX1276_RXEN_on;
	#endif
}

void SX1276_SetFrequency(uint32_t frequency)
{
	uint32_t f = frequency / 61;
	uint8_t data[3];
	data[0] = f >> 16;
	data[1] = f >> 8;
	data[2] = f;

	SX1276_WriteRegisterMulti(SX1276_06_FRFMSB, data, 3);
}

void SX1276_SetMode(bool lora, bool low_freq_mode, uint8_t mode)
{
	uint8_t data = 0x00;

	SX1276_Mode_LoRa=lora;

	if(lora)
	{
		data = data | (1 << 7);
		data = data & ~(1 << 6);
	}
	else
	{
		data = data & ~(1 << 7);
		data = data | (1 << 6);
	}

	if(low_freq_mode)
		data = data | (1 << 3);

	data = data | mode;

	SX1276_WriteReg(SX1276_01_OPMODE, data);
}

void SX1276_SetDetectOptimize(bool auto_if, uint8_t detect_optimize)
{
	uint8_t data = SX1276_ReadReg(SX1276_31_DETECTOPTIMIZE);
	data = (data & 0b01111000) | detect_optimize;
	data = data | (auto_if << 7);

	SX1276_WriteReg(SX1276_31_DETECTOPTIMIZE, data);
}

void SX1276_ConfigModem1(uint8_t bandwidth, uint8_t coding_rate, bool implicit_header_mode)
{
	uint8_t data = 0x00;
	data = data | (bandwidth << 4);
	data = data | (coding_rate << 1);
	data = data | implicit_header_mode;

	SX1276_WriteReg(SX1276_1D_MODEMCONFIG1, data);

	if (bandwidth == SX1276_MODEM_CONFIG1_BW_500KHZ) //datasheet errata reconmendation http://caxapa.ru/thumbs/972894/SX1276_77_8_ErrataNote_1.1_STD.pdf
	{
		SX1276_WriteReg(SX1276_36_LORA_REGHIGHBWOPTIMIZE1, 0x02);
		SX1276_WriteReg(SX1276_3A_LORA_REGHIGHBWOPTIMIZE2, 0x64);
	}
	else
		SX1276_WriteReg(SX1276_36_LORA_REGHIGHBWOPTIMIZE1, 0x03);
}

void SX1276_ConfigModem2(uint8_t spreading_factor, bool tx_continuous_mode, bool rx_payload_crc_on)
{
	uint8_t data = SX1276_ReadReg(SX1276_1E_MODEMCONFIG2);
	data = data & 0b11; // preserve the last 2 bits
	data = data | (spreading_factor << 4);
	data = data | (tx_continuous_mode << 3);
	data = data | (rx_payload_crc_on << 2);

	SX1276_WriteReg(SX1276_1E_MODEMCONFIG2, data);
}

void SX1276_ConfigModem3(bool low_data_rate_optimize, bool agc_auto_on)
{
	uint8_t data = SX1276_ReadReg(SX1276_26_MODEMCONFIG3);
	data = data & 0b11; // preserve the last 2 bits
	data = data | (low_data_rate_optimize << 3);
	data = data | (agc_auto_on << 2);

	SX1276_WriteReg(SX1276_26_MODEMCONFIG3, data);
}

void SX1276_SetPreambleLength(uint16_t length)
{
	uint8_t data[2];
	data[0] = (length >> 8) & 0xFF; // high byte
	data[1] = length & 0xFF; // low byte

	SX1276_WriteRegisterMulti(SX1276_20_PREAMBLEMSB, data, 2);
}

void SX1276_SetDetectionThreshold(uint8_t threshold)
{
	SX1276_WriteReg(SX1276_37_DETECTIONTHRESHOLD, threshold);
}

void SX1276_SetLna(uint8_t gain, bool high_freq_lna_boost)
{
	uint8_t data = SX1276_ReadReg(SX1276_0C_LNA);
	data = data & 0b100; // preserve the third bit
	data = data | (gain << 5);

	if(high_freq_lna_boost)
		data = data | 0b11;

	SX1276_WriteReg(SX1276_0C_LNA, data);
}

void SX1276_SetHopPeriod(uint8_t freq_hop_period)
{
	SX1276_WriteReg(SX1276_24_HOPPERIOD, freq_hop_period);
}

void SX1276_SetPaDac(bool on)
{
	uint8_t data = SX1276_ReadReg(SX1276_4D_PADAC);
	data = data & 0b11111000; // preserve the upper 5 bits

	if(on)
		data = data | 0x07;
	else
		data = data | 0x04;

	SX1276_WriteReg(SX1276_4D_PADAC, data);
}


void SX1276_setTxPower(int SXpower, bool useRFO)
{

  const static uint16_t zzSX1276_Powers[] PROGMEM = {0,100,1000,10000};
//#endif

  uint8_t out;

  switch(SXpower)
    {
    case 0:
      out = 0;
      break;
    case 1:
      out = 1;
      break;
    case 2:
      out = 10;
      break;
    case 3:
      out = 100;
      break;
    default:
      out = 0;
      break;
    };


  uint_farptr_t powerdata = pgm_get_far_address(zzSX1276_Powers);
  RFPowerOut = pgm_read_word_far(powerdata + (SXpower)); // Gui value
  rf_power_mem_p2M = SXpower;

 /********************************************************************/

  if (useRFO)
  {
    // RFO
    if (SXpower > 14)
        SXpower = 14;
    if (SXpower < -1)
        SXpower = -1;
    SX1276_WriteReg(REG_PA_CONFIG, 0x70 | (SXpower+1));
  }
  else
  {
      // PA BOOST
    if (SXpower > 23)
        SXpower = 23;
    if (SXpower < 5)
        SXpower = 5;

    // For PA_DAC_ENABLE, manual says '+20dBm on PA_BOOST when OutputPower=0xf'
    // PA_DAC_ENABLE actually adds about 3dBm to all power levels. We will us it
    // for 21, 22 and 23dBm
    if (SXpower > 20)
    {
        SX1276_WriteReg(REG_4D_PA_DAC, PA_DAC_ENABLE);
        SXpower -= 3;
    }
    else
    {
        SX1276_WriteReg(REG_4D_PA_DAC, PA_DAC_DISABLE);
    }

    // RFM95/96/97/98 does not have RFO pins connected to anything. Only PA_BOOST
    // pin is connected, so must use PA_BOOST
    // Pout = 2 + OutputPower.
    // The documentation is pretty confusing on this topic: PaSelect says the max power is 20dBm,
    // but OutputPower claims it would be 17dBm.
    // My measurements show 20dBm is correct
      SX1276_WriteReg(REG_PA_CONFIG, PA_BOOST | (SXpower - 5));
  }
}


//Set power
// max power: 15dBm (10.8 + 0.6 * MaxPower [dBm])
// output_power: 2 dBm (17-(15-OutputPower) (if pa_boost_pin == true))
//SX1276_SetPaConfig(true, 7, 0);						// Lowest power for the T18

//RFOP = +20 dBm, on PA_BOOST      120 mA
//RFOP = +17 dBm, on PA_BOOST       87 mA
//RFOP = +13 dBm, on RFO_LF/HF pin  29 mA
//RFOP = + 7 dBm, on RFO_LF/HF pin  20 mA

/*
7 dBm	0.0050119 W
8 dBm	0.0063096 W
9 dBm	 0.0079433 W
10 dBm	0.01 W
20 dBm	0.1 W
*/

void SX1276_SetPaConfig(bool pa_boost_pin, uint8_t max_power, uint8_t output_power)
{
	uint8_t data = 0x00;
	data = data | (pa_boost_pin << 7);
	data = data | (max_power << 4);
	data = data | (output_power & 0x0F);

	SX1276_WriteReg(SX1276_09_PACONFIG, data);
}


/*
void SX1276_SetPaConfig(bool pa_boost_pin, uint8_t max_power, uint8_t SXpower)
{
//#if (SX1276PA_GAIN == 17)
//  const static uint16_t zzSX1276_Powers[] PROGMEM = {0,0,0,0};
//#endif
//#if (SX1276PA_GAIN == 20)
  const static uint16_t zzSX1276_Powers[] PROGMEM = {0,100,1000,10000};
//#endif

  uint8_t out;

  switch(SXpower)
    {
    case 0:
      out = 0;
      break;
    case 1:
      out = 1;
      break;
    case 2:
      out = 10;
      break;
    case 3:
      out = 100;
      break;
    default:
      out = 0;
      break;
    };


  uint_farptr_t powerdata = pgm_get_far_address(zzSX1276_Powers);
  RFPowerOut = pgm_read_word_far(powerdata + (SXpower)); // Gui value
  rf_power_mem_p2M = SXpower;

	uint8_t data = 0x00;
	data = data | (pa_boost_pin << 7);
	data = data | (max_power << 4);
	data = data | (SXpower & 0x0F);//data = data | out;

	SX1276_WriteReg(SX1276_09_PACONFIG, data);
}
*/

void SX1276_SetOcp(bool OcpOn, uint8_t OcpTrim)
{
	SX1276_WriteReg(SX1276_0B_OCP, (OcpOn << 5) | OcpTrim);
}

void SX1276_ManagePower()
{
  if (systemBolls.rangeModeIsOn)
    rf_power_p2M = TXPOWER_1;
  else
    rf_power_p2M = g_model.rfOptionValue3;
  if (rf_power_p2M != rf_power_mem_p2M)
  {

// The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
//  driver.setTxPower(23, false);
  // If you are using Modtronix inAir4 or inAir9,or any other module which uses the
  // transmitter RFO pins and not the PA_BOOST pins
  // then you can configure the power transmitter power for -1 to 14 dBm and with useRFO true.
  // Failure to do that will result in extremely low transmit powers.
//  driver.setTxPower(14, true);
    SX1276_setTxPower(rf_power_p2M,false);
  }
}

void SX1276_WritePayloadToFifo(uint8_t* payload, uint8_t length)
{
	SX1276_WriteReg(SX1276_22_PAYLOAD_LENGTH, length);
	SX1276_WriteReg(SX1276_0E_FIFOTXBASEADDR, 0x00);
	SX1276_WriteReg(SX1276_0D_FIFOADDRPTR, 0x00);
	SX1276_WriteRegisterMulti(SX1276_00_FIFO, payload, length);
}

#endif
