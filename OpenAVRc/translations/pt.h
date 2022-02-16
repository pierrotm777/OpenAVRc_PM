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
/* Formatting octal codes available in TR_ strings:
*  \037\x           -sets LCD x-coord (x value in octal)
*  \036             -newline
*  \001 to \034     -extended spacing (value * FW/2)
*  \0               -ends current string
*/

/* NON ZERO TERMINATED STRINGS */
#define LEN_OFFON        "\003"
#define TR_OFFON         "OFF""ON\0"
#define LEN_MMMINV       "\003"
#define TR_MMMINV        "---""INV"
#define LEN_NCHANNELS    "\004"
#define TR_NCHANNELS     "\0014CH\0016CH\0018CH10CH12CH14CH16CH"
#define LEN_VBEEPMODE    "\005"
#define TR_VBEEPMODE     "Mudo ""Alarm""NoKey""Todo "
#define LEN_VRENAVIG     "\003"
#define TR_VRENAVIG      "No REaREb"
#define LEN_VBLMODE      "\004"
#define TR_VBLMODE       "OFF ""Chav""Stks""Tudo""ON\0"
#define LEN_TRNMODE      "\003"
#define TR_TRNMODE       "OFF"" +="" :="
#define LEN_TRNCHN       "\003"
#define TR_TRNCHN        "CH1CH2CH3CH4"
#define LEN_VPERSISTENT        "\014"
#define TR_VPERSISTENT         "OFF\0        ""Flight\0     ""Manual Reset"
#define LEN_DSM_PROTOCOLS      "\004"
#define TR_DSM_PROTOCOLS       "LP45""DSM2""DSMX"
#define LEN_VTRIMINC     "\006"
#define TR_VTRIMINC      "Expo  ""ExFino""Fino  ""Medio ""Largo "
#define LEN_CURVE_TYPES        "\010"
#define TR_CURVE_TYPES         "Standard""Custom\0"
#define LEN_RETA123      "\001"
#define TR_RETA123         "LPMA123ab"

#if defined(PWM_BACKLIGHT)
#define TR_BLONBRIGHTNESS      INDENT "ON Brightn."
#define TR_BLOFFBRIGHTNESS     INDENT "OFF Brightn."
#endif
#if (SERIAL_PROTOCOL==DSM)
 #define TR_DSM2        "LP45\0 ""DSM2\0 ""DSMX\0 "
#else
 #define TR_DSM2
#endif
#if defined(SPIMODULES)
 #define TR_SPIM               "SPIRfMod"

 #define TR_RFPOWER INDENT     "RF Power"
#endif
#if (SERIAL_PROTOCOL==MULTIMODULE)
 #define LEN_MULTIPROTOCOLS    "\006"
 #define TR_MULTIPROTOCOLS     "FlySky""Hubsan""FrSky\0""Hisky\0""V2x2\0 ""DSM\0  ""Devo\0 ""YD717\0""KN\0   ""SymaX\0""SLT\0  ""CX10\0 ""CG023\0""Bayang""ESky\0 ""MT99XX""MJXQ\0 ""Shenqi""FY326\0""SFHSS\0""J6 PRO""FQ777\0""Assan\0""Hontai""OLRS\0 ""FS 2A\0""Q2x2\0 ""Walk.\0""Q303\0 ""GW008\0""DM002\0"
 #define TR_MULTI_CUSTOM       "Custom"
#endif
#if ((SERIAL_PROTOCOL==SUMD) || (SERIAL_PROTOCOL==SBUS))
#define TR_PERIOD                INDENT "Periodo"
#endif
#if (SERIAL_PROTOCOL==MULTIMODULE) || defined(SPIMODULES)
 #define TR_RFTUNEFINE                  INDENT "Freq.fine"
 #define TR_SUBTYPE                     "SubType"
 #define TR_TELEMETRY                   "Telemetry"
 #define TR_MULTI_VIDFREQ               INDENT "Vid. freq."
 #define TR_MULTI_RFPOWER               INDENT "RF Power"
 #define TR_MULTI_OPTION                INDENT "Option"
 #define TR_AUTOBIND                    INDENT "Autobind"
 #define TR_MULTI_DSM_AUTODTECT         INDENT "Autodetect"
 #define TR_MULTI_LOWPOWER              INDENT "Low power"
 #define TR_MULTI_SERVOFREQ             INDENT "Servo rate"
