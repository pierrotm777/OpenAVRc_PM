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
 * PROTO_PPMSIM uses PB5 OC1A
 * 16 Bit Timer running @ 16MHz has a resolution of 0.5us.
 * This should give a PPM resolution of 2048.
*/
static uint16_t PROTO_PPMSIM_cb()
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
    SCHEDULE_MIXER_END_IN_US(22500 + (g_model.PPMFRAMELENGTH * 1000) / 2);
    setupPulsesPPM(PPMSIM);
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


static void PROTO_PPMSIM_reset()
{
  if(g_model.PULSEPOL) PORTB &= ~PIN5_bm;
  else PORTB |= PIN5_bm;
  TCCR1A &= ~(0b11<<COM1A0);
  PROTO_Stop_Callback();
  TIMSK1 &= ~(1<<OCIE1A); // Disable Output Compare A interrupt.
  TIFR1 |= 1<<OCF1A; // Reset Flag.
#if defined(BLUETOOTH)
  uCli.Context = CONTEXT_UCLI;
#endif
}


static void PROTO_PPMSIM_initialize()
{
  PPM16_CONF();

#if defined(FRSKY)
  telemetryPPMInit();
#endif
#if defined(BLUETOOTH)
  uCli.Context = CONTEXT_PUPPY;
#endif

  RptrA = &pulses2MHz.pword[PULSES_WORD_SIZE/2];
  *RptrA = 0;
  PROTO_Start_Callback( &PROTO_PPMSIM_cb);
}


const void * PROTO_PPMSIM_Cmds(enum ProtoCmds cmd)
{
  switch(cmd) {
    case PROTOCMD_INIT: PROTO_PPMSIM_initialize();
    return 0;
    case PROTOCMD_RESET:
      PROTO_PPMSIM_reset();
      return 0;
  case PROTOCMD_GETOPTIONS:
     sendOptionsSettingsPpm();
     return 0;
        default: break;
  }
  return 0;
}

