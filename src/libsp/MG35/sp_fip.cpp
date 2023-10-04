//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - FIP interface functions source file.
 * 								For Technosonic-compatible players ('MP')
 *  \file       sp_fip.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       4.05.2004
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sp_misc.h"
#include "sp_fip.h"
#include "sp_fip_ioctl.h"

#if defined(SP_PLAYER_TECHNOSONIC)
	#include "sp_fip_codes-technosonic.h"
#elif defined(SP_PLAYER_DREAMX108)
	#include "sp_fip_codes-dreamx108.h"
#endif

/// Characters supported by LED
static unsigned char char_table[] = " -0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz-\1";
/// Current LEF data storage
static unsigned char fipram[12];
/// Binary definitions for LED data
static unsigned char led_table[] = 
{ 
    0x00,0x10,0xEE,0x48,0xD6,0xDA,0x78,0xBA,0xBE,0xC8,0xFE,0xFA,0xFC,0xFE,0xA6,0xEE,
    0xB6,0xB4,0xBE,0x7C,0x48,0x4A,0x7C,0x26,0xEC,0xEC,0xEE,0xF4,0xEE,0xFC,0xBA,0xC8,
    0x6E,0x6E,0x6E,0x6C,0x78,0x82,0x02,0x1E,0x3E,0x16,0x5E,0xF6,0xB4,0xFA,0x3C,0x08,
    0x0A,0x2C,0x26,0x1C,0x1C,0x1E,0xF4,0xF8,0x14,0xBA,0x36,0x0E,0x0E,0x0E,0x0C,0x78,
    0x12,0x10,0x00
};

static unsigned char led_special[] = 
{
	0x01, 0x07, 0x01, 0x06, 0x03, 0x07, 0x02, 0x07, 0x05, 0x07, 0x04, 0x07, 0x07, 0x07, 0x07, 0x06, 
	0x07, 0x05, 0x07, 0x04, 0x07, 0x03, 0x07, 0x02, 0x07, 0x01, 0x07, 0x00, 0x09, 0x07, 0x09, 0x06, 
	0x09, 0x05, 0x09, 0x04, 0x09, 0x03, 0x09, 0x02, 0x09, 0x01, 0x09, 0x00, 0x08, 0x07, 0x08, 0x06, 
	0x08, 0x05, 0x08, 0x04, 0x08, 0x03, 0x08, 0x02
};

/// Button codes translation look-up table
static BYTE *button_LUT = NULL;
static DWORD butcode = 0;
static int fip_panel_num_codes = 0;

/// FIP device handle
int fip_handle = -1;

void fip_create_table(BYTE *lut)
{
	memset(lut, 0xff, 65536);

	DWORD *codes[2];
	codes[0] = fip_panel_codes;
	codes[1] = fip_remote_codes;

	fip_panel_num_codes = 0;
	for (int i = 0; i < 2; i++)
	{
		int j;
		for (j = 0; codes[i][j] != 0xffffffff; j++)
	    {
		    lut[codes[i][j] & 0xffff] = j;
	    }
	    if (i == 0)
		    fip_panel_num_codes = j;
	}
}

BOOL fip_init(BOOL applymodule)
{
    fip_deinit();

    if (applymodule == FALSE || module_apply("/drivers/fipmodule.o")) // insert
    {
        fip_handle = open("/dev/fip", O_NONBLOCK, 0);
        if (fip_handle != -1)
        {
            fip_clear();

            button_LUT = (BYTE *)SPmalloc(65536);
    		if (button_LUT == NULL)
			   	return FALSE;
			
			fip_create_table(button_LUT);

			return TRUE;
        }
    }
    
    return FALSE;
}

BOOL fip_deinit()
{
    if (fip_handle == -1)
        return FALSE;
    close(fip_handle);
    fip_handle = -1;

    if (button_LUT != NULL)
    {
    	SPfree(button_LUT);
    	button_LUT = NULL;
    }

    return TRUE;
}