#endif
#define LEN_VCURVEFUNC   "\003"
#define TR_VCURVEFUNC    "---""x>0""x<0""|x|""f>0""f<0""|f|"
#define LEN_VMLTPX       "\011"
#define TR_VMLTPX        "Adicionar""Multipl. ""Trocar\0 0"
#define LEN_VMLTPX2      "\002"
#define TR_VMLTPX2       "+=""*="":="
#define LEN_VMIXTRIMS    "\003"
#define TR_VMIXTRIMS     "OFF""ON\0""Lem""Pfd""Mot""Ail"
#define TR_CSWTIMER          "Tim\0 "
#define TR_CSWSTICKY         "Glue\0"
#define TR_CSWRANGE
#define TR_CSWSTAY
#define LEN_VCSWFUNC     "\005"
#define TR_VCSWFUNC      "---\0 " "a=x\0 " "a\173x\0 ""a>x\0 ""a<x\0 " TR_CSWRANGE "|a|>x""|a|<x""AND\0 ""OR\0  ""XOR\0 " TR_CSWSTAY "a=b\0 ""a>b\0 ""a<b\0 ""^}x\0 ""|^|}x" TR_CSWTIMER TR_CSWSTICKY
#define LEN_VFSWFUNC     "\015"
#if defined(VARIO)
 #define TR_VVARIO        "Vario\0       "
#else
 #define TR_VVARIO        "[Vario]\0     "
#endif
#if defined(AUDIO)
 #define TR_SOUND         "Tocar Audio\0 "
#else
 #define TR_SOUND         "Beep\0        "
#endif
#if defined(HAPTIC)
 #define TR_HAPTIC        "Vibrar\0      "
#else
 #define TR_HAPTIC        "[Vibrar]\0    "
#endif
#if defined(VOICE)
 #define TR_PLAY_TRACK    "Tocar\0       "
 #define TR_PLAY_BOTH       "Play Both\0   "
 #define TR_PLAY_VALUE      "Ouvir Valor\0 "
#else
 #define TR_PLAY_TRACK      "[Tocar Pista]"
 #define TR_PLAY_BOTH       "[Play Both]\0 "
 #define TR_PLAY_VALUE      "[Tocar Valor]"
#endif
#if defined(SDCARD)
 #define TR_SDCLOGS       "SDCARD Logs\0 "
#else
 #define TR_SDCLOGS       "[SDCARD Logs]"
#endif
#ifdef GVARS
 #define TR_ADJUST_GVAR  "Ajuste\0      "
#else
 #define TR_ADJUST_GVAR  "[AdjustGV]\0  "
#endif

#if defined(OVERRIDE_CHANNEL_FUNCTION)
 #define TR_SF_SAFETY        "Seguro\0      "
#else
 #define TR_SF_SAFETY        "---\0         "
#endif

#define TR_VFSWFUNC          TR_SF_SAFETY "Aprendiz\0    ""Ajuste Rapido""Reset\0       " TR_ADJUST_GVAR TR_SOUND TR_PLAY_TRACK TR_PLAY_BOTH TR_PLAY_VALUE TR_VVARIO TR_HAPTIC TR_SDCLOGS "Backlight\0   "
#define LEN_VFSWRESET          "\004"
#if defined(FRSKY)
 #define TR_FSW_RESET_TELEM   "Telm"
 #define TR_MODELISON         INDENT "Model is ON"
#else
 #define TR_FSW_RESET_TELEM
