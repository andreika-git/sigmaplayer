//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - MPEG player source file.
 *  \file       sp_mpeg.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       2.08.2004
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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <time.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_khwl.h>
#include <libsp/sp_fip.h>
#include <libsp/sp_mpeg.h>

/////////////////////////////////////
//#define MPEG_DEBUG
//#define MPEG_BUF_DEBUG
/////////////////////////////////////

#ifndef MPEG_DEBUG
static void empty_msg(const char *,...)
{
}
#define MSG empty_msg
#else
#define MSG msg
#endif


#include "settings.h"

int MPEG_NUM_BUFS[3] = { 8, 0, 0 };
int MPEG_BUF_SIZE[3] = { 32768, 0, 0 };
int MPEG_PACKET_LENGTH = 2048;
int MPEG_NUM_PACKETS = 512;

static MpegPacket *packets = NULL;
static int cur_num_packets = 0;

static int cur_bufidx[3] = { 7, -1, -1 };
static BYTE *bufbase[3][256];
static BYTE bufidx[3][256/*MPEG_NUM_BUFS*/] = { { 0 }, { 0 }, { 0 } };
static bool buf_created[3] = { false, false, false };

static MPEG_VIDEO_FORMAT mpeg_fmt = MPEG_1;

static int mpeg_rate = 0, mpeg_rate_sum = 0, mpeg_rate_pos = 0, mpeg_nr = 0;
static int old_mpglayer = -1;
const int num_rates = 40;
static int mpeg_rates[num_rates];

static int mpeg_width = 0, mpeg_height = 0, mpeg_fps = 0, mpeg_aspect = 0;
static int mpeg_video_muxrate = 0;
static int mpeg_old_width = 0, mpeg_old_height = 0, mpeg_old_fps = 0, mpeg_old_aspect = 0;

static int mpeg_zoom_hscale = 0, mpeg_zoom_vscale = 0, mpeg_zoom_offsetx = 0, mpeg_zoom_offsety = 0;

static KHWL_AUDIO_FORMAT_TYPE mpeg_audio_format = eAudioFormat_UNKNOWN;
static int mpeg_audio_numchans = 0, mpeg_audio_bits = 0, mpeg_sample_rate = 0;
static int mpeg_resample_rate = 0;

static LONGLONG mpeg_wait_last_pts = 0;
static int mpeg_wait_last_depth = 0;
static int mpeg_wait_count_still = 0, mpeg_wait_cnt = 0;
static bool mpeg_show_empty_stack_depth = true;

int ac3_total_frame_size = 0, ac3_next_total_frame_size = 0;

/// Use this to support 8-bit PCM
const int mpeg_min_audio_bits = 16;

static bool mpeg_allow_mpa_parsing = false;
static bool mpeg_show_mpa_parsing = true;

static bool mpeg_is_mpeg1 = false;
static bool mpeg_format_changed = false, mpeg_audio_format_set = false;

static LONGLONG delta_pts = 0;
static LONGLONG last_video_pts = 0;
static LONGLONG mpeg_vop_rate = 24000, mpeg_vop_scale = 1000;

static bool firstchunk = true;
static bool firstdisplayed = false;
static bool use_scr = true;

static bool mpeg_audioonly = false;
static int mpeg_curaudstream = 0;
static int mpeg_curspustream = 0;
static int mpeg_numaudstream = 0;
static int mpeg_numspustream = 0;
static MPEG_SPEED_TYPE mpeg_speed = MPEG_SPEED_STOP;
static bool mpeg_needrestoreaudio = false;

static int mpeg_srate = 0;
static KHWL_AUDIO_FORMAT_TYPE mpeg_aformat = eAudioFormat_UNKNOWN;
static int mpeg_nchannels = 2;
static int mpeg_nbitspersample = 24;

static WORD ac3_frame_sizes[38][3];

static BYTE mpeg_seq_start_packet[256];
static BYTE mpeg_gop_packet[8] = { 0, 0, 1, 0xb8, 0, 8, 0, 0x40 };
static int mpeg_seq_start_packet_len = 0;
static bool mpeg_need_seq_start_packet = false;
static bool mpeg_was_seq_header = false, mpeg_was_picture = false;
static bool mpeg_parse_video = false;

static const BYTE picture_start_code            = 0x00;
static const BYTE pack_start_code 				= 0xBA;
static const BYTE system_header_start_code  	= 0xBB;
static const BYTE sequence_header_start_code  	= 0xB3;
static const BYTE packet_start_padding_code 	= 0xBE;
static const BYTE packet_start_pci_dsi_code 	= 0xBF;
static const BYTE packet_start_video_code1  	= 0xE0;
static const BYTE packet_start_video_code2  	= 0xE2;	// 0xEF
static const BYTE packet_start_audio1_code1  	= 0xC0;
static const BYTE packet_start_audio1_code2  	= 0xCF;
static const BYTE packet_start_audio2_code1  	= 0xD0;
static const BYTE packet_start_audio2_code2  	= 0xDF;
static const BYTE packet_start_private1_code    = 0xBD;

static const int MPEG_WRAP_THRESHOLD = 120000;   // value taken from xine

static const WORD ac3_bitrates[19] = { 32, 40, 48, 56, 64, 80, 96, 112, 128,
									160, 192, 224, 256, 320, 384, 448, 512, 576, 640 };
static const BYTE ac3_channels[8] = { 2, 1, 2, 3, 3, 4, 4, 5 };
static const WORD ac3_freqs[3] = { 48000, 44100, 32000 };

// in frames per msec.
static const int mpeg_fps_table[16] = {	0, 23976, 24000, 25000, 
										29970, 30000, 50000, 59940, 60000, 
										15000, 5000, 10000, 12000, 15000, 0, 0, };
#if 0
static const char *mpeg_fps_strings[16] = { "none", "3/2 pulldown", "film", "PAL", "NTSC", 
										"drop-frame NTSC", "double-rate PAL",
										"double-rate NTSC", "double-rate d.f. NTSC",
										"Xing", "Economy-libmpeg3", "Economy-libmpeg3", 
										"Economy-libmpeg3", "Economy-libmpeg3", "Unknown", "Unknown", };
#endif

bool mpeg_novideo = FALSE;
int mpeg_scr = 0;
bool tell_about_encryption = true;

int fip_audio_format = -1;

/////////////////////////////////////////////////////
typedef struct MpegPacketFIFO
{
	MpegPacket *packet;
	MpegPacketFIFO *next;
} MpegPacketFIFO;

static const int feed_stack_num = 16;
static MpegPacketFIFO feed_stack[feed_stack_num];	//  FIFO
static MpegPacketFIFO *feed_stack_in = feed_stack;		// points to the last being added
static MpegPacketFIFO *feed_stack_out = NULL;				// points to the first being removed

/////////////////////////////////////////////////////

BOOL mpeg_init(MPEG_VIDEO_FORMAT fmt, BOOL audio_only, BOOL mpa_parsing, BOOL need_seq_start_packet)
{
	mpeg_audioonly = audio_only == TRUE;
	mpeg_allow_mpa_parsing = mpa_parsing == TRUE;
	mpeg_show_mpa_parsing = true;
	mpeg_need_seq_start_packet = need_seq_start_packet == TRUE;

	mpeg_setaudiostream(0);
	mpeg_setspustream(0);
	mpeg_numaudstream = 0;
	mpeg_numspustream = 0;

	delta_pts = 0;
	last_video_pts = 0;

	mpeg_vop_rate = 24000;
	mpeg_vop_scale = 1000;

	khwl_stop();

	mpeg_fmt = fmt;

	int var = fmt == MPEG_1 || fmt == MPEG_2 ? 0 : (fmt == MPEG_4 ? 1 : 0); //256;
	khwl_setproperty(KHWL_DECODER_SET, edecVideoStd, sizeof(int), &var); // mpeg4

	var = 0; //256;
    khwl_setproperty(KHWL_VIDEO_SET, evVOBUReverseSpeed, sizeof(int), &var); // normal speed

    var = 0; //256;
    khwl_setproperty(KHWL_VIDEO_SET, evSpeed, sizeof(int), &var); // normal speed

	firstchunk = true;
	firstdisplayed = false;
	use_scr = true;

	mpeg_reset();

	mpeg_setpts(0);

	mpeg_rate = 0;
	mpeg_rate_sum = 0;
	mpeg_rate_pos = 0;
	mpeg_nr = 0;
	old_mpglayer = -1;
	memset(mpeg_rates, 0, num_rates * sizeof(int));

	mpeg_is_mpeg1 = false;
	mpeg_format_changed = false;
	mpeg_audio_format_set = false;
	fip_audio_format = -1;

	mpeg_srate = 0;
	mpeg_aformat = eAudioFormat_UNKNOWN;
	mpeg_nchannels = 2;
	mpeg_nbitspersample = 24;
	
	mpeg_needrestoreaudio = true;
	mpeg_audio_format = eAudioFormat_UNKNOWN;
	mpeg_sample_rate = 0;
	mpeg_audio_numchans = 0; mpeg_audio_bits = 0;
	mpeg_resample_rate = 0;

	if (!audio_only)
	{
		KHWL_WINDOW wnd;
		khwl_getproperty(KHWL_VIDEO_SET, evMaxDisplayWindow, sizeof(wnd), &wnd);
		//wnd.w = 720;
		//wnd.h = 576;
		//mpeg_setframesize(wnd.w, wnd.h);
		MSG("MPEG: Default frame window: %dx%d\n", wnd.w, wnd.h);

		mpeg_width = 0;	mpeg_height = 0; mpeg_fps = 0; mpeg_aspect = 0;
		mpeg_video_muxrate = 0;
		mpeg_old_width = 0; mpeg_old_height = 0; mpeg_old_fps = 0; mpeg_old_aspect = 0;

		mpeg_zoom_reset();
	}

	for(int i = 0; i < 38; i++) 
	{
		int br = ac3_bitrates[i >> 1];
		ac3_frame_sizes[i][0] = (WORD)(2*br);
		ac3_frame_sizes[i][1] = (WORD)((320*br / 147) + (i & 1));
		ac3_frame_sizes[i][2] = (WORD)(3*br);
	}

	tell_about_encryption = true;

	for (int k = 0; k < 3; k++)
	{
		for (int j = 0; j < 256; j++)
			bufbase[k][j] = NULL;
	}

	mpeg_wait_last_pts = 0;
	mpeg_wait_last_depth = 0;
	mpeg_wait_count_still = 0;
	mpeg_wait_cnt = 0;
	mpeg_show_empty_stack_depth = true;
	mpeg_seq_start_packet_len = 0;
	mpeg_was_seq_header = false;
	mpeg_was_picture = false;

	mpeg_parse_video = false;

	for (int j = 0; j < feed_stack_num; j++)
	{
		feed_stack[j].next = &feed_stack[(j + 1) % feed_stack_num];
	}
	feed_stack_in = feed_stack;
	feed_stack_out = NULL;

    return TRUE;
}

