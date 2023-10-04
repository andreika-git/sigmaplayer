//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - KHWL interface functions source file (Win32)
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

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <io.h>

#include "sp_misc.h"
#include "sp_khwl.h"
#include "sp_mpeg.h"
#include "sp_fip.h"
#include "sp_css.h"

#include "resource.h"

//#define DUMP_VIDEO_FRAMES_ORDER
//#define DUMP_VIDEO
//#define DUMP_VIDEO_PACKET

int khwl_handle = -1;

static HANDLE khwl_parser = NULL;
static DWORD  khwl_parser_id = (DWORD)-1;
static HANDLE khwl_parser_mutex = NULL;
static int khwl_rate = 24000, khwl_scale = 1000;

static int khwl_audio_format = 0;
static int khwl_audio_sample_rate = 0, khwl_audio_bits = 0, khwl_audio_channels = 0;
static LONGLONG khwl_audio_pts_per_Mb = 0;

// here we store the data
static MpegPlayStruct mpeg_playstruct_storage[4];

/// Muxed media packets
MpegPlayStruct *MPEG_PLAY_STRUCT  = &mpeg_playstruct_storage[0];
/// Video packets
MpegPlayStruct *MPEG_VIDEO_STRUCT = &mpeg_playstruct_storage[1];
/// Audio packets
MpegPlayStruct *MPEG_AUDIO_STRUCT = &mpeg_playstruct_storage[2];
/// Subpicture packets
MpegPlayStruct *MPEG_SPU_STRUCT   = &mpeg_playstruct_storage[3];

BYTE *BUF_BASE = NULL;

const int yoffset = 110;

const int osd_width = 640, osd_height = 480;

static char *wndtitle = "SigmaPlayer :: version " __DATE__ " " __TIME__ " :: ";

static struct  // BITMAPINFO with 16 colors
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD      bmiColors[256];
} bi;

static HDC hMemDC = NULL;
static HBITMAP hSrcBitmap = NULL;
static HDC curDC = NULL;
static ULONGLONG khwl_pts = 0;

static BYTE tmp_UV[720 * 576/2];
static BYTE tmp_Y[720 * 576];
static int yuv_width = 720, yuv_height = 480;
static BOOL yuv_dirty = FALSE;
static BYTE tv_mask[osd_width * osd_height];
static int tv_brightness = 500, tv_contrast = 500, tv_saturation = 500;

static bool step_mode = false, use_tvmask = false;

static int fip_button = -1;

static int freq_reg = ((1 << 8) | (34 << 2) | 0x02);


#ifdef DUMP_VIDEO_FRAMES_ORDER
#include "sp_msg.h"
#endif


void CALLBACK OSDUpdate( UINT /*uID*/, UINT /*uMsg*/, DWORD /*dwUser*/,
                          DWORD /*dw1*/, DWORD /*dw2*/ )
{
    khwl_osd_update();
}

bool khwl_msgloop();

int khwl_nextfeedpacket(MpegPlayStruct *feed)
{
	MpegPlayStruct *from = MPEG_PLAY_STRUCT;
	MpegPacket *feedin = feed->in;
	if (feedin == NULL)
		return FALSE;

	//!!!!!!!!!!!!!!!!!!!!!!!!
#ifdef DUMP_VIDEO_FRAMES_ORDER
	if (feedin->type == 0)
	{
		char frms[] = " IPB";
		int frm_type = -1;
		if (feedin->pData[0] == 0 && feedin->pData[1] == 0 && feedin->pData[2] == 1 && feedin->pData[3] == 0xb6)
			frm_type = (int)(feedin->pData[4] >> 6);
		msg("(%c) PTS = %d\n", frms[frm_type+1], feedin->pts);
	}
#endif

#ifdef DUMP_VIDEO
	if (feedin->type == 0)
	{
		FILE *fp;
		fp = fopen("out.m4v", "ab");
		fwrite(feedin->pData, feedin->size, 1, fp);
		fclose(fp);
	}
#endif
#ifdef DUMP_VIDEO_PACKET
	if (feedin->type == 0)
	{
		static int p_idx = 0;
		FILE *fp;
		fp = fopen("out_packets.log", "ab");
		//fprintf(fp, "[%4d] flg=%2x siz=%6d pts=" PRINTF_64d "\n", p_idx++, feedin->flags, feedin->size, feedin->pts);
		fprintf(fp, "[%4d] flg=%2x siz=%6d\n", p_idx++, feedin->flags, feedin->size);
		fclose(fp);
	}
#endif

	//!!!!!!!!!!!!!!!!!!!!!!!!

	if (feedin->flags == 2)
		khwl_pts = feedin->pts;
	else if (feedin->flags == 0x80)
	{
		khwl_pts = INT64(90000) * feedin->pts / khwl_rate;
	}
	else if (feedin->flags == 0 && feedin->type == 1)
	{
		khwl_pts += khwl_audio_pts_per_Mb * feedin->size / (1024*1024);
	}

	feed->in = feed->in->next;

	feed->num--;
	feed->in_cnt++;
	from->num++;
	from->out_cnt++;

	if (feedin->next == NULL)
		feed->out = NULL;
	if (from->out == NULL)
		from->in = feedin;
	else
		from->out->next = feedin;
	from->out = feedin;
	feedin->next = NULL;

	////// simulate budidx decrement
	if (feedin->bufidx)
	{
		if (*(feedin->bufidx) > 0)
			(*feedin->bufidx)--;
	}

	return TRUE;
}

