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


#include "menu_general.h"

void startModelTracker();

void startModelTracker()
{

}

#define TELEM_2ND_COLUMN (10*FW)
void displayGpsModel(uint8_t y, char direction, uint16_t bp, uint16_t ap)
{
  IF_GPS_IS_FIXED {
    if (!direction) direction = '-';
    lcdDrawNumberNAtt(5*FW/*TELEM_2ND_COLUMN*/, y, bp / 100, LEFT); // ddd before '.'
    lcdDrawChar(lcdLastPos, y, '@');
    uint8_t mn = bp % 100; // TODO div_t
    if (g_eeGeneral.gpsFormat == 0) {
      lcdDrawChar(lcdLastPos+FWNUM, y, direction);
      lcdDrawNumberNAtt(lcdLastPos+FW+FW+1, y, mn, LEFT|LEADING0, 2); // mm before '.'
      lcdDrawSolidVerticalLine(lcdLastPos, y, 2);
      uint16_t ss = ap * 6;
      lcdDrawNumberNAtt(lcdLastPos+3, y, ss / 1000, LEFT|LEADING0, 2); // ''
      lcdDrawPoint(lcdLastPos, y+FH-2, 0); // small decimal point
      lcdDrawNumberNAtt(lcdLastPos+2, y, ss % 1000, LEFT|LEADING0, 3); // ''
      lcdDrawSolidVerticalLine(lcdLastPos, y, 2);
      lcdDrawSolidVerticalLine(lcdLastPos+2, y, 2);
    } else {
      lcdDrawNumberNAtt(lcdLastPos+FW, y, mn, LEFT|LEADING0, 2); // mm before '.'
      lcdDrawPoint(lcdLastPos, y+FH-2, 0); // small decimal point
      lcdDrawNumberNAtt(lcdLastPos+2, y, ap, LEFT|UNSIGN|LEADING0, 4); // after '.'
      lcdDrawChar(lcdLastPos+1, y, direction);
    }  } else {
    // no fix
    lcdDrawText(5*FW/*TELEM_2ND_COLUMN*/, y, STR_VCSWFUNC+1/*----*/);
  }
}

void menuStatisticsView(uint8_t event)
{
  TITLE(STR_MENUSTAT);

  switch(event) {
  case EVT_KEY_FIRST(KEY_UP):
    chainMenu(menuStatisticsDebug);
    break;

  case EVT_KEY_FIRST(KEY_EXIT):
    chainMenu(menuMainView);
    break;
  }

  lcdDrawText(  1*FW, FH*0, STR_TOTTM1TM2THRTHP);
  putsTimer(    5*FW+5*FWNUM+1, FH*1, timersStates[0].val, 0, 0);
  putsTimer(   12*FW+5*FWNUM+1, FH*1, timersStates[1].val, 0, 0);

  putsTimer(    5*FW+5*FWNUM+1, FH*2, s_timeCumThr, 0, 0);
  putsTimer(   12*FW+5*FWNUM+1, FH*2, s_timeCum16ThrP/16, 0, 0);

  putsTimer(   12*FW+5*FWNUM+1, FH*0, sessionTimer, 0, 0);

#if defined(THRTRACE)
  coord_t traceRd = (s_traceCnt < 0 ? s_traceWr : 0);
  const coord_t x = 5;
  const coord_t y = 60;
  lcdDrawSolidHorizontalLine(x-3, y, MAXTRACE+3+3);
  lcdDrawSolidVerticalLine(x, y-32, 32+3);

  for (coord_t i=0; i<MAXTRACE; i+=6) {
    lcdDrawSolidVerticalLine(x+i+6, y-1, 3);
  }
  for (coord_t i=1; i<=MAXTRACE; i++) {
    lcdDrawSolidVerticalLine(x+i, y-s_traceBuf[traceRd], s_traceBuf[traceRd]);
    traceRd++;
    if (traceRd>=MAXTRACE) traceRd = 0;
    if (traceRd==s_traceWr) break;
  }
#endif
}