BOOL mpeg_deinit()
{
	khwl_stop();
	
	if (!mpeg_audioonly)
	{
		mpeg_zoom_reset();
		khwl_display_clear();
	}

	for (int k = 0; k < 3; k++)
	{
		if (buf_created[k])
		{
			for (int j = 0; j < 256; j++)
				SPSafeFree(bufbase[k][j]);
		}
	}

	if (packets != NULL)
	{
		SPfree(packets);
		packets = NULL;
	}
	mpeg_speed = MPEG_SPEED_STOP;
	return TRUE;
}

BOOL mpeg_reset()
{
	mpeg_init_packets();

	cur_bufidx[0] = cur_bufidx[1] = -1;
	MPEG_NUM_BUFS[0] = MPEG_NUM_BUFS[1] = 0;
	MPEG_BUF_SIZE[0] = MPEG_BUF_SIZE[1] = 0;
	
	for (int k = 0; k < 3; k++)
	{
		for (int j = 0; j < 256; j++)
			bufbase[k][j] = NULL;
		buf_created[k] = false;
	}

    mpeg_feed_init(packets, MPEG_NUM_PACKETS);

	return TRUE;
}

BOOL mpeg_setbuffer_array(MPEG_BUFFER which, BYTE **base, int num_bufs, int buf_size)
{
	MPEG_NUM_BUFS[which] = num_bufs;
	MPEG_BUF_SIZE[which] = buf_size;
	
	for (int i = 0; i < num_bufs; i++)
	{
		bufbase[which][i] = base[i];
	}
	cur_bufidx[which] = num_bufs - 1;
	
	khwl_blockirq(TRUE);
	memset(bufidx[which], 0, MPEG_NUM_BUFS[which]);
	khwl_blockirq(FALSE);

	return TRUE;
}

BOOL mpeg_setbuffer(MPEG_BUFFER which, BYTE *base, int num_bufs, int buf_size)
{
	MPEG_NUM_BUFS[which] = num_bufs;
	MPEG_BUF_SIZE[which] = buf_size;
	
	BYTE *b = base;
	for (int i = 0; i < num_bufs; i++)
	{
		bufbase[which][i] = b;
		b += MPEG_BUF_SIZE[which];
	}
	cur_bufidx[which] = num_bufs - 1;
	
	khwl_blockirq(TRUE);
	memset(bufidx[which], 0, MPEG_NUM_BUFS[which]);
	khwl_blockirq(FALSE);
	
	return TRUE;
}


int mpeg_getbufsize(MPEG_BUFFER which)
{
	return MPEG_BUF_SIZE[which];
}

BYTE *mpeg_getbufbase()
{
	return BUF_BASE;
}

////////////////////////////////////////////////////////

BOOL mpeg_setspeed(MPEG_SPEED_TYPE speed)
{
	int val = 256;

	// some currently forbidden combinations:
#if 0
	// 1) no fwd/rev in paused mode
	if (((speed & MPEG_SPEED_FWD_MASK) == MPEG_SPEED_FWD_MASK
		|| (speed & MPEG_SPEED_REV_MASK) == MPEG_SPEED_REV_MASK)
		&& (mpeg_speed == MPEG_SPEED_PAUSE || mpeg_speed == MPEG_SPEED_STEP))
		return FALSE;
#endif
	// 2) slow fwd only at play
	if (((speed & MPEG_SPEED_SLOW_FWD_MASK) == MPEG_SPEED_SLOW_FWD_MASK)
		&& (mpeg_speed != MPEG_SPEED_NORMAL 
			&& (speed & MPEG_SPEED_SLOW_FWD_MASK) != MPEG_SPEED_SLOW_FWD_MASK))
		return FALSE;
	// 3) slow rev only at play
	if (((speed & MPEG_SPEED_SLOW_REV_MASK) == MPEG_SPEED_SLOW_REV_MASK)
		&& (mpeg_speed != MPEG_SPEED_NORMAL 
			&& (speed & MPEG_SPEED_SLOW_REV_MASK) != MPEG_SPEED_SLOW_REV_MASK))
		return FALSE;
	// 4) pause/step only from play or slow
	if ((speed == MPEG_SPEED_PAUSE || speed == MPEG_SPEED_STEP)
		&& (mpeg_speed != MPEG_SPEED_NORMAL && mpeg_speed != MPEG_SPEED_PAUSE
			&& ((mpeg_speed & MPEG_SPEED_SLOW_FWD_MASK) != MPEG_SPEED_SLOW_FWD_MASK)
			&& ((mpeg_speed & MPEG_SPEED_SLOW_REV_MASK) != MPEG_SPEED_SLOW_REV_MASK)
			&& mpeg_speed != MPEG_SPEED_STEP && !firstchunk))
		return FALSE;

	if (speed == MPEG_SPEED_NORMAL)
	{
		if (mpeg_speed == MPEG_SPEED_PAUSE || mpeg_speed == MPEG_SPEED_STEP)
		{
			mpeg_play();
		}
		else if ((mpeg_speed & MPEG_SPEED_SLOW_FWD_MASK) == MPEG_SPEED_SLOW_FWD_MASK
			|| (mpeg_speed & MPEG_SPEED_SLOW_REV_MASK) == MPEG_SPEED_SLOW_REV_MASK)
		{
			khwl_pause();
			khwl_audioswitch(TRUE);
			//mpeg_needrestoreaudio = true;

			val = 256;
			khwl_setproperty(KHWL_VIDEO_SET, evSpeed, sizeof(int), &val);
			khwl_play(KHWL_PLAY_MODE_NORMAL);
			firstchunk = false;
			firstdisplayed = true;
		}
		else	// fast fwd/rev
		{
			ULONGLONG pts;
			pts = mpeg_getpts();
			khwl_stop();
			val = 256;
			khwl_setproperty(KHWL_VIDEO_SET, evSpeed, sizeof(int), &val);
			khwl_setproperty(KHWL_VIDEO_SET, evVOBUReverseSpeed, sizeof(int), &val);
			
			//khwl_play(KHWL_PLAY_MODE_NORMAL);
			//firstchunk = false;

			mpeg_start();
			mpeg_setpts(pts);
		}
		mpeg_speed = speed;
		return TRUE;
	}
	
	if (speed == MPEG_SPEED_PAUSE || speed == MPEG_SPEED_STEP)
	{
		if (mpeg_speed != MPEG_SPEED_NORMAL 
			&& mpeg_speed != MPEG_SPEED_PAUSE && mpeg_speed != MPEG_SPEED_STEP)
		{
			ULONGLONG pts;
			pts = mpeg_getpts();
			khwl_stop();
			val = 256;
			khwl_setproperty(KHWL_VIDEO_SET, evSpeed, sizeof(int), &val);
			khwl_setproperty(KHWL_VIDEO_SET, evVOBUReverseSpeed, sizeof(int), &val);
			khwl_play(KHWL_PLAY_MODE_NORMAL);
			mpeg_setpts(pts);
		}
		if (speed == MPEG_SPEED_PAUSE)
			khwl_pause();
		else
		{
			khwl_play(KHWL_PLAY_MODE_STEP);
			firstchunk = false;
			firstdisplayed = true;
		}
		mpeg_speed = speed;
		return TRUE;
	}

	// TODO: add FIFO sync...
//	if (firstchunk)
//		return FALSE;


	switch (speed)
	{
	case MPEG_SPEED_REV_4X:
	case MPEG_SPEED_FWD_4X: val = 0x400; break;
	case MPEG_SPEED_REV_8X:
	case MPEG_SPEED_FWD_8X: val = 0x800; break;
	case MPEG_SPEED_REV_16X:
	case MPEG_SPEED_FWD_16X: val = 0x1000; break;
	case MPEG_SPEED_REV_32X:
	case MPEG_SPEED_FWD_32X: val = 0x2000; break;
	case MPEG_SPEED_REV_48X:
	case MPEG_SPEED_FWD_48X: val = 0x3000; break;

	case MPEG_SPEED_SLOW_REV_2X:
	case MPEG_SPEED_SLOW_FWD_2X: val = 0x80; break;
	case MPEG_SPEED_SLOW_REV_4X:
	case MPEG_SPEED_SLOW_FWD_4X: val = 0x40; break;
	case MPEG_SPEED_SLOW_REV_8X:
	case MPEG_SPEED_SLOW_FWD_8X: val = 0x20; break;
	default:
		val = 0;
	}

	int old_mpeg_speed = mpeg_speed;
	mpeg_speed = speed;

	if ((speed & MPEG_SPEED_FWD_MASK) == MPEG_SPEED_FWD_MASK)
	{
		if ((speed & MPEG_SPEED_SLOW_FWD_MASK) == MPEG_SPEED_SLOW_FWD_MASK)
		{
			if ((old_mpeg_speed & MPEG_SPEED_SLOW_FWD_MASK) == MPEG_SPEED_SLOW_FWD_MASK 
				|| old_mpeg_speed == MPEG_SPEED_NORMAL)
				khwl_pause();
			else
				khwl_stop();
			khwl_audioswitch(FALSE);
		}
		else
			khwl_stop();
		khwl_setproperty(KHWL_VIDEO_SET, evSpeed, sizeof(int), &val);
		mpeg_play();
	}
	else if ((speed & MPEG_SPEED_REV_MASK) == MPEG_SPEED_REV_MASK)
	{
		if ((speed & MPEG_SPEED_SLOW_REV_MASK) == MPEG_SPEED_SLOW_REV_MASK)
		{
			if ((old_mpeg_speed & MPEG_SPEED_SLOW_REV_MASK) == MPEG_SPEED_SLOW_REV_MASK 
				|| old_mpeg_speed == MPEG_SPEED_NORMAL)
				khwl_pause();
			else
				khwl_stop();
			khwl_audioswitch(FALSE);
		}
		else
			khwl_stop();
		khwl_setproperty(KHWL_VIDEO_SET, evVOBUReverseSpeed, sizeof(int), &val);
		mpeg_play();
		//mpeg_setpts(pts);
	}
	
	return TRUE;
}

