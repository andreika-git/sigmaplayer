//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - KHWL interface functions source file.
 * 								For Technosonic-compatible players ('MP')
 *  \file       sp_khwl.cpp
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

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include "sp_misc.h"
#include "sp_khwl.h"
#include "sp_khwl_ioctl.h"
#include "sp_mpeg.h"
#include "sp_fip.h"
#include "sp_io.h"
#include "sp_i2c.h"

#include "arch-jasper/hardware.h"


/// Muxed media packets
MpegPlayStruct *MPEG_PLAY_STRUCT  = (MpegPlayStruct *)(0x17c0000 + 0x3fc00  + 768);
/// Video packets
MpegPlayStruct *MPEG_VIDEO_STRUCT = (MpegPlayStruct *)(0x2000000 - 0x800000 - 232);
/// Audio packets
MpegPlayStruct *MPEG_AUDIO_STRUCT = (MpegPlayStruct *)(0x2000000 - 0x800000 - 208);
/// Subpicture packets
MpegPlayStruct *MPEG_SPU_STRUCT   = (MpegPlayStruct *)(0x2000000 - 0x800000 - 184);

/// Buffers storage
BYTE *BUF_BASE = (BYTE *)0x01680000;
//BYTE *BUF_BASE = (BYTE *)0x016c0000;


/// KHWL module handle
int khwl_handle = -1;

/// OSD properties used
KHWL_OSDSTRUCT osd = { NULL };

/// insmod flag
static BOOL module_applied = FALSE;

/// If digital TV (HDTV) is installed.
static BOOL digital_tv = FALSE;

int FRAME_WIDTH = 720;
int FRAME_HEIGHT = 480;


BOOL khwl_init(BOOL applymodule)
{
    khwl_inityuv();
    khwl_deinit();

    if (module_applied == TRUE || applymodule == FALSE || module_apply("/drivers/khwl.o")) // insert
    {
        module_applied = TRUE;

        khwl_handle = open("/dev/realmagichwl0", 0, 0);
        if (khwl_handle != -1)
        {
        	//khwl_reset();

			if (!khwl_restoreparams())
				return FALSE;

            return TRUE;
        }
    }
    return FALSE;
}

BOOL khwl_restoreparams()
{
    int val = 0;
    khwl_setproperty(KHWL_VIDEO_SET, evMacrovisionFlags, sizeof(int), &val);

    int flickerval = 15; // flicker = max
	khwl_setproperty(KHWL_DECODER_SET, edecOsdFlicker, sizeof(int), &flickerval);

	KHWL_WINDOW wnd, osdwnd;
	khwl_getproperty(KHWL_VIDEO_SET, evMaxDisplayWindow, sizeof(wnd), &wnd);
	khwl_setproperty(KHWL_VIDEO_SET, evDestinationWindow, sizeof(wnd), &wnd);
	
	osdwnd = wnd;
	osdwnd.h = 30 * wnd.h / 31;
	khwl_setproperty(KHWL_OSD_SET, eOsdDestinationWindow, sizeof(osdwnd), &osdwnd);

	val = 0;
	khwl_setproperty(KHWL_VIDEO_SET, evForcedProgressiveAlways, sizeof(int), &val);

	KHWL_YUV_WRITE_PARAMS_TYPE yuvparams;
	yuvparams.wWidth = wnd.w;
	yuvparams.wHeight = wnd.h;
	yuvparams.YUVFormat = KHWL_YUV_420_UNPACKED;
			
	khwl_setproperty(KHWL_VIDEO_SET, evYUVWriteParams, sizeof(yuvparams), &yuvparams);

	// do some HDTV-specific checks...
	digital_tv = FALSE;
	if ((inl(JASPER_QUASAR_BASE + QUASAR_DRAM_STARTUP1) & 0x2000) != 0)
	{
		BYTE data[4];
		// test for HDTV 1080I
		i2c_data_in(112, 0, data, 4);
		if (data[0] == 1 && data[1] == 0 && data[2] == 8 && data[3] == 0)
			digital_tv = TRUE;
	}

	return TRUE;
}

