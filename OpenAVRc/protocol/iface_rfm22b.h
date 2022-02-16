#ifndef _IFACE_RFM22B_H_
#define _IFACE_RFM22B_H_

/** OpenLRSng binding **/

// Version number in single uint16 [8bit major][4bit][4bit]
// a.b.c == 0xaabc
#define OPENLRSNG_VERSION 0x0390
static uint16_t version = OPENLRSNG_VERSION;

// Factory setting values, modify via the CLI

//####### RADIOLINK RF POWER (beacon is always 100/13/1.3mW) #######
// 7 == 100mW (or 1000mW with M3)
// 6 == 50mW (use this when using booster amp), (800mW with M3)
// 5 == 25mW
// 4 == 13mW
// 3 == 6mW
// 2 == 3mW
// 1 == 1.6mW
// 0 == 1.3mW
#define DEFAULT_RF_POWER 7

#define DEFAULT_CHANNEL_SPACING 5 // 50kHz
#define DEFAULT_HOPLIST 22,10,19,34,49,41
#define DEFAULT_RF_MAGIC 0xDEADFEED

//  0 -- 4800bps, best range
//  1 -- 9600bps, medium range
//  2 -- 19200bps, medium range
#define DEFAULT_DATARATE 2

#define DEFAULT_BAUDRATE 115200

// TX_CONFIG flag masks
#define SW_POWER            0x04 // enable powertoggle via switch (JR dTX)
#define ALT_POWER           0x08
#define MUTE_TX             0x10 // do not beep on telemetry loss
#define MICROPPM            0x20
#define INVERTED_PPMIN      0x40
#define WATCHDOG_USED       0x80 // read only flag, only sent to configurator

// RX_CONFIG flag masks
#define PPM_MAX_8CH         0x01
#define ALWAYS_BIND         0x02
#define SLAVE_MODE          0x04
#define IMMEDIATE_OUTPUT    0x08
#define STATIC_BEACON       0x10
#define INVERTED_PPMOUT      0x40
#define WATCHDOG_USED       0x80 // read only flag, only sent to configurator

// BIND_DATA flag masks
#define TELEMETRY_OFF       0x00
#define TELEMETRY_PASSTHRU  0x08
#define TELEMETRY_FRSKY     0x10 // covers smartport if used with &
#define TELEMETRY_SMARTPORT 0x18
#define TELEMETRY_MASK      0x18
#define CHANNELS_4_4        0x01
#define CHANNELS_8          0x02
#define CHANNELS_8_4        0x03
#define CHANNELS_12         0x04
#define CHANNELS_12_4       0x05
#define CHANNELS_16         0x06
#define DIVERSITY_ENABLED   0x80
#define DEFAULT_FLAGS       (CHANNELS_8 | TELEMETRY_PASSTHRU)

typedef enum {
    SERIAL_MODE_NONE = 0,
    SERIAL_MODE_SPEKTRUM1024,
    SERIAL_MODE_SPEKTRUM2048,
    SERIAL_MODE_SBUS,
    SERIAL_MODE_SUMD,
    SERIAL_MODE_MULTI,
    SERIAL_MODE_MAX = SERIAL_MODE_MULTI
} serialMode_e;

#define MULTI_OPERATION_TIMEOUT_MS 5000

// helper macro for European PMR channels
#define EU_PMR_CH(x) (445993750L + 12500L * (x)) // valid for ch1-ch16 (Jan 2016  ECC update)

// helper macro for US FRS channels 1-7
#define US_FRS_CH(x) (462537500L + 25000L * (x)) // valid for ch1-ch7

#define DEFAULT_BEACON_FREQUENCY 0 // disable beacon
#define DEFAULT_BEACON_DEADTIME 30 // time to wait until go into beacon mode (30s)
#define DEFAULT_BEACON_INTERVAL 10 // interval between beacon transmits (10s)

#define MIN_DEADTIME 0
#define MAX_DEADTIME 255

#define MIN_INTERVAL 1
#define MAX_INTERVAL 255

#define BINDING_POWER     0x06 // not lowest since may result fail with RFM23BP

#define TELEMETRY_PACKETSIZE 9
#define MAX_PACKETSIZE 21

#define BIND_MAGIC (0xDEC1BE15 + (OPENLRSNG_VERSION & 0xfff0))
#define BINDING_VERSION ((OPENLRSNG_VERSION & 0x0ff0)>>4)

static uint8_t default_hop_list[] = {DEFAULT_HOPLIST};