BOOL mpeg_setframesize(int width, int height, bool noaspect)
{
	if (width <= 0 || height <= 0)
		return FALSE;
	
	mpeg_width = width;
	mpeg_height = height;

	khwl_display_clear();
	khwl_set_window_zoom(noaspect ? KHWL_ZOOMMODE_ASPECT : KHWL_ZOOMMODE_DVD);
	khwl_set_window(-1, -1, width, height, 100, 100, 0, 0);
	
	return TRUE;
}

BOOL mpeg_getframesize(int *width, int *height)
{
	*width = mpeg_width;
	*height = mpeg_height;
	return TRUE;
}

int mpeg_get_fps()
{
	return mpeg_fps;
}

int mpeg_getaspect()
{
	return mpeg_aspect;
}

void mpeg_start()
{
	firstchunk = true;
	MSG("MPEG: MPEG_START\n");
}

BOOL mpeg_is_playing()
{
	return !firstchunk;
}

BOOL mpeg_is_displayed()
{
	return firstdisplayed;
}

MPEG_VIDEO_FORMAT mpeg_get_video_format()
{
	// MPEG1/2 is detected.
	if (mpeg_is_mpeg1)
		return MPEG_1;
	return mpeg_fmt;
}

BOOL mpeg_feed(MPEG_FEED_TYPE type)
{
	static MpegPlayStruct *ps[] = { MPEG_VIDEO_STRUCT, MPEG_AUDIO_STRUCT, MPEG_SPU_STRUCT };

	MpegPlayStruct *feed = ps[type];
	MpegPlayStruct *from = MPEG_PLAY_STRUCT;
	MpegPacket *fromin = from->in;
	if (fromin == NULL)
		return FALSE;

	if (fromin->pts < 0)
		fromin->pts = 0;
	// save video pts for wrap detection and waiting...
	if (fromin->type == 0)
	{
		if (fromin->flags == 2)
			last_video_pts = fromin->pts;
		else if (fromin->flags == 0x80)
			last_video_pts = INT64(90000) * fromin->pts / mpeg_vop_rate;

	}

#if 0
#ifdef WIN32
	if (fromin->type == 0) {
		static int siz = 0;
		FILE *fp;
		fp = fopen("out.m1v", "ab");
		fwrite(fromin->pData, fromin->size, 1, fp);
		fclose(fp);
		siz += fromin->size;
	}
	if (fromin->type == 1) {
		FILE *fp;
		fp = fopen("out.mpa", "ab");
		fwrite(fromin->pData, fromin->size, 1, fp);
		fclose(fp);
	}
#endif
#endif

	khwl_blockirq(TRUE);	

	// advance to the next packet
	from->in = from->in->next;

	// one packet will be removed from 'play' FIFO
	from->num--;
	from->in_cnt++;
	// one packet will be added to 'feed' FIFO
	feed->num++;
	feed->out_cnt++;

	if (fromin->next == NULL)
		from->out = NULL;
	if (feed->out == NULL) 		// all data was processed
		feed->in = fromin;
	else						// otherwise - add it to the queue
		feed->out->next = fromin;
	feed->out = fromin;
	
	fromin->next = NULL;

	khwl_blockirq(FALSE);

	return TRUE;
}

BOOL mpeg_feed_push()
{
	MpegPlayStruct *from = MPEG_PLAY_STRUCT;
	MpegPacket *fromin = from->in;
		
	if (fromin == NULL)
		return FALSE;

	// check if stack is full
	if (feed_stack_in == feed_stack_out)
		return FALSE;

	if (feed_stack_out == NULL)
		feed_stack_out = feed_stack_in;

	feed_stack_in->packet = fromin;

	feed_stack_in = feed_stack_in->next;

	if (fromin->pts < 0)
		fromin->pts = 0;

	khwl_blockirq(TRUE);	

	// advance to the next packet
	from->in = from->in->next;

	// one packet will be removed from 'play' FIFO
	from->num--;
	from->in_cnt++;

	if (fromin->next == NULL)
		from->out = NULL;

	fromin->next = NULL;

	khwl_blockirq(FALSE);
	return TRUE;
}

BOOL mpeg_feed_pop()
{
	static MpegPlayStruct *ps[] = { MPEG_VIDEO_STRUCT, MPEG_AUDIO_STRUCT, MPEG_SPU_STRUCT };

	if (feed_stack_out == NULL)
		return FALSE;

	MpegPacket *fromin = feed_stack_out->packet;
	MpegPlayStruct *feed = ps[fromin->type];

	feed_stack_out = (feed_stack_out->next != feed_stack_in) ? feed_stack_out->next : NULL;
	
	// save video pts for wrap detection and waiting...
	if (fromin->type == 0)
	{
		if (fromin->flags == 2)
			last_video_pts = fromin->pts;
		else if (fromin->flags == 0x80)
			last_video_pts = INT64(90000) * fromin->pts / mpeg_vop_rate;
	}

	khwl_blockirq(TRUE);	

	// one packet will be added to 'feed' FIFO
	feed->num++;
	feed->out_cnt++;

	if (feed->out == NULL) 		// all data was processed
		feed->in = fromin;
	else						// otherwise - add it to the queue
		feed->out->next = fromin;
	feed->out = fromin;

	khwl_blockirq(FALSE);
	return TRUE;
}

BOOL mpeg_feed_isempty()
{
	return (feed_stack_out == NULL) ? TRUE : FALSE;
}

BOOL mpeg_feed_reset()
{
	while (mpeg_feed_pop())
		;
	feed_stack_in = feed_stack;
	feed_stack_out = NULL;
	return TRUE;
}

BOOL mpeg_correct_pts()
{
	MpegPacketFIFO *first = feed_stack_out;
	if (first == NULL)
		return FALSE;
	LONGLONG *p_pts = &first->packet->pts, pts = *p_pts;

	MpegPacketFIFO *cur = first;
	while (cur)
	{
		cur = cur->next;
		if (cur == feed_stack_in)
			break;
		//cur->packet->pts = pts;
		BYTE *b = cur->packet->pData;
		if (b[0] == 0 && b[1] == 0 && b[2] == 1 && b[3] == 0xb6)
			pts += mpeg_vop_scale;
	}
	*p_pts = pts;
	return TRUE;
}

BOOL mpeg_feed_init(MpegPacket *start, int num)
{
	MPEG_PLAY_STRUCT->in = start;

	khwl_blockirq(TRUE);	

	for (int i = 0; i < num-1; i++)
		start[i].next = &start[i+1];
	start[num-1].next = NULL;
	
	MPEG_PLAY_STRUCT->out = &start[num-1];

	MPEG_PLAY_STRUCT->out_cnt = 0;
	MPEG_PLAY_STRUCT->in_cnt = 0;
	MPEG_PLAY_STRUCT->num = num;
	MPEG_PLAY_STRUCT->reserved = 0;

	MPEG_AUDIO_STRUCT->out_cnt = 0;
	MPEG_AUDIO_STRUCT->in_cnt = 0;
	MPEG_AUDIO_STRUCT->num = 0;
	MPEG_AUDIO_STRUCT->reserved = 0;
	MPEG_AUDIO_STRUCT->in = NULL;
	MPEG_AUDIO_STRUCT->out = NULL;

	MPEG_VIDEO_STRUCT->out_cnt = 0;
	MPEG_VIDEO_STRUCT->in_cnt = 0;
	MPEG_VIDEO_STRUCT->num = 0;
	MPEG_VIDEO_STRUCT->reserved = 0;
	MPEG_VIDEO_STRUCT->in = NULL;
	MPEG_VIDEO_STRUCT->out = NULL;

	MPEG_SPU_STRUCT->out_cnt = 0;
	MPEG_SPU_STRUCT->in_cnt = 0;
	MPEG_SPU_STRUCT->num = 0;
	MPEG_SPU_STRUCT->reserved = 0;
	MPEG_SPU_STRUCT->in = NULL;
	MPEG_SPU_STRUCT->out = NULL;

	khwl_blockirq(FALSE);
	
	return TRUE;
}

void mpeg_stop()
{
	mpeg_feed_reset();
	khwl_stop();
	mpeg_speed = MPEG_SPEED_STOP;
	
}

void mpeg_play()
{
	if (mpeg_speed == MPEG_SPEED_NORMAL || 
		mpeg_speed == MPEG_SPEED_PAUSE || mpeg_speed == MPEG_SPEED_STEP || 
		mpeg_speed == MPEG_SPEED_STOP)
	{
		mpeg_speed = MPEG_SPEED_NORMAL;
		int val = 256;
		khwl_setproperty(KHWL_VIDEO_SET, evSpeed, sizeof(int), &val);
		khwl_setproperty(KHWL_VIDEO_SET, evVOBUReverseSpeed, sizeof(int), &val);
		khwl_play(KHWL_PLAY_MODE_NORMAL);
		MSG("MPEG: KHWL_PLAY NORMAL\n");
	} 
	if ((mpeg_speed & MPEG_SPEED_FWD_MASK) == MPEG_SPEED_FWD_MASK)
	{
		if ((mpeg_speed & MPEG_SPEED_SLOW_FWD_MASK) == MPEG_SPEED_SLOW_FWD_MASK)
		{
			khwl_play(KHWL_PLAY_MODE_NORMAL);
			MSG("MPEG: KHWL_PLAY NORMAL\n");
		} else
		{
			khwl_play(KHWL_PLAY_MODE_IFRAME);
			MSG("MPEG: KHWL_PLAY IFRAME\n");
		}
	}
	else if ((mpeg_speed & MPEG_SPEED_REV_MASK) == MPEG_SPEED_REV_MASK)
	{
		if ((mpeg_speed & MPEG_SPEED_SLOW_REV_MASK) == MPEG_SPEED_SLOW_REV_MASK)
		{
			/// \TODO: It seems that mode doesn't work...
			khwl_play(KHWL_PLAY_MODE_REV);
			MSG("MPEG: KHWL_PLAY REV\n");
		} else
		{
			khwl_play(KHWL_PLAY_MODE_IFRAME_REV);
			MSG("MPEG: KHWL_PLAY IFRAME REV\n");
		}
	}

	firstchunk = false;
	firstdisplayed = true;
	
	mpeg_wait_last_pts = 0;
	mpeg_wait_last_depth = 0;
	mpeg_wait_count_still = 0;
	mpeg_wait_cnt = 0;	
	mpeg_show_empty_stack_depth = true;
}

