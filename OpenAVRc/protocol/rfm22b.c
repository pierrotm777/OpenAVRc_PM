// **** bit-banged SPI routines
/*
#define SDO_pin 12//MOSI
#define SDI_pin 11//MISO
#define SCLK_pin 13//SCK
#define IRQ_pin 2//CS_A7105?
#define nSel_pin 4//CS_CYRF6936
#define SDN_pin 9// va à GND
*/

#define  nIRQ_1 (PIND & 0x04)==0x04 //D2
#define  nIRQ_0 (PIND & 0x04)==0x00 //D2

#define  nSEL_on RF_CS_CYRF6936_ACTIVE()//PORTD |= (1<<4) //D4
#define  nSEL_off RF_CS_CYRF6936_INACTIVE()//PORTD &= 0xEF //D4

#define  SCK_on  RF_XCK_ON()//PORTB |= _BV(5)  //B5
#define  SCK_off RF_XCK_OFF()//PORTB &= ~_BV(5) //B5

#define  SDI_on  PORTB |= _BV(3)  //B3
#define  SDI_off PORTB &= ~_BV(3) //B3

#define  SDO_1 RF_MOSI_ON()//(PINB & _BV(4)) == _BV(4) //B4
#define  SDO_0 RF_MOSI_OFF()//(PINB & _BV(4)) == 0x00  //B4

/* see board_megamini.h
#define OUT_J_CYRF6936_CS_N     PIN4_bm
#define RF_CS_CYRF6936_ACTIVE()   PORTJ &= ~(OUT_J_CYRF6936_CS_N)
#define RF_CS_CYRF6936_INACTIVE() PORTJ |= (OUT_J_CYRF6936_CS_N)

#define OUT_H_RF_MOSI             PIN1_bm
#define OUT_H_RF_XCK              PIN2_bm
#define RF_XCK_ON()               PORTH |= (OUT_H_RF_XCK)
#define RF_XCK_OFF()              PORTH &= ~(OUT_H_RF_XCK)
#define RF_MOSI_ON()              PORTH |= (OUT_H_RF_MOSI)
#define RF_MOSI_OFF()             PORTH &= ~(OUT_H_RF_MOSI)
*/



#define NOP() __asm__ __volatile__("nop")

uint8_t spiReadBit(void);
uint8_t spiReadData(void);
uint8_t spiReadRegister(uint8_t address);

void spiWriteBit(uint8_t b);
void spiWriteData(uint8_t i);
void spiWriteRegister(uint8_t address, uint8_t data);

void spiSendAddress(uint8_t i);
void spiSendCommand(uint8_t command);

uint8_t spiReadBit(void)
{
  uint8_t r = 0;
  SCK_on;
  NOP();
  if (SDO_1) {
    r = 1;
  }
  SCK_off;
  NOP();
  return r;
}

uint8_t spiReadData(void)
{
  uint8_t Result = 0;
  SCK_off;
  for (uint8_t i = 0; i < 8; i++) {   //read fifo data byte
    Result = (Result << 1) + spiReadBit();
  }
  return(Result);
}

uint8_t spiReadRegister(uint8_t address)
{
  uint8_t result;
  spiSendAddress(address);
  result = spiReadData();
  nSEL_on;
  return(result);
}

void spiWriteBit(uint8_t b)
{
  if (b) {
    SCK_off;
    NOP();
    SDI_on;
    NOP();
    SCK_on;
    NOP();
  } else {
    SCK_off;
    NOP();
    SDI_off;
    NOP();
    SCK_on;
    NOP();
  }
}

void spiWriteData(uint8_t i)
{
  for (uint8_t n = 0; n < 8; n++) {
    spiWriteBit(i & 0x80);
    i = i << 1;
  }
  SCK_off;
}

void spiWriteRegister(uint8_t address, uint8_t data)
{
  address |= 0x80;
  spiSendCommand(address);
  spiWriteData(data);
  nSEL_on;
}

void spiSendAddress(uint8_t i)
{
  spiSendCommand(i & 0x7f);
}

void spiSendCommand(uint8_t command)
{
  nSEL_on;
  SCK_off;
  nSEL_off;
  for (uint8_t n = 0; n < 8 ; n++) {
    spiWriteBit(command & 0x80);
    command = command << 1;
  }
  SCK_off;
}


// **** bit-banged SPI routines