void __stdcall khwl_parser_proc(void *)
{
	for (;;)
	{
		WaitForSingleObject(khwl_parser_mutex, INFINITE);
		if (MPEG_VIDEO_STRUCT->in != NULL)
			khwl_nextfeedpacket(MPEG_VIDEO_STRUCT);
		if (MPEG_AUDIO_STRUCT->in != NULL)
			khwl_nextfeedpacket(MPEG_AUDIO_STRUCT);
		if (MPEG_SPU_STRUCT->in != NULL)
			khwl_nextfeedpacket(MPEG_SPU_STRUCT);

		ReleaseMutex(khwl_parser_mutex);

		// simulate hard work... :-)
		//Sleep(3);

		// well, it isn't a real step mode 'cause we don't wait for I-frames...
		/*
		if (step_mode)
		{
			khwl_pause();
			step_mode = false;
			
		}
		*/

		
	}
}

extern int fip_lastkey;

KHWL_OSDSTRUCT osd;

static BOOL module_applied = FALSE;

DWORD *backbuf = NULL;
DWORD *osdbuf = NULL;
BYTE *buf = NULL;
KHWL_WINDOW max_wnd, dest_wnd, zoomed_wnd, src_wnd, valid_wnd, osd_wnd;

DWORD vol[2] = { 50, 50 };
int audio_offset = 0;

BYTE p_challenge[10];
BYTE p_key1[5], p_key2[5], p_key_check[5];
int css_variant = 0;

void khwl_calc_audio_pts_rate()
{
	if (khwl_audio_format == eAudioFormat_PCM)
	{
		khwl_audio_pts_per_Mb = khwl_audio_sample_rate * khwl_audio_bits * khwl_audio_channels;
		if (khwl_audio_pts_per_Mb != 0)
			khwl_audio_pts_per_Mb = INT64(90000)*8*1024*1024 / khwl_audio_pts_per_Mb;
	}
}

int khwl_setproperty(KHWL_PROPERTY_SET pset, int id, int size, void *value)
{
	int i;
	DWORD val = *(DWORD *)value;
	if (pset == KHWL_COMMON_SET && id == eDoAudioLater)
	{
		audio_offset = (int)val;
	}
	else if (pset == KHWL_BOARDINFO_SET && id == ebiCommand)
	{
		if (*(int *)value == ebiCommand_VideoHwBlackFrame)
		{
			memset(backbuf, 0, 720*480*4);
			khwl_osd_update();
		}
	}
	else if (pset == KHWL_VIDEO_SET)
	{
		if (id == evDestinationWindow)
			dest_wnd = *((KHWL_WINDOW *)value);
		else if (id == evZoomedWindow)
			zoomed_wnd = *((KHWL_WINDOW *)value);
		else if (id == evSourceWindow)
			src_wnd = *((KHWL_WINDOW *)value);
		else if (id == evValidWindow)
			valid_wnd = *((KHWL_WINDOW *)value);
		else if (id == evYUVWriteParams)
		{
			KHWL_YUV_WRITE_PARAMS_TYPE *params = (KHWL_YUV_WRITE_PARAMS_TYPE *)value;
			yuv_width = params->wWidth;
			yuv_height = params->wHeight;
		}
		else if (id == evBrightness)
			tv_brightness = val;
		else if (id == evContrast)
			tv_contrast = val;
		else if (id == evSaturation)
			tv_saturation = val;
	}
	else if (pset == KHWL_OSD_SET && id == eOsdDestinationWindow)
	{
		osd_wnd = *((KHWL_WINDOW *)value);
	}
	else if (pset == KHWL_TIME_SET)
	{
		if (id == etimSystemTimeClock)
		{
			KHWL_TIME_TYPE *t = (KHWL_TIME_TYPE *)value;
			khwl_pts = t->pts;
		}
		else if (id == etimVideoCTSTimeScale)
		{
			khwl_rate = val;
		}
	}
	else if (pset == KHWL_AUDIO_SET)
	{
		if (id == eaVolumeLeft)
			vol[0] = val;
		else if (id == eaVolumeRight)
			vol[1] = val;
		else if (id == eAudioFormat)
		{
			khwl_audio_format = val;
			khwl_calc_audio_pts_rate();
		}
		else if (id == eAudioSampleRate)
		{
			khwl_audio_sample_rate = val;
			khwl_calc_audio_pts_rate();
		}
		else if (id == eAudioNumberOfBitsPerSample)
		{
			khwl_audio_bits = val;
			khwl_calc_audio_pts_rate();
		}
		else if (id == eAudioNumberOfChannels)
		{
			khwl_audio_channels = val;
			khwl_calc_audio_pts_rate();
		}
	}
	else if (pset == KHWL_EEPROM_SET && id == eEepromAccess)
	{
		KHWL_ADDR_DATA *data = (KHWL_ADDR_DATA *)value;
		FILE *fp = fopen("settings.dat", "rb+");
		if (fp == NULL)
			fp = fopen("settings.dat", "wb");
		if (fp != NULL)
		{
			fseek(fp, data->Addr, SEEK_SET);
			BYTE d = (BYTE)data->Data;
			fwrite(&d, sizeof(BYTE), 1, fp);
			fclose(fp);

		}
	}
	else if (pset == KHWL_DECODER_SET)
	{
		if (id == edecCSSKey1)
		{
			for (i = 0 ; i < size; i++)
			{
				p_key1[i] = ((char *)value)[4-i];
			}

			for (i = 0 ; i < 32; ++i)
			{
				CryptKey(0, i, p_challenge, p_key_check);

				if (memcmp( p_key_check, p_key1, size) == 0)
				{
					css_variant = i;
					break;
				}
			}
		}
		else if (id == edecCSSChlg2)
		{
			for( i = 0 ; i < size; ++i)
			{
				p_challenge[i] = ((char *)value)[9-i];
			}

			CryptKey(1, css_variant, p_challenge, p_key2);
		}
		else if (id == edecForceFixedVOPRate)
		{
			KHWL_FIXED_VOP_RATE_TYPE *params = (KHWL_FIXED_VOP_RATE_TYPE *)value;
			khwl_scale = params->time_incr;
		}
	}

	return 0;
}