BOOL khwl_deinit()
{
    if (khwl_handle == -1)
        return FALSE;
    close(khwl_handle);
    khwl_handle = -1;
    return TRUE;
}

BOOL khwl_reset()
{
    if (khwl_handle == -1)
        return FALSE;
	return ioctl(khwl_handle, KHWL_HARDRESET, 0);
}

int khwl_osd_switch(KHWL_OSDSTRUCT *_osd, BOOL autoupd)
{
	KHWL_WINDOW wnd;
	int ret;
	if (khwl_handle == -1)
        return FALSE;
	osd.flags = autoupd ? 1 : 0;
	ret = ioctl(khwl_handle, KHWL_OSDFB_SWITCH, &osd);

	if (_osd != NULL)
	{
		khwl_getproperty(KHWL_VIDEO_SET, evMaxDisplayWindow, sizeof(wnd), &wnd);
		khwl_setproperty(KHWL_OSD_SET, eOsdDestinationWindow, sizeof(wnd), &wnd);
		*_osd = osd;
	}
	return ret;
}

void khwl_get_osd_size(int *width, int *height)
{
	*width = 640;//osd.width;
	*height = 480;//osd.height;
}


BOOL khwl_osd_update()
{
	ioctl(khwl_handle, KHWL_OSDFB_UPDATE, 0);
	return TRUE;
}

int khwl_osd_setalpha(int alpha)
{
	return ioctl(khwl_handle, KHWL_OSDFB_ALPHA, &alpha);
}

void khwl_osd_setpalette(BYTE *pal, int entry, BYTE r, BYTE g, BYTE b, BYTE a)
{
    // assert(entry >= 0 && entry < 256);
    BYTE y, u, v;
    entry <<= 2;
    khwl_vgargbtotvyuv(r, g, b, &y, &u, &v);
	pal[entry++] = a; // alpha
	pal[entry++] = y;
	pal[entry++] = u;
	pal[entry] = v;
}

void khwl_osd_setfullscreen(BOOL is)
{
	KHWL_WINDOW wnd, osdwnd;
	khwl_getproperty(KHWL_VIDEO_SET, evMaxDisplayWindow, sizeof(wnd), &wnd);
	osdwnd = wnd;
	if (!is)
		osdwnd.h = 30 * wnd.h / 31;
	khwl_setproperty(KHWL_OSD_SET, eOsdDestinationWindow, sizeof(osdwnd), &osdwnd);
}

int khwl_displayYUV(KHWL_YUV_FRAME *f)
{
	int ret = ioctl(khwl_handle, KHWL_DISPLAY_YUV, f);
	return ret;
}

int khwl_setproperty(KHWL_PROPERTY_SET pset, int id, int size, void *value)
{
    KHWL_PROPERTY p;
    p.pset = pset;
    p.id = id; 
    p.size = size;
   	p.v = value;
	return ioctl(khwl_handle, KHWL_SETPROP, &p);
}

int khwl_getproperty(KHWL_PROPERTY_SET pset, int id, int size, void *value)
{
    KHWL_PROPERTY p;
    p.pset = pset;
    p.id = id; 
    p.size = size;
   	p.v = value;
	return ioctl(khwl_handle, KHWL_GETPROP, &p);
}

int khwl_audioswitch(BOOL ison)
{
	int val = ison ? 1 : 0;
	return ioctl(khwl_handle, KHWL_AUDIOSWITCH, &val);
}

int *khwl_get_samplerates()
{
	static int rates[2][12] = 
	{
		// chip rev.A
		{ 16000, 22050, 24000, 32000, 44100, 48000, -1 },
		// chip rev.B supports more samplerates
		{ 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000, -1 },
	};
	return rates[inl(SYS_REVID_REG) & 1];
}

int khwl_play(int mode)
{
	return ioctl(khwl_handle, KHWL_PLAY, &mode);
}

