//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - KHWL misc. video functions source file.
 *  \file       sp_video.cpp
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
#include <fcntl.h>
#include <sys/ioctl.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_khwl.h>

static KHWL_WINDOW dest_wnd;
static KHWL_VIDEOMODE cur_mode = KHWL_VIDEOMODE_NORMAL;
static int cur_aspect = evInAspectRatio_4x3;
static int cur_output = evOutDisplayOption_Normal;
static int frame_left = 0, frame_top = 0, frame_right = 719, frame_bottom = 479;

static int khwl_wide = 0;

int khwl_display_clear()
{
	//return ioctl(khwl_handle, KHWL_DISPLAY_CLEAR, 0);
	int var = ebiCommand_VideoHwBlackFrame;
	return khwl_setproperty(KHWL_BOARDINFO_SET, ebiCommand, sizeof(DWORD), &var);
}

void khwl_set_max_osd_wnd(int width, int osd_width, int height, int osd_height)
{
	KHWL_WINDOW wnd;
	wnd.x = (width * 35) / 720;
	
	int h = height;
	if (h < 0)
		h += 31;
	wnd.y = h / 32;
	
	wnd.w = width * osd_width / 720;

	wnd.h = height * osd_height / 480;
	
	khwl_getproperty(KHWL_OSD_SET, eOsdDestinationWindow, sizeof(wnd), &wnd);

}

void khwl_switch_dvi(bool is_on)
{
	KHWL_ADDR_DATA data;
	if (is_on)
	{
		data.Addr = 8;
		data.Data = 55;
		khwl_setproperty(KHWL_DVI_TRANSMITTER_SET, edtAccessRegister, sizeof(data), &data);
		data.Addr = 9;
		data.Data = 13;
		khwl_setproperty(KHWL_DVI_TRANSMITTER_SET, edtAccessRegister, sizeof(data), &data);
		data.Addr = 10;
		data.Data = 240;
		khwl_setproperty(KHWL_DVI_TRANSMITTER_SET, edtAccessRegister, sizeof(data), &data);
		data.Addr = 12;
		data.Data = 137;
		khwl_setproperty(KHWL_DVI_TRANSMITTER_SET, edtAccessRegister, sizeof(data), &data);
		data.Addr = 13;
		data.Data = 0;
		khwl_setproperty(KHWL_DVI_TRANSMITTER_SET, edtAccessRegister, sizeof(data), &data);
		data.Addr = 14;
		data.Data = 0;
		khwl_setproperty(KHWL_DVI_TRANSMITTER_SET, edtAccessRegister, sizeof(data), &data);
		data.Addr = 15;
		data.Data = 0;
		khwl_setproperty(KHWL_DVI_TRANSMITTER_SET, edtAccessRegister, sizeof(data), &data);
	} else
	{
		data.Addr = 8;
		data.Data = 6;
		khwl_setproperty(KHWL_DVI_TRANSMITTER_SET, edtAccessRegister, sizeof(data), &data);
		data.Addr = 12;
		data.Data = 10;
		khwl_setproperty(KHWL_DVI_TRANSMITTER_SET, edtAccessRegister, sizeof(data), &data);
		data.Addr = 51;
		data.Data = 1;
		khwl_setproperty(KHWL_DVI_TRANSMITTER_SET, edtAccessRegister, sizeof(data), &data);
    }
}

void khwl_setwide(BOOL wide)
{
	khwl_wide = wide ? 1 : 0;
}

