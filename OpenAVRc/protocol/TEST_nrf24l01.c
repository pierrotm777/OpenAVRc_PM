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

const pm_char STR_SUBTYPE_TEST_SPI[] PROGMEM = "BAYA""H8S3""X16A""IRDR""DHD4";

const static RfOptionSettingsvar_t RfOpt_TEST_Ser[] PROGMEM =
{
 /*rfProtoNeed*/PROTO_NEED_SPI | BOOL1USED | BOOL2USED, //can be PROTO_NEED_SPI | BOOL1USED | BOOL2USED | BOOL3USED
 /*rfSubTypeMax*/4,
 /*rfOptionValue1Min*/0,
 /*rfOptionValue1Max*/0,
 /*rfOptionValue2Min*/0,
 /*rfOptionValue2Max*/0,
 /*rfOptionValue3Max*/3,    // RF POWER
};

#define TEST_BIND_COUNT		1000
#define TEST_PACKET_PERIOD	1000
#define TEST_PACKET_SIZE		15
#define TEST_RF_NUM_CHANNELS	4
#define TEST_RF_BIND_CHANNEL	0
#define TEST_RF_BIND_CHANNEL_X16_AH 10
#define TEST_ADDRESS_LENGTH	5

enum TEST_FLAGS
{
// flags going to packet_p2M[2]
 TEST_FLAG_RTH		  	= 0x01,
 TEST_FLAG_HEADLESS	  = 0x02,
 TEST_FLAG_FLIP	    	= 0x08,
 TEST_FLAG_VIDEO	  	= 0x10,
 TEST_FLAG_PICTURE		= 0x20,
// flags going to packet_p2M[3]
 TEST_FLAG_INVERTED	  = 0x80,			// inverted flight on Floureon H101
 TEST_FLAG_TAKE_OFF  	= 0x20,			// take off / landing on X16 AH
 TEST_FLAG_EMG_STOP 	= 0x04|0x08,	// 0x08 for VISUO XS809H-W-HD-G
};

enum TEST_OPTION_FLAGS
{
 TEST_OPTION_FLAG_TELEMETRY	= 0x01,
 TEST_OPTION_FLAG_ANALOGAUX	= 0x02,
};

enum TEST
{
 TEST_BAYANG	 = 0,
 TEST_H8S3D	 = 1,
 TEST_X16_AH  = 2,
 TEST_IRDRONE = 3,
 TEST_DHD_D4	 = 4,
};

#define TEST_sub_protocol g_model.rfSubType
#define TEST_autobind g_model.rfOptionBool2
#define TEST_telemetry g_model.rfOptionBool1
#define TEST_rfid_addr telem_save_data_p2M // shared

static void TEST_init()
{
 NRF24L01_SetTxRxMode(TX_EN);

 XN297_SetTXAddr((uint8_t *)"\x00\x00\x00\x00\x00", TEST_ADDRESS_LENGTH);

 NRF24L01_FlushTx();
 NRF24L01_FlushRx();
 NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     	// Clear data ready, data sent, and retransmit
 NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      	// No Auto Acknowldgement on all data pipes
 NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  	// Enable data pipe 0 only
 NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, TEST_PACKET_SIZE);
 NRF24L01_SetBitrate(NRF24L01_BR_1M);             	// 1Mbps
 NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); 	// No retransmits
 NRF24L01_ManagePower();
 NRF24L01_Activate(0x73);							              // Activate feature register
 NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);		  	// Disable dynamic payload length on all pipes
 NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
 NRF24L01_Activate(0x73);

 switch (TEST_sub_protocol)
  {
  case X16_AH:
  case IRDRONE:
   num_channel_p2M = TEST_RF_BIND_CHANNEL_X16_AH;
   break;
  default:
   num_channel_p2M = TEST_RF_BIND_CHANNEL;
   break;
  }
}

uint16_t test_convert_channel_10b(uint8_t num)
{
 int16_t val=FULL_CHANNEL_OUTPUTS(num)+1023;
 val = limit <int16_t>(0, val, 2047);
 return (val >> 1);
}