#endif
#define TR_FSW_RESET_TIMERS  "Tmr1""Tmr2"
#define TR_FSW_RESET_ROTENC  "REa\0""REb\0"
#define TR_VFSWRESET           TR_FSW_RESET_TIMERS "All\0" TR_FSW_RESET_TELEM TR_FSW_RESET_ROTENC
#define LEN_FUNCSOUNDS         "\004"
#define TR_FUNCSOUNDS          "Bp1\0""Bp2\0""Bp3\0""Wrn1""Wrn2""Chee""Rata""Tick""Sirn""Ring""SciF""Robt""Chrp""Tada""Crck""Alrm"
#define LEN_VTELEMCHNS         "\004"
#define TR_TELEM_RESERVE
#define TR_TELEM_TIME
#define TR_SWR
#define TR_RX_BATT
#define TR_A3_A4
#define TR_A3_A4_MIN
#define TR_ASPD_MAX            "ASp+"
#define TR_TELEM_RSSI_RX     "Rx\0  "
#define TR_TELEM_TIMERS      "Tmr1""Tmr2"
#define TR_VTELEMCHNS        "---\0""Batt" TR_TELEM_TIME TR_TELEM_RESERVE TR_TELEM_RESERVE TR_TELEM_RESERVE TR_TELEM_RESERVE TR_TELEM_RESERVE TR_TELEM_TIMERS TR_SWR "Tx\0 " TR_TELEM_RSSI_RX TR_RX_BATT "A1\0 ""A2\0 " TR_A3_A4 "Alt\0""Rpm\0""Fuel""T1\0 ""T2\0 ""Spd\0""Dist""GAlt""Cell""Cels""Vfas""Curr""Cnsp""Powr""AccX""AccY""AccZ""Hdg\0""VSpd""ASpd""dTE\0" TR_TELEM_RESERVE TR_TELEM_RESERVE TR_TELEM_RESERVE TR_TELEM_RESERVE TR_TELEM_RESERVE "A1-\0""A2-\0" TR_A3_A4_MIN "Alt-""Alt+""Rpm+""T1+\0""T2+\0""Spd+""Dst+" TR_ASPD_MAX "Cel-""Cls-""Vfs-""Cur+""Pwr+" TR_TELEM_RESERVE TR_TELEM_RESERVE TR_TELEM_RESERVE TR_TELEM_RESERVE TR_TELEM_RESERVE "Acc\0""Time"
#define LENGTH_UNIT_IMP        "ft\0"
#define SPEED_UNIT_IMP         "mph"
#define LENGTH_UNIT_METR       "m\0 "
#define SPEED_UNIT_METR        "kmh"
#if defined(IMPERIAL_UNITS)
 #define LENGTH_UNIT        LENGTH_UNIT_IMP
 #define SPEED_UNIT         SPEED_UNIT_IMP
#else
 #define LENGTH_UNIT        LENGTH_UNIT_METR
 #define SPEED_UNIT         SPEED_UNIT_METR
#endif
#define LEN_VTELEMUNIT       "\003"
#define TR_VTELEMUNIT        "V\0 ""A\0 ""m/s""-\0 " SPEED_UNIT LENGTH_UNIT "@\0 ""%\0 ""mA\0""mAh""W\0 "
#define STR_V            (STR_VTELEMUNIT+1)
#define STR_A            (STR_VTELEMUNIT+4)
#define LEN_VTELPROTO    "\007"
#define TR_VTELPROTO     "Nada\0  ""Hub\0   ""WSHHigh""S.Port\0"
#define LEN_AMPSRC           "\003"
#define TR_AMPSRC            "---""A1\0""A2\0""FAS""Cel"
#define LEN_VARIOSRC     "\005"
#if defined(FRSKY_SPORT)
 #define TR_VARIOSRC          "Vario""A1\0  ""A2\0"
#else
 #define TR_VARIOSRC          "Alti\0""Alti+""Vario""A1\0  ""A2\0"
