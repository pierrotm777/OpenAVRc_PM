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

#define JQ6500_QUEUE_LENGTH (32u*2)  //bytes

#define PROMPT_CUSTOM_BASE      0
#define PROMPT_I18N_BASE        256
#define PROMPT_SYSTEM_BASE      480

extern void pushPrompt(uint16_t prompt);

#define PUSH_CUSTOM_PROMPT(p)           pushPrompt(PROMPT_CUSTOM_BASE+(p))
#define PUSH_NUMBER_PROMPT(p)           pushPrompt(PROMPT_I18N_BASE+(p))
#define PUSH_SYSTEM_PROMPT(p)           pushPrompt(PROMPT_SYSTEM_BASE+(p))
#define IS_PLAY_TIME()                  (0) // Todo : Use it ?

#define VOLUME_LEVEL_MAX                7
#define VOLUME_LEVEL_DEF                7
#define setVolume(v)