int khwl_setdisplay(KHWL_TV_OUTPUT_FORMAT_TYPE mode, KHWL_TV_STANDARD_TYPE standard)
{
	static KHWL_TV_OUTPUT_FORMAT_TYPE old_mode = evTvOutputFormat_NONE;
	static int old_device = evOutputDevice_TV;
	static int old_standard = -1;
	static BOOL old_wide = TRUE;

	static const KHWL_DIG_OV_PARAMS dig_params[10] = 
	{
		// YUV-480P
		{ 31469, 5994, 720, 480, 858, 16, 62, 60, 525, 9, 6, 30, 0, 0, 0, 0,
			8, evTvStandard_480P, 0, 0, 1, 1, 0 },
		// YUV-576P
		{ 31250, 5004, 720, 576, 864, 20, 63, 60, 625, 5, 5, 39, 0, 0, 0, 0,
			8, evTvStandard_576P, 0, 0, 1, 1, 0 },
		// YUV-720P
		{ 45000, 6000, 1280, 720,  1650, 110, 40, 220, 750, 5, 5, 20, 0, 0, 1, 1,
			24, evTvStandard_720P, 0, 0, 1, 1, 0 },
		// YUV-720P-50
		{ 37500, 5000, 1280, 720,  1650, 110, 40, 220, 750, 5, 5, 20, 0, 0, 1, 1,
			24, evTvStandard_720P, 0, 0, 1, 1, 0 },
		// YUV-1080I
        { 33716, 5994, 1920, 1080, 2199, 88, 44, 148, 1125, 5, 10, 30, 0, 1, 0, 0,
			24, evTvStandard_1080I, 0, 0, 1, 1, 0 },

		/////////////////////

		// DVI-480P
		{ 31469, 5994, 720, 480, 858, 16, 62, 60, 525, 9, 6, 30, 0, 0, 0, 0,
			24, evTvStandard_480P, 0, 0, 1, 1, 0 },
		// DVI-576P
		{ 31250, 5004, 720, 576, 864, 20, 63, 60, 625, 5, 5, 39, 0, 0, 0, 0,
			24, evTvStandard_576P, 0, 0, 1, 1, 0 },
		// DVI-1080I
        { 33716, 5994, 1920, 1080, 2199, 88, 44, 148, 1125, 5, 10, 30, 0, 1, 1, 1,
			24, evTvStandard_1080I, 0, 0, 1, 1, 0 },
	};

	int use_dvi = false, use_hd = false;
	bool is_on = mode != evTvOutputFormat_OUTPUT_OFF;
	
	int val;
	if (is_on)
	{
		if (standard != old_standard)
		{
			KHWL_WINDOW valid_wnd;
			const KHWL_DIG_OV_PARAMS *dig = NULL;
			valid_wnd.x = valid_wnd.y = 0;

			// set params
			switch (standard)
			{
			case evTvStandard_480P:
				dig = &dig_params[0];
				break;
			case evTvStandard_576P:
				dig = &dig_params[1];
				break;
			case evTvStandard_720P:
				dig = &dig_params[2];
				break;
			case evTvStandard_720P50:
				dig = &dig_params[3];
				break;
			case evTvStandard_1080I:
				dig = &dig_params[4];
				break;
			case evTvStandard_NTSC:
				valid_wnd.w = 720;
				valid_wnd.h = 480;
				break;
			default:
			case evTvStandard_PAL:
				valid_wnd.w = 720;
				valid_wnd.h = 576;
				break;

			// TODO: add HDTV modes...
			}

			if (dig != NULL)
			{
				valid_wnd.w = dig->VideoWidth;
				valid_wnd.h = dig->VideoHeight;

				khwl_setproperty(KHWL_VIDEO_SET, evDigOvOnlyParams, sizeof(KHWL_DIG_OV_PARAMS), (void *)dig);

				old_mode = evTvOutputFormat_NONE;
				old_standard = -1;
				standard = dig->TvHdtvStandard;

				if (mode == evTvOutputFormat_COMPONENT_YUV)
					use_hd = true;
				else if (mode == evTvOutputFormat_DVI)
					use_dvi = true;
			} else
			{
				val = standard;
				khwl_setproperty(KHWL_VIDEO_SET, evTvStandard, sizeof(DWORD), &val);
				if (old_standard != evTvStandard_NTSC && old_standard != evTvStandard_PAL)
					old_mode = evTvOutputFormat_NONE;
				old_standard = standard;
			}

			khwl_setproperty(KHWL_VIDEO_SET, evValidWindow, sizeof(valid_wnd), &valid_wnd);
		}

		if (mode != old_mode)
		{
			val = (mode == evTvOutputFormat_DVI) ? evTvOutputFormat_COMPONENT_RGB_SCART : mode;
			khwl_setproperty(KHWL_VIDEO_SET, evTvOutputFormat, sizeof(DWORD), &val);

			val = use_dvi ? evOutputDevice_HDTV : (use_hd ? evOutputDevice_DigOvOnly : evOutputDevice_TV);
			if (old_device != val)
			{
				khwl_setproperty(KHWL_VIDEO_SET, evOutputDevice, sizeof(DWORD), &val);
				old_device = val;
			}

			KHWL_ADDR_DATA data;
			
			// composite/component?
			data.Addr = 3;
			data.Data = (mode == evTvOutputFormat_COMPOSITE) ? 1 : 0;
			khwl_setproperty(KHWL_BOARDINFO_SET, ebiPIOAccess, sizeof(data), &data);
			
			// DVI
			data.Addr = 5;
			data.Data = use_dvi ? 0 : 1;
			khwl_setproperty(KHWL_BOARDINFO_SET, ebiPIOAccess, sizeof(data), &data);
			
			// RCA-output?
			data.Addr = 6;
			data.Data = (mode != evTvOutputFormat_COMPONENT_RGB_SCART) ? 1 : 0;
			khwl_setproperty(KHWL_BOARDINFO_SET, ebiPIOAccess, sizeof(data), &data);

			khwl_switch_dvi(use_hd || use_dvi);
		}
	
		// Wide
		if (khwl_wide != old_wide)
		{
			KHWL_ADDR_DATA data;
			data.Addr = 7;
			data.Data = khwl_wide ? 1 : 0;
			khwl_setproperty(KHWL_BOARDINFO_SET, ebiPIOAccess, sizeof(data), &data);
			old_wide = khwl_wide;
		}

		// init as normal/wide
		khwl_getproperty(KHWL_VIDEO_SET, evMaxDisplayWindow, sizeof(dest_wnd), &dest_wnd);

		int osd_width, osd_height;
		khwl_get_osd_size(&osd_width, &osd_height);
		khwl_set_max_osd_wnd(dest_wnd.w, osd_width, dest_wnd.h, osd_height);
		
		khwl_restoreparams();
			
		khwl_set_window(-1, -1, -1, -1, 100, 100, 0, 0);
		
	} else
	{
		if (mode != old_mode)
		{
			val = mode;
			khwl_setproperty(KHWL_VIDEO_SET, evTvOutputFormat, sizeof(DWORD), &val);
		}

		if (old_mode == evTvOutputFormat_DVI)
			khwl_switch_dvi(false);

		KHWL_ADDR_DATA data;
		/*
		data.Addr = 5;
		data.Data = 1;
		khwl_setproperty(KHWL_BOARDINFO_SET, ebiPIOAccess, sizeof(data), &data);
		*/

		data.Addr = 6;
		data.Data = 1;
		khwl_setproperty(KHWL_BOARDINFO_SET, ebiPIOAccess, sizeof(data), &data);
	}

	old_mode = mode;

	return TRUE;
}