void rfmInit(uint8_t diversity)
{
  spiWriteRegister(RFM22B_INTEN2, 0x00);    // disable interrupts
  spiWriteRegister(RFM22B_INTEN1, 0x00);    // disable interrupts
  spiWriteRegister(RFM22B_XTALCAP, 0x7F);   // XTAL cap = 12.5pF
  spiWriteRegister(RFM22B_MCUCLK, 0x05);    // 2MHz clock

  spiWriteRegister(RFM22B_GPIOCFG2, (diversity ? 0x17 : 0xFD) ); // gpio 2 ant. sw, 1 if diversity on else VDD
  spiWriteRegister(RFM22B_PREAMLEN, (diversity ? 0x14 : 0x0A) );    // 40 bit preamble, 80 with diversity
  spiWriteRegister(RFM22B_IOPRTCFG, 0x00);    // gpio 0,1,2 NO OTHER FUNCTION.

  #ifdef SWAP_GPIOS
  spiWriteRegister(RFM22B_GPIOCFG0, 0x15);    // gpio0 RX State
  spiWriteRegister(RFM22B_GPIOCFG1, 0x12);    // gpio1 TX State
  #else
  spiWriteRegister(RFM22B_GPIOCFG0, 0x12);    // gpio0 TX State
  spiWriteRegister(RFM22B_GPIOCFG1, 0x15);    // gpio1 RX State
  #endif

  // Packet settings
  spiWriteRegister(RFM22B_DACTL, 0x8C);    // enable packet handler, msb first, enable crc,
  spiWriteRegister(RFM22B_HDRCTL1, 0x0F);    // no broadcast, check header bytes 3,2,1,0
  spiWriteRegister(RFM22B_HDRCTL2, 0x42);    // 4 byte header, 2 byte sync, variable packet size
  spiWriteRegister(RFM22B_PREATH, 0x2A);    // preamble detect = 5 (20bits), rssioff = 2
  spiWriteRegister(RFM22B_SYNC3, 0x2D);    // sync word 3
  spiWriteRegister(RFM22B_SYNC2, 0xD4);    // sync word 2
  spiWriteRegister(RFM22B_SYNC1, 0x00);    // sync word 1 (not used)
  spiWriteRegister(RFM22B_SYNC0, 0x00);    // sync word 0 (not used)
  spiWriteRegister(RFM22B_HDREN3, 0xFF);    // must set all bits
  spiWriteRegister(RFM22B_HDREN2, 0xFF);    // must set all bits
  spiWriteRegister(RFM22B_HDREN1, 0xFF);    // must set all bits
  spiWriteRegister(RFM22B_HDREN0, 0xFF);    // must set all bits

  spiWriteRegister(RFM22B_FREQOFF1, 0x00);    // no offset
  spiWriteRegister(RFM22B_FREQOFF2, 0x00);    // no offset
  spiWriteRegister(RFM22B_FHCH,        0x00);   // set to hop channel 0
}

void rfmClearFIFO(uint8_t diversity)
{
  //clear FIFO, disable multi-packet, enable diversity if needed
  //requires two write ops, set & clear
  spiWriteRegister(RFM22B_OPMODE2, (diversity ? 0x83 : 0x03) );
  spiWriteRegister(RFM22B_OPMODE2, (diversity ? 0x80 : 0x00) );
}

void rfmClearInterrupts(void)
{
  spiWriteRegister(RFM22B_INTEN1, 0x00);
  spiWriteRegister(RFM22B_INTEN2, 0x00);
}

void rfmClearIntStatus(void)
{
  spiReadRegister(RFM22B_INTSTAT1);
  spiReadRegister(RFM22B_INTSTAT2);
}

void rfmSendPacket(uint8_t* pkt, uint8_t size)
{
  spiWriteRegister(RFM22B_PKTLEN, size);   // total tx size
  for (uint8_t i = 0; i < size; i++) {
    spiWriteRegister(RFM22B_FIFO, pkt[i]);
  }
  spiWriteRegister(RFM22B_INTEN1, RFM22B_PACKET_SENT_INTERRUPT);
}

uint16_t rfmGetAFCC(void)
{
  return (((uint16_t) spiReadRegister(RFM22B_AFC0) << 2) | ((uint16_t) spiReadRegister(RFM22B_AFC1) >> 6));
}

uint8_t rfmGetGPIO1(void)
{
  return spiReadRegister(RFM22B_GPIOCFG1);
}

uint8_t rfmGetRSSI(void)
{
  return spiReadRegister(RFM22B_RSSI);
}

uint8_t rfmGetPacketLength(void)
{
  return spiReadRegister(RFM22B_RXPLEN);
}

void rfmGetPacket(uint8_t *buf, uint8_t size)
{
  // Send the package read command
  spiSendAddress(RFM22B_FIFO);
  for (uint8_t i = 0; i < size; i++) {
    buf[i] = spiReadData();
  }
}

void rfmSetTX(void)
{
  spiWriteRegister(RFM22B_OPMODE1, (RFM22B_OPMODE_TX | RFM22B_OPMODE_READY));
  delayMicroseconds(200); // allow for PLL & PA ramp-up, ~200us
}