int khwl_getproperty(KHWL_PROPERTY_SET pset, int id, int size, void *value)
{
	int i;
	if (pset == KHWL_COMMON_SET && id == eDoAudioLater)
	{
		*(DWORD *)value = audio_offset;
	}
	else if (pset == KHWL_TIME_SET && id == etimSystemTimeClock)
		((KHWL_TIME_TYPE *)value)->pts = khwl_pts;
	else if (pset == KHWL_VIDEO_SET)
	{
		if (id == evMaxDisplayWindow)
			*((KHWL_WINDOW *)value) = max_wnd;
		else if (id == evDestinationWindow)
			*((KHWL_WINDOW *)value) = dest_wnd;
		else if (id == evSourceWindow)
			*((KHWL_WINDOW *)value) = src_wnd;
		else if (id == evZoomedWindow)
			*((KHWL_WINDOW *)value) = zoomed_wnd;
		else if (id == evValidWindow)
			*((KHWL_WINDOW *)value) = valid_wnd;
		else if (id == evBrightness)
			*(DWORD *)value = tv_brightness;
		else if (id == evContrast)
			*(DWORD *)value = tv_contrast;
		else if (id == evSaturation)
			*(DWORD *)value = tv_saturation;
	}
	else if (pset == KHWL_OSD_SET && id == eOsdDestinationWindow)
		*((KHWL_WINDOW *)value) = osd_wnd;
	else if (pset == KHWL_TIME_SET && id == etimVideoFrameDisplayedTime)
		((KHWL_TIME_TYPE *)value)->pts = khwl_pts;
	else if (pset == KHWL_EEPROM_SET && id == eEepromAccess)
	{
		KHWL_ADDR_DATA *data = (KHWL_ADDR_DATA *)value;
		FILE *fp = fopen("settings.dat", "rb");
		if (fp != NULL)
		{
			if (fseek(fp, data->Addr, SEEK_SET) == 0)
			{
				BYTE d;
				fread(&d, sizeof(BYTE), 1, fp);
				data->Data = d;
			}
			fclose(fp);

		}
	}
	else if (pset == KHWL_AUDIO_SET && id == eaVolumeLeft)
	{
		*(DWORD *)value = vol[0];
	}
	else if (pset == KHWL_AUDIO_SET && id == eaVolumeRight)
	{
		*(DWORD *)value = vol[1];
	}
	else if (pset == KHWL_DECODER_SET && id == edecCSSChlg)
	{
		for (i = 0 ; i < size; ++i )
			((char *)value)[9-i] = p_challenge[i] = (BYTE)i;
	}
	else if (pset == KHWL_DECODER_SET && id == edecCSSKey2)
	{
		for (i = 0; i < size; ++i)
		{
			((char *)value)[4-i] = p_key2[i];
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////

extern unsigned char fipram[8];

static HBITMAP hbm = 0;

void UpdateYUV()
{
	if (yuv_dirty)
	{
		int maxoff = 720 * 480;
		BYTE *ybuf = (BYTE *)((char *)tmp_Y);
		BYTE *uvbuf = (BYTE *)((char *)tmp_UV);

		for (int i = 0; i < maxoff; i++)
		{
			BYTE r, g, b;
			int o = i;
			int ui = (o / yuv_width) / 4 * yuv_width + (o % yuv_width)/2;
			int vi = (479 - MIN(o / yuv_width, 479)) * 720 + o % yuv_width;
			
			khwl_jpegyuvtorgb(ybuf[i], uvbuf[ui*2+1], uvbuf[ui*2], &r, &g, &b);
			khwl_tvyuvtovgargb(ybuf[i], uvbuf[ui*2+1], uvbuf[ui*2], &r, &g, &b);
			
			if (vi >= 0 && vi < maxoff)
				backbuf[vi] = (b << 16) | (g << 8) | r;
		}

		yuv_dirty = FALSE;
	}
}

void UpdateFIP(HDC hDC)
{
	static HBRUSH hbrw[3] = { 0 };
	
	if (hbrw[0] == 0)
	{
		LOGBRUSH lb;
		lb.lbColor = RGB(49, 1, 15);
		lb.lbStyle = BS_SOLID;
		lb.lbHatch = 0;
		hbrw[0] = CreateBrushIndirect(&lb);
	}
	if (hbrw[1] == 0)
	{
		LOGBRUSH lb;
		lb.lbColor = RGB(178, 242, 249);
		lb.lbStyle = BS_SOLID;
		lb.lbHatch = 0;
		hbrw[1] = CreateBrushIndirect(&lb);
	}
	if (hbrw[2] == 0)
	{
		LOGBRUSH lb;
		lb.lbColor = RGB(255, 10, 0);
		lb.lbStyle = BS_SOLID;
		lb.lbHatch = 0;
		hbrw[2] = CreateBrushIndirect(&lb);
	}

	SelectObject(hMemDC, hbm);
	BitBlt(hDC, 0, 0, 720, yoffset, hMemDC, 0, 0, SRCCOPY);

	RECT r;

	const int w = 2, h = 9, h2 = 5;
	const int cx[] = { w, 0, w+h, w, 0, w+h, w };
	const int cy[] = { w+h+w+h, w+h+w, w+h+w, w+h, w, w, 0 };
	const int cw[] = { h, w, w, h, w, w, h };
	const int ch[] = { w, h, h, w, h, h, w };
	int i, posx = 500;
	int posy = 11;
	for (i = 0; i < 7; i++)
	{
		posx += (w + h + w + w);
		if (i == 3 || i == 5)
			posx += h;
		for (int k = 0; k < 7; k++)
		{
			int bit = (fipram[6-i] >> k) & 1;
			r.left = posx + cx[k];
			r.top = posy + cy[k];
			r.right = r.left + cw[k];
			r.bottom = r.top + ch[k];
			FillRect(hDC, &r, hbrw[!bit ? 0 : (i < 2 ? 2 : 1)] );
		}
	}
	posx = 500;
	if (fip_get_special(FIP_SPECIAL_COLON1))
	{
		r.left = posx + (w+h)*5+w+h2;
		r.top = posy + (w+h/2);
		r.right = r.left + w;
		r.bottom = r.top + w;
		FillRect(hDC, &r, hbrw[1] );
		r.top = posy + (w+h)+(w+h/2);
		r.bottom = r.top + w;
		FillRect(hDC, &r, hbrw[1] );
	}
	if (fip_get_special(FIP_SPECIAL_COLON2))
	{
		r.left = posx + (w+h)*9+w;
		r.top = posy + (w+h/2);
		r.right = r.left + w;
		r.bottom = r.top + w;
		FillRect(hDC, &r, hbrw[1] );
		r.top = posy + (w+h)+(w+h/2);
		r.bottom = r.top + w;
		FillRect(hDC, &r, hbrw[1] );
	}
	if (fip_get_special(FIP_SPECIAL_PLAY))
	{
		POINT p[3];
		const int r = 5;
		const int off = 4;
		p[0].x = posx + off;
		p[0].y = posy + (w+h/2);
		p[1].x = p[0].x - r;
		p[1].y = p[0].y - r;
		p[2].x = p[0].x - r;
		p[2].y = p[0].y + r;
		HBRUSH oldbr = (HBRUSH)SelectObject(hDC, hbrw[1]);
		Polygon(hDC, p, 3);
		SelectObject(hDC, oldbr);
	}
	if (fip_get_special(FIP_SPECIAL_PAUSE))
	{
		const int d = 8;
		const int off = 5;
		r.left = posx + off;
		r.top = posy + (w);
		r.right = r.left + w;
		r.bottom = r.top + d;
		FillRect(hDC, &r, hbrw[2]);
		r.left = posx + off + w*2;
		r.right = r.left + w;
		FillRect(hDC, &r, hbrw[2]);
	}

	HBRUSH oldbr = (HBRUSH)SelectObject(hDC, hbrw[1]);
	const int rad = 15, srad = 7;
	const int off = 5;
	int cposx = posx - rad - off;
	int cposy = posy + rad;
	const int cpie[12][4] = 
	{ 
		{ -66, 74, -30, 95 },
		{ -95, 30, -74, 66 },
		{ -97, -20, -97, 20 },
		{ -74, -66, -95, -30 },
		{ -30, -95, -66, -74 },
		{ 20, -97, -20, -97 },
		{ 66, -74, 30, -95 },
		{ 95, -30, 74, -66 },
		{ 97, 20, 97, -20 },
		{ 74, 66, 95, 30 },
		{ 30, 95, 66, 74 },
		{ -20, 97, 20, 97 },

	};
	for (i = FIP_SPECIAL_CIRCLE_1; i <= FIP_SPECIAL_CIRCLE_12; i++)
	{
		if (fip_get_special(i))
		{
			POINT v1, v2;
			POINT l1, l2;
			int j = i - FIP_SPECIAL_CIRCLE_1;
			v1.x = cpie[j][0];
			v1.y = cpie[j][1];
			v2.x = cpie[j][2];
			v2.y = cpie[j][3];
			l1.x = v1.x + cposx;
			l1.y = v1.y + cposy;
			l2.x = v2.x + cposx;
			l2.y = v2.y + cposy;
			Pie(hDC, cposx - rad, cposy - rad, cposx + rad, cposy + rad, l1.x, l1.y, l2.x, l2.y);
		}
	}
	SelectObject(hDC, ::GetStockObject(BLACK_BRUSH));
	Ellipse(hDC, cposx - srad, cposy - srad, cposx + srad, cposy + srad);
	SelectObject(hDC, oldbr);
}

inline void FillHorzLine(BYTE *d, int x1, int x2, int y, BYTE color)
{
	d += y * osd_width + x1;
	memset(d, color, x2 - x1 + 1);
}

inline void FillVertLine(BYTE *d, int x, int y1, int y2, BYTE color)
{
	d += y1 * osd_width + x;
	y2 -= y1;
	for (int i = 0; i <= y2; i++)
	{
		*d = color;
		d += osd_width;
	}
}

static void CreateTvMask()
{
	const int num_scan_lines = 13;
	int i, scan_lines[2][num_scan_lines] = 
	{
		{ 45, 47, 48, 50, 52, 54, 57, 62, 68, 77, 92, 122, 212 },
		{ 37, 39, 40, 42, 44, 46, 49, 54, 60, 69, 84, 114, 204 },
	};

	memset(tv_mask, 0xff, osd_width * osd_height);
	for (i = 0; i <= 23; i++)
	{
		memset(tv_mask + i * osd_width, 0, osd_width);
		memset(tv_mask + (i + 456) * osd_width, 0, osd_width);
	}
	for (i = 0; i < osd_height; i++)
	{
		memset(tv_mask + i * osd_width, 0, 32);
		memset(tv_mask + i * osd_width + 608, 0, 32);
	}
	for (i = 0; i < num_scan_lines; i++)
	{
		FillHorzLine(tv_mask, 0, scan_lines[0][i], 36 - i, 0);
		FillHorzLine(tv_mask, 0, scan_lines[0][i], 443 + i, 0);
		FillHorzLine(tv_mask, osd_width - scan_lines[0][i], osd_width - 1, 36 - i, 0);
		FillHorzLine(tv_mask, osd_width - scan_lines[0][i], osd_width - 1, 443 + i, 0);
		FillVertLine(tv_mask, 44 - i, 0, scan_lines[1][i], 0);
		FillVertLine(tv_mask, 595 + i, 0, scan_lines[1][i], 0);
		FillVertLine(tv_mask, 44 - i, osd_height - scan_lines[1][i], osd_height - 1, 0);
		FillVertLine(tv_mask, 595 + i, osd_height - scan_lines[1][i], osd_height - 1, 0);
	}
}

void Generate_yuv2rgb_tables(void);
void new_rgbtoyuv(BYTE R, BYTE G, BYTE B, BYTE *y, BYTE *u, BYTE *v);
void new_yuvtorgb(BYTE y, BYTE u, BYTE v, BYTE *R, BYTE *G, BYTE *B);


LRESULT FAR PASCAL CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_ERASEBKGND:
		return 0;
	case WM_PAINT:
		{
			if (buf != NULL)
			{
				PAINTSTRUCT ps;
				BeginPaint((HWND)khwl_handle, &ps);

				HDC hDC = GetDC((HWND)khwl_handle);
				HBITMAP hBitmap;

				if (hDC != curDC)
				{
					if (hMemDC != NULL)
						DeleteDC(hMemDC);
					hMemDC = CreateCompatibleDC(hDC);
					if (hSrcBitmap != NULL)
						DeleteObject(hSrcBitmap);
					hSrcBitmap = CreateCompatibleBitmap(hDC, (WORD)720, 480);
					curDC = hDC;
				}

				int i;
				RGBQUAD pal[256] = { 0 };
				for (i = 0; i < 256; i++)
				{
					khwl_tvyuvtovgargb(buf[8+i*4+1], buf[8+i*4+2], buf[8+i*4+3],
							&pal[i].rgbRed, &pal[i].rgbGreen, &pal[i].rgbBlue);
				}

				UpdateYUV();

				BYTE *bb = buf + 8 + 1024 + 640*480, *b = bb;
				BYTE *tvmm = tv_mask + 640*480, *tvm = tvmm;
				DWORD *osd = osdbuf, *src = backbuf;
				BYTE *ach = buf + 8;
				
				/// \TODO: use MMX/SSE for optimization...
				for (i = 0; i < 480; i++)
				{
					bb -= 640;
					tvmm -= 640;
					for (int ix = 0; ix < 720; ix++)
					{
						register int dix = ix * 640 / 720;
						b = bb + dix;
						tvm = tvmm + dix;
						int a = ach[*b << 2];
						if (*tvm || !use_tvmask)
						{
							if (a == 0)
								*osd = *src;
							else if (a == 0xff)
								*osd = *(DWORD *)(pal + *b);
							else
							{
								int y = (*src >> 16) & 0xff;
								*osd = ((a * (pal[*b].rgbRed - y) / 255 + y) << 16);
								y = (*src >> 8) & 0xff;
								*osd |= ((a * (pal[*b].rgbGreen - y) / 255 + y) << 8);
								y = (*src) & 0xff;
								*osd |= ((a * (pal[*b].rgbBlue - y) / 255 + y));
							}
						} else
							*osd = 0;
						osd++;
						src++;
					}
				}
				
				SetDIBits(hDC, hSrcBitmap, 0, 480, osdbuf, (BITMAPINFO *)&bi, DIB_RGB_COLORS);
				hBitmap = (HBITMAP)SelectObject(hMemDC, hSrcBitmap);
				
				BitBlt(hDC, 0, yoffset, 720, 480, hMemDC, 0, 0, SRCCOPY);

				//Rectangle(hDC, zoomed_wnd.x, zoomed_wnd.y+yoffset, zoomed_wnd.x + zoomed_wnd.w, zoomed_wnd.y+zoomed_wnd.h+yoffset);

				UpdateFIP(hDC);

				SelectObject(hMemDC, hBitmap);
				ReleaseDC((HWND)khwl_handle, hDC);

				EndPaint((HWND)khwl_handle, &ps);
			}
		}
		break;
	case WM_KEYDOWN:
		{
			struct
			{
				int vk_key;
				int fip_key;
			} keytabl[] =
			{
				{ VK_LEFT, FIP_KEY_LEFT },
				{ VK_RIGHT, FIP_KEY_RIGHT },
				{ VK_UP, FIP_KEY_UP },
				{ VK_DOWN, FIP_KEY_DOWN },
				{ VK_HOME, FIP_KEY_REWIND },
				{ VK_END, FIP_KEY_FORWARD },
				{ VK_PRIOR, FIP_KEY_SKIP_PREV },
				{ VK_NEXT, FIP_KEY_SKIP_NEXT },
				{ VK_ESCAPE, FIP_KEY_RETURN },
				{ 13, FIP_KEY_ENTER },
				{ VK_BACK, FIP_KEY_RETURN },
			
				{ VK_SPACE, FIP_KEY_PLAY },
				{ VK_TAB, FIP_KEY_STOP },

				{ '1', FIP_KEY_ONE },
				{ '2', FIP_KEY_TWO },
				{ '3', FIP_KEY_THREE },
				{ '4', FIP_KEY_FOUR },
				{ '5', FIP_KEY_FIVE },
				{ '6', FIP_KEY_SIX },
				{ '7', FIP_KEY_SEVEN },
				{ '8', FIP_KEY_EIGHT },
				{ '9', FIP_KEY_NINE },
				{ '0', FIP_KEY_ZERO },

				{ 'A', FIP_KEY_ANGLE },
				{ 'B', FIP_KEY_AB },
				{ 'C', FIP_KEY_CANCEL },
				{ 'D', FIP_KEY_AUDIO },
				{ 'E', FIP_KEY_TITLE },
				{ 'I', FIP_KEY_PBC },
				{ 'L', FIP_KEY_SLOW },
				{ 'M', FIP_KEY_MENU },
				{ 'N', FIP_KEY_PN },
				{ 'O', FIP_KEY_PROGRAM },
				{ 'P', FIP_KEY_PAUSE },
				{ 'R', FIP_KEY_REPEAT },
				{ 'S', FIP_KEY_SEARCH },
				{ 'T', FIP_KEY_SUBTITLE },
				{ 'U', FIP_KEY_MUTE },
				{ 'V', FIP_KEY_VMODE },
				{ 'Z', FIP_KEY_ZOOM },

				{ 187, FIP_KEY_VOLUME_UP },
				{ 189, FIP_KEY_VOLUME_DOWN },

				{ VK_F1, FIP_KEY_POWER },
				{ VK_F2, FIP_KEY_EJECT },
				{ VK_F3, FIP_KEY_SETUP },
				{ VK_F4, FIP_KEY_OSD },
				{ -1, -1 }
			};

			if (wParam != 0)
			{
				for (int i = 0; keytabl[i].vk_key != -1; i++)
				{
					if ((int)wParam == keytabl[i].vk_key)
					{
						fip_lastkey = keytabl[i].fip_key;
						break;
					}
				}
			}
			if (wParam == VK_F9)
			{
				use_tvmask = !use_tvmask;
				InvalidateRect((HWND)khwl_handle, NULL, FALSE);
				UpdateWindow((HWND)khwl_handle);
			}
		break;
		}
	case WM_KEYUP:
		//fip_lastkey = 0;
		break;
	case WM_MOUSEMOVE:
		{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		static char buf[256];
		if (y >= yoffset)
		{
			sprintf(buf, "%s (%d, %d)", wndtitle, x, y-yoffset);
			SetWindowText(hWnd, buf);
		}
		else
		{
			struct
			{
				int fip_key;
				int x1, y1, x2, y2;
			} keytabl[] =
			{
				{ FIP_KEY_FRONT_EJECT, 418, 28, 445, 42 },
				{ FIP_KEY_FRONT_PLAY, 421, 68, 439, 85 },
				{ FIP_KEY_FRONT_STOP, 458, 68, 476, 85 },
				{ FIP_KEY_FRONT_PAUSE, 497, 68, 515, 85 },
				{ FIP_KEY_FRONT_SKIP_PREV, 579, 68, 597, 85 },
				{ FIP_KEY_FRONT_SKIP_NEXT, 609, 68, 627, 85 },
				{ FIP_KEY_FRONT_REWIND, 653, 68, 671, 85 },
				{ FIP_KEY_FRONT_FORWARD, 683, 68, 701, 85 },
				{ -1, -1, -1, -1, -1 }
			};

			fip_button = -1;
			for (int i = 0; keytabl[i].fip_key != -1; i++)
			{
				if (x >= keytabl[i].x1 && x <= keytabl[i].x2 
					&& y >= keytabl[i].y1 && y <= keytabl[i].y2)
				{
					fip_button = keytabl[i].fip_key;
					break;
				}
			}

			if (fip_button != -1)
				SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(32649)));
			else
				SetCursor(::LoadCursor(NULL, IDC_ARROW));

			SetWindowText(hWnd, wndtitle);
		}
		break;
		}
	case WM_LBUTTONDOWN:
		{
			if (fip_button != -1)
				fip_lastkey = fip_button;
		}
		break;
	default:
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
	}
	return((LRESULT)NULL);
}