void khwl_transform(KHWL_VIDEOMODE mode, int *x, int *y)
{
	switch (mode)
	{
	case KHWL_VIDEOMODE_PANSCAN:
		*x = 4 * (*x) / 3;
		break;
	case KHWL_VIDEOMODE_LETTERBOX:
		*y = 3 * (*y) / 4;
		break;
	case KHWL_VIDEOMODE_HCENTER:
		*x = 3 * (*x) / 4;
		break;
	case KHWL_VIDEOMODE_VCENTER:
		*y = 4 * (*y) / 3;
		break;
	default:
		;
	}
}

void khwl_inversetransform(KHWL_VIDEOMODE mode, int *x, int *y)
{
	switch (mode)
	{
	case KHWL_VIDEOMODE_PANSCAN:
		*x = 3 * (*x) / 4;
		break;
	case KHWL_VIDEOMODE_LETTERBOX:
		*y = 4 * (*y) / 3;
		break;
	case KHWL_VIDEOMODE_HCENTER:
		*x = 4 * (*x) / 3;
		break;
	case KHWL_VIDEOMODE_VCENTER:
		*y = 3 * (*y) / 4;
		break;
	default:
		;
	}
}

void khwl_getscreensize(int *width, int *height)
{
	KHWL_WINDOW wnd;
	khwl_getproperty(KHWL_VIDEO_SET, evMaxDisplayWindow, sizeof(wnd), &wnd);
	*width = wnd.w;
	*height = wnd.h;
}