void rfmSetRX(void)
{
  spiWriteRegister(RFM22B_INTEN1, RFM22B_RX_PACKET_RECEIVED_INTERRUPT);
  spiWriteRegister(RFM22B_OPMODE1, (RFM22B_OPMODE_RX | RFM22B_OPMODE_READY));
  delayMicroseconds(200);  // allow for PLL ramp-up, ~200us
}

void rfmSetCarrierFrequency(uint32_t f)
{
  uint16_t fb, fc, hbsel;
  if (f < 480000000) {
    hbsel = 0;
    fb = f / 10000000 - 24;
    fc = (f - (fb + 24) * 10000000) * 4 / 625;
  } else {
    hbsel = 1;
    fb = f / 20000000 - 24;
    fc = (f - (fb + 24) * 20000000) * 2 / 625;
  }
  spiWriteRegister(RFM22B_BANDSEL, 0x40 + (hbsel ? 0x20 : 0) + (fb & 0x1f));
  spiWriteRegister(RFM22B_CARRFREQ1, (fc >> 8));
  spiWriteRegister(RFM22B_CARRFREQ0, (fc & 0xff));
  delayMicroseconds(200); // VCO / PLL calibration delay
}

void rfmSetChannel(uint8_t ch)
{
  spiWriteRegister(RFM22B_FHCH, ch);
}

void rfmSetDirectOut(uint8_t enable)
{
 static uint8_t r1 = 0, r2 = 0, r3 = 0;
  if (enable) {
    r1 = spiReadRegister(RFM22B_DACTL);
    r2 = spiReadRegister(RFM22B_MODCTL2);
    r3 = spiReadRegister(RFM22B_FREQDEV);
    // setup for direct output, i.e. beacon tones
    spiWriteRegister(RFM22B_DACTL, 0x00);    //disable packet handling
    spiWriteRegister(RFM22B_MODCTL2, 0x12);    // trclk=[00] no clock, dtmod=[01] direct using SPI, fd8=0 eninv=0 modtyp=[10] FSK
    spiWriteRegister(RFM22B_FREQDEV, 0x02);    // fd (frequency deviation) 2*625Hz == 1.25kHz
  } else {
    // restore previous values
    spiWriteRegister(RFM22B_DACTL, r1);
    spiWriteRegister(RFM22B_MODCTL2, r2);
    spiWriteRegister(RFM22B_FREQDEV, r3);
  }
}

void rfmSetHeader(uint8_t iHdr, uint8_t bHdr)
{
  spiWriteRegister(RFM22B_TXHDR3+iHdr, bHdr);
  spiWriteRegister(RFM22B_CHKHDR3+iHdr, bHdr);
}


void rfmSetModemRegs(struct rfm22_modem_regs* r)
{
  spiWriteRegister(RFM22B_IFBW,        r->r_1c);
  spiWriteRegister(RFM22B_AFCLPGR,   r->r_1d);
  spiWriteRegister(RFM22B_AFCTIMG,   r->r_1e);
  spiWriteRegister(RFM22B_RXOSR,      r->r_20);
  spiWriteRegister(RFM22B_NCOFF2,     r->r_21);
  spiWriteRegister(RFM22B_NCOFF1,     r->r_22);
  spiWriteRegister(RFM22B_NCOFF0,     r->r_23);
  spiWriteRegister(RFM22B_CRGAIN1,   r->r_24);
  spiWriteRegister(RFM22B_CRGAIN0,   r->r_25);
  spiWriteRegister(RFM22B_AFCLIM,     r->r_2a);
  spiWriteRegister(RFM22B_TXDR1,      r->r_6e);
  spiWriteRegister(RFM22B_TXDR0,      r->r_6f);
  spiWriteRegister(RFM22B_MODCTL1,  r->r_70);
  spiWriteRegister(RFM22B_MODCTL2,  r->r_71);
  spiWriteRegister(RFM22B_FREQDEV,  r->r_72);
}

void rfmSetPower(uint8_t power)
{
  spiWriteRegister(RFM22B_TXPOWER, power);
  _delay_us(25);//delayMicroseconds(25); // PA ramp up/down time
}

void RFM22B_ManagePower()
{
 if (systemBolls.rangeModeIsOn)
  rf_power_p2M = TXPOWER_1;
 else
  rf_power_p2M = g_model.rfOptionValue3;
 if (rf_power_p2M != rf_power_mem_p2M)
  {
   rfmSetPower(rf_power_p2M);
  }
}

void rfmSetReadyMode(void)
{
  spiWriteRegister(RFM22B_OPMODE1, RFM22B_OPMODE_READY);
}

void rfmSetStepSize(uint8_t sp)
{
  spiWriteRegister(RFM22B_FHS, sp);
}