// HW frequency limits
#if (RFMTYPE == 868)
#  define MIN_RFM_FREQUENCY 848000000
#  define MAX_RFM_FREQUENCY 888000000
#  define DEFAULT_CARRIER_FREQUENCY 868000000  // Hz  (ch 0)
#  define BINDING_FREQUENCY 868000000 // Hz
#elif (RFMTYPE == 915)
#  define MIN_RFM_FREQUENCY 895000000
#  define MAX_RFM_FREQUENCY 935000000
#  define DEFAULT_CARRIER_FREQUENCY 915000000  // Hz  (ch 0)
#  define BINDING_FREQUENCY 915000000 // Hz
#else
#  define MIN_RFM_FREQUENCY 413000000
#  define MAX_RFM_FREQUENCY 463000000
#  define DEFAULT_CARRIER_FREQUENCY 435000000  // Hz  (ch 0)
#  define BINDING_FREQUENCY 435000000 // Hz
#endif

#define MAXHOPS      24
#define PPM_CHANNELS 16

uint8_t activeProfile = 0;
uint8_t defaultProfile = 0;

struct tx_config {
  uint8_t  rfm_type;
  uint32_t max_frequency;
  uint32_t console_baud_rate;
  uint32_t flags;
  uint8_t  chmap[16];
} tx_config;

// 0 - no PPM needed, 1=2ch ... 0x0f=16ch
#define TX_CONFIG_GETMINCH() (tx_config.flags >> 28)
#define TX_CONFIG_SETMINCH(x) (tx_config.flags = (tx_config.flags & 0x0fffffff) | (((uint32_t)(x) & 0x0f) << 28))

// 27 bytes
struct RX_config {
  uint8_t  rx_type; // RX type fillled in by RX, do not change
  uint8_t  pinMapping[13];
  uint8_t  flags;
  uint8_t  RSSIpwm; //0-15 inject composite, 16-31 inject quality, 32-47 inject RSSI, 48-63 inject quality & RSSI on two separate channels
  uint32_t beacon_frequency;
  uint8_t  beacon_deadtime;
  uint8_t  beacon_interval;
  uint16_t minsync;
  uint8_t  failsafeDelay;
  uint8_t  ppmStopDelay;
  uint8_t  pwmStopDelay;
} rx_config;

// 18 bytes
struct bind_data {
  uint8_t version;
  uint32_t serial_baudrate;
  uint32_t rf_frequency;
  uint32_t rf_magic;
  uint8_t rf_power;
  uint8_t rf_channel_spacing;
  uint8_t hopchannel[MAXHOPS];
  uint8_t modem_params;
  uint8_t flags;
} bind_data;

struct rfm22_modem_regs {
  uint32_t bps;
  uint8_t  r_1c, r_1d, r_1e, r_20, r_21, r_22, r_23, r_24, r_25, r_2a, r_6e, r_6f, r_70, r_71, r_72;
} modem_params[] = {
  { 4800, 0x1a, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x1b, 0x1e, 0x27, 0x52, 0x2c, 0x23, 0x30 }, // 50000 0x00
  { 9600, 0x05, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x20, 0x24, 0x4e, 0xa5, 0x2c, 0x23, 0x30 }, // 25000 0x00
  { 19200, 0x06, 0x40, 0x0a, 0xd0, 0x00, 0x9d, 0x49, 0x00, 0x7b, 0x28, 0x9d, 0x49, 0x2c, 0x23, 0x30 }, // 25000 0x01
  { 57600, 0x05, 0x40, 0x0a, 0x45, 0x01, 0xd7, 0xdc, 0x03, 0xb8, 0x1e, 0x0e, 0xbf, 0x00, 0x23, 0x2e },
  { 125000, 0x8a, 0x40, 0x0a, 0x60, 0x01, 0x55, 0x55, 0x02, 0xad, 0x1e, 0x20, 0x00, 0x00, 0x23, 0xc8 },
};

#define DATARATE_COUNT (sizeof(modem_params) / sizeof(modem_params[0]))

struct rfm22_modem_regs bind_params =
{ 9600, 0x05, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x20, 0x24, 0x4e, 0xa5, 0x2c, 0x23, 0x30 };

// prototype
void fatalBlink(uint8_t blinks);

/*
#include <avr/eeprom.h>

// Save EEPROM by writing just changed data
void myEEPROMwrite(int16_t addr, uint8_t data)
{
  uint8_t retries = 5;
  while ((--retries) && (data != eeprom_read_byte((uint8_t *)addr))) {
    eeprom_write_byte((uint8_t *)addr, data);
  }
  if (!retries) {
    fatalBlink(2);
  }
}
*/

//#define NOP() __asm__ __volatile__("nop")


//also usable for RFM23B 1W module
#define delayMicroseconds(d)    _delay_us(d)

// register addresses
#define RFM22B_INTSTAT1     0x03
#define RFM22B_INTSTAT2     0x04
#define RFM22B_INTEN1        0x05
#define RFM22B_INTEN2        0x06
#define RFM22B_OPMODE1     0x07
#define RFM22B_OPMODE2     0x08
#define RFM22B_XTALCAP      0x09
#define RFM22B_MCUCLK       0x0A
#define RFM22B_GPIOCFG0    0x0B
#define RFM22B_GPIOCFG1    0x0C
#define RFM22B_GPIOCFG2    0x0D
#define RFM22B_IOPRTCFG    0x0E