#endif
#define LEN_VTELEMSCREENTYPE      "\004"
#define TR_VTELEMSCREENTYPE       "Nums""Bars"
#define LEN_GPSFORMAT    "\004"
#define TR_GPSFORMAT     "HMS NMEA"
#define LEN2_VTEMPLATES  13
#define LEN_VTEMPLATES   "\015"
#define TR_VTEMPLATES    "Tirar Mixagem""4CH Simples\0 ""Anular Motor\0""V-Tail\0      ""Elevon\\Zagi\0 ""eCCPM\0       ""Mixagem Heli\0""Testar Servos"
#define LEN_VSWASHTYPE   "\004"
#define TR_VSWASHTYPE    "--- ""120 ""120X""140 ""90\0"
#define LEN_VKEYS        "\005"
#define TR_VKEYS         "Menu\0""Sair\0""Desce""Sobe\0""Direi""Esqda"
#define LEN_VRENCODERS   "\003"
#define TR_VRENCODERS    "REa""REb"
#define TR_POTS_VSRCRAW      "P1\0 ""P2\0 ""P3\0 "
#define TR_SW_VSRCRAW        "3POS"
#define TR_9X_3POS_SWITCHES  "ID0""ID1""ID2"
#define TR_LOGICALSW          "L1\0""L2\0""L3\0""L4\0""L5\0""L6\0""L7\0""L8\0""L9\0""L10""L11""L12""L13""L14""L15""L16""L17""L18""L19""L20"
#define TR_TRIMS_SWITCHES      "tRl""tRr""tEd""tEu""tTd""tTu""tAl""tAr"
#define TR_ROTARY_ENCODERS   "REa\0""REb\0"
#define TR_ROTENC_SWITCHES   "REA""REB""REN"
#define TR_PHYS_SWITCHES     "THR""RUD""ELE""AIL""GEA""TRN"
#define TR_ON_ONE_SWITCHES     "ON\0""One"
#define TR_EXTRA_3POS_SWITCHES  "XD0""XD1""XD2"
#define LEN_VSRCRAW            "\004"
#define LEN_VSWITCHES          "\003"
#define TR_VSWITCHES         "---" TR_9X_3POS_SWITCHES TR_PHYS_SWITCHES TR_TRIMS_SWITCHES TR_ROTENC_SWITCHES TR_EXTRA_3POS_SWITCHES TR_LOGICALSW TR_ON_ONE_SWITCHES
#if defined(HELI)
 #define TR_CYC_VSRCRAW   "CYC1""CYC2""CYC3"
#else
 #define TR_CYC_VSRCRAW   "[C1]""[C2]""[C3]"
#endif
#define TR_VSRCRAW       "---\0""Lem\0""Pfd\0""Mot\0""Ail\0" TR_POTS_VSRCRAW TR_ROTARY_ENCODERS "MAX\0" TR_CYC_VSRCRAW "TrmL" "TrmP" "TrmM" "TrmA" TR_SW_VSRCRAW
#define LEN_VTMRMODES    "\003"
#define TR_VTMRMODES     "OFF""ABS""MTs""MT%""MTt"

/* ZERO TERMINATED STRINGS */
#define INDENT                 "\001"
#define LEN_INDENT             1
#define INDENT_WIDTH           (FW/2)
#define TR_ENTER             "[MENU]"
#define TR_EXIT                "[EXIT]"
#define TR_POPUPS            TR_ENTER "\010" TR_EXIT
#define OFS_EXIT             sizeof(TR_ENTER)
#define TR_MENUWHENDONE      CENTER"\006" TR_ENTER "QDO PRONTO"
#define TR_FREE                "Livre"
#define TR_DELETEMODEL         "EXCLUI MODELO"
#define TR_COPYINGMODEL        "Copiando modelo"
#define TR_MOVINGMODEL         "Movendo modelo"
#define TR_LOADINGMODEL        "Carregar modelo"
#define TR_NAME                "Nome"

