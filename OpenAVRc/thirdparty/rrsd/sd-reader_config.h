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


/*  This file could be moded */
/*   From the original one   */
/*  Thanks to Roland Riegel  */
/*   Original header above   */

/*
 * Copyright (c) 2006-2012 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef SD_READER_CONFIG_H
#define SD_READER_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * \addtogroup config Sd-reader configuration
 *
 * @{
 */

/**
 * \file
 * Common sd-reader configuration used by all modules (license: GPLv2 or LGPLv2.1)
 *
 * \note This file contains only configuration items relevant to
 * all sd-reader implementation files. For module specific configuration
 * options, please see the files fat_config.h, partition_config.h
 * and sd_raw_config.h.
 */

/**
 * Controls allocation of memory.
 *
 * Set to 1 to use malloc()/free() for allocation of structures
 * like file and directory handles, set to 0 to use pre-allocated
 * fixed-size handle arrays.
 */
#define USE_DYNAMIC_MEMORY 0

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