#define RFM22B_IFBW           0x1C
#define RFM22B_AFCLPGR      0x1D
#define RFM22B_AFCTIMG      0x1E
#define RFM22B_RXOSR         0x20
#define RFM22B_NCOFF2       0x21
#define RFM22B_NCOFF1       0x22
#define RFM22B_NCOFF0       0x23
#define RFM22B_CRGAIN1     0x24
#define RFM22B_CRGAIN0     0x25
#define RFM22B_RSSI           0x26
#define RFM22B_AFCLIM       0x2A
#define RFM22B_AFC0          0x2B
#define RFM22B_AFC1          0x2C

#define RFM22B_DACTL    0x30
#define RFM22B_HDRCTL1    0x32
#define RFM22B_HDRCTL2    0x33
#define RFM22B_PREAMLEN  0x34
#define RFM22B_PREATH      0x35
#define RFM22B_SYNC3        0x36
#define RFM22B_SYNC2        0x37
#define RFM22B_SYNC1        0x38
#define RFM22B_SYNC0        0x39

#define RFM22B_TXHDR3      0x3A
#define RFM22B_TXHDR2      0x3B
#define RFM22B_TXHDR1      0x3C
#define RFM22B_TXHDR0      0x3D
#define RFM22B_PKTLEN       0x3E
#define RFM22B_CHKHDR3    0x3F
#define RFM22B_CHKHDR2   0x40
#define RFM22B_CHKHDR1   0x41
#define RFM22B_CHKHDR0   0x42
#define RFM22B_HDREN3     0x43
#define RFM22B_HDREN2     0x44
#define RFM22B_HDREN1     0x45
#define RFM22B_HDREN0     0x46
#define RFM22B_RXPLEN      0x4B

#define RFM22B_TXPOWER   0x6D
#define RFM22B_TXDR1        0x6E
#define RFM22B_TXDR0        0x6F

#define RFM22B_MODCTL1      0x70
#define RFM22B_MODCTL2      0x71
#define RFM22B_FREQDEV      0x72
#define RFM22B_FREQOFF1     0x73
#define RFM22B_FREQOFF2     0x74
#define RFM22B_BANDSEL      0x75
#define RFM22B_CARRFREQ1  0x76
#define RFM22B_CARRFREQ0  0x77
#define RFM22B_FHCH           0x79
#define RFM22B_FHS             0x7A
#define RFM22B_TX_FIFO_CTL1    0x7C
#define RFM22B_TX_FIFO_CTL2    0x7D
#define RFM22B_RX_FIFO_CTL     0x7E
#define RFM22B_FIFO            0x7F

// register fields
#define RFM22B_OPMODE_POWERDOWN    0x00
#define RFM22B_OPMODE_READY    0x01  // enable READY mode
#define RFM22B_OPMODE_TUNE      0x02  // enable TUNE mode
#define RFM22B_OPMODE_RX      0x04  // enable RX mode
#define RFM22B_OPMODE_TX      0x08  // enable TX mode
#define RFM22B_OPMODE_32K      0x10  // enable internal 32k xtal
#define RFM22B_OPMODE_WUT    0x40  // wake up timer
#define RFM22B_OPMODE_LBD     0x80  // low battery detector

#define RFM22B_PACKET_SENT_INTERRUPT               0x04
#define RFM22B_RX_PACKET_RECEIVED_INTERRUPT   0x02

void rfmInit(uint8_t diversity);
void rfmClearInterrupts(void);
void rfmClearIntStatus(void);
void rfmClearFIFO(uint8_t diversity);
void rfmSendPacket(uint8_t* pkt, uint8_t size);

uint16_t rfmGetAFCC(void);
uint8_t rfmGetGPIO1(void);
uint8_t rfmGetRSSI(void);
uint8_t rfmGetPacketLength(void);
void rfmGetPacket(uint8_t *buf, uint8_t size);

void rfmSetTX(void);
void rfmSetRX(void);
void rfmSetCarrierFrequency(uint32_t f);
void rfmSetChannel(uint8_t ch);
void rfmSetDirectOut(uint8_t enable);
void rfmSetHeader(uint8_t iHdr, uint8_t bHdr);
void rfmSetModemRegs(struct rfm22_modem_regs* r);
void rfmSetPower(uint8_t p);
void rfmSetReadyMode(void);
void rfmSetStepSize(uint8_t sp);

static uint16_t CRC16_value;

inline void CRC16_reset()
{
  CRC16_value = 0;
}

void CRC16_add(uint8_t c) // CCITT polynome
{
  uint8_t i;
  CRC16_value ^= (uint16_t)c << 8;
  for (i = 0; i < 8; i++) {
    if (CRC16_value & 0x8000) {
      CRC16_value = (CRC16_value << 1) ^ 0x1021;
    } else {
      CRC16_value = (CRC16_value << 1);
    }
  }
}

