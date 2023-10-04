//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - user settings header file
 *  \file       settings.h
 *  \author     bombur
 *  \version    0.1
 *  \date       12.01.2005
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 */
//////////////////////////////////////////////////////////////////////////

#ifndef SP_SETTINGS_H
#define SP_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "version.h"

/// This is version identification mask
const DWORD SP_VERSION_MASK = 0xCFDA9900;

const int max_settings = 64;

/// Settings IDs (no more than max_settings)
typedef enum
{
	SETTING_VERSION = 0,	// not used normally

	SETTING_AUDIOOUT,		// 0 = analog, 1 = digital

	SETTING_HDTV,			// 0 = off, 1 = 480p, 2 = 720p, 3 = 1080i
	
	SETTING_DVI,			// !RESERVED! 0 = off, X = number read from cfg file??
	
	SETTING_TVSTANDARD,		// 0 = "NTSC", 1 = "PAL", 2 = "480P", 3 = "576P", 4 = "720P", 5 = "1080I"

	SETTING_TVOUT,			// 0 = "C/S-Video", 1 = "C/YPbPr", 2 = "C/RGB"
	
	SETTING_TVTYPE,			// 0 = 4:3 letterbox, 1 = 4:3 panscan, 2 = 16:9, 3 = 16:9 panscan
	
	SETTING_HQ_JPEG,		// 0 = off, 1 = on

	SETTING_DVD_PARENTAL,
	SETTING_DVD_MV,
	
	SETTING_DVD_LANG_MENU,
	
	SETTING_DVD_LANG_AUDIO,
	
	SETTING_DVD_LANG_SPU,

	SETTING_VOLUME,

	SETTING_BALANCE,

	SETTING_BRIGHTNESS_CONTRAST_SATURATION_PAL,
	SETTING_BRIGHTNESS_CONTRAST_SATURATION_NTSC,
	SETTING_BRIGHTNESS_CONTRAST_SATURATION_OTHER,

	SETTING_GUI_0,
	SETTING_GUI_1,
	SETTING_GUI_2,
	SETTING_GUI_3,
	SETTING_GUI_4,
	SETTING_GUI_5,
	SETTING_GUI_6,
	SETTING_GUI_7,
	SETTING_GUI_8,
	SETTING_GUI_9,
	SETTING_GUI_10,
	SETTING_GUI_11,
	SETTING_GUI_12,
	SETTING_GUI_13,
	SETTING_GUI_14,
	SETTING_GUI_15,

	SETTING_DVD_ID1,		// most recent
	SETTING_DVD_POS1,
	SETTING_DVD_DATA11,
	SETTING_DVD_DATA12,
	SETTING_DVD_ID2,
	SETTING_DVD_POS2,
	SETTING_DVD_DATA21,
	SETTING_DVD_DATA22,
	SETTING_DVD_ID3,
	SETTING_DVD_POS3,
	SETTING_DVD_DATA31,
	SETTING_DVD_DATA32,
	SETTING_DVD_ID4,
	SETTING_DVD_POS4,
	SETTING_DVD_DATA41,
	SETTING_DVD_DATA42,
	SETTING_DVD_ID5,
	SETTING_DVD_POS5,
	SETTING_DVD_DATA51,
	SETTING_DVD_DATA52,

	SETTING_HDD_SPEED,
	

} SETTING_SET;

///////////////////////////////////////////////////

/// Initialize settings (and read EEPROM)
BOOL settings_init(bool set_defaults = false);

/// Store settings value (and write EEPROM if allowed)
BOOL settings_set(SETTING_SET id, int val, bool no_eeprom = false);

/// Get stored settings value from cache
int settings_get(SETTING_SET id);

/// Get max. allowed value for given settings ID
int settings_getmax(SETTING_SET id);

///////////////////////////////////////////////////
/// Special functions

DWORD get_firmware_address();

// not yet

#ifdef __cplusplus
}
#endif

#endif // of SP_SETTINGS_H