BOOL fip_clear()
{
    int i;

    if (fip_handle == -1)
        return FALSE;
    // clear FIP
    for (i = 0; i < 10; i++)
    {
        fipram[i] = 0;
//        ioctl(fip_handle, FIP_DISPLAY_SYMBOL, i << 16);
    }   
    return TRUE;
}

BOOL fip_write_char(int ch, int pos)
{
    int lr;
    unsigned char old_fip1, old_fip2 = 0;

    if (pos < 1 || pos - 1 > 6 || fip_handle == -1)
        return FALSE;
    ch &= 255;
    for (lr = 0; lr < 66; lr++)
    {
        if (char_table[lr] == (BYTE)ch)
            break;
    }
    // saving 
    old_fip1 = fipram[pos-1];

    // clear
    if (pos == 2)
    {
        old_fip2 = fipram[pos-2];

        fipram[pos-1] &= 192;
        fipram[pos-2] &= 127;
    } else
        fipram[pos-1] &= 128;
    // set
    if (pos == 2)
    {
        fipram[pos-1] |= led_table[lr] >> 2;
        fipram[pos-2] |= (led_table[lr] << 6) & ~127;
    } else
        fipram[pos-1] |= led_table[lr] >> 1;
    // write to device
//    if (old_fip1 != fipram[pos-1])
//        ioctl(fip_handle, FIP_DISPLAY_SYMBOL, fipram[pos-1] | ((pos-1) << 16));
//    if (pos == 2 && old_fip2 != fipram[pos-2])
//        ioctl(fip_handle, FIP_DISPLAY_SYMBOL, fipram[pos-2] | ((pos-2) << 16));
    return TRUE;
}

BOOL fip_write_string(const char *str)
{
    int i, len;
    if (str == NULL || fip_handle == -1)
        return FALSE;
    len = strlen(str);
    if (len > 7) 
        len = 7;
    for (i = 0; i < 7; i++)
        fip_write_char(i < len ? str[len - i - 1] : ' ', i + 1);
    return TRUE;
}

BOOL fip_write_special_char(int pos, int shift, BOOL onoff)
{
	BYTE ch = fipram[pos];
	if (onoff)
		fipram[pos] = ch | (1 << shift);
	else
		fipram[pos] = ch & ~(1 << shift);
	if (fipram[pos] == ch)
		return TRUE;
//	ioctl(fip_handle, FIP_DISPLAY_SYMBOL, fipram[pos] | (pos << 16));
	return TRUE;
}

BOOL fip_write_special(int id, BOOL onoff)
{
	if (id < 0 || id > 27)
		return FALSE;
	return fip_write_special_char(led_special[id*2], led_special[id*2+1], onoff);
}

BOOL fip_get_special(int id)
{
	if (id < 0 || id > 27)
		return FALSE;
	int pos = led_special[id*2];
	int shift = led_special[id*2+1];
	return (fipram[pos] >> shift) & 1;
}

int fip_read_button(BOOL blocked)
{
    if (fip_handle == -1 || button_LUT == NULL)
        return 0;
    ioctl(fip_handle, FIP_BUTTON_READ, &butcode);
    if (butcode == 0)
    	return FIP_KEY_NONE;

#if 0
	BYTE bc = button_LUT[butcode & 0xffff];
#ifdef SP_PLAYER_DREAMX108
	// special case - front panel STOP button
	if (butcode == 0x00080000)
		return FIP_KEY_FRONT_STOP;
	// special case - front panel PREV button
	if (butcode == 0x00010000)
		return FIP_KEY_FRONT_SKIP_PREV;
#endif

	if (bc == 0xff)
		return FIP_KEY_NONE;
	if (bc < fip_panel_num_codes)
	{
		if (butcode == fip_panel_codes[bc])
			return (unsigned int)bc + fip_code_offset[0];
	}
	if (butcode == fip_remote_codes[bc])
		return (unsigned int)bc + fip_code_offset[1];
#endif
	return FIP_KEY_NONE;
}