/////////////////////////////////////////////

static BOOL init(void)
{
	//InitCommonControls();
	
	WNDCLASS  wc;

	wc.style = (UINT)NULL; 
	wc.lpfnWndProc = (WNDPROC)MainWindowProc; 
	wc.cbClsExtra = 0; 
	wc.cbWndExtra = 0; 
	wc.hInstance = GetModuleHandle(NULL);   
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName ="SPInitClass";
	return (RegisterClass(&wc) != 0);
}

static void cwnd()
{
	HWND hWnd;
	int w = 720,h = 480+yoffset;
	int xcoord = GetSystemMetrics(SM_CXSCREEN)/2-w/2;
	int ycoord = GetSystemMetrics(SM_CYSCREEN)/2-h/2;
	hWnd = CreateWindow("SPInitClass", wndtitle, WS_OVERLAPPED|WS_SYSMENU, xcoord, ycoord, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);
	RECT dstrect;
		GetClientRect(hWnd, &dstrect);
		int dx = w*2 - (dstrect.right - dstrect.left);
		int dy = h*2 - (dstrect.bottom - dstrect.top);
		SetWindowPos(hWnd, HWND_TOP, GetSystemMetrics(SM_CXSCREEN) / 2 - dx / 2,
				GetSystemMetrics(SM_CYSCREEN) / 2 - dy / 2, dx, dy, 0);

	khwl_handle = (int)hWnd;
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

BOOL khwl_init(BOOL /*applymodule*/)
{
	khwl_inityuv();

    khwl_deinit();

	init();
	cwnd();

	buf = (BYTE *)SPmalloc(640*480 + 1024 + 8);
	backbuf = (DWORD *)SPmalloc(720 * 480 * sizeof(DWORD));
	memset(backbuf, 0, 720*480*sizeof(DWORD));
	osdbuf = (DWORD *)SPmalloc(720 * 480 * sizeof(DWORD));

	int bs = 1048576;
	BUF_BASE = (BYTE *)VirtualAlloc((void *)0x016c0000/*0x01680000*/, bs, MEM_RESERVE, PAGE_READWRITE);
	BUF_BASE = (BYTE *)VirtualAlloc(BUF_BASE, bs, MEM_COMMIT, PAGE_READWRITE);
	if (BUF_BASE == NULL)
	{
		//int err = GetLastError();
		BUF_BASE = (BYTE *)VirtualAlloc(NULL, bs, MEM_COMMIT, PAGE_READWRITE);
	}
	if (BUF_BASE == NULL)
		return FALSE;


	khwl_parser_mutex = CreateMutex(NULL, FALSE, NULL);
	khwl_parser = CreateThread (NULL, 16384, (LPTHREAD_START_ROUTINE)khwl_parser_proc,
					(void *)0, CREATE_SUSPENDED, &khwl_parser_id);

	memset(&bi, 0, sizeof(bi));
	
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = 720;
	bi.bmiHeader.biHeight = 480;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;

	if (hbm == NULL)
	{
		hbm = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_FIP));
		
		if (hbm == NULL)
		{
			int err = GetLastError();
			char tmp[25];
			sprintf(tmp, "Resources loading error = %d", err);
			MessageBox((HWND)khwl_handle, tmp, "Error", MB_OK|MB_ICONEXCLAMATION);
			return FALSE;
		}
	}

	max_wnd.x = 0;
	max_wnd.y = 0;
	max_wnd.w = 720;
	max_wnd.h = 480;

	src_wnd = zoomed_wnd = valid_wnd = dest_wnd = max_wnd;
	osd_wnd = max_wnd;

	CreateTvMask();

    return TRUE;
}