void mpeg_play_normal()
{
	int val = 256;
	khwl_setproperty(KHWL_VIDEO_SET, evSpeed, sizeof(int), &val);
	khwl_play(KHWL_PLAY_MODE_NORMAL);
	MSG("MPEG: Play NORMAL\n");
	firstchunk = false;
	firstdisplayed = true;
}

BOOL mpeg_init_packets()
{
	if (packets != NULL && cur_num_packets != MPEG_NUM_PACKETS)
	{
		SPfree(packets);
		packets = NULL;
	}
	if (packets == NULL)
	{
		packets = (MpegPacket *)SPmalloc(MPEG_NUM_PACKETS * sizeof(MpegPacket));
		if (packets == NULL)
			return FALSE;
	}
	cur_num_packets = MPEG_NUM_PACKETS;

	khwl_blockirq(TRUE);
	for (int i = 0; i < MPEG_NUM_PACKETS; i++)
	{
		memset(packets + i, 0, sizeof(MpegPacket));
	}
	khwl_blockirq(FALSE);
	return TRUE;
}

int mpeg_feed_getstackdepth()
{
	if (mpeg_speed == MPEG_SPEED_STOP)
		return 0;
	if (firstchunk || mpeg_speed != MPEG_SPEED_NORMAL)
		return MPEG_NUM_PACKETS;
	return MPEG_NUM_PACKETS - MPEG_PLAY_STRUCT->num;
}

int mpeg_wait(BOOL onetime)
{
	//khwl_stop();
	if (!onetime)
		mpeg_play_normal();	// be sure we play
#if 0
	LONGLONG last_msecs = last_video_pts;
	do
	{
		if (mpeg_feed_getstackdepth() == 0)
			return 1;
		if (!mpeg_audioonly)
		{
			LONGLONG msecs = (LONGLONG)mpeg_getpts();
			LONGLONG diff = last_msecs - msecs;
			if (diff > 9000)	// 100 ms
				usleep(MIN((int)(1000 * diff / 90), 10000));
			else
				return 1;
		}
	} while (!onetime);
#endif

	do
	{
		int depth = mpeg_feed_getstackdepth();
		if (depth == 0)
		{
			if (mpeg_show_empty_stack_depth)
			{
				msg("MPEG: * stack depth=0\n");
				mpeg_show_empty_stack_depth = false;
			}
			//return 1;
		}
		if (!mpeg_audioonly)
		{
			KHWL_TIME_TYPE displ;
			displ.pts = 0;
			displ.timeres = 90000;
			khwl_getproperty(KHWL_TIME_SET, etimVideoFrameDisplayedTime, sizeof(displ), &displ);
			if (Abs((LONGLONG)displ.pts - mpeg_wait_last_pts) < 9000)
			{
				//msg("*** dpts = %d\n", (int)displ.pts);
				mpeg_wait_count_still++;
			}
			else
				mpeg_wait_count_still = 0;
			mpeg_wait_last_pts = displ.pts;
		} else
		{
			if (depth == mpeg_wait_last_depth)
			{
				//msg("*** depth=%d\n", depth);
				mpeg_wait_count_still++;
			}
			else
				mpeg_wait_count_still = 0;
			mpeg_wait_last_depth = depth;
		}
		usleep(100000);
		mpeg_wait_cnt++;
		if (mpeg_wait_count_still > 15)
		{
			mpeg_wait_last_pts = 0;
			mpeg_wait_last_depth = 0;
			mpeg_wait_count_still = 0;
			mpeg_wait_cnt = 0;
			//mpeg_show_empty_stack_depth = true;
			return 1;
		}
	} while (!onetime);

	return 0;
}

MpegPacket *mpeg_feed_getlast()
{
	MpegPlayStruct *from = MPEG_PLAY_STRUCT;

	// if playing and buffer is empty...
#ifndef WIN32
/*
	static int empty_cnt = 0;
	if (from->num == MPEG_NUM_PACKETS && !mpeg_audioonly && mpeg_fmt == MPEG_2 && !firstchunk && mpeg_speed == MPEG_SPEED_NORMAL)
	{
		if (++empty_cnt > 0)
		{
			empty_cnt = 0;
			khwl_pause();
			mpeg_start();
			msg("MPEG: Buffer is empty. Resync...\n");
		}
	} else
		empty_cnt = 0;
*/
#endif
	return from->in;
}

BYTE *mpeg_getcurbuf(MPEG_BUFFER which)
{
	return bufbase[which][cur_bufidx[which]];
}

int mpeg_getpacketlength()
{
	return MPEG_PACKET_LENGTH;
}

void mpeg_setpts(ULONGLONG estpts)
{
	KHWL_TIME_TYPE stc;
	stc.pts = estpts;
	stc.timeres = 90000;
	khwl_setproperty(KHWL_TIME_SET, etimSystemTimeClock, sizeof(stc), &stc);
	last_video_pts = estpts;
}

ULONGLONG mpeg_getpts()
{
	KHWL_TIME_TYPE stc;
	stc.pts = 0;
	stc.timeres = 90000;
	khwl_getproperty(KHWL_TIME_SET, etimSystemTimeClock, sizeof(stc), &stc);
	return stc.pts;
}

void mpeg_setaudiostream(int id)
{
	mpeg_curaudstream = id;
}

int mpeg_getaudiostream()
{
	return mpeg_curaudstream;
}

int mpeg_getaudiostreamsnum()
{
	return mpeg_numaudstream;
}

void mpeg_setspustream(int id)
{
	mpeg_curspustream = id;
}

int mpeg_getspustream()
{
	return mpeg_curspustream;
}

int mpeg_getspustreamsnum()
{
	return mpeg_numspustream;
}

void mpeg_resetstreams()
{
	mpeg_numaudstream = 0;
	mpeg_numspustream = 0;
}

int mpeg_find_free_blocks(const MPEG_BUFFER which)
{
	const int num_tries = 1;
	static int buffull = false;

	int badcnt;
	for (badcnt = 0; badcnt < num_tries; badcnt++)
	{
		cur_bufidx[which] = MPEG_NUM_BUFS[which] - 1;

		khwl_blockirq(TRUE);	
		while (cur_bufidx[which] >= 0)
		{
			if (bufidx[which][cur_bufidx[which]] == 0)
				break;
			cur_bufidx[which]--;
		}
		khwl_blockirq(FALSE);

		// when paused, just fill the buffer and wait...
		// (full buffer is absolutely normal in this mode)
		if (mpeg_speed == MPEG_SPEED_PAUSE ||
			mpeg_speed == MPEG_SPEED_STEP)
			break;
		
		if (firstchunk)
		{
			if ((MPEG_NUM_BUFS[which] <= 16 && cur_bufidx[which] <= 3)
				|| (cur_bufidx[which] <= MPEG_NUM_BUFS[which]/2))
			{
				mpeg_play();
				usleep(100);
			}
		} 

		// wait for khwl to process packets 
		// (we need to free the first buffers)
		if (cur_bufidx[which] >= 0)
		{
			buffull = false;
			break;
		}

		if (!buffull)
		{
#ifdef MPEG_BUF_DEBUG			
			msg("MPEG: .\n");
#endif
			buffull = true;
		}

		usleep(50);
		//DWORD mask = 0xffffffff;
		//khwl_happeningwait(&mask);
	}
	
	if (cur_bufidx[which] < 0)	// very bad, but still let's try again later...
	{
		cur_bufidx[which] = MPEG_NUM_BUFS[which]-1;	// no matter which...
		return 0;
	}

	return 1;
}

void mpeg_setbufidx(MPEG_BUFFER which, MpegPacket *packet)
{
	khwl_blockirq(TRUE);	
	bufidx[which][cur_bufidx[which]]++;
	khwl_blockirq(FALSE);

	// set bufidx
	if (packet != NULL)
		packet->bufidx = bufidx[which] + cur_bufidx[which];
}

void mpeg_release_packet(MPEG_BUFFER which, MpegPacket *packet)
{
	khwl_blockirq(TRUE);
	if (bufidx[which][cur_bufidx[which]] > 0)
		bufidx[which][cur_bufidx[which]]--;
	khwl_blockirq(FALSE);

	// set bufidx
	if (packet != NULL)
		packet->bufidx = NULL;
}

////////////////////////////////////////////////////////////////////////////////////
// parser

START_CODE_TYPES mpeg_findpacket(BYTE * &buf, BYTE *base, int &startCounter)
{
	static const BYTE startCode[3] = { 0x00, 0x00, 0x01 };

    // mini DFA
    for (;;) 
	{
		BYTE *bbuf = buf;
		if (!mpeg_incParsingBufIndex(buf, base, 1)) 
			return START_CODE_END;
		register BYTE b = *bbuf;
		if (startCounter == sizeof(startCode)) 
		{
			switch (b)
			{
			case pack_start_code:
				return START_CODE_PACK;
			case system_header_start_code:
				return START_CODE_SYSTEM;
			case packet_start_pci_dsi_code:
				return START_CODE_PCI;
			case packet_start_private1_code:
				return START_CODE_PRIVATE1;
			case packet_start_padding_code:
				return START_CODE_PADDING;
			case sequence_header_start_code:
				buf -= 4;	// leave this header to video parser
				return START_CODE_MPEG_VIDEO_ELEMENTARY;
			}
			if (b >= packet_start_video_code1 && b <= packet_start_video_code2)
				return START_CODE_MPEG_VIDEO;
			if (b >= packet_start_audio1_code1 && b <= packet_start_audio1_code2)
				return START_CODE_MPEG_AUDIO1;
#if 0
			if (b >= packet_start_audio2_code1 && b <= packet_start_audio2_code2)
				return START_CODE_MPEG_AUDIO2;
#endif
            startCounter = 0;
        } else 
		{
            if (b == startCode[startCounter]) 
                startCounter++;
            else if (startCounter != 2 || b != 0)	// remove this code?
                startCounter = 0;
        }
    }

	// we won't get here
	return START_CODE_UNKNOWN;
}