#define TR_TIMER               "Cronom"
#define TR_ELIMITS             "E.Limits"
#define TR_ETRIMS              "E.Trims"
#define TR_TRIMINC             "Trim Step"
#define TR_TTRACE              "T-Trace"
#define TR_TSWITCH             "T-Switch"
#define TR_TTRIM               "T-Trim"
#define TR_BEEPCTR             "Ctr Beep"
#define TR_PROTO               INDENT"Proto"
#define TR_PPMFRAME            "PPM frame"
#define TR_MS                  "ms"
#define TR_SWITCH              "Chave"
#define TR_TRIMS               "Trims"
#define TR_FADEIN              "Aparecer"
#define TR_FADEOUT             "Ocultar"
#define TR_DEFAULT             "(default)"
#define TR_CHECKTRIMS          "\006Check\012Trims"
#define OFS_CHECKTRIMS         (9*FW)
#define TR_SWASHTYPE           "Ciclico Tipo"
#define TR_COLLECTIVE          "Coletivo"
#define TR_SWASHRING           "Anel Coletivo"
#define TR_ELEDIRECTION        "PFD Sentido"
#define TR_AILDIRECTION        "AIL Sentido"
#define TR_COLDIRECTION        "COL Sentido"
#define TR_MODE                INDENT"Modo"
#define TR_NOFREEEXPO          "No free expo!"
#define TR_NOFREEMIXER         "No free mixer!"
#define TR_INSERTMIX           "INSERIR MIX"
#define TR_EDITMIX             "EDITAR MIX"
#define TR_SOURCE              INDENT"Fonte"
#define TR_WEIGHT              "Quantia"
#define TR_EXPO                "Exponen"
#define TR_SIDE                "Lado"
#define TR_DIFFERENTIAL        "Diferencial"
#define TR_OFFSET              INDENT"Desvio"
#define TR_TRIM                "Trim"
#define TR_DREX                "DRex"
#define DREX_CHBOX_OFFSET      30
#define TR_CURVE               "Curvas"
#define TR_FLMODE              "Fase"
#define TR_MIXWARNING          "Aviso"
#define TR_OFF                 "OFF"
#define TR_MULTPX              "Aplicar"
#define TR_DELAYDOWN           "Atraso Dn"
#define TR_DELAYUP             "Atraso Up"
#define TR_SLOWDOWN            "Lento  Dn"
#define TR_SLOWUP              "Lento  Up"
#define TR_MIXER               "MIXAGENS"
#define TR_CV                  "CV"
#define TR_GV                  "GV"
#define TR_ACHANNEL            "A\004Canal"
#define TR_RANGE               INDENT"Range"
#define TR_ALARM               INDENT"Alarme"
#define TR_USRDATA             "UsrData"
#define TR_BLADES              INDENT"Helice"
#define TR_SCREEN              "Tela"
#define TR_SOUND_LABEL         "Som"
#define TR_LENGTH              INDENT"Comprimento"
#define TR_BEEP_LENGTH         INDENT "Beep Length"
#define TR_SPKRPITCH           INDENT"Pitch"
#define TR_HAPTIC_LABEL        "Vibrar"
#define TR_HAPTICSTRENGTH      INDENT"Forca"
#define TR_CONTRAST            "Contraste"
#define TR_ALARMS_LABEL        "Alarmes"
#define TR_BATTERY_RANGE       "Battery Range"
#define TR_BATTERYWARNING      INDENT"Bateria Baixa"
#define TR_INACTIVITYALARM     INDENT"Inactividade"
#define TR_MEMORYWARNING       INDENT"Memoria Baixa"
#define TR_ALARMWARNING        INDENT"Som Off"
#define TR_RENAVIG             "RotEnc Navig"
#define TR_MINUTEBEEP          INDENT "Beep Minuto"
#define TR_BEEPCOUNTDOWN       INDENT "Beep Regressivo"
#define TR_PERSISTENT          INDENT "Persist."
#define TR_BACKLIGHT_LABEL     "Backlight"
#define TR_BLDELAY             INDENT "Tempo Backlight"
#define TR_SPLASHSCREEN        "Splash screen"
#define TR_THROTTLEWARNING     "Avisa Acel"
#define TR_SWITCHWARNING       "Avisa Chav"
#define TR_TIMEZONE            "Time Zone"
#define TR_ADJUST_RTC          "Adjust RTC"
#define TR_GPS                 "GPS"
#define TR_RXCHANNELORD        "Ordem Canal RX"
#define TR_STICKS              "Sticks"
#define TR_POTS                "Pots"
#define TR_SLAVE               "Escravo"
#define TR_MODESRC             "Modo\006% Fonte"
#define TR_MULTIPLIER          "Multiplicar"
#define TR_CAL                 "Cal"
#define TR_VTRIM               "Trim- +"
#define TR_MENUTOSTART         CENTER"\011" TR_ENTER " INICIAR"
#define TR_SETMIDPOINT         CENTER"\005CENTRAR STICK/POT"
#define TR_MOVESTICKSPOTS      CENTER"\005MOVER STICKS/POTs"
#define TR_RXBATT              "Rx Batt:"
#define TR_TXnRX               "Tx:\0Rx:"
#define OFS_RX                 4
#define TR_ACCEL               "Acc:"
#define TR_NODATA              CENTER"SEM DADOS"
#define TR_TOTTM1TM2THRTHP     "\037\146SES\036TM1\037\146TM2\036THR\037\146TH%"
#define TR_US                  "us"
#define TR_MENUTORESET         CENTER TR_ENTER" Reinicia"
#define TR_PPM_TRAINER         "TR"
#define TR_CH                  "CH"
#define TR_MODEL               "MODEL"
#define TR_FP                  "FP"
#define TR_MIX                 "MIX"
#define TR_EEPROMLOWMEM        "EEPROM BX Memoria"
#define TR_ALERT               "\016ALERT"
#define TR_PRESSANYKEYTOSKIP   "Pulsar Tecla p/ sair"
#define TR_THROTTLENOTIDLE     "**ACELERADOR ATIVO**"
#define TR_ALARMSDISABLED      "Desabilita Alarmes"
#define TR_PRESSANYKEY         "\010Pressione Tecla"
#define TR_BADEEPROMDATA       "EEPRON INVALIDA"
#define TR_EEPROMFORMATTING    "Formatando EEPROM"
#define TR_EEPROMOVERFLOW      "EEPROM CHEIA"
#define TR_MENURADIOSETUP      "AJUSTAR RADIO"
#define TR_MENUTRAINER         "TRAINER"
#define TR_MENUVERSION         "VERSAO"
#define TR_MENUDIAG            "DIAGNOST"
#define TR_MENUANA             "ANALOGICOS"
#define TR_MENUCALIBRATION     "CALIBRAGEM"
#define TR_TRIMS2OFFSETS       "\006Trims => Offsets"
#define TR_MENUMODELSEL        "MODELOS"
#define TR_MENUSETUP           "SETUP"
#define TR_MENUFLIGHTPHASE     "MODO DE VOO"
#define TR_MENUFLIGHTPHASES    "MODOS DE VOO"
#define TR_MENUHELISETUP       "HELI SETUP"
#if defined(PPM_CENTER_ADJUSTABLE) || defined(PPM_LIMITS_SYMETRICAL)
 #define TR_MENUINPUTS          "STICKS"
 #define TR_MENULIMITS          "SAIDAS"