BOOL khwl_restoreparams()
{

	return TRUE;
}

BOOL khwl_deinit()
{
    if (khwl_handle == -1)
        return FALSE;

	SPfree(buf);
	SPfree(backbuf);
	SPfree(osdbuf);

	CloseHandle(khwl_parser);
	CloseHandle(khwl_parser_mutex);

	VirtualFree(BUF_BASE, 0, MEM_RELEASE);
	BUF_BASE = NULL;

	DestroyWindow((HWND)khwl_handle);
    khwl_handle = -1;
    return TRUE;
}

BOOL khwl_reset()
{
    if (khwl_handle == -1)
        return FALSE;

	return 0;
}

int khwl_osd_switch(KHWL_OSDSTRUCT *_osd, BOOL autoupd)
{
	static int TimerID = -1;
	if (khwl_handle == -1)
        return FALSE;
	if (_osd->flags != 0 || autoupd)
	{
		if (TimerID != -1)
			timeKillEvent(TimerID);
		TimerID = timeSetEvent(1000, 1, OSDUpdate, 0, TIME_PERIODIC);
	}
	osd.addr = buf;
	osd.bpp = 8;
	osd.width = 640;
	osd.height = 480;
	*_osd = osd;


	return 0;
}

void khwl_get_osd_size(int *width, int *height)
{
	*width = 640;//osd.width;
	*height = 480;//osd.height;
}

