//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - FIP interface functions source file (Win32)
 *  \file       sp_fip.c
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

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "sp_misc.h"
#include "sp_fip.h"

static unsigned char char_table[] = " -0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz-\1";
unsigned char fipram[12];
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

int fip_handle = -1;

int fip_lastkey = 0;

#include "../MP/sp_fip_codes-technosonic.h"

extern bool khwl_msgloop();
extern int khwl_handle;

BOOL fip_init(BOOL /*applymodule*/)
{
	fip_deinit();
	fip_handle = 1;
	return TRUE;
}

BOOL fip_deinit()
{
    if (fip_handle == -1)
        return FALSE;

    fip_handle = -1;
    return TRUE;
}

BOOL fip_clear()
{
    int i;

    if (fip_handle == -1)
        return FALSE;
    // clear FIP
    for (i = 0; i < 12; i++)
    {
        fipram[i] = 0;
    }   
    return TRUE;
}

BOOL fip_write_char(int ch, int pos)
{
    int lr;

    if (pos < 1 || pos - 1 > 6 || fip_handle == -1)
        return FALSE;
    ch &= 255;
    for (lr = 0; lr < 66; lr++)
        if (char_table[lr] == ch)
            break;

	fipram[pos-1] &= 128;
    fipram[pos-1] |= led_table[lr] >> 1;

    return TRUE;
}

BOOL fip_write_special_char(int pos, int shift, BOOL onoff)
{
	BYTE ch = fipram[pos];
	if (onoff)
		fipram[pos] = (unsigned char)(ch | (1 << shift));
	else
		fipram[pos] = (unsigned char)(ch & ~(1 << shift));
	if (fipram[pos] == ch)
		return TRUE;
	
	return TRUE;
}

BOOL fip_write_special(int id, BOOL onoff)
{
	if (id < 0 || id > 27)
		return FALSE;
	fip_write_special_char(led_special[id*2], led_special[id*2+1], onoff);
	InvalidateRect((HWND)khwl_handle, NULL, FALSE);
	UpdateWindow((HWND)khwl_handle);
	return TRUE;
}

BOOL fip_get_special(int id)
{
	if (id < 0 || id > 27)
		return FALSE;
	int pos = led_special[id*2];
	int shift = led_special[id*2+1];
	return (fipram[pos] >> shift) & 1;
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

	InvalidateRect((HWND)khwl_handle, NULL, FALSE);
	UpdateWindow((HWND)khwl_handle);

    return TRUE;
}

int fip_read_button(BOOL )
{
    if (fip_handle == -1)
        return 0;

	if (!khwl_msgloop())
		return -1;

	int ret = fip_lastkey;
	fip_lastkey = 0;
	return ret;
}
