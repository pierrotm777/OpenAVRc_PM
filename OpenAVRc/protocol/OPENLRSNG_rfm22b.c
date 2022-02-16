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



const static RfOptionSettingsvar_t RfOpt_OPENLRSNG_Ser[] PROGMEM = {
/*rfProtoNeed*/PROTO_NEED_SPI, //can be PROTO_NEED_SPI | BOOL1USED | BOOL2USED | BOOL3USED
/*rfSubTypeMax*/0,
/*rfOptionValue1Min*/-128,
/*rfOptionValue1Max*/127,
/*rfOptionValue2Min*/0,
/*rfOptionValue2Max*/0,
/*rfOptionValue3Max*/7,
};

void OPENLRSNG_init()//setup(void)
{
//  setupSPI();
//#ifdef SDN_pin
//  pinMode(SDN_pin, OUTPUT); //SDN
//  digitalWrite(SDN_pin, 0);
//#endif
  //LED and other interfaces
//  pinMode(Red_LED, OUTPUT); //RED LED
//  pinMode(Green_LED, OUTPUT); //GREEN LED
//#ifdef Red_LED2
//  pinMode(Red_LED2, OUTPUT); //RED LED
//  pinMode(Green_LED2, OUTPUT); //GREEN LED
//#endif
//  pinMode(BTN, INPUT); //Button
#ifdef TX_MODE1
  pinMode(TX_MODE1, INPUT);
  digitalWrite(TX_MODE1, HIGH);
#endif
#ifdef TX_MODE2
  pinMode(TX_MODE2, INPUT);
  digitalWrite(TX_MODE2, HIGH);
#endif
//  pinMode(PPM_IN, INPUT); //PPM from TX
//  digitalWrite(PPM_IN, HIGH); // enable pullup for TX:s with open collector output
#if defined (RF_OUT_INDICATOR)
  pinMode(RF_OUT_INDICATOR, OUTPUT);
  digitalWrite(RF_OUT_INDICATOR, LOW);
#endif
  //buzzerInit();

  //setupRfmInterrupt();

  //sei();

  setupProfile();
  txReadEeprom();
/*
#ifdef __AVR_ATmega32U4__
  Serial.begin(0); // Suppress warning on overflow on Leonardo
#else
  Serial.begin(tx_config.console_baud_rate);
#endif
*/
  buzzerOn(BZ_FREQ);
  digitalWrite(BTN, HIGH);
  Red_LED_ON ;

  //consoleFlush();

  consolePrint("OpenLRSng TX starting ");
  printVersion(version, getConsoleSerial());
  consolePrint(" on HW " XSTR(BOARD_TYPE) "\n");

  delay(100);

  checkSetupPpm();

  checkBND();

  checkButton();

  Red_LED_OFF;
  buzzerOff();

  setupRcSerial();

  uint32_t timeout = millis() + (serialMode == SERIAL_MODE_MULTI ? 20000 : 2000);
  while (ppmAge == 255 && millis() < timeout) {
    processSerial();
  }

  if (newMultiProfileSelected(false)) {
    activeProfile = multiProfile - 1;
  }

  configureProfile();
}


const static uint8_t ZZ_OPENLRSNGInitSequence[] PROGMEM = {
};

static void OPENLRSNG_init()
{
}


static void OPENLRSNG_send_data_packet()
{
}

static void OPENLRSNG_send_bind_packet()
{
  //bindMode();
  //uint32_t prevsend = millis();
  uint8_t  tx_buf[sizeof(bind_data) + 1];
  bool  sendBinds = 1;

  tx_buf[0] = 'b';
  memcpy(tx_buf + 1, &bind_data, sizeof(bind_data));

  init_rfm(1);

  //consoleFlush();

  //Red_LED_OFF;

  while (bndMode) {
    if (sendBinds & (millis() - prevsend > 200))
    {
      prevsend = millis();
      //Green_LED_ON;
      //buzzerOn(BZ_FREQ);
      tx_packet(tx_buf, sizeof(bind_data) + 1);
      //Green_LED_OFF;
      //buzzerOff();
      rx_reset();
      _delay_ms(50);//delay(50);
      if (RF_Mode == RECEIVED) {
        rfmGetPacket(tx_buf, 1);
        if (tx_buf[0] == 'B') {
          sendBinds = 0;
        }
      }
    }

    //if (!digitalRead(BTN)) {
    //  sendBinds = 1;
    //}

/*
    while (consoleDataAvailable())
    {
      //Red_LED_ON;
      //Green_LED_ON;
      switch (consoleRead()) {
#ifdef CLI
      case '\n':
      case '\r':
#ifdef CLI_ENABLED
        consolePrint("Enter menu...\n");
        handleCLI();
#else
        consolePrint("CLI not available, use configurator!\n");
#endif
        break;
#endif
      case '#':
        scannerMode();
        break;
#ifdef CONFIGURATOR
      case 'B':
        binaryMode();
        break;
#endif
      default:
        break;
      }
      //Red_LED_OFF;
      //Green_LED_OFF;
    }
*/

/*
    if (!bndMode) {
      processSerial();
    }
*/
    //watchdogReset();
  }
}

static uint16_t OPENLRSNG_bind_cb()
{
  SCHEDULE_MIXER_END_IN_US(18000); // Schedule next Mixer calculations.
  OPENLRSNG_send_bind_packet();
  heartbeat |= HEART_TIMER_PULSES;
  CALCULATE_LAT_JIT(); // Calculate latency and jitter.
  return 18000U *2;
}

static uint16_t OPENLRSNG_cb()
{
  SCHEDULE_MIXER_END_IN_US(12000); // Schedule next Mixer calculations.
  OPENLRSNG_send_data_packet();
  heartbeat |= HEART_TIMER_PULSES;
  CALCULATE_LAT_JIT(); // Calculate latency and jitter.
  return 12000U *2;
}


static void OPENLRSNG_initialize(uint8_t bind)
{
  OPENLRSNG_init();
  if (bind) {
  PROTO_Start_Callback( OPENLRSNG_bind_cb);
  } else {
  PROTO_Start_Callback( OPENLRSNG_cb);
  }
}

const void *OPENLRSNG_Cmds(enum ProtoCmds cmd)
{
  switch(cmd) {
  case PROTOCMD_INIT:
    OPENLRSNG_initialize(0);
    return 0;
  case PROTOCMD_RESET:
    PROTO_Stop_Callback();
    return 0;
  case PROTOCMD_BIND:
    OPENLRSNG_initialize(1);
    return 0;
  case PROTOCMD_GETOPTIONS:
          SetRfOptionSettings(pgm_get_far_address(RfOpt_OPENLRSNG_Ser),
                        STR_DUMMY,      //Sub proto
                        STR_DUMMY,      //Option 1 (int)
                        STR_DUMMY,      //Option 2 (int)
                        STR_RFPOWER,      //Option 3 (uint 0 to 31)
                        STR_DUMMY,      //OptionBool 1
                        STR_DUMMY,      //OptionBool 2
                        STR_DUMMY       //OptionBool 3
                        );
    return 0;
  default:
    break;
  }
  return 0;
}