bool khwl_msgloop()
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (GetMessage(&msg, NULL, 0, 0) == 0)
			return false;
		//if (TranslateAccelerator(hWnd, hAccel, &msg) == 0)
		{
			TranslateMessage(&msg); 
			DispatchMessage(&msg);
		}
	}
	return true;
}

BOOL khwl_osd_update()
{
	InvalidateRect((HWND)khwl_handle, NULL, FALSE);
	UpdateWindow((HWND)khwl_handle);

	return khwl_msgloop();
}

int khwl_osd_setalpha(int )
{
	return 0;
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

void khwl_osd_setfullscreen(BOOL)
{
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
	return rates[0/*inl(SYS_REVID_REG) & 1*/];
}

#define MIN(a, b) (((a) <= (b)) ? (a) : (b))
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))

int khwl_displayYUV(KHWL_YUV_FRAME *f)
{
	if (f->uv_buf != NULL)
	{
		memcpy(tmp_UV + f->uv_offs, f->uv_buf, f->uv_num);
		yuv_dirty = TRUE;
	}
	if (f->y_buf != NULL)
	{
		memcpy(tmp_Y + f->y_offs, f->y_buf, f->y_num);
		yuv_dirty = TRUE;
	}
	return 0;
}

int khwl_audioswitch(BOOL /*ison*/)
{
	//int val = ison ? 1 : 0;
	//return ioctl(khwl_handle, KHWL_AUDIOSWITCH, &val);
	return TRUE;
}