#if (COMPILE_TX != 1)
extern uint16_t failsafePPM[PPM_CHANNELS];
#endif

#define EEPROM_SIZE 1024 // EEPROM is 1k on 328p and 32u4
#define ROUNDUP(x) (((x)+15)&0xfff0)
#define MIN256(x)  (((x)<256)?256:(x))
#if (COMPILE_TX == 1)
#define EEPROM_DATASIZE MIN256(ROUNDUP((sizeof(tx_config) + sizeof(bind_data) + 4) * 4 + 3))
#else
#define EEPROM_DATASIZE MIN256(ROUNDUP(sizeof(rx_config) + sizeof(bind_data) + sizeof(failsafePPM) + 6))
#endif


bool accessEEPROM(uint8_t dataType, bool write)
{
  void *dataAddress = NULL;
  uint16_t dataSize = 0;

  uint16_t addressNeedle = 0;
  uint16_t addressBase = 0;
  uint16_t CRC = 0;

  do {
start:
#if (COMPILE_TX == 1)
    if (dataType == 0) {
      dataAddress = &tx_config;
      dataSize = sizeof(tx_config);
      addressNeedle = (sizeof(tx_config) + sizeof(bind_data) + 4) * activeProfile;
    } else if (dataType == 1) {
      dataAddress = &bind_data;
      dataSize = sizeof(bind_data);
      addressNeedle = sizeof(tx_config) + 2;
      addressNeedle += (sizeof(tx_config) + sizeof(bind_data) + 4) * activeProfile;
    } else if (dataType == 2) {
      dataAddress = &defaultProfile;
      dataSize = 1;
      addressNeedle = (sizeof(tx_config) + sizeof(bind_data) + 4) * 4; // defaultProfile is stored behind all 4 profiles
    }
#else
    if (dataType == 0) {
      dataAddress = &rx_config;
      dataSize = sizeof(rx_config);
      addressNeedle = 0;
    } else if (dataType == 1) {
      dataAddress = &bind_data;
      dataSize = sizeof(bind_data);
      addressNeedle = sizeof(rx_config) + 2;
    } else if (dataType == 2) {
      dataAddress = &failsafePPM;
      dataSize = sizeof(failsafePPM);
      addressNeedle = sizeof(rx_config) + sizeof(bind_data) + 4;
    }
#endif
    addressNeedle += addressBase;
    CRC16_reset();

    for (uint8_t i = 0; i < dataSize; i++, addressNeedle++) {
      if (!write) {
        *((uint8_t*)dataAddress + i) = eeprom_read_byte((uint8_t *)(addressNeedle));
      } else {
        //myEEPROMwrite(addressNeedle, *((uint8_t*)dataAddress + i));
      }

      CRC16_add(*((uint8_t*)dataAddress + i));
    }

    if (!write) {
      CRC = eeprom_read_byte((uint8_t *)addressNeedle) << 8 | eeprom_read_byte((uint8_t *)(addressNeedle + 1));

      if (CRC16_value == CRC) {
        // recover corrupted data
        // write operation is performed after every successful read operation, this will keep all cells valid
        write = true;
        addressBase = 0;
        goto start;
      } else {
        // try next block
      }
    } else {
      //myEEPROMwrite(addressNeedle++, CRC16_value >> 8);
      //myEEPROMwrite(addressNeedle, CRC16_value & 0x00FF);
    }
    addressBase += EEPROM_DATASIZE;
  } while (addressBase <= (EEPROM_SIZE - EEPROM_DATASIZE));
  return (write); // success on write, failure on read
}

bool bindReadEeprom()
{
  if (accessEEPROM(1, false) && (bind_data.version == BINDING_VERSION)) {
    return true;
  }
  return false;
}

void bindWriteEeprom()
{
  accessEEPROM(1, true);
}

void bindInitDefaults(void)
{
  bind_data.version = BINDING_VERSION;
  bind_data.serial_baudrate = DEFAULT_BAUDRATE;
  bind_data.rf_power = DEFAULT_RF_POWER;
  bind_data.rf_frequency = DEFAULT_CARRIER_FREQUENCY;
  bind_data.rf_channel_spacing = DEFAULT_CHANNEL_SPACING;

  bind_data.rf_magic = DEFAULT_RF_MAGIC;

  memset(bind_data.hopchannel, 0, sizeof(bind_data.hopchannel));
  memcpy(bind_data.hopchannel, default_hop_list, sizeof(default_hop_list));

  bind_data.modem_params = DEFAULT_DATARATE;
  bind_data.flags = DEFAULT_FLAGS;
}

#if (COMPILE_TX == 1)
#define TX_PROFILE_COUNT  4

void profileSet()
{
  accessEEPROM(2, true);
}