void khwl_transformcoord(KHWL_VIDEOMODE mode, int *x, int *y, int /*width*/, int /*height*/)
{
	// correction for NTSC/PAL
/*
	int sw, sh;
	khwl_getscreensize(&sw, &sh);
	if (width != 0)
		*x = *x * sw / width;
	if (height != 0)
		*y = *y * sh / height;
*/	
	// correction for aspect ratio
	khwl_transform(mode, x, y);
	// correction for black bands (letterbox & hcenter)
	*x += dest_wnd.x;
	*y += dest_wnd.y;
}

void khwl_getdestwindow(KHWL_VIDEOMODE mode, KHWL_WINDOW *wnd)
{
	int screenWidth, screenHeight;
	khwl_getscreensize(&screenWidth, &screenHeight);
	
	int displayWidth = screenWidth;
	int displayHeight = screenHeight;

	khwl_transform(mode, &displayWidth, &displayHeight);
	
	wnd->x = (screenWidth - displayWidth) / 2;
	wnd->y = (screenHeight - displayHeight) / 2;
	wnd->w = displayWidth;
	wnd->h = displayHeight;

	//khwl_setproperty(KHWL_VIDEO_SET, evDestinationWindow, sizeof(*wnd), wnd);
}


void khwl_setvideomode(KHWL_VIDEOMODE mode, BOOL set_hw)
{
	KHWL_IN_ASPECT_RATIO_TYPE aspect = evInAspectRatio_4x3;
	KHWL_OUT_DISPLAY_OPTION_TYPE output = evOutDisplayOption_Normal;
	int panscan = 0;
	KHWL_ADDR_DATA pio7 = { 7, 0 };
	pio7.Data = 0;

	switch (mode)
	{
	case KHWL_VIDEOMODE_NONE:
		aspect = evInAspectRatio_none;
		output = evOutDisplayOption_Normal;
		break;
	case KHWL_VIDEOMODE_NORMAL:
		aspect = evInAspectRatio_4x3;
		output = evOutDisplayOption_Normal;
		break;
	case KHWL_VIDEOMODE_VCENTER:
		aspect = evInAspectRatio_4x3;
		output = evOutDisplayOption_4x3to16x9_VertCenter;
		khwl_wide = 1;
		break;
	case KHWL_VIDEOMODE_HCENTER:
		aspect = evInAspectRatio_4x3;
		output = evOutDisplayOption_4x3to16x9_HorzCenter;
		khwl_wide = 1;
		break;
	case KHWL_VIDEOMODE_WIDE:
		aspect = evInAspectRatio_16x9;
		output = evOutDisplayOption_Normal;
		khwl_wide = 1;
		break;
	case KHWL_VIDEOMODE_PANSCAN:
		aspect = evInAspectRatio_16x9;
		output = evOutDisplayOption_16x9to4x3_PanScan;
		panscan = 1;
		break;
	case KHWL_VIDEOMODE_LETTERBOX:
		aspect = evInAspectRatio_16x9;
		output = evOutDisplayOption_16x9to4x3_LetterBox;
		break;
	}

	if (set_hw && (mode != cur_mode || aspect != cur_aspect || output != cur_output))
	{
		cur_mode = mode;
		cur_aspect = aspect;
		cur_output = output;

		khwl_set_window(-1, -1, -1, -1, 100, 100, 0, 0);
/*
		//khwl_getdestwindow(mode, &dest_wnd);
		khwl_getproperty(KHWL_VIDEO_SET, evMaxDisplayWindow, sizeof(dest_wnd), &dest_wnd);

		KHWL_IN_ASPECT_RATIO_TYPE a = (aspect == evInAspectRatio_none) ? evInAspectRatio_4x3 : aspect;
		khwl_setproperty(KHWL_VIDEO_SET, evInAspectRatio, sizeof(a), &a);
		
		khwl_setproperty(KHWL_VIDEO_SET, evOutDisplayOption, sizeof(output), &output);

		khwl_setproperty(KHWL_VIDEO_SET, evDestinationWindow, sizeof(dest_wnd), &dest_wnd);

		khwl_setproperty(KHWL_VIDEO_SET, evForcePanScanDefaultSize, sizeof(panscan), &panscan);
*/
		pio7.Data = khwl_wide ? 1 : 0;
		khwl_setproperty(KHWL_BOARDINFO_SET, ebiPIOAccess, sizeof(pio7), &pio7);
	}
	cur_mode = mode;
	cur_aspect = aspect;
	cur_output = output;
}

