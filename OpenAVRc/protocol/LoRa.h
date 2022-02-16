// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef LORA_H
#define LORA_H

/*
CONFIGURATION
*/

enum stateMachineDef {SETUP = 0, TRANSMIT = 1, RECEIVE = 2, BIND = 3 };

// Dubugging - select any
//#define DEBUG_CH_FREQ
//#define DEBUG_RADIO_EXCH
//#define DEBUG_ANALYZER
//#define TX_SERVO_TESTER
//#define DEBUG_RX_OUTPUT

// Transmitter or Receiver - select one
#define TX_module
//#define RX_module

// Communication type - select one
#define PPM_module  // using ICP for TX or declared for TX
//#define IBUS_module   // using UART
//#define SBUS_module   // using UART at 100000N2
//#define MSP_module   // using UART

// Transmitting power in dBm: 2 to 20, default 17
uint8_t tx_power_low = 4;    //  4=>2.5mW; 6=>5mW; 10=>10mW
uint8_t tx_power_high = 10;  //  12=>16mW; 14=>25mW; 16=>40mW; 18=>63mW; 20=>100mW

int power_thr_high = -90;  // testing: 190 (-65); flying:  180 (-75)
int power_thr_low = -100;   // testing: 180 (-75); flying:  160 (-95)
uint8_t tx_power_step = 2;
#define TX_POWER_DELAY_FILTER sizeof(hop_list)*2

// Signal bandwidth and frame time
// full range 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, 500E3.
// measured (16uint8_ts from Transmitter, 6uint8_ts from Receiver :
//    SF6,BW500 => 10ms; SF6,BW250 => 20ms; SF6,BW125 => 40ms; SF6,BW62,5 => 80ms
unsigned long BW_low = 125E3;   //Hz
unsigned long BW_high = 250E3;  //Hz
unsigned long F_rate_low = 45000;  //us
unsigned long F_rate_high = 25000; //us
uint8_t spread_factor = 6;

#define base_frequency  868100000
#define frequency_step  100000
const uint8_t hop_list[] = {5,7,12};
#define LOST_FRAMES_COUNT 30  // sizeof(hop_list) * 10


// Servos & channels
#define SERVO_CHANNELS 8
volatile unsigned int Servo_Buffer[SERVO_CHANNELS] = {1500, 1500, 1000, 1500, 1500, 1500, 1500, 1500};
unsigned int Servos[SERVO_CHANNELS] = {1500, 1500, 1000, 1500, 1500, 1500, 1500, 1500};
unsigned int Servo_Failsafe[SERVO_CHANNELS] = {1500, 1500, 900, 1500, 1500, 1500, 1500, 150};
#define FAILSAFE_DELAY_MS 800

#define SERVO_SHIFT 0   //(-16)   // PPM = -16  |  others = 0

// inject RSSI signal into Channel (CH1 = 0)
#define INJECT_RSSI_IN_CH 7

// PPM
#define PPM_OUT 8
#define PPM_OUT_HIGH PORTB |= _BV(0)
#define PPM_OUT_LOW PORTB &= ~_BV(0)
#define PPM_IN  8 // ICP1

// RFM95 to Arduino HW connection
#define NSS_PIN 10
#define NRESET_PIN A0
#define DIO0_PIN 2  // must be IRQ assignable

/*
END CONFIGURATION
*/


#define LORA_DEFAULT_SPI           SPI
#define LORA_DEFAULT_SPI_FREQUENCY 8E6
#define LORA_DEFAULT_SS_PIN        10
#define LORA_DEFAULT_RESET_PIN     9
#define LORA_DEFAULT_DIO0_PIN      2


#define PA_OUTPUT_RFO_PIN          0
#define PA_OUTPUT_PA_BOOST_PIN     1

class LoRaClass : public Stream {
public:
  LoRaClass();

  int begin(long frequency);
  void end();

  int beginPacket(int implicitHeader = false);
  int endPacket(bool async = false);

  int parsePacket(int size = 0);
  int packetRssi();
  float packetSnr();
  long packetFrequencyError();

  // from Print
  virtual size_t Lora_write(uint8_t uint8_t);
  virtual size_t Lora_write(const uint8_t *buffer, size_t size);

  // from Stream
  virtual int Lora_available();
  virtual int Lora_read();
  virtual int Lora_peek();
  virtual void Lora_flush();


  void onReceive(void(*callback)(int));
  void onTxDone(void(*callback)());

  void receive(int size = 0);

  void idle();
  void sleep();

  void setTxPower(int level, int outputPin = PA_OUTPUT_PA_BOOST_PIN);
  void setFrequency(long frequency);
  void setSpreadingFactor(int sf);
  void setSignalBandwidth(long sbw);
  void setCodingRate4(int denominator);
  void setPreambleLength(long length);
  void setSyncWord(int sw);
  void enableCrc();
  void disableCrc();
  void enableInvertIQ();
  void disableInvertIQ();

  void setOCP(uint8_t mA); // Over Current Protection control

  // deprecated
  void crc() { enableCrc(); }
  void noCrc() { disableCrc(); }

  uint8_t random();

  void setPins(int ss = LORA_DEFAULT_SS_PIN, int reset = LORA_DEFAULT_RESET_PIN, int dio0 = LORA_DEFAULT_DIO0_PIN);
//  void setSPI(SPIClass& spi);
  void setSPIFrequency(uint32_t frequency);

  void dumpRegisters(Stream& out);

private:
  void explicitHeaderMode();
  void implicitHeaderMode();

  void handleDio0Rise();
  bool isTransmitting();

  int getSpreadingFactor();
  long getSignalBandwidth();

  void setLdoFlag();

  uint8_t readRegister(uint8_t address);
  void writeRegister(uint8_t address, uint8_t value);
  uint8_t singleTransfer(uint8_t address, uint8_t value);

  static void onDio0Rise();

private:
//  SPISettings _spiSettings;
//  SPIClass* _spi;
  int _ss;
  int _reset;
  int _dio0;
  long _frequency;
  int _packetIndex;
  int _implicitHeaderMode;
  void (*_onReceive)(int);
  void (*_onTxDone)();
};

//extern LoRaClass LoRa;

#endif