inline ULONGLONG mpeg_extractTimingStampfromPESheader(BYTE * &buf)
{
	BYTE *tmp = buf;
	return ((((tmp[4] >> 1) & 0x7F) +  
		 ((tmp[3] << 7) & 0x7F80) +
		 ((tmp[2] << 14) & 0x3F8000) +
		 ((tmp[1] << 22) & 0x3FC00000) +
		 ((tmp[0] << 29) & INT64(0x1C0000000))) & INT64(0x1FFFFFFFF));
}

LONGLONG mpeg_parse_program_stream_pack_header(BYTE * &buf, BYTE * /*base*/)
{
	LONGLONG scr = 0;
	mpeg_is_mpeg1 = (buf[0] & 0x40) == 0;
	int mux_rate = 0;
	if (mpeg_is_mpeg1) 
	{
		scr  = (buf[0] & 0x02) << 30;
		scr |= (buf[1] & 0xFF) << 22;
		scr |= (buf[2] & 0xFE) << 14;
		scr |= (buf[3] & 0xFF) <<  7;
		scr |= (buf[4] & 0xFE) >>  1;

		mux_rate = (buf[5] & 0x7F) << 15;
		mux_rate |= (buf[6] & 0x7F) << 7;
		mux_rate |= (buf[7] & 0xFE) >> 1;
	} else
	{
		scr  = (buf[0] & 0x38) << 27;
		scr |= (buf[0] & 0x03) << 28;
		scr |=  buf[1] << 20;
		scr |= (buf[2] & 0xF8) << 12;
		scr |= (buf[2] & 0x03) << 13;
		scr |=  buf[3] << 5;
		scr |= (buf[4] & 0xF8) >> 3;
/*
		mux_rate = (buf[6] & 0x7f) << 15;
		mux_rate |= (buf[7] << 7);
		mux_rate |= (buf[8] & 0xfe) >> 1;
*/
		mux_rate = (buf[6]) << 14;
		mux_rate |= (buf[7]) << 6;
		mux_rate |= (buf[8] & 0xfc) >> 2;
	}
	
	mpeg_rate_sum += mux_rate;
	if (mpeg_nr >= num_rates)
	{
		mpeg_rate_sum -= mpeg_rates[mpeg_rate_pos];
		mpeg_rate = mpeg_rate_sum / num_rates;		// here just for optimisation
	}
	else
	{
		mpeg_nr++;
		mpeg_rate = mpeg_rate_sum / mpeg_nr;
	}
	mpeg_rates[mpeg_rate_pos] = mux_rate;
	
	mpeg_rate_pos = (mpeg_rate_pos + 1) % num_rates;

	buf += 8;
	return scr;
}

int mpeg_getrate(BOOL now)
{
	if (now)
	{
		// at least get video mux.rate
		if (mpeg_rate == 0)
			return mpeg_video_muxrate * 50;
	}
	else if (mpeg_nr < num_rates)
		return 0;
	return mpeg_rate * 50;
}

//////////////////////////////////////////////////////////

int mpeg_setaudioparams(MpegAudioPacketInfo *paudio)
{
	if (paudio == NULL) 
		return -1;

	KHWL_AUDIO_FORMAT_TYPE audioformat = paudio->type;
	int numberofchannels, numberofbitspersample, samplerate;

	if (audioformat == eAudioFormat_PCM)
	{
		samplerate = paudio->samplerate;
		numberofchannels = paudio->numberofchannels;
		numberofbitspersample = paudio->numberofbitspersample;
	}
	else if (audioformat == eAudioFormat_MPEG1)
	{
		samplerate = paudio->samplerate;
		numberofchannels = paudio->numberofchannels;
		numberofbitspersample = 16;
	}
	else
	{
		samplerate = 48000;
		numberofchannels = mpeg_nchannels;
		numberofbitspersample = mpeg_nbitspersample;
	}

	if (samplerate != mpeg_srate || audioformat != mpeg_aformat ||
		numberofchannels != mpeg_nchannels ||
		numberofbitspersample != mpeg_nbitspersample || mpeg_needrestoreaudio)
	{
		if (paudio->fromstream)
			mpeg_needrestoreaudio = false;
		msg("MPEG: setaudio(format=%d, rate=%d, nc=%d, nbs=%d)\n", audioformat,
					samplerate, numberofchannels, numberofbitspersample);

		khwl_audioswitch(FALSE);
		mpeg_audio_format = audioformat;
		mpeg_sample_rate = samplerate;
		mpeg_audio_numchans = numberofchannels;
		mpeg_audio_bits = numberofbitspersample;
		mpeg_resample_rate = 0;
		int newsrate = mpeg_set_sample_rate(samplerate);
		if (newsrate != samplerate || mpeg_audio_bits < mpeg_min_audio_bits)
		{
			if (audioformat == eAudioFormat_PCM && newsrate > 0)
			{
				mpeg_resample_rate = newsrate;
				numberofbitspersample = mpeg_min_audio_bits;
				msg("MPEG: Audio PCM resampling from %d/%d to %d/%d.\n", 
					samplerate, mpeg_audio_bits,
					mpeg_resample_rate, mpeg_min_audio_bits);
			} else
			{
				msg_error("MPEG: Samplerate %d not supported by decoder.\n", samplerate);
				return -1;
			}
		}
		int var;
		if (audioformat == eAudioFormat_PCM)
			var = eAudioDigitalOutput_Pcm;
		else if (audioformat == eAudioFormat_DTS)
			var = eAudioDigitalOutput_Compressed;
		else
			var = settings_get(SETTING_AUDIOOUT) == 1 ? eAudioDigitalOutput_Compressed : eAudioDigitalOutput_Pcm;
		khwl_setproperty(KHWL_AUDIO_SET, eAudioDigitalOutput, sizeof(DWORD), &var);
		if (audioformat == eAudioFormat_PCM)
		{
			khwl_setproperty(KHWL_AUDIO_SET, eAudioNumberOfChannels, sizeof(DWORD), &numberofchannels);
			khwl_setproperty(KHWL_AUDIO_SET, eAudioNumberOfBitsPerSample, sizeof(DWORD), &numberofbitspersample);
		}
		khwl_setproperty(KHWL_AUDIO_SET, eAudioFormat, sizeof(KHWL_AUDIO_FORMAT_TYPE), &audioformat);
		khwl_audioswitch(TRUE);
		
		mpeg_srate = samplerate;
		mpeg_aformat = audioformat;
		mpeg_nchannels = numberofchannels;
		mpeg_nbitspersample = numberofbitspersample;

		mpeg_format_changed = true;
		mpeg_audio_format_set = true;
	}

	if (paudio->fromstream && audioformat != fip_audio_format)
	{
		if (audioformat == eAudioFormat_AC3)
		{
			fip_write_special(FIP_SPECIAL_DOLBY, 1);
			fip_write_special(FIP_SPECIAL_DTS, 0);
		}
		else if (audioformat == eAudioFormat_DTS)
		{
			fip_write_special(FIP_SPECIAL_DTS, 1);
			fip_write_special(FIP_SPECIAL_DOLBY, 0);
		}
		else
		{
			fip_write_special(FIP_SPECIAL_DOLBY, 0);
			fip_write_special(FIP_SPECIAL_DTS, 0);
		}
		fip_audio_format = audioformat;
	}

	//is_audio_set = TRUE;
	return 0;
}

MpegAudioPacketInfo mpeg_getaudioparams()
{
	MpegAudioPacketInfo apinfo;
	apinfo.type = mpeg_audio_format;
	apinfo.samplerate = mpeg_sample_rate;
	apinfo.numberofchannels = mpeg_audio_numchans;
	apinfo.numberofbitspersample = mpeg_audio_bits;
	apinfo.fromstream = TRUE;
	return apinfo;
}

int mpeg_set_sample_rate(int samplerate)
{
	int i, *r = khwl_get_samplerates();
	for (i = 0; r[i] > 0; i++)
	{
		if (r[i] == samplerate)
		{
			goto found;
		}
	}
	// find 2x upsample rates
	for (i = 0; r[i] > 0; i++)
	{
		if (r[i] == samplerate * 2)
		{
			samplerate *= 2;
			goto found;
		}
	}
	// find 1/2 downsample rates
	for (i = 0; r[i] > 0; i++)
	{
		if (r[i] == samplerate / 2)
		{
			samplerate /= 2;
			goto found;
		}
	}
	// find closest downsample freq.
	for (i = 1; r[i] > 0; i++)
	{
		if (r[i] > samplerate)
		{
			samplerate = r[i - 1];
			goto found;
		}
	}
/*
	// find closest upsample freq.
	for (--i; i > 0; i--)
	{
		if (r[i - 1] < samplerate)
		{
			samplerate = r[i];
			goto found;
		}
	}
*/
	return -1;

found:
	msg("MPEG: * Samplerate = %d.\n", samplerate);
	khwl_setproperty(KHWL_AUDIO_SET, eAudioSampleRate, sizeof(DWORD), &samplerate);
	return samplerate;
}