#else
 #define TR_MENUINPUTS          "DR/EXPO"
 #define TR_MENULIMITS          "LIMITES"
#endif
#define TR_MENUCURVES          "CURVAS"
#define TR_MENUCURVE           "CURVA"
#define TR_MENULOGICALSWITCHES  "CHAVES ESPECIAIS"
#define TR_MENUCUSTOMFUNC      "FUNCAO ESPECIAL"
#define TR_MENUTELEMETRY       "TELEMETRIA"
#define TR_MENUTEMPLATES       "MODELOS"
#define TR_MENUSTAT            "REGISTROS"
#define TR_MENUDEBUG           "DEPURAR"
#define TR_RXNUM               "RxNum"
#define TR_LIMIT               INDENT"Limite"
#define TR_MINRSSI             "Min Rssi"
#define TR_LATITUDE            "Latitude"
#define TR_LONGITUDE           "Longitude"
#define TR_GPSCOORD            "Gps Coords"
#define TR_VARIO               "Vario"
#define TR_POWEROFF            "\006SHUT DOWN ?"
#define TR_SHUTDOWN            "DESLIGANDOo"
#define TR_SAVEMODEL           "Saving model settings"
#define TR_BATT_CALIB          "CALIBRAR BATT"
#define TR_VOLTAGE             INDENT"Volts"
#define TR_CURRENT             INDENT"Amperes"
#define TR_SELECT_MODEL        "Selec. Modelo"
#define TR_CREATE_MODEL        "Criar Modelo"
#define TR_COPY_MODEL          "Copy Model"
#define TR_MOVE_MODEL          "Move Model"
#define TR_BACKUP_MODEL        "Salvar Modelo"
#define TR_DELETE_MODEL        "Apagar Modelo"
#define TR_RESTORE_MODEL       "Restaura Modelo"
#define TR_SDCARD_ERROR        "SDCARD Erro"
#define TR_NO_SDCARD           "Sem SDCARD"
#define TR_INCOMPATIBLE        "Incompativel"
#define TR_WARNING             "AVISO"
#define TR_EEPROMWARN          "EEPROM"
#define TR_THROTTLEWARN        "ACELERAD"
#define TR_ALARMSWARN          "ALARMES"
#define TR_SWITCHWARN          "CHAVES"
#define TR_INVERT_THR          "Inverte Acel?"
#define TR_SPEAKER_VOLUME      INDENT "Volume"
#define TR_FUNC                "Funcao"
#define TR_DELAY               "Atraso"
#define TR_SD_CARD             "SD CARD"
#define TR_NO_MODELS_ON_SD     "Sem Modelo no SD"
#define TR_PLAY_FILE           "Play"
#define TR_DELETE_FILE         "Apagar"
#define TR_COPY_FILE           "Copiar"
#define TR_RENAME_FILE         "Renomear"
#define TR_REMOVED             "Remover"
#define TR_SD_INFO             "Dados SD"
#define TR_NA                  "N/A"
#define TR_TIME                "Time"
#if defined(IMPERIAL_UNITS)
 #define TR_TXTEMP              "Temp. TX\037\164@F"