void profileInit()
{
  accessEEPROM(2, false);
  if (defaultProfile > TX_PROFILE_COUNT) {
    defaultProfile = 0;
    profileSet();
  }
  activeProfile = defaultProfile;
}

void setDefaultProfile(uint8_t profile)
{
  defaultProfile = profile;
  profileSet();
}

void txInitDefaults()
{
  tx_config.max_frequency = MAX_RFM_FREQUENCY;
  tx_config.console_baud_rate = DEFAULT_BAUDRATE;
  tx_config.flags = 0x00;
  TX_CONFIG_SETMINCH(5); // 6ch
  for (uint8_t i = 0; i < 16; i++) {
    tx_config.chmap[i] = i;
  }
}

void bindRandomize(bool randomChannels)
{
  uint8_t emergency_counter = 0;
  uint8_t c;
  uint32_t t = 0;
  while (t == 0) {
    t = micros();
  }
  srandom(t);

  bind_data.rf_magic = 0;
  for (c = 0; c < 4; c++) {
    bind_data.rf_magic = (bind_data.rf_magic << 8) + (random() % 255);
  }

  if (randomChannels) {
    // TODO: verify if this works properly
    for (c = 0; (c < MAXHOPS) && (bind_data.hopchannel[c] != 0); c++) {
again:
      if (emergency_counter++ == 255) {
        bindInitDefaults();
        return;
      }

      uint8_t ch = (random() % 50) + 1;

      // don't allow same channel twice
      for (uint8_t i = 0; i < c; i++) {
        if (bind_data.hopchannel[i] == ch) {
          goto again;
        }
      }

      // don't allow frequencies higher then tx_config.max_frequency
      uint32_t real_frequency = bind_data.rf_frequency + (uint32_t)ch * (uint32_t)bind_data.rf_channel_spacing * 10000UL;
      if (real_frequency > tx_config.max_frequency) {
        goto again;
      }

      bind_data.hopchannel[c] = ch;
    }
  }
}

void txWriteEeprom()
{
  accessEEPROM(0,true);
  accessEEPROM(1,true);
}

void txReadEeprom()
{
  if ((!accessEEPROM(0, false)) || (!accessEEPROM(1, false)) || (bind_data.version != BINDING_VERSION)) {
    txInitDefaults();
    bindInitDefaults();
    bindRandomize(true);
    txWriteEeprom();
  }
}
#endif

#if (COMPILE_TX != 1)
// following is only needed on receiver
void failsafeSave(void)
{
  accessEEPROM(2, true);
}

void failsafeLoad(void)
{
  if (!accessEEPROM(2, false)) {
    memset(failsafePPM, 0, sizeof(failsafePPM));
  }
}

void rxInitHWConfig();

void rxInitDefaults(bool save)
{
  rxInitHWConfig();

  rx_config.flags = ALWAYS_BIND;
  rx_config.RSSIpwm = 255; // off
  rx_config.failsafeDelay = 10; //1s
  rx_config.ppmStopDelay = 0;
  rx_config.pwmStopDelay = 0;
  rx_config.beacon_frequency = DEFAULT_BEACON_FREQUENCY;
  rx_config.beacon_deadtime = DEFAULT_BEACON_DEADTIME;
  rx_config.beacon_interval = DEFAULT_BEACON_INTERVAL;
  rx_config.minsync = 3000;

  if (save) {
    accessEEPROM(0, true);
    memset(failsafePPM, 0, sizeof(failsafePPM));
    failsafeSave();
  }
}

void rxReadEeprom()
{
  if (!accessEEPROM(0, false)) {
    rxInitDefaults(1);
  }
}

#endif

/** OpenLRSng binding **/

//####### COMMON FUNCTIONS #########

#define AVAILABLE    0
#define TRANSMIT    1
#define TRANSMITTED  2
#define RECEIVE    3
#define RECEIVED  4

uint8_t twoBitfy(uint16_t in);
uint8_t countSetBits(uint16_t x);
uint16_t servoUs2Bits(uint16_t x);
uint16_t servoBits2Us(uint16_t x);
uint8_t getPacketSize(struct bind_data *bd);
uint8_t getChannelCount(struct bind_data *bd);

void packChannels(uint8_t config, volatile uint16_t PPM[], uint8_t *p);
void unpackChannels(uint8_t config, volatile uint16_t PPM[], uint8_t *p);

uint32_t delayInMs(uint16_t d);
uint32_t delayInMsLong(uint8_t d);

void scannerMode(void);
void printVersion(uint16_t v);
void fatalBlink(uint8_t blinks);

void RFM22B_Int(void);
void init_rfm(uint8_t isbind);
void tx_reset(void);
void rx_reset(void);
void setHopChannel(uint8_t ch);

uint8_t tx_done(void);
void tx_packet(uint8_t* pkt, uint8_t size);
uint32_t getInterval(struct bind_data *bd);

