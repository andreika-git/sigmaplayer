//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - KHWL driver's header.
 *  \file       sp_khwl.h
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

#ifndef SP_KHWL_H
#define SP_KHWL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sp_khwl_prop.h"
#include "sp_video.h"


// This trick transforms [0..255] range into [0..65535] range (instead of about 0..255*256)
#define RANGE8TO16(x) (((x) << 8) | (x))

/// Play modes
enum KHWL_PLAY_MODE_TYPE
{
	KHWL_PLAY_MODE_NORMAL = 0,
	KHWL_PLAY_MODE_STEP = 1,
	KHWL_PLAY_MODE_IFRAME = 2,
	KHWL_PLAY_MODE_REV = 5,
	KHWL_PLAY_MODE_IFRAME_REV = 6,
};

////////////////////////////////////
/// Structures:

/// OSD setup struct
typedef struct _KHWL_OSDSTRUCT
{
	void *addr;
	int width;
	int height;
	int bpp;
	int flags;

} KHWL_OSDSTRUCT;

/// Window coords and size
typedef struct _KHWL_WINDOW
{
	int x, y;
	int w, h;

} KHWL_WINDOW;


/// YUV frame
typedef struct 
{
	void *y_buf;
	void *uv_buf;
	int y_offs, uv_offs;
	int y_num, uv_num;

} KHWL_YUV_FRAME;

/// Address-data pair (used for EEPROM and PIO)
typedef struct 
{
    DWORD Addr;
    DWORD Data;

} KHWL_ADDR_DATA;

/// Used for hapeningwait()
typedef struct 
{
	DWORD timeout_microsecond;
	DWORD mask;

} KHWL_WAITABLE;

extern KHWL_OSDSTRUCT osd;

////////////////////////////////////
/// Functions:

/// Loads driver module and opens it.
BOOL khwl_init(BOOL applymodule);

/// Closes driver handle and unloads module
BOOL khwl_deinit();

/// Resets the driver
BOOL khwl_reset();

/// OSD switch (enable). Fills given structure with params.
int khwl_osd_switch(KHWL_OSDSTRUCT *osd, BOOL autoupd);

/// Update OSD. Returns FALSE when user pressed 'exit' for Win32.
BOOL khwl_osd_update();

/// Set general alpha for OSD (0-65535)
int khwl_osd_setalpha(int alpha);

void khwl_get_osd_size(int *width, int *height);

void khwl_osd_setfullscreen(BOOL is);

/// Set OSD palette entry
void khwl_osd_setpalette(BYTE *pal, int entry, BYTE r, BYTE g, BYTE b, BYTE a);

/// Set KHWL property
int khwl_setproperty(KHWL_PROPERTY_SET pset, int id, int size, void *value);
/// Get KHWL property
int khwl_getproperty(KHWL_PROPERTY_SET pset, int id, int size, void *value);


/// Initialize color conversion tables
void khwl_inityuv();
/// RGB -> YUV
void khwl_vgargbtotvyuv(BYTE R, BYTE G, BYTE B, BYTE *y, BYTE *u, BYTE *v);
/// YUV -> RGB
void khwl_tvyuvtovgargb(BYTE y, BYTE u, BYTE v, BYTE *R, BYTE *G, BYTE *B);
/// JPEG YUV -> RGB
void khwl_jpegyuvtorgb(BYTE y, BYTE u, BYTE v, BYTE *R, BYTE *G, BYTE *B);

/// Display YUV buffer
int khwl_displayYUV(KHWL_YUV_FRAME *f);

/// Audio switch
int khwl_audioswitch(BOOL ison);

/// Get supported audio sample rates list (the last is -1).
int *khwl_get_samplerates();

/// Decoder play (mode = 0, 2, 6 ?)
int khwl_play(int mode);

/// Decoder stop
int khwl_stop();

/// Decoder pause
int khwl_pause();

/// Restore params
BOOL khwl_restoreparams();

/// Block/Unblock irq for data change
BOOL khwl_blockirq(BOOL block);

/// Wait to process media packets
BOOL khwl_happeningwait(DWORD *mask);

/// Poll khwl device
BOOL khwl_poll(WORD mask, DWORD timeout);

/// IDE turn on/off
BOOL khwl_ideswitch(BOOL ison);

/// Get CPU frequency
int khwl_getfrequency();

/// Set CPU frequency (100 <= freq <= 202)
/// Caution! Use this in predicaments only!
BOOL khwl_setfrequency(int freq);

/// Get hardware string
char *khwl_gethw();

#ifdef __cplusplus
}
#endif

#endif // of SP_KHWL_H