void mpeg_PCM_to_LPCM(BYTE *buf, int len)
{
	if (mpeg_audio_bits == 16)
	{
		register int i;
		if (((DWORD)buf & 3) == 0)		// DWORD-aligned version
		{
			register DWORD *dw = (DWORD *)buf;
			int ds = len / 4;
			for (i = ds - 1; i >= 0; i--, dw++)
			{
				*dw = ((*dw & 0x00ff00ff) << 8) | ((*dw & 0xff00ff00) >> 8);
			}
			
			if (len & 3)
			{
				register WORD *w = (WORD *)dw;
				*w = (WORD)(((*w & 0xff) << 8) | (*w >> 8));
			}
		}
		else if (((DWORD)buf & 1) == 0)		// WORD-aligned version
		{
			register WORD *w = (WORD *)buf;
			int ws = len / 2;
			for (i = ws - 1; i >= 0; i--, w++)
			{
				*w = (WORD)(((*w & 0xff) << 8) | (*w >> 8));
			}
		}
		else
		{
			register BYTE tmp;
			for (i = len - 1; i >= 0; i -= 2, buf += 2)
			{
				tmp = buf[0];
				buf[0] = buf[1];
				buf[1] = tmp;
			}
		}
	}
}

BOOL mpeg_is_resample_needed()
{
	return mpeg_resample_rate != 0;
}

int mpeg_PCM_resample_to_LPCM(BYTE *buf, int len, BYTE *out, int *olen)
{
	if (mpeg_resample_rate == 0 || mpeg_sample_rate == 0)
		return -1;
	int inlen, outlen;
	// 8->16 bits
	if (mpeg_audio_bits == 8 && mpeg_audio_bits < mpeg_min_audio_bits)
	{
		outlen = *olen / 2;
		inlen = outlen * mpeg_sample_rate / mpeg_resample_rate;
		if (len < inlen)
		{
			inlen = len;
			outlen = len * mpeg_resample_rate / mpeg_sample_rate;
		}
	
		const int FRAC_SHIFT = 10, FRAC_MUL = 1 << FRAC_SHIFT;
		int ind = mpeg_sample_rate > mpeg_resample_rate ? 
			mpeg_sample_rate * FRAC_MUL / mpeg_resample_rate : FRAC_MUL;
		int outd = mpeg_resample_rate > mpeg_sample_rate ? 
			mpeg_resample_rate * FRAC_MUL / mpeg_sample_rate : FRAC_MUL;
		register int inpos = 0, outpos = 0, oldoutpos = 0;
		for (int i = mpeg_resample_rate > mpeg_sample_rate ? inlen - 1 : outlen - 1; i >= 0; i--)
		{
			// input contains non-aligned WORDs
			register BYTE *b = buf + (inpos >> FRAC_SHIFT);
			register WORD *o = (WORD *)out + (outpos >> FRAC_SHIFT);
			inpos += ind;
			outpos += outd;
			WORD w = (WORD)(*b - 0x80);		// little-endian -> big-endian
			for (int j = (outpos - oldoutpos) >> FRAC_SHIFT; j > 0; j--)
				*o++ = w;
			oldoutpos = outpos;
		}

		*olen = outlen * 2;
	}
	else if (mpeg_audio_bits == 8)
	{
		outlen = *olen;
		inlen = outlen * mpeg_sample_rate / mpeg_resample_rate;
		if (len < inlen)
		{
			inlen = len;
			outlen = len * mpeg_resample_rate / mpeg_sample_rate;
		}
	
		const int FRAC_SHIFT = 10, FRAC_MUL = 1 << FRAC_SHIFT;
		int ind = mpeg_sample_rate > mpeg_resample_rate ? 
			mpeg_sample_rate * FRAC_MUL / mpeg_resample_rate : FRAC_MUL;
		int outd = mpeg_resample_rate > mpeg_sample_rate ? 
			mpeg_resample_rate * FRAC_MUL / mpeg_sample_rate : FRAC_MUL;
		register int inpos = 0, outpos = 0, oldoutpos = 0;
		for (int i = mpeg_resample_rate > mpeg_sample_rate ? inlen - 1 : outlen - 1; i >= 0; i--)
		{
			register BYTE *b = buf + (inpos >> FRAC_SHIFT);
			register BYTE *o = out + (outpos >> FRAC_SHIFT);
			inpos += ind;
			outpos += outd;
			for (int j = (outpos - oldoutpos) >> FRAC_SHIFT; j > 0; j--)
				*o++ = *b;
			oldoutpos = outpos;
		}

		*olen = outlen;
	}
	else if (mpeg_audio_bits == 16)
	{
		outlen = *olen / 2;
		inlen = outlen * mpeg_sample_rate / mpeg_resample_rate;
		len /= 2;
		if (len < inlen)
		{
			inlen = len;
			outlen = len * mpeg_resample_rate / mpeg_sample_rate;
		}
	
		const int FRAC_SHIFT = 10, FRAC_MUL = 1 << FRAC_SHIFT;
		int ind = mpeg_sample_rate > mpeg_resample_rate ? 
			mpeg_sample_rate * FRAC_MUL / mpeg_resample_rate : FRAC_MUL;
		int outd = mpeg_resample_rate > mpeg_sample_rate ? 
			mpeg_resample_rate * FRAC_MUL / mpeg_sample_rate : FRAC_MUL;
		register int inpos = 0, outpos = 0, oldoutpos = 0;
		for (int i = mpeg_resample_rate > mpeg_sample_rate ? inlen - 1 : outlen - 1; i >= 0; i--)
		{
			// input contains non-aligned WORDs
			register BYTE *b = buf + (inpos >> FRAC_SHIFT) * 2;
			register WORD *o = (WORD *)out + (outpos >> FRAC_SHIFT);
			inpos += ind;
			outpos += outd;
			WORD w = (WORD)((*b << 8) | b[1]);		// little-endian -> big-endian
			for (int j = (outpos - oldoutpos) >> FRAC_SHIFT; j > 0; j--)
				*o++ = w;
			oldoutpos = outpos;
		}

		*olen = outlen * 2;
		inlen *= 2;
	}
	else
		return -1;
	
	return inlen;
}

WORD mpeg_calc_crc16(BYTE *buf, DWORD bitsize)
{
	DWORD n;
	WORD tmpchar, crcmask, tmpi;
	crcmask = tmpchar = 0;
	WORD crc = 0xffff;			// start with inverted value of 0

	// start with byte 2 of header
	for (n = 16;  n < bitsize;  n++)
	{
		if (n < 32 || n >= 48) // skip the 2 bytes of the crc itself
		{
			if (n%8 == 0)
			{
				crcmask = 1 << 8;
				tmpchar = buf[n/8];
			}
			crcmask >>= 1;
			tmpi = (WORD)(crc & 0x8000);
			crc <<= 1;
			
			if (!tmpi ^ !(tmpchar & crcmask))
				crc ^= 0x8005;
		}
	}
	crc &= 0xffff;	// invert the result
	return crc;
}