int khwl_play(int mode)
{
	if (mode == KHWL_PLAY_MODE_STEP)
	{
		if (!step_mode)
		{
			khwl_pause();
			step_mode = true;
		}
		return 0;
	}
	int last;
	while ((last = ResumeThread(khwl_parser)) > 1)
		;
	step_mode = false;
	return 0;
}

int khwl_pause()
{
	WaitForSingleObject(khwl_parser_mutex, INFINITE);
	SuspendThread(khwl_parser);
	ReleaseMutex(khwl_parser_mutex);
	return 0;
}

int khwl_stop()
{
	WaitForSingleObject(khwl_parser_mutex, INFINITE);
	SuspendThread(khwl_parser);

	for (;;)
	{
		if (MPEG_VIDEO_STRUCT->in != NULL)
		{
			khwl_nextfeedpacket(MPEG_VIDEO_STRUCT);
			continue;
		}
		if (MPEG_AUDIO_STRUCT->in != NULL)
		{
			khwl_nextfeedpacket(MPEG_AUDIO_STRUCT);
			continue;
		}
		if (MPEG_SPU_STRUCT->in != NULL)
		{
			khwl_nextfeedpacket(MPEG_SPU_STRUCT);
			continue;
		}
		break;
	}


	ReleaseMutex(khwl_parser_mutex);

	return 0;
}