void menuStatisticsDebug(uint8_t event)
{
  TITLE(STR_MENUDEBUG);

  switch(event) {
  case EVT_KEY_FIRST(KEY_ENTER):
    g_tmr1Latency_min = -1;
    g_tmr1Latency_max = 0;
    g_guibuild_min = -1;
    g_guibuild_max = 0;
    g_lcddraw_min = -1;
    g_lcddraw_max = 0;
    maxMixerDuration  = 0;
    AUDIO_KEYPAD_UP();
    break;

  case EVT_KEY_FIRST(KEY_UP):
    chainMenu(menutModelTracker);
    break;
  case EVT_KEY_FIRST(KEY_DOWN):
    chainMenu(menuStatisticsView);
    break;
  case EVT_KEY_FIRST(KEY_EXIT):
    chainMenu(menuMainView);
    break;
  }

#define COLDEBUG1 10*FW
#define COLDEBUG2 15*FW
#define COLDEBUG3 19*FW
#define OFSDEBUG  3*FW

  lcdDrawTextLeft(1*FH, STR_COMPUTE);
  lcdDrawText(COLDEBUG1, 1*FH, STR_MAX);
  lcdDrawText(COLDEBUG2, 1*FH, STR_MIN);

  lcdDrawText(FW/2, 2*FH, STR_PROTOCOL);
  lcdDrawNumberNAtt(COLDEBUG1+OFSDEBUG, 2*FH, (g_tmr1Latency_max/2), UNSIGN);
  lcdDrawNumberNAtt(COLDEBUG2+OFSDEBUG, 2*FH, (g_tmr1Latency_min/2), UNSIGN);

  lcdDrawText(FW/2, 3*FH, STR_GUIBUILD);
  lcdDrawNumberNAtt(COLDEBUG1+OFSDEBUG, 3*FH, DURATION_MS_PREC2(g_guibuild_max), PREC2);
  lcdDrawNumberNAtt(COLDEBUG2+OFSDEBUG, 3*FH, DURATION_MS_PREC2(g_guibuild_min), PREC2);

  lcdDrawText(FW/2, 4*FH, STR_LCDDRAW);
  lcdDrawNumberNAtt(COLDEBUG1+OFSDEBUG, 4*FH, DURATION_MS_PREC2(g_lcddraw_max), PREC2);
  lcdDrawNumberNAtt(COLDEBUG2+OFSDEBUG, 4*FH, DURATION_MS_PREC2(g_lcddraw_min), PREC2);

  lcdDrawText(FW/2, 5*FH, STR_MIXERlowcase);
  lcdDrawNumberNAtt(COLDEBUG2+OFSDEBUG, 5*FH, DURATION_MS_PREC2(maxMixerDuration), PREC2);
  lcdDrawText(FW/2, 6*FH, STR_FREESRAM);
  lcdDrawNumberNAtt(COLDEBUG2+OFSDEBUG, 6*FH, stackAvailable(), UNSIGN);

//  lcdDrawTextLeft(6*FH, STR_FREERAMINB);
//  lcdDrawNumberNAtt(14*FW, 6*FH, freeRam(), UNSIGN);

  lcdDrawText(4*FW, 7*FH+1, STR_MENUTORESET);
  lcd_status_line();
}

#define TR_LAT           		 "Lat:"
extern const pm_char STR_LAT[];
const pm_char STR_LAT[] PROGMEM = TR_LAT;
#define TR_LON           		 "Lon:"
extern const pm_char STR_LON[];
const pm_char STR_LON[] PROGMEM = TR_LON;

#define TR_SEARCHMODEL           CENTER TR_ENTER" Search Model"
extern const pm_char STR_SEARCHMODEL[];
const pm_char STR_SEARCHMODEL[] PROGMEM = TR_SEARCHMODEL;

#define TR_MENUTRACKER           "TRACKER"
extern const pm_char STR_MENUTRACKER[];
const pm_char STR_MENUTRACKER[] PROGMEM = TR_MENUTRACKER;

void menutModelTracker(uint8_t event)
{
  TITLE(STR_MENUTRACKER);

  switch(event) {
  case EVT_KEY_FIRST(KEY_ENTER):
    /*
    g_tmr1Latency_min = -1;
    g_tmr1Latency_max = 0;
    g_guibuild_min = -1;
    g_guibuild_max = 0;
    g_lcddraw_min = -1;
    g_lcddraw_max = 0;
    maxMixerDuration  = 0;
    */
    startModelTracker();
    AUDIO_KEYPAD_UP();
    break;

  case EVT_KEY_FIRST(KEY_DOWN):
    chainMenu(menuStatisticsDebug);
    break;
  case EVT_KEY_FIRST(KEY_EXIT):
    chainMenu(menuMainView);
    break;
  }

  uint8_t line=1*FH+1;

  // Latitude du modèle
  lcdDrawTextLeft(line, STR_LAT);
  displayGpsModel(line, telemetryData.value.gpsLatitudeNS, telemetryData.value.gpsLatitude_bp, telemetryData.value.gpsLatitude_ap);
  // Longitude du modèle
  line+=1*FH+1;
  lcdDrawTextLeft(line, STR_LON);
  displayGpsModel(line, telemetryData.value.gpsLongitudeEW, telemetryData.value.gpsLongitude_bp, telemetryData.value.gpsLongitude_ap);

  line+=1*FH+1;
  //Altitude du modèle
  lcdDrawTextAtIndex(0, line, STR_VTELEMCHNS, TELEM_GPSALT, 0);
  lcdPutsTelemetryChannelValue(TELEM_2ND_COLUMN-2*FW, line, TELEM_GPSALT-1, telemetryData.value.gpsAltitude, 0);
  //Vitesse du modèle
  //lcdDrawTextAtIndex(TELEM_2ND_COLUMN+FW, line, STR_VTELEMCHNS, TELEM_SPEED, 0);
  //lcdPutsTelemetryChannelValue(LCD_W-3*FW, line, TELEM_SPEED-1, telemetryData.value.gpsSpeed_bp, 0);
  line+=1*FH+1;
  //Distance parcourue par le modèle
  lcdDrawTextAtIndex(0, line, STR_VTELEMCHNS, TELEM_DIST, 0);
  lcdPutsTelemetryChannelValue(TELEM_2ND_COLUMN-2*FW, line, TELEM_DIST-1, telemetryData.value.gpsDistance, 0);
  line+=1*FH+1;
  //Cap suivi par le modèle
  lcdDrawTextAtIndex(0, line, STR_VTELEMCHNS, TELEM_HDG, 0);
  lcdPutsTelemetryChannelValue(TELEM_2ND_COLUMN-2*FW, line, TELEM_HDG-1, telemetryData.value.gpsCourse_bp, 0);
  line+=1*FH+1;

  lcdDrawText(4*FW, 7*FH+1, STR_SEARCHMODEL);
  lcd_status_line();
}