static int old_sw = 720, old_sh = 480;
static KHWL_ZOOMMODE zoom_mode = KHWL_ZOOMMODE_YUV;

void khwl_set_window_source(int src_width, int src_height)
{
	old_sw = src_width;
	old_sh = src_height;
}

void khwl_set_window_zoom(KHWL_ZOOMMODE zm)
{
	zoom_mode = zm;
}

void khwl_set_window_frame(int fr_left, int fr_top, int fr_right, int fr_bottom)
{
	frame_left = MIN(MAX(fr_left, 0), 719) & 0x3ffe;
	frame_top = MIN(MAX(fr_top, 0), 479) & 0x3ffe;
	frame_right = MIN(MAX(fr_right, 0), 719);
	frame_bottom = MIN(MAX(fr_bottom, 0), 479);

	if ((frame_right - frame_left + 1) & 1)
		frame_right--;
	if ((frame_bottom - frame_top + 1) & 1)
		frame_bottom--;

	frame_right = MAX(frame_left, frame_right);
	frame_bottom = MAX(frame_top, frame_bottom);
}

void khwl_get_window_frame(int *fr_left, int *fr_top, int *fr_right, int *fr_bottom)
{
	*fr_left = frame_left;
	*fr_top = frame_top;
	*fr_right = frame_right;
	*fr_bottom = frame_bottom;
}

void khwl_apply_aspect(KHWL_WINDOW *dstwnd, int maxw, int maxh, int src_width, int src_height, int offsx, int offsy)
{
	int dstw = dstwnd->w;
	int dsth = dstwnd->h;

	int which = -1;

	// process special DVD modes first
	if (cur_output == evOutDisplayOption_4x3to16x9_VertCenter)
	{
		src_width = 4;
		src_height = 3;
		which = 3;
	}
	else if (cur_output == evOutDisplayOption_4x3to16x9_HorzCenter)
	{
		src_width = 4;
		src_height = 3;
		which = 2;
	}
	else if (cur_output == evOutDisplayOption_16x9to4x3_PanScan)
	{
		src_width = 16;
		src_height = 9;
		which = 0;
	}
	else if (cur_output == evOutDisplayOption_16x9to4x3_LetterBox)
	{
		src_width = 16;
		src_height = 9;
		which = 1;
	}
	else if (cur_aspect == evInAspectRatio_4x3)
	{
		if (3 * src_width < 4 * src_height)
			which = 0;
		else
			which = 1;
	} 
	else if (cur_aspect == evInAspectRatio_16x9)	// Wide
	{
		if (9 * src_width < 16 * src_height)
			which = 2;
		else
			which = 3;
	}

	switch (which)
	{
	case 0:
		dstw = 3 * dstw * src_width / (4 * src_height);
		dsth = dsth;
		break;
	case 1:
		dstw = dstw;
		dsth = 4 * dsth * src_height / (3 * src_width);
		break;
	case 2:
		dstw = 9 * dstw * src_width / (16 * src_height);
		dsth = dsth;
		break;
	case 3:
		dstw = dstw;
		dsth = 16 * dsth * src_height / (9 * src_width);
		break;
	default:
		;
	}
				
	dstwnd->x = (maxw - dstw)/2;
	dstwnd->y = (maxh - dsth)/2;
	
	int x = Abs(dstwnd->x), y = Abs(dstwnd->y);
	dstwnd->x += MAX(MIN(offsx, x), -x);
	dstwnd->y += MAX(MIN(offsy, y), -y);
	
	dstwnd->w = dstw;
	dstwnd->h = dsth;

}