int mpeg_extractpacket(BYTE * &buf, BYTE *base, MpegPacket *packet, START_CODE_TYPES type, BOOL parse_video)
{
	BYTE *startbuf = buf;
	if (packet != NULL) 
	{
		WORD pes_packet_length = 0;
		BYTE pes_header_data_length = 0;
		BYTE pes_header_data_length0 = 3;
		BYTE pts_dts_flags = 0;
		BYTE pes_extension_flag = 0;
		BOOL encrypted = FALSE;
		BOOL need_skip = FALSE;

		if (type == START_CODE_MPEG_VIDEO_ELEMENTARY)
		{
			pes_packet_length = (WORD)(MPEG_PACKET_LENGTH + 3);
		} else
		{
			pes_packet_length = (WORD)(((buf[0] & 0xFF) << 8) + (buf[1] & 0xFF));

			if (mpeg_is_mpeg1) 
			{
				if (!mpeg_incParsingBufIndex(buf, base, 2))
					return -1;
				//pes_header_data_length = 1;
				pes_header_data_length = 1;
				pes_header_data_length0 = 0;
				if (mpeg_eofBuf(buf, base))
					return -1;
				// stuffing
				while ((*buf & 0x80) == 0x80) 
				{
					if (!mpeg_incParsingBufIndex(buf, base, 1))
						return -1;
					pes_header_data_length0++;
				}
				if (mpeg_eofBuf(buf, base))
					return -1;
				// STD_buffer_scale, STD_buffer_size
				if ((*buf & 0xc0) == 0x40) 
				{
					if (!mpeg_incParsingBufIndex(buf, base, 2))
						return -1;
					pes_header_data_length0 += 2;
				}
				if (mpeg_eofBuf(buf, base))
					return -1;
				if ((*buf & 0xe0) == 0x20)
				{
					pes_header_data_length += 4;
					if (*buf & 0x10)
						pes_header_data_length += 5;
					pts_dts_flags = 2;
				}
				// mpeg-2 pes
				else if ((*buf & 0xc0) == 0x80)
				{
					pts_dts_flags = (BYTE)(((*buf) >> 6) & 0x3);
					pes_extension_flag = (BYTE)((*buf++) & 0x1);
					pes_header_data_length = (BYTE)(pes_header_data_length + (*buf++));
				} 

				// don't use SCR for MPEG-1 (or VCD) stream.
				// \TODO: Is it correct?
				use_scr = false;
				
			} else
			{
				encrypted = (buf[2] & 0x30) != 0;
				if (!mpeg_incParsingBufIndex(buf, base, 3))
					return -1;
				pes_header_data_length0 = 3;
				pts_dts_flags = (BYTE)(((*buf) >> 6) & 0x3);
				pes_extension_flag = (BYTE)((*buf++) & 0x1);
				pes_header_data_length = *buf++;
				if (mpeg_eofBuf(buf, base))
					return -1;
			}
			switch(pts_dts_flags) 
			{
			case 0x3:
				// no dts info for audio packets, but pts is good
			case 0x1:
				// ???
			case 0x2:
				// base timestamp is set by caller
				packet->pts += mpeg_extractTimingStampfromPESheader(buf);
				packet->flags = 2;
				break;
			default:
				// no timing info
				packet->pts = 0;
				break;
			}
			buf += pes_header_data_length;
			if (mpeg_eofBuf(buf, base))
				return -1;
		}

		if (!use_scr)
			packet->scr = 0;
		
		int pack_length = 0;
        MpegAudioPacketInfo plaudioinfo;
		bool forbid_setaudio = false;

		switch (type)
		{
		case START_CODE_MPEG_VIDEO:
		case START_CODE_MPEG_VIDEO_ELEMENTARY:
			{
				mpeg_was_seq_header = false;
				mpeg_was_picture = false;
				packet->type = 0;
				// find frame width & height
				BYTE *b = buf;
				// header
				if (b[0] == 0 && b[1] == 0 && b[2] == 1 && b[3] == 0xb3)
				{
					int wh = (b[4] << 16) | (b[5] << 8)  | b[6];
					mpeg_width = ((wh >> 12) + 15) & ~15;
					mpeg_height = ((wh & 0xfff) + 15) & ~15;
					mpeg_fps = mpeg_fps_table[b[7] & 0xf];
					
					mpeg_aspect = (b[7] >> 4) & 0xf;	/* 1 = 1:1, 2 = 4:3, 3 = 16:9 */
					
					mpeg_was_seq_header = true;

					if (mpeg_is_mpeg1)
						mpeg_video_muxrate = (b[8] << 10) | (b[9] << 2) | (b[10] >> 6);
					else
						mpeg_video_muxrate = (b[8] << 10) | (b[9] << 2) | (b[10] >> 6);
					if (mpeg_video_muxrate == 0x3FFFF)
						mpeg_video_muxrate = 0;

					if (mpeg_width != mpeg_old_width || mpeg_height != mpeg_old_height || mpeg_fps != mpeg_old_fps
						|| mpeg_aspect != mpeg_old_aspect)
					{
						//mpeg_setframesize(mpeg_width, mpeg_height);
						mpeg_old_width = mpeg_width;
						mpeg_old_height = mpeg_height;
						mpeg_old_fps = mpeg_fps;
						mpeg_old_aspect = mpeg_aspect;
					}

					b += 4;
					b += 7;
					if (mpeg_eofBuf(b, base))
					{
						buf = b;
						return -1;
					}
					if (b[0] & 2) 
						b += 64;
					if (b[0] & 1)
						b += 64;
					b += 1;
					if (mpeg_eofBuf(b, base))
					{
						buf = b;
						return -1;
					}
					if (mpeg_need_seq_start_packet /*&& b - buf >= 12*/)
					{
						/*
						mpeg_seq_start_packet_len = b - buf;
						memcpy(mpeg_seq_start_packet, buf, mpeg_seq_start_packet_len);
						mpeg_need_seq_start_packet = false;
						*/

						if (mpeg_is_mpeg1)
						{
							mpeg_seq_start_packet_len = 12;
							memcpy(mpeg_seq_start_packet, buf, mpeg_seq_start_packet_len);
							mpeg_seq_start_packet[8] |= 3;
							mpeg_seq_start_packet[9] |= 0xff;
							mpeg_seq_start_packet[10] |= 0xff;
							mpeg_seq_start_packet[11] &= ~3;
						} else
							mpeg_seq_start_packet_len = 0;

						memcpy(mpeg_seq_start_packet + mpeg_seq_start_packet_len, mpeg_gop_packet, sizeof(mpeg_gop_packet));
						mpeg_seq_start_packet_len += sizeof(mpeg_gop_packet);
						mpeg_need_seq_start_packet = false;
					}

				}
#if 1
				else if (parse_video && !mpeg_is_mpeg1)
				{
					int psize = pes_packet_length - pes_header_data_length - pes_header_data_length0 - pack_length;
					for (BYTE *endb = buf + psize; b < endb; b++)
					{
						if (b[0] == 0 && b[1] == 0 && b[2] == 1 && b[3] == 0)
						{
							// test picture type
							int pic_type = (b[5] >> 3) & 3;
							if (pic_type == 1)
							{
								mpeg_was_picture = true;
								pes_packet_length -= (WORD)(b - buf);
								buf = b;
								break;
							}
						}
					}
				}
#endif
#if 0					
				while (mpeg_parse_video)
				{
					// extension
					if (b[0] == 0 && b[1] == 0 && b[2] == 1 && b[3] == 0xb5)
					{
						b += 4;
						switch (b[0] & 0xf0) 
						{
						case 0x10: /* sequence extension */
							b += 6; break;	
						case 0x20: /* sequence display extension */
							b += 1 + 1 + 1 + 1 + 4;  break; 
						case 0x30:	/* quant matrix extension */
							{
							if (b[0] & 8) 
								b += 64;
							if (b[0] & 4)
								b += 64;
							break;
							}
						case 0x70:	/* picture display extension for Pan & Scan */
							b += 4; break;

						case 0x80:	/* picture coding extension */
							b += 5; break;
						}
						continue;
					}
					// GOP
					if (b[0] == 0 && b[1] == 0 && b[2] == 1 && b[3] == 0xb8)
					{
						b += 8;
						bool gop_start = true;
					}
					break;
				}
#endif
			}
			break;
		case START_CODE_MPEG_AUDIO1:			// MPEG-1 L1, L2, L2.5
		case START_CODE_MPEG_AUDIO2:
			{
			// this is not good, but it works...
			int substreamid = *(startbuf - 1);
			packet->type = 1;
			mpeg_numaudstream = MAX(mpeg_numaudstream, (substreamid & 0x07)+1);

			if ((substreamid & 0x1f) != mpeg_curaudstream)		// skip 
			{
				need_skip = TRUE;
				break;
			}

			packet->nframeheaders = 0xffff;
			packet->firstaccessunitpointer = 0;
			forbid_setaudio = true;

#if 0
			if (type == START_CODE_MPEG_AUDIO2)
			{
				WORD nfh = ((buf[0] & 0xFF) << 8) + (buf[1] & 0xFF);
				packet->nframeheaders = 0xffff;
				packet->firstaccessunitpointer = 0;
				pack_length = nfh + 6;	// ???
				buf += nfh + 6;
			} else
				pack_length = 1;
#endif

			DWORD header = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
			if (mpeg_allow_mpa_parsing && (header & 0xffe00000) == 0xffe00000)
			{
				plaudioinfo.samplerate = 48000;
				plaudioinfo.numberofchannels = 2;

				DWORD mpglayer = 4 - ((header >> 17) & 0x3);
				bool has_CRC = ((header >> 16) & 0x1) == 0;
				DWORD sample_rate_index = (header >> 10) & 3;
				DWORD bitrate_index = (header >> 12) & 0xf;
				DWORD emphasis = header & 3;
				DWORD mode = (header >> 6) & 3;
				DWORD modeext = (header >> 4) & 3;
				//msg("MPEG: *FFE* layer=%d, old=%d\n", mpglayer, old_mpglayer);
				if (has_CRC && mpglayer == 1)
				{
					int bound = mode == 1 ? 4 + modeext * 4 : 32;
					int pbits = 4* ((mode == 3 ? 1 : 2) * bound + (32 - bound));
					pbits += 4*8 + 16;
					//int crcsize = (pbits + 7) / 8; 
					WORD wCRC16 = mpeg_calc_crc16(buf, pbits);
					// read out crc from frame (it follows frame header)
					WORD fileCRC = (WORD)((buf[4] << 8) | buf[5]);
					has_CRC = wCRC16 != fileCRC;
				} 
				else if (!mpeg_audio_format_set)	// give it a chance
					has_CRC = false;

				// layer change currently not supported in MPEG-1/2 container...
				if ((int)mpglayer == old_mpglayer || old_mpglayer == -1)
				{
					// mpeg1l3 currently not supported in MPEG-1/2 container...
					if (!has_CRC && sample_rate_index < 3 && mpglayer < 3 && bitrate_index < 15 && emphasis != 2)
					{
						DWORD version_idx = 0;
						if (mpglayer == 2)		// mpeg1 L2 extensions...
						{
							if (((header >> 19) & 0x1) == 0)	// lsf
								version_idx++;
							if (((header >> 20) & 0x1) == 0)	// mpeg25
								version_idx++;
						}
						

						const DWORD mpa_freq_tab[] = { 44100, 48000, 32000 };
						plaudioinfo.samplerate = mpa_freq_tab[sample_rate_index] >> version_idx/*(lsf + mpeg25)*/;
						plaudioinfo.numberofchannels = (mode == 3/*mono*/) ? 1 : 2;
						old_mpglayer = mpglayer;
						forbid_setaudio = false;

						if (mpeg_sample_rate != plaudioinfo.samplerate || 
							mpeg_audio_numchans != plaudioinfo.numberofchannels ||
							mpeg_show_mpa_parsing)
						{
							msg("MPEG: * Audio stream format detected: %d kHz %s\n", plaudioinfo.samplerate, 
								plaudioinfo.numberofchannels == 1 ? "mono" : "stereo");
							mpeg_show_mpa_parsing = false;
						}
					}
				}
			} else 
			if (mpeg_needrestoreaudio)
			{
				plaudioinfo.samplerate = 48000;
				plaudioinfo.numberofchannels = 2;
				forbid_setaudio = false;
			}

			plaudioinfo.type = eAudioFormat_MPEG1;
			break;
			}
		case START_CODE_PRIVATE1:				// DVD private
			{
			int substreamid = *buf++;
			pack_length = 1;
			if ((substreamid & 0xE0) == 0x20)		// DVD SPU substream
			{
				int spu_id = (substreamid & 0x1f);
				mpeg_numspustream = MAX(mpeg_numspustream, spu_id+1);
				packet->type = 2;

				if (spu_id != mpeg_curspustream && mpeg_curspustream != -2)
				{
					need_skip = TRUE;
					break;
				}
			}
			else if (substreamid == 0x70 && (packet->nframeheaders & 0xFC) == 0x00) // SVCD OGT spu...
			{
				int spu_id = *buf++;
				mpeg_numspustream = MAX(mpeg_numspustream, spu_id+1);
				packet->type = 2;
				
				if (spu_id != mpeg_curspustream)
				{
					need_skip = TRUE;
					break;
				}
			}
			else if ((substreamid & 0xFC) == 0x00) // CVD spu...
			{
				int spu_id = (substreamid & 0x03);
				mpeg_numspustream = MAX(mpeg_numspustream, spu_id+1);
				packet->type = 2;
				
				if (spu_id != mpeg_curspustream)
				{
					need_skip = TRUE;
					break;
				}
			}
			else if ((substreamid & 0xF0) == 0x80) 	// AC3/DTS
			{
				BYTE nframeheaders = *buf;
				packet->nframeheaders = 0;
				packet->firstaccessunitpointer = (WORD)(((buf[1] & 0xFF) << 8) + (buf[2] & 0xFF));
				buf += 3;
				pack_length += 3;

				mpeg_numaudstream = MAX(mpeg_numaudstream, (substreamid & 0x07)+1);
				packet->type = 1;

				if ((substreamid & 0x07) != mpeg_curaudstream)		// skip 
				{
					need_skip = TRUE;
					break;
				}
							
				if ((substreamid & 0xF8) == 0x88)
				{
					plaudioinfo.type = eAudioFormat_DTS;
					packet->nframeheaders = nframeheaders;
				}
				else
					plaudioinfo.type = eAudioFormat_AC3;
			}
			else if ((substreamid & 0xf0) == 0xa0) 		// MPEG-2 lpcm
			{
				int track = substreamid & 0x07;

				mpeg_numaudstream = MAX(mpeg_numaudstream, track+1);
				packet->type = 1;

				if (track != mpeg_curaudstream)		// skip 
				{
					buf += 6;
					pack_length += 6;
					need_skip = TRUE;
					break;
				}

				packet->nframeheaders = 0xffff;//buf[0];
				packet->firstaccessunitpointer = (WORD)(((buf[1] & 0xFF) << 8) + (buf[2] & 0xFF));
				const DWORD lpcm_freq_tab[] = { 48000, 96000, 44100, 32000 };
				plaudioinfo.samplerate = lpcm_freq_tab[(buf[4] >> 4) & 0x3];
				plaudioinfo.numberofchannels = (buf[4] & 0x7) + 1;
				switch ((buf[4] >> 6) & 3) 
				{
				case 1:
					plaudioinfo.numberofbitspersample = 20;
					break;
				case 2:
					plaudioinfo.numberofbitspersample = 24;
					break;
				case 3:
					plaudioinfo.numberofbitspersample = 32;
					break;
				default: // = 0
					plaudioinfo.numberofbitspersample = 16;
				}
				plaudioinfo.dynrange = buf[5];
				buf += 6;

				pack_length += 6;
				plaudioinfo.type = eAudioFormat_PCM;
			}
			break;
			}
		default:
			;
		}
		
		packet->pData = buf;
		packet->size = pes_packet_length - pes_header_data_length - pes_header_data_length0 - pack_length;

		if (need_skip)
			return 0;
		
		if (encrypted)
		{
			packet->encryptedinfo = 0x8000 | (105 - pes_header_data_length - pack_length);
			if (tell_about_encryption)
			{
				MSG("MPEG: !!CSS-encrypted!!\n");
				tell_about_encryption = false;
			}
		}

		mpeg_format_changed = false;
		if (packet->type == 1 && !forbid_setaudio)	// audio
		{
			plaudioinfo.fromstream = 1;
			mpeg_setaudioparams(&plaudioinfo);
		}

		return 1;
	}
	return -1;
}