int khwl_stop()
{
	return ioctl(khwl_handle, KHWL_STOP, 0);
}

int khwl_pause()
{
	return ioctl(khwl_handle, KHWL_PAUSE, 0);
}

BOOL khwl_blockirq(BOOL block)
{
	unsigned irqstat;
	irqstat = inl(JASPER_INT_CONTROLLER_BASE + INT_INTEN);
	if (block)
		irqstat &= ~Q2H_RISC_INT;
	else
		irqstat |= Q2H_RISC_INT;
	outl(irqstat, JASPER_INT_CONTROLLER_BASE + INT_INTEN);
	return TRUE;
}

BOOL khwl_happeningwait(DWORD *mask)
{
	KHWL_WAITABLE w;
	w.mask = *mask;
	w.timeout_microsecond = 400000;
	ioctl(khwl_handle, KHWL_WAIT, &w);
	*mask = w.mask;
	return TRUE;
}

BOOL khwl_poll(WORD mask, DWORD timeout)
{
	pollfd pfd;
	pfd.events = mask;
	pfd.fd = khwl_handle;
	return poll(&pfd, 1, timeout) >= 0;
}

BOOL khwl_ideswitch(BOOL ison)
{
	KHWL_ADDR_DATA data;
	// switch IDE?
	data.Addr = 4;
	data.Data = ison ? 1 : 0;
	khwl_setproperty(KHWL_BOARDINFO_SET, ebiPIOAccess, sizeof(data), &data);
	return TRUE;
}

int khwl_getfrequency()
{
   	int temp = inl(JASPER_QUASAR_BASE + QUASAR_DRAM_PLLCONTROL);

	int div = (temp >> 8) & 0xff;
	int mul = (temp >> 2) & 63;

	int freq = (27 * (mul + 2)) / ((div + 2) * 2);
	return freq;
}

BOOL khwl_setfrequency(int freq)
{
	if (freq < 100 || freq > 202)
		return FALSE;

   	int temp = inl(JASPER_QUASAR_BASE + QUASAR_DRAM_PLLCONTROL);
   	temp = temp & 0xF000;
	int div = (temp >> 8) & 0xff;
	//int mul = (temp >> 2) & 63;

	int mul = (freq * ((10 * div+20)*20) / 270 - 20 /* + 5 */) / 10;
	if (mul < 0 || mul > 63)
		return FALSE;
	
	temp = temp | 0x02;
	temp = temp | (div << 8);
	temp = temp | (mul << 2);

	outl(temp & 0x7FFF, JASPER_QUASAR_BASE + QUASAR_DRAM_PLLCONTROL);
	return TRUE;
}

char *khwl_gethw()
{
	static char hw[256];
/*
	char b[128];
	b[0] = '\0';
	khwl_getproperty(KHWL_BOARDINFO_SET, ebiBoardNameString, 256, b);

	DWORD v_ebiDeviceId, v_ebiSubId, v_ebiBoardVersion, v_ebiHwLibVersion, v_ebiUcodeVersion;
	khwl_getproperty(KHWL_BOARDINFO_SET, ebiDeviceId, sizeof(DWORD), &v_ebiDeviceId);
    khwl_getproperty(KHWL_BOARDINFO_SET, ebiSubId, sizeof(DWORD), &v_ebiSubId);
    khwl_getproperty(KHWL_BOARDINFO_SET, ebiBoardVersion, sizeof(DWORD), &v_ebiBoardVersion);
    khwl_getproperty(KHWL_BOARDINFO_SET, ebiHwLibVersion, sizeof(DWORD), &v_ebiHwLibVersion);
    khwl_getproperty(KHWL_BOARDINFO_SET, ebiUcodeVersion , sizeof(DWORD), &v_ebiUcodeVersion);
*/
	sprintf(hw, "%X.%c", inl(SYS_CHIPID_REG), inl(SYS_REVID_REG) + 'A');

	return hw;
}