uint32_t tx_start = 0;
volatile uint8_t RF_Mode = 0;
volatile uint32_t lastReceived = 0;
//volatile uint16_t PPM[PPM_CHANNELS] = { 512, 512, 512, 512, 512, 512, 512, 512 , 512, 512, 512, 512, 512, 512, 512, 512 };
const static uint8_t pktsizes[8] = { 0, 7, 11, 12, 16, 17, 21, 0 };

void RFM22B_Int()
{
  if (RF_Mode == TRANSMIT) {
    RF_Mode = TRANSMITTED;
  } else if (RF_Mode == RECEIVE) {
    RF_Mode = RECEIVED;
    //lastReceived = millis();
  }
}

uint8_t getPacketSize(struct bind_data *bd)
{
  return pktsizes[(bd->flags & 0x07)];
}

uint8_t getChannelCount(struct bind_data *bd)
{
  return (((bd->flags & 7) / 2) + 1 + (bd->flags & 1)) * 4;
}

uint32_t getInterval(struct bind_data *bd)
{
  uint32_t ret;
  // Sending an 'x' byte packet at 'y' bps takes approx. (emperical):
  // usec = (x + 15 {20 w/ diversity}) * 8200000 / bps
#define BYTES_AT_BAUD_TO_USEC(bytes, bps, div) ((uint32_t)((bytes) + (div?20:15)) * 8200000L / (uint32_t)(bps))

  ret = (BYTES_AT_BAUD_TO_USEC(getPacketSize(bd), modem_params[bd->modem_params].bps, bd->flags&DIVERSITY_ENABLED) + 2000);

  if (bd->flags & TELEMETRY_MASK) {
    ret += (BYTES_AT_BAUD_TO_USEC(TELEMETRY_PACKETSIZE, modem_params[bd->modem_params].bps, bd->flags&DIVERSITY_ENABLED) + 1000);
  }

  // round up to ms
  ret = ((ret + 999) / 1000) * 1000;

  // enable following to limit packet rate to 50Hz at most
#ifdef LIMIT_RATE_TO_50HZ
  if (ret < 20000) {
    ret = 20000;
  }
#endif

  return ret;
}

uint8_t twoBitfy(uint16_t in)
{
  if (in < 256) {
    return 0;
  } else if (in < 512) {
    return 1;
  } else if (in < 768) {
    return 2;
  } else {
    return 3;
  }
}

void packChannels(uint8_t config, volatile uint16_t PPM[], uint8_t *p)
{
  uint8_t i;
  for (i = 0; i <= (config / 2); i++) { // 4ch packed in 5 bytes
    p[0] = (PPM[0] & 0xff);
    p[1] = (PPM[1] & 0xff);
    p[2] = (PPM[2] & 0xff);
    p[3] = (PPM[3] & 0xff);
    p[4] = ((PPM[0] >> 8) & 3) | (((PPM[1] >> 8) & 3) << 2) | (((PPM[2] >> 8) & 3) << 4) | (((PPM[3] >> 8) & 3) << 6);
    p += 5;
    PPM += 4;
  }
  if (config & 1) { // 4ch packed in 1 byte;
    p[0] = (twoBitfy(PPM[0]) << 6) | (twoBitfy(PPM[1]) << 4) | (twoBitfy(PPM[2]) << 2) | twoBitfy(PPM[3]);
  }
}

void unpackChannels(uint8_t config, volatile uint16_t PPM[], uint8_t *p)
{
  uint8_t i;
  for (i=0; i<=(config/2); i++) { // 4ch packed in 5 bytes
    PPM[0] = (((uint16_t)p[4] & 0x03) << 8) + p[0];
    PPM[1] = (((uint16_t)p[4] & 0x0c) << 6) + p[1];
    PPM[2] = (((uint16_t)p[4] & 0x30) << 4) + p[2];
    PPM[3] = (((uint16_t)p[4] & 0xc0) << 2) + p[3];
    p+=5;
    PPM+=4;
  }
  if (config & 1) { // 4ch packed in 1 byte;
    PPM[0] = (((uint16_t)p[0] >> 6) & 3) * 333 + 12;
    PPM[1] = (((uint16_t)p[0] >> 4) & 3) * 333 + 12;
    PPM[2] = (((uint16_t)p[0] >> 2) & 3) * 333 + 12;
    PPM[3] = (((uint16_t)p[0] >> 0) & 3) * 333 + 12;
  }
}

uint16_t servoUs2Bits(uint16_t x)
{
// conversion between microseconds 800-2200 and value 0-1023
// 808-1000 == 0 - 11     (16us per step)
// 1000-1999 == 12 - 1011 ( 1us per step)
// 2000-2192 == 1012-1023 (16us per step)

  uint16_t ret;

  if (x < 800) {
    ret = 0;
  } else if (x < 1000) {
    ret = (x - 799) / 16;
  } else if (x < 2000) {
    ret = (x - 988);
  } else if (x < 2200) {
    ret = (x - 1992) / 16 + 1011;
  } else {
    ret = 1023;
  }

  return ret;
}