static void TEST_send_packet(uint8_t bind)
{
 uint8_t i;
 if (bind)
  {
   if (TEST_telemetry)
    {
     packet_p2M[0]= 0xA3; // telemetry is enabled
    }
   else
    {
     packet_p2M[0] = 0xA4;
    }
   for(i = 0 ; i < 5; i++)
    packet_p2M[i+1] = TEST_rfid_addr[i];
   for(i = 0; i < 4; i++)
    packet_p2M[i+6] = channel_used_p2M[i];
   switch (TEST_sub_protocol)
    {
    case X16_AH:
     packet_p2M[10] = 0x00;
     packet_p2M[11] = 0x00;
     break;
    case IRDRONE:
     packet_p2M[10] = 0x30;
     packet_p2M[11] = 0x01;
     break;
    case DHD_D4:
     packet_p2M[10] = 0xC8;
     packet_p2M[11] = 0x99;
     break;
    default:
     packet_p2M[10] = TEST_rfid_addr[0];	// txid[0]
     packet_p2M[11] = TEST_rfid_addr[1];	// txid[1]
     break;
    }
  }
 else
  {
   uint16_t val;
   uint8_t dyntrim = 1;
   switch (TEST_sub_protocol)
    {
    case X16_AH:
    case IRDRONE:
     packet_p2M[0] = 0xA6;
     break;
    default:
     packet_p2M[0] = 0xA5;
     break;
    }

   packet_p2M[1] = 0xFA;
   //Flags packet_p2M[2]
   packet_p2M[2] = 0x00;
   if(FULL_CHANNEL_OUTPUTS(4)>0)
    packet_p2M[2] = TEST_FLAG_FLIP;
   if(FULL_CHANNEL_OUTPUTS(5)>0)
    packet_p2M[2] |= TEST_FLAG_RTH;
   if(FULL_CHANNEL_OUTPUTS(6)>0)
    packet_p2M[2] |= TEST_FLAG_PICTURE;
   if(FULL_CHANNEL_OUTPUTS(7)>0)
    packet_p2M[2] |= TEST_FLAG_VIDEO;
   if(FULL_CHANNEL_OUTPUTS(8)>0)
    {
     packet_p2M[2] |= TEST_FLAG_HEADLESS;
     dyntrim = 0;
    }
   //Flags packet_p2M[3]
   packet_p2M[3] = 0x00;
   if(FULL_CHANNEL_OUTPUTS(9)>0)
    packet_p2M[3] = TEST_FLAG_INVERTED;
   if(FULL_CHANNEL_OUTPUTS(10)>0)
    dyntrim = 0;
   if(FULL_CHANNEL_OUTPUTS(11)>0)
    packet_p2M[3] |= TEST_FLAG_TAKE_OFF;
   if(FULL_CHANNEL_OUTPUTS(12)>0)
    packet_p2M[3] |= TEST_FLAG_EMG_STOP;
   //Aileron
   val = test_convert_channel_10b(3);
   packet_p2M[4] = (val>>8) + (dyntrim ? ((val>>2) & 0xFC) : 0x7C);
   packet_p2M[5] = val & 0xFF;
   //Elevator
   val = test_convert_channel_10b(1);
   packet_p2M[6] = (val>>8) + (dyntrim ? ((val>>2) & 0xFC) : 0x7C);
   packet_p2M[7] = val & 0xFF;
   //Throttle
   val = test_convert_channel_10b(2);
   packet_p2M[8] = (val>>8) + 0x7C;
   packet_p2M[9] = val & 0xFF;
   //Rudder
   val = test_convert_channel_10b(0);
   packet_p2M[10] = (val>>8) + (dyntrim ? ((val>>2) & 0xFC) : 0x7C);
   packet_p2M[11] = val & 0xFF;
  }
 switch (TEST_sub_protocol)
  {
  case TEST_H8S3D:
   packet_p2M[12] = TEST_rfid_addr[2];	// txid[2]
   packet_p2M[13] = 0x34;
   break;
  case TEST_X16_AH:
   packet_p2M[12] = 0;
   packet_p2M[13] = 0;
   break;
  case TEST_IRDRONE:
   packet_p2M[12] = 0xE0;
   packet_p2M[13] = 0x2E;
   break;
  case TEST_DHD_D4:
   packet_p2M[12] = 0x37;	//0x17 during bind
   packet_p2M[13] = 0xED;
   break;
  default:
   packet_p2M[12] = TEST_rfid_addr[2];	// txid[2]
   packet_p2M[13] = 0x0A;
   break;
  }
 packet_p2M[14] = 0;
 for (uint8_t i = 0; i < TEST_PACKET_SIZE-1; i++)
  packet_p2M[14] += packet_p2M[i];

 NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? num_channel_p2M:channel_used_p2M[channel_index_p2M++]);
 channel_index_p2M %= TEST_RF_NUM_CHANNELS;

// clear packet status bits and TX FIFO
 NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
 NRF24L01_FlushTx();

 XN297_WritePayload(packet_p2M, TEST_PACKET_SIZE);

 NRF24L01_SetTxRxMode(TXRX_OFF);
 NRF24L01_SetTxRxMode(TX_EN);

// Power on, TX mode, 2byte CRC
// Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
 XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));

 if (TEST_telemetry)
  {
   // switch radio to rx as soon as packet is sent
   while (!(NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_TX_DS)));
   NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x03);
  }

 NRF24L01_ManagePower();
}

