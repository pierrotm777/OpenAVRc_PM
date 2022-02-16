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


/*
 * PROTO_PPM16 uses PB6 OC1B for channels 1-8, PB5 OC1A for channels 9-16.
 * 16 Bit Timer running @ 16MHz has a resolution of 0.5us.
 * This should give a PPM resolution of 2048.
*/
static uint16_t PROTO_PPM16A_cb()
{
  if(*RptrA == 0) { // End of timing events.
    RptrA = &pulses2MHz.pword[PULSES_WORD_SIZE/2];
    // Set the PPM idle level.
    if (g_model.PULSEPOL) {
      TCCR1A = (TCCR1A | (1<<COM1A1)) & ~(1<<COM1A0); // Clear
    }
    else {
      TCCR1A |= (0b11<<COM1A0); // Set
    }
    TCCR1C = 1<<FOC1A; // Strobe FOC1A.
    TCCR1A = (TCCR1A | (1<<COM1A0)) & ~(1<<COM1A1); // Toggle OC1x on next match.

    // Schedule next Mixer calculations.
    SCHEDULE_MIXER_END_IN_US(22500 + ((g_model.PPMFRAMELENGTH * 1000) / 2));
    setupPulsesPPM(PPM16LAST); // Channels 8-16.
    heartbeat |= HEART_TIMER_PULSES;
    CALCULATE_LAT_JIT(); // Show how long to setup pulses and ISR jitter.
    return PULSES_SETUP_TIME_US *2;
  }
  else if (*(RptrA +1) == 0) { // Look ahead one timing event.
    // Need to prevent next toggle.
    // Read pin and store before disconnecting switching output.
    if(PINB & PIN5_bm) PORTB |= PIN5_bm;
    else PORTB &= ~PIN5_bm;
    TCCR1A &= ~(0b11<<COM1A0);
    return *RptrA++;
  }
  else // Toggle pin.
    return *RptrA++;
}



static uint16_t PROTO_PPM16B_cb2()
{
  if(*RptrB == 0) { // End of timing events.
    RptrB = &pulses2MHz.pword[0];
    // Set the PPM idle level.
    if (g_model.PULSEPOL) {
      TCCR1A = (TCCR1A | (1<<COM1B1)) & ~(1<<COM1B0); // Clear
    }
    else {
      TCCR1A |= (0b11<<COM1B0); // Set
    }
    TCCR1C = 1<<FOC1B; // Strobe FOC1.
    TCCR1A = (TCCR1A | (1<<COM1B0)) & ~(1<<COM1B1); // Toggle OC1x on next match.

    setupPulsesPPM(PPM16FIRST);
    heartbeat |= HEART_TIMER_PULSES;
    dt = TCNT1 - OCR1B; // Show how long to setup pulses and ISR jitter.
    return PULSES_SETUP_TIME_US *2;
  }
  else if (*(RptrB +1) == 0) { // Look ahead one timing event.
    // Need to prevent next toggle.
    // Read pin and store before disconnecting switching output.
    if(PINB & PIN6_bm) PORTB |= PIN6_bm;
    else PORTB &= ~PIN6_bm;
    TCCR1A &= ~(0b11<<COM1B0);
    return *RptrB++;
  }
  else // Toggle pin.
    return *RptrB++;
}


void PROTO_PPM16B_cb1()
{
uint16_t half_us = PROTO_PPM16B_cb2();

  if(! half_us) {
    PROTO_PPM_Cmds(PROTOCMD_RESET);
    return;
  }

//  dt = TCNT1 - OCR1B; // Calculate latency and jitter.
  if(dt > g_tmr1Latency_max) g_tmr1Latency_max = dt;
  if(dt < g_tmr1Latency_min) g_tmr1Latency_min = dt;

  OCR1B += half_us;
}


static void PROTO_PPM16_reset()
{
  // Make pin idle state before disconnecting switching output.
  if(g_model.PULSEPOL) PORTB &= ~PIN6_bm;
  else PORTB |= PIN6_bm;
  TCCR1A &= ~(0b11<<COM1B0);
  TIMSK1 &= ~(1<<OCIE1B); // Disable Output Compare B interrupt.
  if(g_model.PULSEPOL) PORTB &= ~PIN5_bm;
  else PORTB |= PIN5_bm;
  TCCR1A &= ~(0b11<<COM1A0);
  TIFR1 |= 1<<OCF1B | 1<<OCF1A; // Reset Flag.
  PROTO_Stop_Callback();
  WAIT_PUPIL();
}

static void PROTO_PPM16_initialize()
{
  PPM16_CONF();

#if defined(FRSKY)
  telemetryPPMInit();
#endif

  RptrA = &pulses2MHz.pword[PULSES_WORD_SIZE/2];
  *RptrA = 0;
  PROTO_Start_Callback( &PROTO_PPM16A_cb);


  RptrB = &pulses2MHz.pword[0];
  *RptrB = 0;

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    ocr1b_function_ptr = PROTO_PPM16B_cb1; // Setup function pointer used in ISR.
    OCR1B = TCNT1 + (16000U *2);
    TIFR1 |= 1<<OCF1B; // Reset Flag.
    TIMSK1 |= 1<<OCIE1B; // Enable Output Compare interrupt.
  }
}


const void * PROTO_PPM16_Cmds(enum ProtoCmds cmd)
{
  switch(cmd) {
    case PROTOCMD_INIT: PROTO_PPM16_initialize();
    return 0;
    case PROTOCMD_RESET:
      PROTO_PPM16_reset();
    return 0;
  case PROTOCMD_GETOPTIONS:
     sendOptionsSettingsPpm();
     return 0;
        default: break;
  }
  return 0;
}

