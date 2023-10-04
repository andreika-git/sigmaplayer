//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - settings setting/getting source file
 *  \file       settings.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       4.07.2004
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


#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_flash.h>
#include <libsp/sp_eeprom.h>

#include <gui/rect.h>
#include <gui/text.h>

#include "settings.h"

const DWORD cur_version = (DWORD)(SP_VERSION_MASK | 0x0D);

static int num_settings = 0;
static bool was_reset = false;

static struct
{
	DWORD size;
	DWORD def;
	DWORD max;
	DWORD value;
	int addr;
} settings[max_settings] = 
{
	{ 4, cur_version, 0xFFFFFFFF },	//	SETTING_VERSION

	{ 1, 0, 1, }, // SETTING_AUDIOOUT

	{ 1, 0, 3, }, // SETTING_HDTV
	
	{ 4, 0, 255, }, // SETTING_DVI
	
	{ 1, 1, 5, }, // SETTING_TVSTANDARD	// 1 = "PAL"
	{ 1, 0, 2, }, // SETTING_TVOUT		// 0 = "C/S-Video"
	
	{ 1, 0, 3, }, // SETTING_TVTYPE,	// 0 = 4:3 letterbox
	
	{ 1, 0, 1, }, // SETTING_HQ_JPEG,
	
	{ 1, 0, 8, }, // SETTING_DVD_PARENTAL,
	{ 1, 1, 1, }, // SETTING_DVD_MV,
	
	{ 2, 0x656e, 0xffff, }, // SETTING_DVD_LANG_MENU 'en',
	
	{ 2, 0x656e, 0xffff, }, // SETTING_DVD_LANG_AUDIO 'en',
	
	{ 2, 0x656e, 0xffff, }, // SETTING_DVD_LANG_SPU 'en',

	{ 1, 80, 100, }, // SETTING_VOLUME,

	{ 1, 100, 200, }, // SETTING_BALANCE,


	{ 4, (500<<20)|(500<<10)|500, (1000<<20)|(1000<<10)|1000, }, // SETTING_BRIGHTNESS_CONTRAST_SATURATION_PAL,
	{ 4, (500<<20)|(500<<10)|500, (1000<<20)|(1000<<10)|1000, }, // SETTING_BRIGHTNESS_CONTRAST_SATURATION_NTSC,
	{ 4, (500<<20)|(500<<10)|500, (1000<<20)|(1000<<10)|1000, }, // SETTING_BRIGHTNESS_CONTRAST_SATURATION_OTHER,

	{ 4, 0, 0xffffffff, }, // SETTING_GUI_0,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_1,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_2,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_3,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_4,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_5,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_6,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_7,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_8,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_9,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_10,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_11,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_12,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_13,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_14,
	{ 4, 0, 0xffffffff, }, // SETTING_GUI_15,

	{ 4, 0, 0xffffffff, }, // SETTING_DVD_ID1,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_POS1,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_DATA11,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_DATA12,

	{ 4, 0, 0xffffffff, }, // SETTING_DVD_ID2,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_POS2,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_DATA21,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_DATA22,

	{ 4, 0, 0xffffffff, }, // SETTING_DVD_ID3,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_POS3,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_DATA31,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_DATA32,

	{ 4, 0, 0xffffffff, }, // SETTING_DVD_ID4,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_POS4,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_DATA41,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_DATA42,

	{ 4, 0, 0xffffffff, }, // SETTING_DVD_ID5,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_POS5,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_DATA51,
	{ 4, 0xffffffff, 0xffffffff, }, // SETTING_DVD_DATA52,

	{ 1, 0, 2, }, // SETTING_HDD_SPEED,

	{ 0 },
};

int settings_get_eeprom(int id)
{
	return eeprom_get_value(settings[id].addr, settings[id].size);
}

BOOL settings_init(bool set_defaults)
{
	settings[0].addr = 0;
	int i;
	for (i = 1; i < max_settings; i++)
	{
		if (settings[i].size == 0)
		{
			num_settings = i;
			break;
		}
		settings[i].addr = settings[i-1].addr + settings[i-1].size;
	}

	DWORD stored_version = settings_get_eeprom(SETTING_VERSION);

	bool reset = (stored_version & 0xffffff00) != (cur_version & 0xffffff00);
	if ((stored_version & 0xff) < (cur_version & 0xff) || set_defaults)
	{
		was_reset = true;
		reset = true;
	}

	for (i = 0; i < num_settings; i++)
	{
		if (reset)
			settings_set((SETTING_SET)i, settings[i].def);
		else
		{
			DWORD from_eeprom = settings_get_eeprom(i);
			if (from_eeprom <= settings[i].max)
				settings[i].value = from_eeprom;
			else
				settings_set((SETTING_SET)i, settings[i].def);
		}
	}
	return TRUE;
}

BOOL settings_set(SETTING_SET id, int val, bool no_eeprom)
{
	if (id < num_settings && (DWORD)val <= settings[id].max)
	{
		settings[id].value = val;
		if (!no_eeprom)
		{
			eeprom_set_value(settings[id].addr, val, settings[id].size);
			msg("Settings: %d = %d\n", id, val);
		}
		return TRUE;
	}
	return FALSE;
}

int settings_get(SETTING_SET id)
{
	return id < num_settings ? settings[id].value : 0;
}

int settings_getmax(SETTING_SET id)
{
	return id < num_settings ? settings[id].max : 0;
}

DWORD get_firmware_address()
{
	DWORD addr = 0;
	msg("Flash: Detecting firmware address...\n");
	// find ROMFS address
#ifdef WIN32
	addr = 0x6000;
#else
	for (int i = 0x6000; i <= 0x10000; i += 0x1000)
	{
		BYTE *data = (BYTE *)i;
		if (memcmp(data, "-rom1fs-", 8) == 0)
		{
			addr = (DWORD)i;
			break;
		}
	}
#endif

	if (addr == 0)
	{
		msg("Flash: Cannot find ROMFS address!\n");
		return 0;
	}
	msg("Flash: ROMFS found @ 0x%08x\n", addr);

	return addr;
}
