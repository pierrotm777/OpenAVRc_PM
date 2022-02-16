#ifndef PXX_h
#define PXX_h



void PXX_begin();
void PXX_send();
void PXX_prepare(int16_t channels[16]);


uint8_t  PXX_pulses[64];
int PXX_length = 0;
uint16_t PXX_pcmCrc;
uint32_t PXX_pcmOnesCount;
uint16_t PXX_serialByte;
uint16_t PXX_serialBitCount;
bool PXX_sendUpperChannels;

void PXX_crc(uint8_t data);
void PXX_putPcmSerialBit(uint8_t bit);
void PXX_putPcmPart(uint8_t value);
void PXX_putPcmFlush();
void PXX_putPcmBit(uint8_t bit);
void PXX_putPcmByte(uint8_t byte);
void PXX_putPcmHead();
uint16_t PXX_limit(uint16_t low,uint16_t val, uint16_t high);
void PXX_preparePulses(int16_t channels[8]);

#endif