static void TEST_check_rx(void)
{
 if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
  {
   // data received from model
   XN297_ReadPayload(packet_p2M, TEST_PACKET_SIZE);
   NRF24L01_WriteReg(NRF24L01_07_STATUS, 255);

   NRF24L01_FlushRx();
   uint8_t check = packet_p2M[0];
   for (uint8_t i = 1; i < TEST_PACKET_SIZE-1; i++)
    check += packet_p2M[i];
   // decode data , check sum is ok as well, since there is no crc
   if (packet_p2M[0] == 0x85 && packet_p2M[14] == check)
    {

     frskyStreaming = frskyStreaming ? FRSKY_TIMEOUT10ms : FRSKY_TIMEOUT_FIRST;
     telemetryData.analog[TELEM_ANA_A1].set((packet_p2M[3]<<7) + (packet_p2M[4]>>2), g_model.telemetry.channels[TELEM_ANA_A1].type);
     telemetryData.analog[TELEM_ANA_A2].set((packet_p2M[5]<<7) + (packet_p2M[6]>>2), g_model.telemetry.channels[TELEM_ANA_A2].type);
     telemetryData.rssi[0].set(packet_p2M[7]);

     receive_seq_p2M++;
    }
  }
}

static void TEST_initialize_channels()
{
 if(TEST_sub_protocol==DHD_D4)
  channel_used_p2M[0]=(temp_rfid_addr_p2M[2]&0x07)|0x01;
 else
  channel_used_p2M[0]=0;
 channel_used_p2M[1] = (temp_rfid_addr_p2M[0]&0x1F)+0x10;
 channel_used_p2M[2] = channel_used_p2M[1]+0x20;
 channel_used_p2M[3] = channel_used_p2M[2]+0x20;
 channel_index_p2M = 0;
}

static uint16_t TEST_callback()
{

 SCHEDULE_MIXER_END_IN_US(TEST_PACKET_PERIOD);
 if (!bind_counter_p2M) //if(IS_BIND_DONE)
  {
   if(!packet_count_p2M)
    TEST_send_packet(0);
   packet_count_p2M++;
   if (TEST_telemetry)
    {
     rfState16_p2M++;
     if (rfState16_p2M > 500)
      {
       telemetryData.rssi[1].set(receive_seq_p2M);
       rfState16_p2M = 0;
       receive_seq_p2M = 0;
      }
     if (packet_count_p2M > 1)
      TEST_check_rx();

     packet_count_p2M %= 5;
    }
   else
    packet_count_p2M %= 2;
  }
 else
  {
   if(!packet_count_p2M)
    TEST_send_packet(1);
   packet_count_p2M++;
   packet_count_p2M %= 4;
   bind_counter_p2M--;

   if (!bind_counter_p2M)
    {
     XN297_SetTXAddr(TEST_rfid_addr, TEST_ADDRESS_LENGTH);
     XN297_SetRXAddr(TEST_rfid_addr, TEST_ADDRESS_LENGTH);
    }
  }
 heartbeat |= HEART_TIMER_PULSES;
 CALCULATE_LAT_JIT(); // Calculate latency and jitter.
 return TEST_PACKET_PERIOD *2;
}

static void TEST_initialize(uint8_t bind)
{
 loadrfidaddr_rxnum(0);
 TEST_rfid_addr[0] = temp_rfid_addr_p2M[0];
 TEST_rfid_addr[1] = temp_rfid_addr_p2M[1];
 TEST_rfid_addr[2] = temp_rfid_addr_p2M[2];
 TEST_rfid_addr[3] = temp_rfid_addr_p2M[3];
 TEST_rfid_addr[4] = temp_rfid_addr_p2M[0];

 NRF24L01_Initialize();
 TEST_initialize_channels();
 TEST_init();
 if (bind || TEST_autobind)
  {
   bind_counter_p2M = TEST_BIND_COUNT * 4;
   if (TEST_autobind)
    {
     PROTOCOL_SetBindState(1000); // 5 Sec
    }
  }
 PROTO_Start_Callback(TEST_callback);
}

const void *TEST_Cmds(enum ProtoCmds cmd)
{
 switch(cmd)
  {
  case PROTOCMD_INIT:

   TEST_initialize(0);
   return 0;
  case PROTOCMD_RESET:
   PROTO_Stop_Callback();
   NRF24L01_Reset();
   return 0;
  case PROTOCMD_BIND:
   TEST_initialize(1);
   return 0;
  case PROTOCMD_GETOPTIONS:
   SetRfOptionSettings(pgm_get_far_address(RfOpt_TEST_Ser),
                       STR_SUBTYPE_TEST_SPI,      //Sub proto
                       STR_DUMMY,      //Option 1 (int)
                       STR_DUMMY,      //Option 2 (int)
                       STR_RFPOWER,    //Option 3 (uint 0 to 31)
                       STR_TELEMETRY,      //OptionBool 1
                       STR_AUTOBIND,      //OptionBool 2
                       STR_DUMMY       //OptionBool 3
                      );
   return 0;
  default:
   break;
  }
 return 0;
}