void khwl_set_window(int src_width, int src_height, int frame_width, int frame_height, int hscale, int vscale, int offsx, int offsy)
{
	KHWL_WINDOW wnd, dstwnd;
	khwl_getproperty(KHWL_VIDEO_SET, evMaxDisplayWindow, sizeof(dstwnd), &dstwnd);

	KHWL_WINDOW curdst, maxdst;
	khwl_getproperty(KHWL_VIDEO_SET, evDestinationWindow, sizeof(curdst), &curdst);
	khwl_getproperty(KHWL_VIDEO_SET, evMaxDisplayWindow, sizeof(maxdst), &maxdst);

	if (src_width < 0)
		src_width = old_sw;
	if (src_height < 0)
		src_height = old_sh;

	int frame_x = 0, frame_y = 0;
	if (frame_width > 0 && frame_height > 0)
	{
		wnd.x = 0;
		wnd.y = 0;
		wnd.w = frame_width;
		wnd.h = frame_height;
		khwl_setproperty(KHWL_VIDEO_SET, evZoomedWindow, sizeof(wnd), &wnd);
		wnd.x = 0;
		wnd.y = 0;
		wnd.w = (frame_width + 15) & ~15;
		wnd.h = (frame_height + 15) & ~15;
		khwl_setproperty(KHWL_VIDEO_SET, evSourceWindow, sizeof(wnd), &wnd);

		KHWL_YUV_WRITE_PARAMS_TYPE yuvparams;
		yuvparams.wWidth = (WORD)wnd.w;
		yuvparams.wHeight = (WORD)wnd.h;
		yuvparams.YUVFormat = KHWL_YUV_420_UNPACKED;
		khwl_setproperty(KHWL_VIDEO_SET, evYUVWriteParams, sizeof(yuvparams), &yuvparams);
	} else
	{
		khwl_getproperty(KHWL_VIDEO_SET, evSourceWindow, sizeof(wnd), &wnd);
		frame_x = wnd.x;
		frame_y = wnd.y;
		frame_width = wnd.w;
		frame_height = wnd.h;
	}

	if (src_width < 0)
		src_width = frame_width;
	if (src_height < 0)
		src_height = frame_height;

	int sw, sh, sx, sy;
	
	sx = src_width / 2;
	sy = src_height / 2;
	sw = src_width;
	sh = src_height;
	
	if (hscale < 10)
		hscale = 10;
	if (hscale > 400)
		hscale = 400;
	if (vscale < 10)
		vscale = 10;
	if (vscale > 400)
		vscale = 400;

#if 0
	msg("* zmode=%d, hscale=%d, vscale=%d, src=(%dx%d).\n", zoom_mode, hscale, vscale, src_width, src_height);
#endif

	if (zoom_mode == KHWL_ZOOMMODE_YUV)
	{
		if (hscale < 100)
			dstwnd.w = dstwnd.w * hscale / 100;
		else
			sw = src_width * 100 / hscale;
		if (vscale < 100)
			dstwnd.h = dstwnd.h * vscale / 100;
		else
			sh = src_height * 100 / vscale;

		khwl_apply_aspect(&dstwnd, maxdst.w, maxdst.h, src_width, src_height, 0, 0);
		
		wnd.x = sx - sw/2;
		wnd.y = sy - sh/2;
		wnd.w = sw;
		wnd.h = sh;
		wnd.x += MAX(MIN(offsx, wnd.x), -wnd.x);
		wnd.y += MAX(MIN(offsy, wnd.y), -wnd.y);
		wnd.x += (frame_width - src_width)/2;
		wnd.y += (frame_height - src_height)/2;
		khwl_setproperty(KHWL_VIDEO_SET, evZoomedWindow, sizeof(wnd), &wnd);

		khwl_setproperty(KHWL_VIDEO_SET, evDestinationWindow, sizeof(dstwnd), &dstwnd);
	} 
	else if (zoom_mode == KHWL_ZOOMMODE_ASPECT || zoom_mode == KHWL_ZOOMMODE_DVD)
	{
#if 0
		if (zoom_mode == KHWL_ZOOMMODE_DVD && vscale == 100 && hscale == 100)
			khwl_setvideomode(cur_mode, TRUE);
		else
#endif
		{
			if (hscale == vscale)	// constrain proportions...
			{
				dstwnd.w = MAX(dstwnd.w * hscale / 100, 64);
				dstwnd.h = dstwnd.h * dstwnd.w / maxdst.w;
			} else
			{
				dstwnd.w = MAX(dstwnd.w * hscale / 100, 64);
				dstwnd.h = MAX(dstwnd.h * vscale / 100, 64);
			}
			
			// fix aspect ratio for anamorphic modes only
			int old_asp = cur_aspect;
			int old_out = cur_output;
			if (zoom_mode == KHWL_ZOOMMODE_DVD && (cur_mode == KHWL_VIDEOMODE_NORMAL || cur_mode == KHWL_VIDEOMODE_WIDE))
				cur_aspect = evInAspectRatio_none;
			
			khwl_apply_aspect(&dstwnd, maxdst.w, maxdst.h, src_width, src_height, offsx, offsy);
			
			cur_aspect = evInAspectRatio_4x3;
			khwl_setproperty(KHWL_VIDEO_SET, evInAspectRatio, sizeof(cur_aspect), &cur_aspect);
			
			cur_output = evOutDisplayOption_Normal;
			khwl_setproperty(KHWL_VIDEO_SET, evOutDisplayOption, sizeof(cur_output), &cur_output);

			khwl_setproperty(KHWL_VIDEO_SET, evDestinationWindow, sizeof(dstwnd), &dstwnd);
			
			int panscan = 0;
			khwl_setproperty(KHWL_VIDEO_SET, evForcePanScanDefaultSize, sizeof(panscan), &panscan);

			cur_aspect = old_asp;
			cur_output = old_out;

			//cur_mode = KHWL_VIDEOMODE_NONE;
		}
	}
#if 0
	msg("zoom(%d,%d)=%d,%d %dx%d\n", hscale, vscale, wnd.x, wnd.y, wnd.w, wnd.h);
	msg(" frm=%d,%d %dx%d, src=%dx%d,\n", frame_x, frame_y, frame_width, frame_height, src_width, src_height);

	msg("* olddst=%d,%d %dx%d\n", curdst.x, curdst.y, curdst.w, curdst.h);
	msg("* maxdst=%d,%d %dx%d\n", maxdst.x, maxdst.y, maxdst.w, maxdst.h);
	msg("*    dst=%d,%d %dx%d\n", dstwnd.x, dstwnd.y, dstwnd.w, dstwnd.h);
#endif
	old_sw = src_width;
	old_sh = src_height;
}

void khwl_set_display_settings(int brightness, int contrast, int saturation)
{
	if (brightness != -1)
	{
		brightness = MIN(MAX(brightness, 0), 1000);
		khwl_setproperty(KHWL_VIDEO_SET, evBrightness, sizeof(brightness), &brightness);
	}
	if (contrast != -1)
	{
		contrast = MIN(MAX(contrast, 0), 1000);
		khwl_setproperty(KHWL_VIDEO_SET, evContrast, sizeof(contrast), &contrast);
	}
	if (saturation != -1)
	{
		saturation = MIN(MAX(saturation, 0), 1000);
		khwl_setproperty(KHWL_VIDEO_SET, evSaturation, sizeof(saturation), &saturation);
	}
}

void khwl_get_display_settings(int *brightness, int *contrast, int *saturation)
{
	if (brightness != NULL)
		khwl_getproperty(KHWL_VIDEO_SET, evBrightness, sizeof(brightness), brightness);
	if (contrast != NULL)
		khwl_getproperty(KHWL_VIDEO_SET, evContrast, sizeof(contrast), contrast);
	if (saturation != NULL)
		khwl_getproperty(KHWL_VIDEO_SET, evSaturation, sizeof(saturation), saturation);
}