uint16_t servoBits2Us(uint16_t x)
{
  uint16_t ret;

  if (x < 12) {
    ret = 808 + x * 16;
  } else if (x < 1012) {
    ret = x + 988;
  } else if (x < 1024) {
    ret = 2000 + (x - 1011) * 16;
  } else {
    ret = 2192;
  }

  return ret;
}

uint8_t countSetBits(uint16_t x)
{
  x  = x - ((x >> 1) & 0x5555);
  x  = (x & 0x3333) + ((x >> 2) & 0x3333);
  x  = x + (x >> 4);
  x &= 0x0F0F;
  return (x * 0x0101) >> 8;
}

// non linear mapping
// 0 - 0
// 1-99    - 100ms - 9900ms (100ms res)
// 100-189 - 10s  - 99s   (1s res)
// 190-209 - 100s - 290s (10s res)
// 210-255 - 5m - 50m (1m res)
uint32_t delayInMs(uint16_t d)
{
  uint32_t ms;
  if (d < 100) {
    ms = d;
  } else if (d < 190) {
    ms = (d - 90) * 10UL;
  } else if (d < 210) {
    ms = (d - 180) * 100UL;
  } else {
    ms = (d - 205) * 600UL;
  }
  return ms * 100UL;
}

// non linear mapping
// 0-89    - 10s - 99s
// 90-109  - 100s - 290s (10s res)
// 110-255 - 5m - 150m (1m res)
uint32_t delayInMsLong(uint8_t d)
{
  return delayInMs((uint16_t)d + 100);
}

void init_rfm(uint8_t isbind)
{
  #ifdef SDN_pin
  digitalWrite(SDN_pin, 1);
  delay(50);
  digitalWrite(SDN_pin, 0);
  delay(50);
  #endif
  rfmSetReadyMode(); // turn on the XTAL and give it time to settle
  delayMicroseconds(600);
  rfmClearIntStatus();
  rfmInit(bind_data.flags&DIVERSITY_ENABLED);
  rfmSetStepSize(bind_data.rf_channel_spacing);

  uint32_t magic = isbind ? BIND_MAGIC : bind_data.rf_magic;
  for (uint8_t i = 0; i < 4; i++) {
    rfmSetHeader(i, (magic >> 24) & 0xff);
    magic = magic << 8; // advance to next byte
  }

  if (isbind) {
    rfmSetModemRegs(&bind_params);
    rfmSetPower(BINDING_POWER);
    rfmSetCarrierFrequency(BINDING_FREQUENCY);
  } else {
    rfmSetModemRegs(&modem_params[bind_data.modem_params]);
    rfmSetPower(bind_data.rf_power);
    rfmSetCarrierFrequency(bind_data.rf_frequency);
  }
}

void tx_reset(void)
{
  //tx_start = micros();
  RF_Mode = TRANSMIT;
  rfmSetTX();
}

void rx_reset(void)
{
  rfmClearFIFO(bind_data.flags & DIVERSITY_ENABLED);
  rfmClearIntStatus();
  RF_Mode = RECEIVE;
  rfmSetRX();
}

void check_module(void)
{
  if (rfmGetGPIO1() == 0) {
    // detect the locked module and reboot
    //Serial.println("RFM Module Locked");
    //Red_LED_ON;
    init_rfm(0);
    rx_reset();
    //Red_LED_OFF;
  }
}

void setHopChannel(uint8_t ch)
{
  uint8_t magicLSB = (bind_data.rf_magic & 0xFF) ^ ch;
  rfmSetHeader(3, magicLSB);
  rfmSetChannel(bind_data.hopchannel[ch]);
}

void tx_packet_async(uint8_t* pkt, uint8_t size)
{
  rfmSendPacket(pkt, size);
  rfmClearIntStatus();
  tx_reset();
}

void tx_packet(uint8_t* pkt, uint8_t size)
{
  tx_packet_async(pkt, size);
  //while ((RF_Mode == TRANSMIT) && ((micros() - tx_start) < 100000));
/*
  #ifdef TX_TIMING_DEBUG
  if (RF_Mode == TRANSMIT) {
    Serial.println("TX timeout!");
  }
  Serial.print("TX took:");
  Serial.println(micros() - tx_start);
  #endif
*/
}

uint8_t tx_done()
{
  if (RF_Mode == TRANSMITTED)
  {
    /*
    #ifdef TX_TIMING_DEBUG
    Serial.print("TX took:");
    Serial.println(micros() - tx_start);
    #endif
    */
    RF_Mode = AVAILABLE;
    return 1; // success
  }
  else if ((RF_Mode == TRANSMIT) && ((micros() - tx_start) > 100000))
  {
    RF_Mode = AVAILABLE;
    return 2; // timeout
  }
  return 0;
}