BOOL khwl_blockirq(BOOL block)
{
	if (block)
		WaitForSingleObject(khwl_parser_mutex, INFINITE);
	else
		ReleaseMutex(khwl_parser_mutex);
		
	return TRUE;
}

BOOL khwl_happeningwait(DWORD *)
{
	// do nothing
	khwl_msgloop();
	Sleep(1);
	return TRUE;
}

BOOL khwl_poll(WORD , DWORD )
{
	khwl_msgloop();
	Sleep(1);
	return TRUE;
}

BOOL khwl_ideswitch(BOOL )
{
	return TRUE;
}

int khwl_getfrequency()
{
   	int temp = freq_reg;

	int div = (temp >> 8) & 0xff;
	int mul = (temp >> 2) & 63;

	int freq = (27 * (mul + 2)) / ((div + 2) * 2);
	return freq;
}

BOOL khwl_setfrequency(int freq)
{
	if (freq < 100 || freq > 202)
		return FALSE;

   	int temp = freq_reg;
   	temp = temp & 0xF000;
	int div = (temp >> 8) & 0xff;
	//int mul = (temp >> 2) & 63;

	int mul = (freq * ((10 * div+20)*20) / 270 - 20 /* + 5 */) / 10;
	if (mul < 0 || mul > 63)
		return FALSE;
	
	temp = temp | 0x02;
	temp = temp | (div << 8);
	temp = temp | (mul << 2);

	freq_reg = temp & 0x7FFF;
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
	sprintf(hw, "%X.%c", 0x8500/*inl(SYS_CHIPID_REG)*/, 0/*inl(SYS_REVID_REG)*/ + 'A');

	return hw;
}

/////////////////////////////////////////////////////////

extern int main(int argc, char *argv[]);

int PASCAL WinMain (HINSTANCE /*hInst*/, HINSTANCE /*hPrevInst*/, LPSTR lpszCmdLine, int /*nCmdShow*/)
{
	char *args[2] = { lpszCmdLine, NULL };

	AllocConsole();

	int hCrt = _open_osfhandle((long) GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
	FILE *hf = _fdopen( hCrt, "w" );
	//FILE *hf = fopen("sp.log", "w");
	*stdout = *hf;
	int ret = setvbuf( stdout, NULL, _IONBF, 0 ); 

	hCrt = _open_osfhandle((long) GetStdHandle(STD_ERROR_HANDLE), _O_TEXT);
	hf = _fdopen( hCrt, "w" );
	*stderr = *hf;
	ret = setvbuf( stderr, NULL, _IONBF, 0 ); 

	return main(1, args);
}