#else
 #define TR_TXTEMP              "Temp. TX\037\164@C"
#endif
#define TR_SD_INFO_TITLE       "SD INFO"
#define TR_SD_TYPE             "Tipe"
#define TR_SD_SIZE             "Tamanho"
#define TR_TYPE                INDENT "Tipe"
#define TR_GLOBAL_VARS         "Variaveis"
#define TR_GLOBAL_V            "GLOBAL V."
#define TR_GLOBAL_VAR          "Global Variable"
#define TR_OWN                 "Propr."
#define TR_DATE                "Data"
#define TR_ROTARY_ENCODER      "R.Encs"
#define TR_EDIT                "Edit"
#define TR_INSERT_BEFORE       "Insert Before"
#define TR_INSERT_AFTER        "Insert After"
#define TR_COPY                "Copy"
#define TR_MOVE                "Move"
#define TR_PASTE               "Paste"
#define TR_DELETE              "Delete"
#define TR_INSERT              "Insert"
#define TR_RESET_FLIGHT        "Reset Flight"
#define TR_RESET_TIMER1        "Reset Timer1"
#define TR_RESET_TIMER2        "Reset Timer2"
#define TR_RESET_TELEMETRY     "Reset Telemetry"
#define TR_STATISTICS          "Statistics"
#define TR_SAVE_TIMERS         "Save Timers"
#define TR_SPEAKER             INDENT"Speaker"
#define TR_BUZZER              INDENT"Buzzer"
#define TR_MODULE_BIND         "[Bnd]"
#define TR_MODULE_RANGE        "[Prt]"
#define TR_RESET_BTN           "[Reset]"
#define TR_SET                 "[Set]"
#define TR_RESET               "Reset"
#define TR_FAS_OFFSET          INDENT "FAS Ofs"
#if defined(X_ANY) || defined(BLUETOOTH)
 #define TR_X_ANY                        "X ANY"
 #define TR_ACTIVED                      "Active"
 #define TR_NUMBER                       "Number"
 #define TR_CHANNEL                      "Channel "
 #define TR_NB_REPEAT                    "Nb repeat  "
 #define LEN_SWITCHES_VALUES             "\005"
 #define TR_SWITCHES_VALUES              "Sw.--""L1-L4""L1-L8""Sw.4 ""Sw.8 ""Sw.16"
 #define TR_ANGLE_SENSOR                 "0:360"
 #define TR_PROP                         "Prop."
 #define LEN_XANY_EXPO                   "\005"
 #define TR_XANY_EXPO                    "0%\0  ""25%\0 ""37.5%""50%\0 "
#endif
#if defined(BLUETOOTH)
 #define TR_BLUETOOTH                     "BLUETOOTH"
 #define TR_NOBLUETOOTH                   "\3NO BLUETOOTH"
 #define TR_CONNECTED                     "\036\5CONECTEd"
 #define TR_BT_ROLE                       "Role"
 #define TR_BT_PIN                        "Pin"
 #define TR_BT_M_S                        "\006""Slave\0""Master"
 #define TR_BT_PAIR                       "Pair"
 #define TR_AUTOCON                       "Auto-con."
 #define TR_RESCANN                       "Re-Scan"
#endif