/*
void scannerMode(void)
{
  // Spectrum analyser 'submode'
  char c;
  uint32_t nextConfig[4] = { 0, 0, 0, 0 };
  uint32_t startFreq = MIN_RFM_FREQUENCY, endFreq = MAX_RFM_FREQUENCY, nrSamples = 500, stepSize = 50000;
  uint32_t currentFrequency = startFreq;
  uint32_t currentSamples = 0;
  uint8_t nextIndex = 0;
  uint8_t rssiMin = 0, rssiMax = 0;
  uint32_t rssiSum = 0;
  Serial.println("scanner mode");
  rx_reset();

  while (startFreq != 1000) { // if startFreq == 1000, break (used to exit scannerMode)
    while (Serial.available()) {
      c = Serial.read();

      switch (c) {
      case 'D':
        Serial.print('D');
        Serial.print(MIN_RFM_FREQUENCY);
        Serial.print(',');
        Serial.print(MAX_RFM_FREQUENCY);
        Serial.println(',');
        break;

      case 'S':
        currentFrequency = startFreq;
        currentSamples = 0;
        break;

      case '#':
        nextIndex = 0;
        nextConfig[0] = 0;
        nextConfig[1] = 0;
        nextConfig[2] = 0;
        nextConfig[3] = 0;
        break;

      case ',':
        nextIndex++;

        if (nextIndex == 4) {
          nextIndex = 0;
          startFreq = nextConfig[0] * 1000UL; // kHz -> Hz
          endFreq   = nextConfig[1] * 1000UL; // kHz -> Hz
          nrSamples = nextConfig[2]; // count
          stepSize  = nextConfig[3] * 1000UL;   // kHz -> Hz

          // set IF filtter BW (kha)
          if (stepSize < 20000) {
            spiWriteRegister(RFM22B_IFBW, 0x32);   // 10.6kHz
          } else if (stepSize < 30000) {
            spiWriteRegister(RFM22B_IFBW, 0x22);   // 21.0kHz
          } else if (stepSize < 40000) {
            spiWriteRegister(RFM22B_IFBW, 0x26);   // 32.2kHz
          } else if (stepSize < 50000) {
            spiWriteRegister(RFM22B_IFBW, 0x12);   // 41.7kHz
          } else if (stepSize < 60000) {
            spiWriteRegister(RFM22B_IFBW, 0x15);   // 56.2kHz
          } else if (stepSize < 70000) {
            spiWriteRegister(RFM22B_IFBW, 0x01);   // 75.2kHz
          } else if (stepSize < 100000) {
            spiWriteRegister(RFM22B_IFBW, 0x03);   // 90.0kHz
          } else {
            spiWriteRegister(RFM22B_IFBW, 0x05);   // 112.1kHz
          }
        }

        break;

      default:
        if ((c >= '0') && (c <= '9')) {
          c -= '0';
          nextConfig[nextIndex] = nextConfig[nextIndex] * 10 + c;
        }
      }
    }

    if (currentSamples == 0) {
      // retune base
      rfmSetCarrierFrequency(currentFrequency);
      rssiMax = 0;
      rssiMin = 255;
      rssiSum = 0;
      delay(1);
    }

    if (currentSamples < nrSamples) {
      uint8_t val = rfmGetRSSI();
      rssiSum += val;

      if (val > rssiMax) {
        rssiMax = val;
      }

      if (val < rssiMin) {
        rssiMin = val;
      }

      currentSamples++;
    } else {
      Serial.print(currentFrequency / 1000UL);
      Serial.print(',');
      Serial.print(rssiMax);
      Serial.print(',');
      Serial.print(rssiSum / currentSamples);
      Serial.print(',');
      Serial.print(rssiMin);
      Serial.println(',');
      currentFrequency += stepSize;

      if (currentFrequency > endFreq) {
        currentFrequency = startFreq;
      }

      currentSamples = 0;
    }
  }
}
*/

/*
// Print version, either x.y or x.y.z (if z != 0)
#ifdef USE_CONSOLE_SERIAL
void printVersion(uint16_t v, Serial_ *serial)
#else
void printVersion(uint16_t v, HardwareSerial *serial)
#endif
{
  if (serial) {
    serial->print(v >> 8);
    serial->print('.');
    serial->print((v >> 4) & 0x0f);
    if (version & 0x0f) {
      serial->print('.');
      serial->print(v & 0x0f);
    }
  }
}


void printVersion(uint16_t v)
{
  printVersion(v, &Serial);
}

// Halt and blink failure code
void fatalBlink(uint8_t blinks)
{
  while (1) {
    for (uint8_t i=0; i < blinks; i++) {
      Red_LED_ON;
      delay(100);
      Red_LED_OFF;
      delay(100);
    }
    delay(300);
  }
}
*/
//####### COMMON FUNCTIONS #########

#endif