int mpeg_parse_ac3_header(BYTE *buf, int len, int *bitrate, int *sample_rate, int *channels, int *lfe, int *framesize)
{
	if (len < 7)
		return -1;
	// check sync.word
	if (buf[0] != 0xb || buf[1] != 0x77)
		return -1;

    int bsid = (buf[5] >> 3) & 0x1f;

	// if E-AC-3, not AC-3
	if (bsid > 10)
		return -2;
	
    int fscod = (buf[4] >> 6) & 3;
    if (fscod == 3)
        return -3;

    int frmsizecod = buf[4] & 0x3f;
    if (frmsizecod > 37)
        return -4;

	int frame_size = ac3_frame_sizes[frmsizecod][fscod] * 2;
	ac3_total_frame_size = ac3_next_total_frame_size;
	ac3_next_total_frame_size += frame_size;

	if (framesize != NULL)
		*framesize = frame_size;
/*
	msg("frame = %d, bps = %d\n", frame_size,
		(ac3_bitrates[frmsizecod >> 1] * 1000) >> halfratecod);
*/

	// fill in extended info
	if (bitrate != NULL)
	{
		int halfratecod = MAX(bsid, 8) - 8;

		*bitrate = (ac3_bitrates[frmsizecod >> 1] * 1000) >> halfratecod;
		if (sample_rate != NULL)
			*sample_rate = ac3_freqs[fscod] >> halfratecod;
		int acmod = (buf[6] >> 5) & 7;
		if (channels != NULL)
			*channels = ac3_channels[acmod];
		if (lfe != NULL)
		{
			int lfe_offs = 4;
			if ((acmod & 1) && acmod != 1) 
				lfe_offs -= 2;
			if (acmod & 4)
				lfe_offs -= 2;
			if (acmod == 2)
				lfe_offs -= 2;

			*lfe = (buf[6] >> lfe_offs) & 1;
		}
	}

	return 0;
}

BOOL mpeg_audio_format_changed()
{
	return mpeg_format_changed;
}

LONGLONG mpeg_detect_and_fix_pts_wrap(MpegPacket *packet)
{
	LONGLONG diff = last_video_pts - packet->pts;
	LONGLONG scr = packet->scr & (~SPTM_SCR_FLAG);
	if (use_scr && Abs(packet->pts - scr) > MPEG_WRAP_THRESHOLD)
	{
		packet->scr = 0;
		use_scr = false;
	}
	if (last_video_pts != 0 && (int)diff > MPEG_WRAP_THRESHOLD) 
	{
		// fix packet data
		packet->pts += diff;
		packet->scr = (scr + diff) | SPTM_SCR_FLAG;
		
		return diff;
    }
    
	return 0;
}

BOOL mpeg_needs_seq_start_header(MpegPacket **cur_packet)
{
	if (mpeg_was_seq_header)
		return TRUE;
#if 1
	// for MPEG2 we can only wait for native seq.headers
	if (!mpeg_is_mpeg1 && !mpeg_was_picture)
		return FALSE;
#endif

	if (mpeg_seq_start_packet_len < 1)
		return FALSE;

	// first, save current packet
	MpegPacket tmp_packet;
	memcpy((BYTE *)&tmp_packet + 4, (BYTE *)*cur_packet + 4, sizeof(MpegPacket) - 4);

	MpegPacket *packet = NULL;
	packet = mpeg_feed_getlast();
	if (packet == NULL)		// well, it won't really help
		return FALSE;

	memset((BYTE *)packet + 4, 0, sizeof(MpegPacket) - 4);
	//packet->type = 0;
	//packet->flags = 0;
	//packet->bufidx = 0;
	packet->pData = mpeg_seq_start_packet;
	packet->size = mpeg_seq_start_packet_len;
	mpeg_feed(MPEG_FEED_VIDEO);

	*cur_packet = mpeg_feed_getlast();
	if (*cur_packet == NULL)		// well, it won't really help
		return FALSE;
	memcpy((BYTE *)*cur_packet + 4, (BYTE *)&tmp_packet + 4, sizeof(MpegPacket) - 4);

	return TRUE;
}

void mpeg_set_scale_rate(DWORD *scale, DWORD *rate)
{
	if (*scale < 1)
		*scale = 1;
	if (*rate < 1)
		*rate = 1;

	if (*scale < 1000)
	{
		int d = 1000 / *scale;
		*scale *= d;
		*rate *= d;
	} 
	if (*rate >= 64000)
	{
		*rate = (int)(((LONGLONG)*rate * 1000) / *scale);
		*scale = 1000;
	}
	if (*scale == 1000 && *rate == 23975)
	{
		*rate = 24000;
		*scale = 1001;
	}
	if (*scale == 1000 && *rate == 23976)
	{
		*rate = 24000;
		*scale = 1001;
	}
	if (*scale == 1000)
	{
		if (*rate >= 29992 && *rate <= 30007)
		{
			if (*rate != 30000)
			{
				*rate = 30000;
				*scale = 1001;
			}
		}
	}
	
	khwl_setproperty(KHWL_TIME_SET, etimVOPTimeIncrRes, sizeof(int), rate);
	khwl_setproperty(KHWL_TIME_SET, etimVideoCTSTimeScale, sizeof(int), rate);
	khwl_setproperty(KHWL_TIME_SET, etimAudioCTSTimeScale, sizeof(int), rate);
	
	KHWL_FIXED_VOP_RATE_TYPE voprate;
	voprate.force = 1;
	voprate.time_incr = *scale;
	voprate.incr_res = *rate;
	mpeg_vop_rate = *rate;
	mpeg_vop_scale = *scale;
	khwl_setproperty(KHWL_DECODER_SET, edecForceFixedVOPRate, sizeof(voprate), &voprate);

	mpeg_fps = (int)(INT64(1000) * mpeg_vop_rate / mpeg_vop_scale);
}

void mpeg_zoom_scroll()
{
	const int zoom_delta = 25;
	const int scroll_delta = 15;
	
	MSG("MPEG: zoom_scroll(%d,%d %d,%d)\n", mpeg_zoom_hscale, mpeg_zoom_vscale, mpeg_zoom_offsetx, mpeg_zoom_offsety);

	khwl_set_window(mpeg_width, mpeg_height, mpeg_width, mpeg_height, 
					100 + zoom_delta*mpeg_zoom_hscale, 100 + zoom_delta*mpeg_zoom_vscale, 
					mpeg_zoom_offsetx*scroll_delta, mpeg_zoom_offsety*scroll_delta);
}

BOOL mpeg_zoom_hor(int scale)
{
	mpeg_zoom_hscale = scale;
	mpeg_zoom_scroll();
	return TRUE;
}

BOOL mpeg_zoom_ver(int scale)
{
	mpeg_zoom_vscale = scale;
	mpeg_zoom_scroll();
	return TRUE;
}

BOOL mpeg_scroll(int offsetx, int offsety)
{
	mpeg_zoom_offsetx = offsetx;
	mpeg_zoom_offsety = offsety;
	mpeg_zoom_scroll();
	return TRUE;
}

BOOL mpeg_zoom_reset()
{
	if (mpeg_zoom_hscale != 0 || mpeg_zoom_vscale != 0 || mpeg_zoom_offsetx != 0 || mpeg_zoom_offsety != 0)
	{
		mpeg_zoom_hscale = 0;
		mpeg_zoom_vscale = 0;
		mpeg_zoom_offsetx = 0;
		mpeg_zoom_offsety = 0;
		mpeg_zoom_scroll();
		return TRUE;
	}
	return FALSE;
}
