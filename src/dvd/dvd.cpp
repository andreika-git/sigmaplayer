//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - DVD player source file. Uses libdvdnav.
 *  \file       dvd/dvd.cpp
 *  \author     bombur
 *  \version    0.21
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
#include <inttypes.h>
#include <time.h>
#include <byteswap.h>
#include <time.h>
#include <sys/time.h>

#include <dvdnav.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_fip.h>
#include <libsp/sp_khwl.h>
#include <libsp/sp_mpeg.h>
#include <libsp/sp_cdrom.h>

#include "script.h"

#include <dvd.h>

#include "dvd-internal.h"
#include "dvd_misc.h"
#include "media.h"
#include "settings.h"


class DvdLanguage
{
public:
	const char *str;
	WORD code;
};


#include "dvd-langs.inc.cpp"

/////////////////////////////////////
#define DVD_DEBUG
#define DVD_USE_MACROVISION
// It's a must if we use our buffering!!!
#define DVD_USE_CACHE

/////////////////////////////////////

#ifndef DVD_DEBUG
static void empty_msg(char *,...)
{
}
#define MSG empty_msg
#define MSG_ERROR empty_msg
#else
#define MSG if (dvd_msg) msg
#define MSG_ERROR msg_error
#endif

//////////////////////////////////////////////////////////////////////

//#define DVD_PTS_DEBUG 1
//#define DVD_PACKETS_DEBUG 1

const int dvd_num_saved = 5;

int MPEG_PACKET_LENGTH = 0;

bool no_spu = false;
bool no_khwl_play = false;
#ifdef WIN32
bool dvd_msg = true;
#else
bool dvd_msg = false;
#endif

static int cur_titleid, cur_chapid, cur_titlpos;
static int old_titleid, old_chapid;
static int cur_pack;
static LONGLONG cur_cell_pts = 0, old_cell_pts = 0, cur_cell_length = 0, cur_vobu_pts = 0;
static LONGLONG cur_pgc_length = 0;
static LONGLONG saved_pts = 0;
static int old_vob_id = -1, old_vob_cell_id = -1;
static bool vts_changed = false;
static bool dvd_menu_ahead = false;
static bool need_user_reset = false;
static int number_of_angles = 1;

static bool dvd_play_from_drive = false;

static int dvd_skipoffs = 0;
static int num_packets = 0;

static dvdnav_t *dvd = NULL;
static pci_t *cur_pci = NULL;
static dsi_t *cur_dsi = NULL;

static DWORD dvd_ID = 0;
static bool dvd_was_saved = false;
static DWORD dvd_saved_pos = 0xffffffff;
static DWORD dvd_saved_title = 0xffffffff;
static int dvd_saved_vmode = -1, dvd_saved_audio_stream = -1, dvd_saved_spu_stream = -1;

static LONGLONG dvd_vobu_start_ptm = 0;
static LONGLONG pts_base = 0;
static int dvd_vobu_start_pos = 0;
static bool dvd_seamless = true;
static bool dvd_lastbtnupdate = false;

static bool dvd_not_played_yet = true;
static bool dvd_waiting_to_play = false;

static int dvd_mv_flags = -1;
static bool dvd_use_mv = true;

static int read_max_allowed_time = 180;

extern bool msg_debug;

extern "C" 
{
	int mv_cgms_flags = 0;
}

static int dvd_read_errors = 0;
static bool wait_for_user = false;
static int wait_pause = 0;
static ULONGLONG wait_time = 0;
static int wait_iter = 0;
static int dvd_cur_button = -1;
static bool dvd_buttons_enabled = false;
static bool dvd_button_selected = false;

static bool need_to_set_pts = false;
static bool need_to_check_pts = false;
static LONGLONG vobu_sptm = 0;

static LONGLONG dvd_vobu_last_ptm = 0;

static bool need_skip = false, was_video = false, can_skip = false;
static int num_skip = 0;

static bool dvd_scan = false;

static bool new_frame_size = false;
static int dvd_video_info_cnt = 0;

static int spu_command = 0;
static MPEG_SPEED_TYPE dvd_speed = MPEG_SPEED_NORMAL;
static KHWL_VIDEOMODE dvd_vmode = KHWL_VIDEOMODE_NORMAL;
static int dvd_spu_channel_letterbox = -1;
static int dvd_spu_channel_panscan = -1;
static KHWL_SPU_BUTTON_TYPE last_upd_but;

static int dvd_spu_stream = -1, dvd_old_spu_stream = -1;
static int dvd_aud_stream = -1, dvd_old_aud_stream = -1;

static int forced_spu = -1;
static WORD forced_spu_lang = 0xffff;
static int forced_audio = -1;
static WORD forced_audio_lang = 0xffff;

static WORD deflang_menu = 0x656e, deflang_audio = 0x656e, deflang_spu = 0x656e;

static BYTE *dvdlang_lut = NULL;

static void dvd_clearhighlights();
static void dvd_spu_enable(bool onoff);
static void dvd_buttons_enable(bool onoff);
static int dvd_player_loop_internal(bool feed = true);

typedef struct DVD_BUTTON
{
	DWORD left, top, right, bottom;
} DVD_BUTTON;

const int max_buttons = 256;
DVD_BUTTON cur_buttons[max_buttons];
int cur_num_buttons = 0;
bool need_to_check_buttons = true;

////////////////////////////////////////////////////////
#if 0

typedef struct DVD_BUTTON_QUEUE
{
	DVD_BUTTON_QUEUE *next;
	int i;
	int bn;
	int mode;
	LONGLONG pts;
} DVD_BUTTON_QUEUE;

DVD_BUTTON_QUEUE buttons[max_buttons];
DVD_BUTTON_QUEUE *butqueue_first = NULL, *butqueue_last = NULL;

void dvd_init_buttons_queue()
{
	for (int i = 0; i < max_buttons; i++)
		buttons[i].i = -1;
	butqueue_first = NULL;
	butqueue_last = NULL;
}

/// Adds button to the queue. 
/// \TODO: Optimise!
void dvd_add_button(int bn, int mode, LONGLONG pts)
{
	DVD_BUTTON_QUEUE *newbut = buttons;
	for (int i = 0; i < max_buttons; i++, newbut++)
	{
		if (newbut->i == -1)
		{
			if (butqueue_last == NULL)
				butqueue_first = newbut;
			else
				butqueue_last->next = newbut;
			butqueue_last = newbut;
			newbut->i = i;
			newbut->bn = bn;
			newbut->mode = mode;
			newbut->pts = pts;
			newbut->next = NULL;
			break;
		}
	}
}

void dvd_update_buttons(LONGLONG curpts)
{
	/*if (dvd_cur_button == -1)
	{
		dvd_clearhighlights();
		return;
	}
	*/
	DVD_BUTTON_QUEUE *cur = butqueue_first, *prev = NULL;
	while (cur != NULL)
	{
		if (cur->pts <= curpts)
		{
			dvd_update_button(cur->bn, cur->mode);
			// remove button from the queue
			cur->i = -1;
			if (prev == NULL)
				butqueue_first = cur->next;
			else
				prev->next = cur->next;
			if (cur == butqueue_last)
				butqueue_last = prev;
		} else
			prev = cur;
		cur = cur->next;
	}
}

void dvd_print_buttons_queue()
{
	MSG("DVD: Buttons queue:\n");
	DVD_BUTTON_QUEUE *cur = butqueue_first;
	while (cur != NULL)
	{
		MSG("DVD:  but=%d (%d) pts=%d\n", cur->bn, cur->i, cur->pts);
		cur = cur->next;
	}
}
#endif

/////////////////////////////////////////////////////////////////////////

void dvd_invalid()
{
	script_error_callback(SCRIPT_ERROR_INVALID);
}

void dvd_setdebug(BOOL ison)
{
	dvd_msg = ison == TRUE;
}

BOOL dvd_getdebug()
{
	return dvd_msg;
}

void dvd_fip_init()
{
	// write dvd-specific FIP stuff...
	fip_write_special(FIP_SPECIAL_DVD, 1);
	const char *digits = "  00000";
	fip_write_string(digits);
	fip_write_special(FIP_SPECIAL_COLON1, 1);
	fip_write_special(FIP_SPECIAL_COLON2, 1);
}

///////////////////////////////////////////////////////////

int dvd_open(const char *path)
{	
	if (dvdnav_open(&dvd, path) != DVDNAV_STATUS_OK)
	{
		msg_error("Media: Couldn't open DVD: %s\n", path);
		return -1;
	}

	if (dvdnav_set_readahead_flag(dvd, 1) != DVDNAV_STATUS_OK) 
	{
    	msg_error("Media: Error on set_readahead_flag: %s\n", dvdnav_err_to_string(dvd));
    	return -2;
  	}

  	// set the languages
  	if (dvdnav_menu_language_select(dvd, dvd_getdeflang_menu()) != DVDNAV_STATUS_OK 
		|| dvdnav_audio_language_select(dvd, dvd_getdeflang_audio()) != DVDNAV_STATUS_OK 
	  || dvdnav_spu_language_select(dvd, dvd_getdeflang_spu()) != DVDNAV_STATUS_OK) 
	{
    	msg_error("Media: Error on setting languages: %s\n", dvdnav_err_to_string(dvd));
    	return -3;
  	}

  	// set the PGC positioning flag to have position information relatively to the
   	// whole feature instead of just relatively to the current chapter
  	if (dvdnav_set_PGC_positioning_flag(dvd, 1) != DVDNAV_STATUS_OK) 
  	{
    	msg_error("Media: Error on set_PGC_positioning_flag: %s\n", dvdnav_err_to_string(dvd));
		return -4;
  	}
	dvdnav_set_cache_memory(dvd, mpeg_getbufbase());
	
	dvd_make_crc_table();
	dvd_was_saved = false;
	dvd_saved_pos = 0xffffffff;
	dvd_saved_title = 0xffffffff;
	dvd_ID = dvd_get_disc_ID(dvd_play_from_drive ? NULL : path);
	dvd_get_saved();
	script_player_saved_callback();
	
	return 0;
}

int dvd_close()
{
	if (dvdnav_close(dvd) != DVDNAV_STATUS_OK) 
		return -1;
	return 0;
}

int dvd_reset()
{
	if (dvdnav_reset(dvd) != DVDNAV_STATUS_OK) 
		return -1;
	return 0;
}

//////////////////////////////////////////////////////////////////

int dvd_play(const char *dvdpath, bool play_from_drive)
{
	fip_clear();
	fip_write_string("LoAd");

	cur_titleid = 0;
    cur_chapid = 0;
	cur_titlpos = 0;
	
	old_titleid = 0;
    old_chapid = 0;

	cur_cell_pts = 0; cur_cell_length = 0; cur_vobu_pts = 0;
	old_cell_pts = 0;
	cur_pgc_length = 0;
	old_vob_id = -1;
	old_vob_cell_id = -1;
	vts_changed = false;

	dvd_not_played_yet = true;
	dvd_waiting_to_play = false;

	dvd_mv_flags = -1;

	dvd = NULL;
	cur_pci = NULL;
	cur_dsi = NULL;
	dvd_read_errors = 0;
	no_spu = false;

	dvd_play_from_drive = play_from_drive;

	MEDIA_TYPES mt = MEDIA_TYPE_DVD;

    // Open the disc or file.
	int ret = media_open(dvdpath, mt);
	if (ret < 0)
	{
		MSG_ERROR("DVD: media open FAILED.\n");
		return ret;
	}

	MSG("DVD: start...\n");

    khwl_stop();
	khwl_display_clear();

	mpeg_init(MPEG_2, FALSE, FALSE, FALSE);
	mpeg_setbuffer(MPEG_BUFFER_1, mpeg_getbufbase(), 8, 32768);

///////////////////////////////////////////////////

	MPEG_PACKET_LENGTH = mpeg_getpacketlength();

	num_packets = 0;
    
	pts_base = 0;
	dvd_vobu_start_ptm = 0;
	dvd_vobu_start_pos = 0;
	need_to_set_pts = false;
	need_to_check_pts = false;
	saved_pts = 0;
	vobu_sptm = 0;

	wait_for_user = false;
	wait_pause = 0;
	wait_time = 0;
	wait_iter = 0;

	need_skip = false;
	can_skip = false;
	was_video = false;
	num_skip = 0;
	dvd_menu_ahead = false;
	need_user_reset = false;

	dvd_scan = false;

	dvd_seamless = true;
	dvd_speed = MPEG_SPEED_NORMAL;

	spu_command = 0;

	dvd_spu_enable(true);

	dvd_buttons_enable(false);

	dvd_spu_channel_letterbox = -1;
	dvd_spu_channel_panscan = -1;
	dvd_spu_stream = -1; dvd_old_spu_stream = -1;
	dvd_aud_stream = -1; dvd_old_aud_stream = -1;

	cur_num_buttons = 0;
	need_to_check_buttons = true;

	new_frame_size = false;
	dvd_video_info_cnt = 0;
	dvd_lastbtnupdate = false;

	forced_spu = -1;
	forced_spu_lang = 0xffff;
	forced_audio = -1;
	forced_audio_lang = 0xffff;

///////////////////////////////////////////
	SPSafeFree(dvdlang_lut);
	dvdlang_lut = (BYTE *)SPcalloc(65536);
	for (int i = 0; i < 256; i++)
	{
		dvdlang_lut[dvdlangs[i].code] = (BYTE)i;
		if (dvdlangs[i].code == 0xffff)
			break;
	}
	
	MSG("DVD: Setting default audio params.\n");
	MpegAudioPacketInfo defaudioparams;
	defaudioparams.type = eAudioFormat_AC3;
	defaudioparams.samplerate = 48000;
	defaudioparams.fromstream = 0;
	mpeg_setaudioparams(&defaudioparams);

	dvd_use_mv = settings_get(SETTING_DVD_MV) != 0;

	dvd_fip_init();
	fip_write_special(FIP_SPECIAL_PLAY, 1);
	fip_write_special(FIP_SPECIAL_PAUSE, 0);

	script_audio_info_callback("");
	script_video_info_callback("");

	khwl_set_window_zoom(KHWL_ZOOMMODE_DVD);

    return 0;
}

BOOL dvd_pause()
{
	dvd_button_pause();
	return TRUE;
}

void dvd_clearhighlights()
{
	MSG("DVD: Clear highlights...\n");
	if (!no_spu)
	{
		static KHWL_SPU_BUTTON_TYPE but;
		memset(&but, 0, sizeof(but));
		khwl_setproperty(KHWL_SUBPICTURE_SET, eSubpictureUpdateButton, sizeof(but), &but);
		last_upd_but = but;
	}
#if 0
	dvd_init_buttons_queue();
#endif
	dvd_button_selected = false;
}

void dvd_spu_enable(bool onoff)
{
	if (!no_spu)
	{
		if (onoff)
			spu_command |= KHWL_SPU_ENABLE;
		else
			spu_command &= ~(KHWL_SPU_ENABLE);
		khwl_setproperty(KHWL_SUBPICTURE_SET, eSubpictureCmd, sizeof(spu_command), &spu_command);
	}
}

void dvd_buttons_enable(bool onoff)
{
	if (!no_spu)
	{
		if (onoff)
		{
			spu_command |= (KHWL_SPU_ENABLE | KHWL_SPU_BUTTONS_ENABLE);
		} else
		{
			spu_command &= ~(KHWL_SPU_BUTTONS_ENABLE);
			dvd_clearhighlights();
		}
		khwl_setproperty(KHWL_SUBPICTURE_SET, eSubpictureCmd, sizeof(spu_command), &spu_command);

		dvd_buttons_enabled = onoff;

		MSG("DVD: SPU buttons: %s\n", onoff ? "enable" : "disable");
	}
}

void dvd_update_button(int bn, int mode)
{
	/// we handle different aspect ratios (like dxr3 does)
	if (cur_pci != NULL)
	{
		if (bn > 0 && bn <= (int)cur_pci->hli.hl_gi.btn_ns)
		{
			dvd_button_selected = true;
			
			btni_t *button_ptr = NULL;
			int b1 = cur_pci->hli.hl_gi.btngr1_dsp_ty;
			int b2 = cur_pci->hli.hl_gi.btngr2_dsp_ty;
			int b3 = cur_pci->hli.hl_gi.btngr3_dsp_ty;
			MSG("DVD: * button%d: gr1=%d gr2=%d gr3=%d (ngr=%d)\n", bn, b1, b2, b3, cur_pci->hli.hl_gi.btngr_ns);
			
			DWORD gr = 6, gr_res = 0;
			// use a letterbox button group for letterboxed anamorphic menus on tv out
			if (dvd_vmode == KHWL_VIDEOMODE_LETTERBOX && dvd_spu_channel_letterbox >= 0)
			{
				gr = 2;
				gr_res = 2;
			}
			if (dvd_vmode == KHWL_VIDEOMODE_PANSCAN && dvd_spu_channel_panscan >= 0)
			{
				gr = 4;
				gr_res = 4;
			}
			// (otherwise use a normal 4:3 or widescreen button group)
			DWORD i, btns_per_group = cur_pci->hli.hl_gi.btngr_ns == 0 ? 0 : 36 / cur_pci->hli.hl_gi.btngr_ns;
			for (i = 0; button_ptr == NULL && i < cur_pci->hli.hl_gi.btngr_ns; i++)
			{
				int dsp_ty = 0;
				switch (i)
				{
				case 0:
					dsp_ty = cur_pci->hli.hl_gi.btngr1_dsp_ty;
					break;
				case 1:
					dsp_ty = cur_pci->hli.hl_gi.btngr2_dsp_ty;
					break;
				case 2:
					dsp_ty = cur_pci->hli.hl_gi.btngr3_dsp_ty;
					break;
				}
				if ((dsp_ty & gr) == gr_res)
					button_ptr = &cur_pci->hli.btnit[i * btns_per_group + bn - 1];
			}
			bool needs_scale = false;
			if (button_ptr == NULL)		// this is really bad - we aren't sure that our scaling is good! :-(
			{
				button_ptr = &cur_pci->hli.btnit[bn - 1];
				needs_scale = true;
				MSG("DVD: * Using our scaling...\n");
			} else
			{
				MSG("DVD: * Button group %d matched!\n", i);
			}

			/*if (dvdnav_get_highlight_area(cur_pci, bn, mode, &hl) == DVDNAV_STATUS_OK)
			*/
			if (button_ptr != NULL)
			{
				KHWL_SPU_BUTTON_TYPE but;
				but.left = button_ptr->x_start;
				but.top = button_ptr->y_start;
				but.right = button_ptr->x_end+1;
				but.bottom = button_ptr->y_end+1;
				if (needs_scale)
				{
					int w, h;
					mpeg_getframesize(&w, &h);
					khwl_transformcoord(dvd_vmode, &but.left, &but.top, w, h);
					khwl_transformcoord(dvd_vmode, &but.right, &but.bottom, w, h);
				}

				int pal = 0;
				if (button_ptr->btn_coln > 0 && mode > 0)
					pal = cur_pci->hli.btn_colit.btn_coli[button_ptr->btn_coln-1][mode - 1];
				but.color = pal >> 16;
				but.contrast = pal & 0xffff;
				if (!no_spu)
				{
					khwl_setproperty(KHWL_SUBPICTURE_SET, eSubpictureUpdateButton, sizeof(but), &but);
					last_upd_but = but;
					dvd_lastbtnupdate = true;
				}
				
				MSG("DVD: * update_button (%d %d %d %d, %08x)\n", but.left, but.top, but.right, but.bottom, pal);
			}
		}
	}
}



void dvd_getcharlang(BYTE *strlang, WORD lang)
{
	strlang[0] = (BYTE)((lang >> 8) & 0xff);
	if (strlang[0] <= 32) 
		strlang[0] = '?';
	else if (strlang[0] == 0xff)
		strlang[0] = '-';
	strlang[1] = (BYTE)(lang & 0xff);
	if (strlang[1] <= 32) 
		strlang[1] = '?';
	else if (strlang[1] == 0xff)
		strlang[1] = '-';
	strlang[2] = '\0';
}

void dvd_getstringlang(BYTE *strlang, WORD lang)
{
	strlang[0] = (BYTE)((lang >> 8) & 0xff);
	if (strlang[0] <= 32) 
		strlang[0] = '?';
	else if (strlang[0] == 0xff)
		strlang[0] = '-';
	strlang[1] = (BYTE)(lang & 0xff);
	if (strlang[1] <= 32) 
		strlang[1] = '?';
	else if (strlang[1] == 0xff)
		strlang[1] = '-';
	strlang[2] = '\0';
}

WORD dvd_getintlang(BYTE *strlang)
{
	if (strlang[0] <= 32 || strlang[1] <= 32)
		return 0xffff;
	WORD lang = (WORD)((tolower(strlang[0]) << 8) | tolower(strlang[1]));
	return lang;
}

void dvd_setdeflang_menu(char *l)
{
	deflang_menu = dvd_getintlang((BYTE *)l);
}
char *dvd_getdeflang_menu()
{
	static char l[3];
	dvd_getcharlang((BYTE *)l, deflang_menu);
	return l;
}
void dvd_setdeflang_audio(char *l)
{
	deflang_audio = dvd_getintlang((BYTE *)l);
}

char *dvd_getdeflang_audio()
{
	static char l[3];
	dvd_getcharlang((BYTE *)l, deflang_audio);
	return l;
}
void dvd_setdeflang_spu(char * l)
{
	deflang_spu = dvd_getintlang((BYTE *)l);
}
char *dvd_getdeflang_spu()
{
	static char l[3];
	dvd_getcharlang((BYTE *)l, deflang_spu);
	return l;
}

//////////////////////////////////////////////////

int dvd_get_spu_mode(KHWL_VIDEOMODE vmode)
{
	if (vmode == KHWL_VIDEOMODE_LETTERBOX)
		return 1;
	else if (vmode == KHWL_VIDEOMODE_PANSCAN)
		return 2;
	// wide
	return 0;
}

int dvd_get_spu_stream_from_user(int stream_id, WORD lang)
{
	if (stream_id < 0 || lang == 0xffff)
		return -1;
	// first, test saved stream id
	int logstream = dvd_get_spu_logical_stream(dvd, (BYTE)stream_id, dvd_get_spu_mode(dvd_vmode));
	WORD slang = logstream >= 0 ? dvdnav_spu_stream_to_lang(dvd, (BYTE)logstream) : (WORD)0xffff;
	if (logstream >= 0 && slang == lang)
		return stream_id;
	// it seems that the streams were changed - find by language
	for (int stream = 0; stream < 32; stream++)
	{
		logstream = dvd_get_spu_logical_stream(dvd, (BYTE)stream, dvd_get_spu_mode(dvd_vmode));
		slang = logstream >= 0 ? dvdnav_spu_stream_to_lang(dvd, (BYTE)logstream) : (WORD)0xffff;
		if (logstream >= 0 && slang == lang)
			return stream;
	}
	return -1;
}

int dvd_get_audio_stream_from_user(int stream_id, WORD lang)
{
	if (stream_id < 0 || lang == 0xffff)
		return -1;
	// first, test saved stream id
	int logstream = dvd_get_audio_logical_stream(dvd, (BYTE)stream_id);
	WORD slang = logstream >= 0 ? dvdnav_audio_stream_to_lang(dvd, (BYTE)logstream) : (WORD)0xffff;
	if (logstream >= 0 && slang == lang)
		return stream_id;
	// it seems that the streams were changed - find by language
	for (int stream = 0; stream < 8; stream++)
	{
		logstream = dvd_get_audio_logical_stream(dvd, (BYTE)stream);
		slang = logstream >= 0 ? dvdnav_audio_stream_to_lang(dvd, (BYTE)logstream) : (WORD)0xffff;
		if (logstream >= 0 && slang == lang)
			return stream;
	}
	return -1;
}

BOOL dvd_setspeed(MPEG_SPEED_TYPE speed)
{
	if (dvd_speed == speed && speed != MPEG_SPEED_STEP)
	{
		MSG("DVD: Speed %d already set!\n", speed);
		return FALSE;
	}

	if (!mpeg_setspeed(speed))
		return FALSE;

	dvd_speed = speed;

	need_skip = false;
	num_skip = 0;

	// restore pts
	if (speed == MPEG_SPEED_NORMAL)
	{
		need_to_set_pts = true;
	} else
	{
		dvd_clearhighlights();
		need_user_reset = true;
	}

	// set 'fip' symbols
	if (dvd_speed == MPEG_SPEED_PAUSE)
	{
		fip_write_special(FIP_SPECIAL_PLAY, 0);
		fip_write_special(FIP_SPECIAL_PAUSE, 1);
	}
	else if (dvd_speed == MPEG_SPEED_STEP)
	{
		fip_write_special(FIP_SPECIAL_PLAY, 1);
		fip_write_special(FIP_SPECIAL_PAUSE, 1);
	}
	else if (dvd_speed != MPEG_SPEED_STOP)
	{
		fip_write_special(FIP_SPECIAL_PLAY, 1);
		fip_write_special(FIP_SPECIAL_PAUSE, 0);
	}

	if ((dvd_speed & MPEG_SPEED_FAST_FWD_MASK) == MPEG_SPEED_FAST_FWD_MASK
		|| (dvd_speed & MPEG_SPEED_FAST_REV_MASK) == MPEG_SPEED_FAST_REV_MASK)
	{
		need_skip = true;
		dvdnav_set_readahead_flag(dvd, 0);
	} else
	{
		dvdnav_set_readahead_flag(dvd, 1);
	}

	MSG("DVD: Speed set to %d\n", dvd_speed);
	
	return TRUE;
}

MPEG_SPEED_TYPE dvd_getspeed()
{
	return dvd_speed;
}

void dvd_user_reset()
{	
	if (dvd_speed != MPEG_SPEED_NORMAL)
	{
		MSG("DVD: Resetting speed to normal.\n");
		dvd_setspeed(MPEG_SPEED_NORMAL);
		script_speed_callback();
	}
	if (mpeg_zoom_reset())
	{
		MSG("DVD: Reset zoom&scroll to 0.\n");
		script_zoom_scroll_reset_callback();
	}
	need_user_reset = false;
}

#define VALID_XWDA(offset) \
		(((offset) & SRI_END_OF_CELL) != SRI_END_OF_CELL && \
		((offset) & 0x80000000))

BOOL dvd_skip(dsi_t *dsi)
{
	/*
	   float stime[19] = { 
	    120,	//  0 | 18
	     60,    //  1 | 17
		 30,    //  2 | 16
		 10,    //  3 | 15
		7.5,    //  4 | 14
		  7,    //  5 | 13
		6.5,    //  6 | 12
		  6,    //  7 | 11
		5.5,    //  8 | 10
		  5,    //  9 |  9
		4.5,    // 10 |  8
		  4,    // 11 |  7
		3.5,    // 12 |  6
		  3,    // 13 |  5
		2.5,    // 14 |  4
		  2,    // 15 |  3
		1.5,    // 16 |  2
		  1,    // 17 |  1
		0.5     // 18 |  0
	};
  	 */
	
	static int saved_bl = 0;
	static int lastdelta = -16;
	int skip = 0;
	switch (dvd_speed)
	{
	case MPEG_SPEED_NORMAL:
		saved_bl = 0;
		return TRUE;
	case MPEG_SPEED_FWD_8X:	// 1
		skip = 18-dvd_skipoffs;
		break;
	case MPEG_SPEED_FWD_16X: // 2
		skip = 18-1-dvd_skipoffs;
		break;
	case MPEG_SPEED_FWD_32X: // 4
		skip = 18-3-dvd_skipoffs;
		break;
	case MPEG_SPEED_FWD_48X: // 6
		skip = 18-5-dvd_skipoffs;
		break;
	case MPEG_SPEED_REV_8X:
		skip = 2+dvd_skipoffs;
		break;
	case MPEG_SPEED_REV_16X:
		skip = 3+dvd_skipoffs;
		break;
	case MPEG_SPEED_REV_32X:
		skip = 5+dvd_skipoffs;
		break;
	case MPEG_SPEED_REV_48X:
		skip = 7+dvd_skipoffs;
		break;
	default:
		return TRUE;
	}

	int numbl = 0, lenbl = 0, delta = 0;
	dvdnav_get_position(dvd, (unsigned int *)&numbl, (unsigned int *)&lenbl);
	if (saved_bl != 0)
	{
		numbl = saved_bl;
		//numbl -= dsi->dsi_gi.vobu_ea;
		saved_bl = 0;
	} else
	{
		if ((dvd_speed & MPEG_SPEED_FAST_FWD_MASK) == MPEG_SPEED_FAST_FWD_MASK)
		{
			while (skip <= 18 && !VALID_XWDA(dsi->vobu_sri.fwda[skip]))
				skip++;
			if (skip <= 18)
				delta = (int)(dsi->vobu_sri.fwda[skip] & SRI_END_OF_CELL);
			if (VALID_XWDA(dsi->vobu_sri.next_video))
			{
				int nv = (int)(dsi->vobu_sri.next_video & SRI_END_OF_CELL);
				if (delta < nv)
					delta = nv;
			}
		}
		else if ((dvd_speed & MPEG_SPEED_FAST_REV_MASK) == MPEG_SPEED_FAST_REV_MASK)
		{
			while (skip >= 0 && !VALID_XWDA(dsi->vobu_sri.bwda[skip]))
				skip--;
			if (skip >= 0)
				delta = -(int)(dsi->vobu_sri.bwda[skip] & SRI_END_OF_CELL);
			lastdelta = delta != 0 ? delta : -16;
		}

		numbl += delta;
		//if (delta < 0 && skip == 0)
		if (delta <= 0 && skip <= 0)
		{
			numbl += lastdelta;		// TODO: is it correct?
			saved_bl = numbl;
			dvdnav_prev_pg_search(dvd);
			MSG("DVD: jump_prev_page!\n");
			return TRUE;
			//dvdnav_get_position(dvd, (unsigned int *)&newnumbl, (unsigned int *)&lenbl);
			//numbl += newnumbl + lenbl;
		} else
			saved_bl = 0;

		can_skip = false;
		was_video = false;
	}
	if (numbl >= lenbl)
	{
		can_skip = false;
		was_video = false;
		return FALSE;
	}
	
	MSG("DVD: (skip=%2d  numbl=%8d)\n", skip, numbl);

	if (dvdnav_sector_search(dvd, numbl, SEEK_SET) != DVDNAV_STATUS_OK) 
		return FALSE;
	dvd_seamless = false;

	return TRUE;
}

void dvd_update_info()
{
	static int old_secs = 0;
	unsigned int pos, len;
	KHWL_TIME_TYPE displ;
	displ.pts = 0;
	displ.timeres = 90000;

	if (mpeg_is_displayed())
		khwl_getproperty(KHWL_TIME_SET, etimVideoFrameDisplayedTime, sizeof(displ), &displ);

#if 0
	dvd_update_buttons(displ.pts);
#endif
	if (old_titleid != cur_titleid || old_chapid != cur_chapid || (LONGLONG)displ.pts != saved_pts)
	{
		if (cur_titleid >= 0 && cur_chapid >= 0 && (LONGLONG)displ.pts >= 0)
		{
			if (cur_titleid != old_titleid)
			{
				uint64_t tim;
				dvd_get_time(dvd, -1, -1, &tim);
				int titlelength = (int)(tim / 90000);
				script_totaltime_callback(titlelength);

				int numchapters = 0;
				dvdnav_get_number_of_parts(dvd, cur_titleid, &numchapters);
				script_dvd_title_callback(cur_titleid, numchapters);
			}
			if (cur_chapid != old_chapid)
			{
				script_dvd_chapter_callback(cur_chapid);
			}

			old_titleid = cur_titleid;
			old_chapid = cur_chapid;
			saved_pts = displ.pts;

			dvdnav_get_position_in_title(dvd, &pos, &len);
			dvd_vobu_start_pos = pos;
			MSG("DVD: -- Title %d, Chapter %d, %d/%d --\n", cur_titleid, cur_chapid, pos, len);

			if (len != 0)
				cur_titlpos = 100 * pos / len;

			char fip_out[10];
			int secs = (int)(displ.pts / 90000);
			//int secs = (int)((displ.pts + old_cell_pts - pts_base) / 90000);
			int ccid = cur_chapid;
			if (ccid < 0)
				ccid = 0;
			if (ccid > 99)
				ccid = 99;
			if (secs < 0)
				secs = 0;
			if (secs >= 10*3600)
				secs = 10*3600-1;
			if (secs != old_secs)
			{
				script_time_callback(secs);
				old_secs = secs;
			}
			fip_out[0] = (char)((ccid / 10) + '0');
			fip_out[1] = (char)((ccid % 10) + '0');
			fip_out[2] = (char)((secs/3600) + '0');
			int secs3600 = secs%3600;
			fip_out[3] = (char)(((secs3600/60)/10) + '0');
			fip_out[4] = (char)(((secs3600/60)%10) + '0');
			fip_out[5] = (char)(((secs3600%60)/10) + '0');
			fip_out[6] = (char)(((secs3600%60)%10) + '0');
			fip_out[7] = '\0';
			/*
			sprintf(fip_out, "%02d%1d%02d%02d", ccid, secs/3600, (secs%3600)/60,
				(secs%3600)%60);
			*/
			fip_write_string(fip_out);
		}
	}
}

void dvd_update_audio_info(BYTE *buf, int len)
{
	const char *afmt[] = { "Unknown", "MPEG-L1", "MPEG-L2", "Dolby AC3", "PCM", "DTS" };
	MpegAudioPacketInfo apinfo = mpeg_getaudioparams();
	SPString info, subinfo;
	if (apinfo.type > 5)
		apinfo.type = eAudioFormat_UNKNOWN;
	info.Printf("%s", afmt[apinfo.type]);
	int lfe = 0, bitrate = 0;
	if (apinfo.type != eAudioFormat_UNKNOWN)
	{
		if (apinfo.type == eAudioFormat_AC3)
		{
			apinfo.samplerate = 0;
			apinfo.numberofchannels = 0;
			mpeg_parse_ac3_header(buf, len, &bitrate, NULL/*&apinfo.samplerate*/, 
					&apinfo.numberofchannels, &lfe, NULL);
		}
		if (bitrate > 0)
		{
			subinfo.Printf("%d kbps ", (bitrate + 500) / 1000);
		}
		if (apinfo.samplerate > 0)
		{
			subinfo.Printf("%d Hz ", apinfo.samplerate);
		}
		if (apinfo.numberofbitspersample > 0)
		{
			subinfo.Printf("%d bits ", apinfo.numberofbitspersample,
				apinfo.numberofchannels == 1 ? "mono" : "stereo");
		}
		if (apinfo.numberofchannels > 0)
		{
			if (apinfo.numberofchannels > 2)
				subinfo.Printf("%d.%d", apinfo.numberofchannels, lfe);
			else
				subinfo.Printf("%s", apinfo.numberofchannels == 1 ? "mono" : "stereo");
		}
		if (subinfo != "")
			info = info + " @ " + subinfo;
	}

	script_audio_info_callback(info);
}

void dvd_update_video_info()
{
	const char *vfmt[] = { "Unknown", "MPEG-1", "MPEG-2", "", "MPEG-4" };
	SPString info;
	int vf = mpeg_get_video_format();
	if (vf > 3)
		vf = 0;
	
	info.Printf("%s", vfmt[vf]);
	
	int rate = (8*mpeg_getrate(FALSE) + 500) / 1000;
	if (rate > 0)
		info.Printf(" @ %d kbps", rate);
	
	script_video_info_callback(info);
}

void dvd_get_cur(int *title, int *chapter)
{
	*title = cur_titleid;
	*chapter = cur_chapid;
}

int dvd_player_loop_internal(bool feed)
{     
	int feed_cnt = 0;
get_some_more:

	bool was_feed = false;

	// if user paused before play actually started...
	if (dvd_waiting_to_play)
	{
		dvd_update_info();
		return 0;
	}

	dvd_not_played_yet = false;

	if (wait_for_user || wait_pause > 0)
	{
		// if no more packets
		if (mpeg_wait(TRUE))
		{
			// update last button highlight 
			// Some new SPU data could be sent since the last highlight event
			// (or resolution/ratio could be changed...)
			if (!no_spu && last_upd_but.right != 0 && last_upd_but.bottom != 0)
			{
				if (dvd_lastbtnupdate)
				{
					MSG("DVD: Last button update...\n");
					dvd_lastbtnupdate = false;
				}

				khwl_setproperty(KHWL_SUBPICTURE_SET, eSubpictureUpdateButton, sizeof(last_upd_but), &last_upd_but);

				// update 1 time
				//last_upd_but.right = 0;
			}
		}
		dvd_update_info();
	}
	// wait for user commands
	if (wait_for_user)
		return 0;

	if (wait_pause > 0)
	{
		const int wait_delay = 300;
		int sl = wait_pause < wait_delay ? wait_pause * 1000 : wait_delay * 1000;
		usleep(sl);
		// now get current 'time'
		ULONGLONG newt = clock() * 1000 / CLOCKS_PER_SEC;
		
		//wait_pause -= (int)(newt - wait_time);
		wait_pause -= 300;
		
		wait_time = newt;
		wait_iter++;
		if (wait_pause <= 0)	// at last!
		{
			wait_pause = 0;
			dvdnav_still_skip(dvd);
			MSG("DVD: ...skipped: %d cycles.\n", wait_iter);
		} else
			return 0;
	}

	BYTE *buf = NULL;
	
	MEDIA_EVENT event = MEDIA_EVENT_OK;
	int len = 0;

	int ret = media_get_next_block(&buf, &event, &len);
	if (ret > 0 && buf == NULL)
	{
		MSG_ERROR("DVD: Not initialized. STOP!\n");
		return 1;
	}
	BYTE *base = buf;

	if (ret == -1)
	{
		if (need_skip)
		{
			// perhaps, there was a NAV packet error, so just skip a sector...
			if (dvd_skip_sector(dvd) == 0)
				return 0;
		}
      	dvd_read_errors++;
		MSG_ERROR("DVD: Error getting next block (tries=%d): %s\n", dvd_read_errors, media_geterror());
		need_to_set_pts = true;
		if (dvd_read_errors > 3)
		{
			dvd_read_errors = 0;
			if (dvdnav_current_title_info(dvd, &cur_titleid, &cur_chapid) && cur_titleid > 0)
			{
				dvdnav_part_play(dvd, cur_titleid, ++cur_chapid);
				MSG_ERROR("DVD: Too much errors! Skip to next part...\n");
				int next_cur_titleid, next_cur_chapid;
				if (!dvdnav_current_title_info(dvd, &next_cur_titleid, &next_cur_chapid))
					next_cur_chapid = -1;
				if (cur_chapid != next_cur_chapid)
				{
					MSG_ERROR("DVD: Cannot skip to next part. Stop playing...\n");
					return 1;
				}
				return 0;
			} else
			{
				MSG_ERROR("DVD: Too much errors! Stop playing...\n");
				return 1;
			}
		}
      	return -1;
    } else
		if (ret == 0)		// wait...
			return 0;
	if (dvd_read_errors > 0)
		dvd_read_errors--;

	if (dvd_scan)
		dvd_seamless = false;

#ifdef DVD_PACKETS_DEBUG
	MSG("DVD: ==packet %d (%08x)\n", event, base);
#endif

	switch ((int)event) 
	{
    case DVDNAV_BLOCK_OK:
		{
			// if started paused, don't fill buffer
			if (dvd_speed == MPEG_SPEED_PAUSE && num_packets == 0)
			{
				return 0;
			}

			START_CODE_TYPES sct;
			LONGLONG scr = 0;
			int cnt = 0;
			while ((sct = mpeg_findpacket(buf, base, cnt)) != START_CODE_END)
			{
				cnt = 0;
				bool datapacket = false;
				switch (sct)
				{
				case START_CODE_PACK:
					scr = mpeg_parse_program_stream_pack_header(buf, base);
					scr += pts_base;
					if (scr < 0)
						scr = 0;
					// this is needed for 'khwl'
					scr |= SPTM_SCR_FLAG;
					break;
				case START_CODE_SYSTEM:
				case START_CODE_PCI:
					{
					int len = (buf[0] << 8) | buf[1];
					if (!mpeg_incParsingBufIndex(buf, base, len + 2)) 
						goto brk;
					break;
					}
				case START_CODE_MPEG_VIDEO:
				case START_CODE_MPEG_AUDIO1:
				case START_CODE_MPEG_AUDIO2:
				case START_CODE_PRIVATE1:
					datapacket = true;
					break;
				default:;
				};

				if (datapacket)
				{
					if (dvd_scan)
						break;
					MpegPacket *packet = NULL;

					if (sct != START_CODE_MPEG_VIDEO && dvd_speed != MPEG_SPEED_NORMAL)
						break;
							
					packet = mpeg_feed_getlast();
					if (packet == NULL)		// well, it won't really help
						break;

					memset((BYTE *)packet + 4, 0, sizeof(MpegPacket) - 4);

					// delayed stream change for forced streams detection...
					if (sct == START_CODE_PRIVATE1 && dvd_spu_stream != dvd_old_spu_stream)
					{
						/*if (dvd_menu_ahead)		// force ">0x80" streams?
							dvd_spu_stream &= 0x1f;
						mpeg_setspustream(dvd_spu_stream);
						MSG("DVD: Set SPU stream: %d (%d)\n", dvd_spu_stream, dvd_old_spu_stream);
						dvd_old_spu_stream = dvd_spu_stream;
						*/
					}

					// delayed stream change for forced streams detection...
					/*
					if (sct != START_CODE_MPEG_VIDEO && dvd_aud_stream != dvd_old_aud_stream)
					{
						dvd_old_aud_stream = dvd_aud_stream;
						if (dvd_menu_ahead)		// force ">0x80" streams?
							dvd_aud_stream &= 0x1f;
						mpeg_setaudiostream(dvd_aud_stream);
						MSG("DVD: Set Audio stream: %d (%d)\n", dvd_aud_stream, dvd_old_aud_stream);
						dvd_old_aud_stream = dvd_aud_stream;
					}
					*/

					packet->pts = pts_base;
					if (mpeg_extractpacket(buf, base, packet, sct, FALSE) != 1)
						break;

					// sometimes, we get 'fake' packets that 'khwl' doesn't like 
					if (packet->size < 1)
						break;

					if (mpeg_audio_format_changed())
					{
						dvd_update_audio_info(buf, 7);
					}

					if (packet->flags == 2)
					{
						if (need_to_check_pts)
						{
							//if (mpeg_detect_pts_wrap(pts))
							//{
							//	need_to_set_pts = true;
							//	vobu_sptm = packet->pts;
							//}
						}

						if (new_frame_size)
						{
							int w, h;
							mpeg_getframesize(&w, &h);
							if (w != 0 && h != 0)
							{
								script_framesize_callback(w, h);
								new_frame_size = false;
							}
							int fps = mpeg_get_fps();
							if (fps > 0)
								script_framerate_callback(fps);
						}

						if (dvd_video_info_cnt++ > 63)
						{
							dvd_update_video_info();
							dvd_video_info_cnt = 0;
						}
					}

					packet->scr = scr;
					packet->vobu_sptm = (vobu_sptm + pts_base) | SPTM_SCR_FLAG;

					if (feed)
					{
						if (packet->flags == 2 && packet->type == 0)
						{
							//MSG("DVD: setpts(%d)\n", packet->pts);
							//mpeg_setpts(packet->pts);
#if 0
							LONGLONG stc = mpeg_getpts();
							LONGLONG good_scr = packet->pts;
							if (dvd_speed == MPEG_SPEED_NORMAL && stc > good_scr + dvd_good_delta_stc)
							{
#ifndef WIN32
								//khwl_pause();
								khwl_stop();
								//pts_base += stc - good_scr;
								mpeg_start();
								msg("DVD: Resync (%d > %d)...\n", (int)stc, (int)good_scr);
#endif
							}
#endif

							if (need_to_set_pts)
								need_to_set_pts = false;
						}

						// increase bufidx
						mpeg_setbufidx(MPEG_BUFFER_1, packet);
						

						if (packet->type == 0 && packet->pts != 0)
						{
#ifdef DVD_PTS_DEBUG
#ifdef WIN32
							MSG("DVD: %d (%d) %I64d = %I64d [%I64d]\n", 
#else
							MSG("DVD: %d (%d) %Ld = %Ld [%Ld]\n", 
#endif
							num_packets, packet->type, packet->pts - pts_base, 
							packet->pts, packet->scr & ~SPTM_SCR_FLAG);
#endif
							was_video = true;
						}

						// send to khwl
						mpeg_feed((MPEG_FEED_TYPE)packet->type);
						was_feed = true;

					} else
					{
						// skip
					}
					num_packets++;
					break;	// do not find any more packets in this block
				}
			}
brk:		;
		}
		break;
    case DVDNAV_NOP:
      // Nothing to do here.
      break;
    case DVDNAV_STILL_FRAME: 
      /* We have reached a still frame. A real player application would wait
       * the amount of time specified by the still's length while still handling
       * user input to make menus and other interactive stills work.
       * A length of 0xff means an indefinite still which has to be skipped
       * indirectly by some user interaction. */
      {
		if (feed)
		{
			dvdnav_still_event_t *still_event = (dvdnav_still_event_t *)buf;
			if (still_event->length < 0xff)
			{
				// we'll finish after the next few cycles
				wait_time = clock() * 1000 / CLOCKS_PER_SEC;
				wait_pause = still_event->length * 1000;
				wait_iter = 0;
				
				MSG("DVD: Waiting %d seconds of still frame\n", still_event->length);
			}
			else
			{
				wait_for_user = true;
				MSG("DVD: Indefinite length still frame - waiting for user interaction...\n");
			}
			dvd_seamless = false;

			if (dvd_speed != MPEG_SPEED_NORMAL)
			{
				dvd_setspeed(MPEG_SPEED_NORMAL);
				script_speed_callback();
			}
			mpeg_play();

		} else
			dvdnav_still_skip(dvd);
      }
      break;
    
    case DVDNAV_WAIT:
		/* We have reached a point in DVD playback, where timing is critical.
		 * Player application with internal fifos can introduce state
		 * inconsistencies, because libdvdnav is always the fifo's length
		 * ahead in the stream compared to what the application sees.
		 * Such applications should wait until their fifos are empty
		 * when they receive this type of event. */
		if (feed)
		{
			MSG("DVD: Skipping wait condition\n");
			mpeg_wait(FALSE);
			mpeg_start();

			MSG("DVD: Skipping DONE\n");
		}
		dvdnav_wait_skip(dvd);
		break;

    case DVDNAV_SPU_CLUT_CHANGE:
		if (feed)
		{
			if (dvd_speed == MPEG_SPEED_NORMAL)
			{
				MSG("DVD: CLUT change\n");
				dvd_clearhighlights();

				need_to_check_buttons = true;

				int *clut = (int *)buf;
				if (!no_spu)
				{
					for (int i = 0; i < 16; i++)
						clut[i] = bswap_32(clut[i]);

					khwl_setproperty(KHWL_SUBPICTURE_SET, eSubpictureUpdatePalette, sizeof(KHWL_SPU_PALETTE_ENTRY) * 16, buf);
				}
			}
		}
      break;
    
    case DVDNAV_SPU_STREAM_CHANGE:
      {
    	if (feed)
		{
			if (dvd_speed == MPEG_SPEED_NORMAL)
			{
				dvd_clearhighlights();


				need_to_check_buttons = true;

				dvdnav_spu_stream_change_event_t *stream_event = 
						(dvdnav_spu_stream_change_event_t *) buf;
				int stream;
				if (dvd_vmode == KHWL_VIDEOMODE_LETTERBOX)
				{
					stream = stream_event->physical_letterbox;
					dvd_spu_channel_letterbox = stream;
				}
				else if (dvd_vmode == KHWL_VIDEOMODE_PANSCAN)
				{
					stream = stream_event->physical_pan_scan;
					dvd_spu_channel_panscan = stream;
				}
				else
					stream = stream_event->physical_wide;
				bool in_menu = dvdnav_is_domain_vts(dvd) == 0;
				int logstream = (stream & 0x80) == 0 ? 
						dvd_get_spu_logical_stream(dvd, (BYTE)stream, dvd_get_spu_mode(dvd_vmode)) : -1;
				WORD lang = logstream >= 0 ? dvdnav_spu_stream_to_lang(dvd, (BYTE)logstream) : (WORD)0xffff;
				BYTE strlang[3];
				dvd_getcharlang(strlang, lang);
				MSG("DVD: SPU stream event: %d (%d %d %d), log=%d, lang='%c%c'\n", stream,
					stream_event->physical_wide, stream_event->physical_letterbox, stream_event->physical_pan_scan,
						logstream, strlang[0], strlang[1]);
				
				// reset saved pos if user entered menu
				if (in_menu)
				{
					dvd_savepos(true);
				}
			
				// \TODO: handle dvd_menu_ahead

				// drop current stream if not user-defined
				if (!in_menu && (stream & 0x80) == 0x80 && forced_spu == -1)
				{
					dvd_spu_stream = -1;
					lang = 0xffff;
					logstream = -1;
					MSG("DVD: - Dropping SPU stream to %d (was %d)\n", dvd_spu_stream, dvd_old_spu_stream);
				}
				// accept any stream in menu or only valid language streams otherwise
				else if ((/*logstream >= 0 && lang != 0xffff && */forced_spu == -1) || in_menu)
				{
					dvd_spu_stream = stream;
					MSG("DVD: - Set SPU stream: %d (was %d)\n", dvd_spu_stream, dvd_old_spu_stream);
/*					
					if (!in_menu)
					{
						forced_spu = -1;
						forced_spu_lang = 0xffff;
						MSG("DVD: - Resetting user's subtitle choice.\n");
					}
*/
				} 
				// set user-forced value in all other cases
				else
				{
					dvd_spu_stream = dvd_get_spu_stream_from_user(forced_spu, forced_spu_lang);
					
					logstream = dvd_get_spu_logical_stream(dvd, (BYTE)dvd_spu_stream, dvd_get_spu_mode(dvd_vmode));
					lang = logstream >= 0 ? dvdnav_spu_stream_to_lang(dvd, (BYTE)logstream) : (WORD)0xffff;
					dvd_getcharlang(strlang, lang);

					MSG("DVD: - User-ignoring SPU stream! (user set %d,'%c%c')\n", dvd_spu_stream, strlang[0], strlang[1]);
				}

				script_spu_stream_callback(logstream >= 0 ? logstream+1 : 0);
				script_spu_lang_callback(dvdlangs[dvdlang_lut[lang]].str);
				
				mpeg_setspustream(dvd_spu_stream);
				dvd_old_spu_stream = dvd_spu_stream;
			}
		}
      	break;
      }
    
    case DVDNAV_AUDIO_STREAM_CHANGE:
	  {
		if (feed)
		{
			if (dvd_speed == MPEG_SPEED_NORMAL)
			{
				dvdnav_audio_stream_change_event_t *stream_event = 
					(dvdnav_audio_stream_change_event_t *) buf;
				BYTE strlang[3];

				int stream = stream_event->physical;
				bool in_menu = dvdnav_is_domain_vts(dvd) == 0;
				int logstream = dvd_get_audio_logical_stream(dvd, (BYTE)stream);
				WORD lang = logstream >= 0 ? dvdnav_audio_stream_to_lang(dvd, (BYTE)logstream) : (WORD)0xffff;
				dvd_getcharlang(strlang, lang);

				MSG("DVD: Audio stream change event (%d), log=%d, lang='%c%c'\n", stream,
					logstream, strlang[0], strlang[1]);
				// we don't care about the language, we just need a valid logical stream.
				if (forced_audio == -1/*logstream >= 0*/ || in_menu)
				{
					dvd_aud_stream = stream;
					MSG("DVD: - Audio stream change: %d\n", stream);
/*
					if (!in_menu)
					{
						forced_audio = -1;
						forced_audio_lang = 0xffff;
						MSG("DVD: - Resetting user's audio choice.\n");
					}
*/
				}
				else
				{
					dvd_aud_stream = dvd_get_audio_stream_from_user(forced_audio, forced_audio_lang);

					logstream = dvd_get_audio_logical_stream(dvd, (BYTE)dvd_spu_stream);
					lang = logstream >= 0 ? dvdnav_audio_stream_to_lang(dvd, (BYTE)logstream) : (WORD)0xffff;
					dvd_getcharlang(strlang, lang);

					MSG("DVD: - User-ignoring audio stream! (user set %d,'%c%c')\n", dvd_aud_stream, strlang[0], strlang[1]);
				}

				script_audio_stream_callback(logstream >= 0 ? logstream+1 : 0);
				script_audio_lang_callback(dvdlangs[dvdlang_lut[lang]].str);

				mpeg_setaudiostream(dvd_aud_stream);
			}
		}
		break;
	  }
    
    case DVDNAV_HIGHLIGHT:
      {
      	if (feed)
      	{
			//dvdnav_highlight_area_t hl;
			dvdnav_highlight_event_t *highlight_event = (dvdnav_highlight_event_t *)buf;
			MSG("DVD: Selected button %d (pts=%d)\n", highlight_event->buttonN, (int)highlight_event->pts);

			//dvdnav_get_current_highlight(dvd, &dvd_cur_button);
			dvd_cur_button = highlight_event->buttonN;
			
			dvd_update_button(highlight_event->buttonN, highlight_event->display);
			/*dvd_add_button(highlight_event->buttonN, highlight_event->display, 
							(LONGLONG)(signed int)highlight_event->pts + pts_base);
			*/

			// try updating immediately?
			//if (mpeg_is_playing())
			//	dvd_update_info();
		}
		break;
      }
    
    case DVDNAV_VTS_CHANGE:
	  {
      	// Some status information like video aspect and video scale permissions do
       	// not change inside a VTS. Therefore this event can be used to query such
       	// information only when necessary and update the decoding/displaying accordingly.

		vts_changed = true;
	    if (feed)
		{
			int aspect = dvdnav_get_video_aspect(dvd);
			int permission = dvdnav_get_video_scale_permission(dvd);
			
			const char *asp[] = { "4:3", "? (#1)", "? (#2)", "16:9" };
			MSG("DVD: VTS change. Aspect = %s (Perm = %x)\n", asp[aspect], permission);

			if (dvd_speed == MPEG_SPEED_NORMAL)
				khwl_stop();
			
			dvd_buttons_enable(false);

			need_to_check_buttons = true;

			cur_titleid = 0;
			cur_chapid = 0;

			// change video mode
			int tvtype = settings_get(SETTING_TVTYPE);
			dvd_vmode = KHWL_VIDEOMODE_NORMAL;
			if (aspect == 0)
			{
				if (tvtype == 0 || tvtype == 1)
					dvd_vmode = KHWL_VIDEOMODE_NORMAL;
				else 
				{
					if ((permission & 3) == 0)
						dvd_vmode = (tvtype == 2) ? KHWL_VIDEOMODE_HCENTER : KHWL_VIDEOMODE_VCENTER;
					else if ((permission & 2) == 2)
						dvd_vmode = KHWL_VIDEOMODE_HCENTER;
					else 
						dvd_vmode = KHWL_VIDEOMODE_VCENTER;
				}
			} 
			else if (aspect == 3)
			{
				if (tvtype == 2 || tvtype == 3)
					dvd_vmode = KHWL_VIDEOMODE_WIDE;
				else 
				{
					if ((permission & 3) == 0)
						dvd_vmode = (tvtype == 0) ? KHWL_VIDEOMODE_LETTERBOX : KHWL_VIDEOMODE_PANSCAN;
					else if ((permission & 1) == 1)
						dvd_vmode = KHWL_VIDEOMODE_PANSCAN;
					else 
						dvd_vmode = KHWL_VIDEOMODE_LETTERBOX;
					
				}
			}
			MSG("DVD: set vmode = %d\n", dvd_vmode);

			khwl_display_clear();
			khwl_setvideomode(dvd_vmode, TRUE);
			new_frame_size = true;

			script_dvd_menu_callback(dvdnav_is_domain_vts(dvd) != 1);

			if (dvd_speed == MPEG_SPEED_NORMAL)
			{
				mpeg_resetstreams();
				mpeg_start();
			}
			dvd_seamless = false;
		}
		break;
	  }
    
    case DVDNAV_CELL_CHANGE:
      /* Some status information like the current Title and Part numbers do not
       * change inside a cell. Therefore this event can be used to query such
       * information only when necessary and update the decoding/displaying
       * accordingly. */
      {
      	dvdnav_cell_change_event_t *cell_change = (dvdnav_cell_change_event_t *)buf;

		int tt, ptt;
		if (dvdnav_current_title_info(dvd, &tt, &ptt) == DVDNAV_STATUS_ERR)
			break;
		//dvdnav_get_position(dvd, (uint32_t *)&pos, (uint32_t *)&len);
		MSG("DVD: Cell change event: (Title %d, Chapter %d)\n", tt, ptt);
		//MSG("DVD: Title %d, Chapter %d\n", cell_change->pgN, cell_change->cellN);
		MSG("DVD: - p_s=%d p_l=%d pgc_l=%d c_s=%d c_l=%d\n", (int)cell_change->pg_start, (int)cell_change->pg_length,
				(int)cell_change->pgc_length, (int)cell_change->cell_start, (int)cell_change->cell_length);
		
		cur_titleid = tt;//cell_change->pgN;
		cur_chapid = ptt;//cell_change->cellN;
		
		cur_cell_pts = cell_change->cell_start;
		cur_cell_length = cell_change->cell_length;
		cur_pgc_length = cell_change->pgc_length;

		if (feed)
      	{
			if (dvd_speed == MPEG_SPEED_NORMAL)
			{
				dvd_update_info();
				need_to_check_pts = true;

			}
			//MSG("DVD: At position %.0f%%%% inside the feature\n", 100 * (double)pos / (double)len);
		}
		break;	
      }
    
    case DVDNAV_NAV_PACKET:
      /* A NAV packet provides PTS discontinuity information, angle linking information and
       * button definitions for DVD menus. Angles are handled completely inside libdvdnav.
       * For the menus to work, the NAV packet information has to be passed to the overlay
       * engine of the player so that it knows the dimensions of the button areas. */
      {
		/* Applications with fifos should not use these functions to retrieve NAV packets,
		 * they should implement their own NAV handling, because the packet you get from these
		 * functions will already be ahead in the stream which can cause state inconsistencies.
		 * Applications with fifos should therefore pass the NAV packet through the fifo
		 * and decoding pipeline just like any other data. */

		cur_pci = dvdnav_get_current_nav_pci(dvd);
		cur_dsi = dvdnav_get_current_nav_dsi(dvd);

		if (was_video)
		{
			was_video = false;
			can_skip = true;
		}

		//if (!need_skip)
		{
			LONGLONG cell_pts = old_cell_pts;
			bool vob_changed = false;
			if (cur_dsi->dsi_gi.vobu_vob_idn != old_vob_id || 
				cur_dsi->dsi_gi.vobu_c_idn != old_vob_cell_id || vts_changed)
			{
				//if (!vts_changed)
				cell_pts = cur_cell_pts;
				
				// patch for weird DVDs...
				if (cur_pci->pci_gi.vobu_s_ptm > cell_pts)
					cell_pts = 0;

				if (cur_dsi->dsi_gi.vobu_vob_idn != old_vob_id)
				{
					MSG("DVD: ** VOB changed!\n");
					vob_changed = true;
				}
				
				old_vob_id = cur_dsi->dsi_gi.vobu_vob_idn;
				old_vob_cell_id = cur_dsi->dsi_gi.vobu_c_idn;
				MSG("DVD: ** VOBU: cell changed! newvts=%d\n", (int)vts_changed);
				MSG("DVD: * vob_id: %d, cell_id: %d\n", (int)cur_dsi->dsi_gi.vobu_vob_idn, 
						(int)cur_dsi->dsi_gi.vobu_c_idn);
				MSG("DVD: * start=%d, end=%d (old=%d), cell_pts=%d)\n", 
					(int)cur_pci->pci_gi.vobu_s_ptm, (int)cur_pci->pci_gi.vobu_e_ptm, 
					(int)dvd_vobu_last_ptm, (int)cur_cell_pts);
				MSG("DVD: * vob: %d, s=%d, e=%d\n", (int)cur_vobu_pts, (int)cur_dsi->sml_pbi.vob_v_s_s_ptm,
					(int)cur_dsi->sml_pbi.vob_v_e_e_ptm);
				
				if (cell_pts < pts_base)
				{
					if (!dvd_seamless && !need_skip && !dvd_scan && dvd_speed == MPEG_SPEED_NORMAL)
					{
						khwl_stop();
						mpeg_start();
					}
				}

				// see if we have more than 1 angle
				int ca, na = 1;
				dvdnav_get_angle_info(dvd, &ca, &na);
				if (na != number_of_angles)
				{
					MSG("DVD: Angles detected: %d!\n", na);

					fip_write_special(FIP_SPECIAL_CAMERA, na > 1 ? 1 : 0);
					
					// turn off read-ahead?
					if (na > 1)
					{
						MSG("DVD: * read-ahead = off\n");
						dvdnav_set_readahead_flag(dvd, 0);
					}
					else if (!need_skip)
					{
						MSG("DVD: * read-ahead = on\n");
						dvdnav_set_readahead_flag(dvd, 1);
					}
					
					number_of_angles = na;
				}

#if 0
				// save current position on cell boundary
				if (cur_vobu_pts != 0)
					dvd_savepos(false);
#endif
			}
			vts_changed = false;
			old_cell_pts = cell_pts;

			cur_vobu_pts = cur_pci->pci_gi.vobu_s_ptm + cell_pts;
			
			if (cur_pci->pci_gi.vobu_s_ptm != dvd_vobu_last_ptm)
			{
				vobu_sptm = cur_pci->pci_gi.vobu_s_ptm;
				LONGLONG delta = dvd_vobu_last_ptm - vobu_sptm;

				LONGLONG oldptsbase = pts_base;
				// don't change pts for multi-angle vobu changes
				if (dvd_seamless)
				{
					if (number_of_angles <= 1)
						pts_base += delta;

					MSG("DVD: * pts_base=%d (old=%d)\n", (int)pts_base, (int)oldptsbase);
				}
				else
				{
					if (delta != 0 && !need_skip && !dvd_scan && dvd_speed == MPEG_SPEED_NORMAL)
					{
						khwl_stop();
						mpeg_start();
						need_to_set_pts = true;
					}
					LONGLONG et = dvdnav_convert_time(&cur_pci->pci_gi.e_eltm); // &cur_dsi->dsi_gi.c_eltm

					pts_base = et + cur_cell_pts - vobu_sptm;

					MSG("DVD: Cell PTS = %d + %d, VOBU_s_ptm=%d.\n", 
								(int)et, (int)cur_cell_pts, (int)vobu_sptm);
					MSG("DVD: * pts_base = %d (old=%d)\n", (int)pts_base,
								(int)oldptsbase);
					
				}

				dvd_seamless = true;

#ifdef DVD_USE_MACROVISION
				// Macrovision APS
				int apstb = (cur_pci->pci_gi.vobu_cat >> 14) & 3;
				int mv_flags = (mv_cgms_flags << 2) | apstb;
				if (dvd_mv_flags != mv_flags)
				{
					if (dvd_use_mv)
					{
						MSG("DVD: Macrovision flag (%d) set.\n", mv_flags);
						khwl_setproperty(KHWL_VIDEO_SET, evMacrovisionFlags, sizeof(int), &mv_flags);
					} else
					{
						MSG("DVD: Macrovision flag (%d) SKIPPED.\n", mv_flags);
					}
					dvd_mv_flags = mv_flags;
				}
#endif				

				if (delta != 0)
				{
					/*need_to_set_pts = true;*/
				}
			}
			dvd_vobu_start_ptm = cur_pci->pci_gi.vobu_s_ptm;
			dvd_vobu_last_ptm = cur_pci->pci_gi.vobu_e_ptm;
			
		}

		if (need_skip && can_skip)
		{
			/////////// TODO: this is experimental!
#if 0
			KHWL_TIME_TYPE displ;
			displ.pts = 0;
			displ.timeres = 90000;
			khwl_getproperty(KHWL_TIME_SET, etimVideoFrameDisplayedTime, sizeof(displ), &displ);
			if (displ.pts >= (ULONGLONG)pts)
#endif
			{
				if (!dvd_skip(cur_dsi))
				{
					MSG_ERROR("DVD: Error seeking next block/program: %s\n", media_geterror());
					media_free_block(base);
					dvd_setspeed(MPEG_SPEED_NORMAL);
					script_speed_callback();
					return -1;
				}
			}
			break;
		}

		if (feed)
		{
			int button;
		  
	  		// compare current buttons and old buttons
			
			bool thesame = true;
			if (cur_num_buttons != cur_pci->hli.hl_gi.btn_ns)
				thesame = false;
			else if (need_to_check_buttons)
			{
				for (button = 0; button < cur_num_buttons; button++) 
	  			{
					DVD_BUTTON &b = cur_buttons[button];
					btni_t *btni = &(cur_pci->hli.btnit[button]);
					if (b.left != btni->x_start || b.top != btni->y_start || 
						b.right != btni->x_end || b.bottom != btni->y_end)
					{
						thesame = false;
						break;
					}
				}
			}
			need_to_check_buttons = false;
			
			if (cur_pci->hli.hl_gi.hli_ss == 1)
			{
				MSG("DVD: ! Menu ahead (was=%d)!\n", dvd_menu_ahead);
				// patch for non-accurate menus
				//if (dvd_menu_ahead && (cur_num_buttons == cur_pci->hli.hl_gi.btn_ns))
				//	thesame = true;
				dvd_menu_ahead = true;

				if (forced_spu != -1)
				{
					MSG("DVD: Resetting user's subtitle choice to -1\n");
					forced_spu = -1;
					forced_spu_lang = 0xffff;
				}
				if (forced_audio != -1)
				{
					MSG("DVD: Resetting user's audio choice to -1\n");
					forced_audio = -1;
					forced_audio_lang = 0xffff;
				}

				// special 'allow all' mode
				if (dvd_spu_stream < 0)
				{
					MSG("DVD: Enabling 'allow-all-streams' SPU mode.\n");
					mpeg_setspustream(-2);
				}

				need_user_reset = true;

			} else
			{
				if (dvd_menu_ahead)
				{
					MSG("DVD: ! Menu end!\n");
					dvd_menu_ahead = false;

					// restore current SPU stream
					MSG("DVD: Restoring SPU stream to %d.\n", dvd_spu_stream);
					mpeg_setspustream(dvd_spu_stream);
				}
			}

			if (!dvdnav_is_domain_vts(dvd) && need_user_reset)
			{
				dvd_user_reset();
			}
			
			if (!thesame)
				cur_num_buttons = cur_pci->hli.hl_gi.btn_ns;

			if (cur_pci->hli.hl_gi.btn_ns > 0) 
			{
				if (!thesame)
				{
					MSG("DVD: Found %i DVD menu buttons...\n", cur_pci->hli.hl_gi.btn_ns);

		  			for (button = 0; button < cur_pci->hli.hl_gi.btn_ns; button++) 
		  			{
						btni_t *btni = &(cur_pci->hli.btnit[button]);
						MSG("DVD: Button %i top-left @ (%i,%i), bottom-right @ (%i,%i)\n", 
							button + 1, btni->x_start, btni->y_start, btni->x_end, btni->y_end);

						cur_buttons[button].left = btni->x_start;
						cur_buttons[button].top = btni->y_start;
						cur_buttons[button].right = btni->x_end;
						cur_buttons[button].bottom = btni->y_end;
		  			}
				}
				if (!dvd_buttons_enabled)
					dvd_buttons_enable(true);
				if (cur_pci->hli.hl_gi.fosl_btnn > 0) 
				{
					if (dvd_cur_button != cur_pci->hli.hl_gi.fosl_btnn)
					{
						MSG("DVD: * force button = %d!\n", cur_pci->hli.hl_gi.fosl_btnn);
					}
					dvd_cur_button = cur_pci->hli.hl_gi.fosl_btnn;
					
				}
				if (!dvd_button_selected)
				{
					//dvdnav_get_current_highlight(dvd, &dvd_cur_button);
					MSG("DVD: * selecting button = %d:\n", dvd_cur_button);
					if (dvd_cur_button > cur_num_buttons)
						dvd_cur_button = cur_num_buttons - 1;
					else if (dvd_cur_button < 1)
						dvd_cur_button = 1;
					dvdnav_button_select(dvd, cur_pci, dvd_cur_button);
				}
				
			} else
			{
				if (dvd_buttons_enabled)
					dvd_buttons_enable(false);
			}
			dvd_update_info();
		}
		break;
      }
    
    case DVDNAV_HOP_CHANNEL:
      {
      	// This event is issued whenever a non-seamless operation has been executed.
      	// Applications with fifos should drop the fifos content to speed up responsiveness.
		
		if (feed)
		{
			if (dvd_speed == MPEG_SPEED_NORMAL)
			{
				MSG("DVD: HOP channel!\n");

				khwl_stop();
				dvd_buttons_enable(false);
				need_to_check_buttons = true;

				mpeg_start();
				
				// \TODO: this is a test...
				mpeg_setpts(0);
				dvd_seamless = false;
			}
		}
      	break;
      }

    case DVDNAV_STOP:
      {
		// Playback should end here.
		if (feed)
		{
			MSG("DVD: STOP!!!\n");
		}
		// we may not free the block
		dvd_savepos(true);
		return 1;
      }
      
    default:		// skip
		MSG_ERROR("DVD: Unknown event (%i)\n", event);
      	break;
    }

	// don't forget to free not used block!
	// NOTE: it does nothing if our cache is used!
	media_free_block(base);

	if (was_feed && feed_cnt++ < 2)
		goto get_some_more;

	return 0;
}

int dvd_player_loop()
{
	return dvd_player_loop_internal(true);
}

int dvd_get_next_block(BYTE **buf, int *event, int *len)
{
	struct timeval tv1, tv2;
	// measure time (see below)
	BOOL need_measure = (dvd_speed == MPEG_SPEED_NORMAL) ? mpeg_is_playing() : FALSE;
	if (need_measure)
		gettimeofday(&tv1, NULL);

#ifndef DVD_USE_CACHE
		if (dvdnav_get_next_block(dvd, *buf, event, len) == DVDNAV_STATUS_ERR)
			return -1;
		return 1;
#else
		dvdnav_reset_cache_flag(dvd);
		if (dvdnav_get_next_cache_block(dvd, buf, (int *)event, len) == DVDNAV_STATUS_ERR)
			return -1;

	// we check if decoder waits too long - audio/video mistiming will happen then (only raw-read counts)
	if (need_measure && *event == DVDNAV_BLOCK_OK)
	{
		// we don't check mpeg_feed_getstackdepth(). It may not be empty if long delay occurs.
		// we just check if we read from disk way too long...
		gettimeofday(&tv2, NULL);
		int read_dtime = (int)((tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec - tv1.tv_usec) / 1000);
		if (read_dtime > read_max_allowed_time)
		{
			msg("DVD: Disc read Time (%d msec) > limit!\n", read_dtime);
			khwl_stop();
			mpeg_start();
			msg("DVD: Resync...\n");
		}
	}

	return dvdnav_get_cache_flag(dvd);
#endif
}

int dvd_free_block(BYTE *data)
{
#ifdef DVD_USE_CACHE
	if (dvdnav_free_cache_block(dvd, data) != DVDNAV_STATUS_OK) 
		return -1;
#endif
	return 0;
}

BOOL dvd_stop()
{
	mpeg_deinit();

	dvd_savepos(false);
	dvd_not_played_yet = true;

	SPSafeFree(dvdlang_lut);

	dvd_spu_enable(false);

	if (dvd != NULL)
	{
		if (media_close() < 0) 
		{
			MSG_ERROR("DVD: Error on dvdnav_close: %s\n", dvdnav_err_to_string(dvd));
			return FALSE;
		}
	}
	
	fip_clear();
	dvd_fip_init();
	khwl_setvideomode(KHWL_VIDEOMODE_NONE, TRUE);

	dvd = NULL;

	cur_pack = -1;
	
    return TRUE;
}

bool dvd_end_still(bool skipstill = true)
{
	dvd_user_reset();
	/*
	if (wait_for_user)
		need_to_set_pts = true;
	*/
	wait_for_user = false;
	bool ret = false;
	if (wait_pause > 0)
	{
		if (skipstill)
		{
			MSG("DVD: Fast-skipping still frame...\n");
			dvdnav_still_skip(dvd);
			// we don't need to do anything more...
			ret = true;
		}
		wait_pause = 0;
	}

	mpeg_start();
	return ret;
}

int dvd_getnumtitles()
{
	int titl = 0;
	dvdnav_get_number_of_titles(dvd, &titl);
	return titl;
}

int dvd_getnumchapters(int title)
{
	int chap;
	dvdnav_get_number_of_parts(dvd, title, &chap);
	return chap;
}

BOOL dvd_seek_titlepart(int title, int part)
{
	if (title < 1 || part < 1)
		return FALSE;
	dvd_end_still();
	dvd_setspeed(MPEG_SPEED_NORMAL);
	script_speed_callback();
	
	MSG("DVD: seek to title %d, part %d.\n", title, part);
	
	if (!dvdnav_is_domain_vts(dvd))
	{
		MSG_ERROR("DVD: Cannot seek - not in play!\n");
		return FALSE;
	}
	if (dvdnav_part_play(dvd, title, part) != DVDNAV_STATUS_OK)
	{
		MSG_ERROR("DVD: seek error.\n");
		return FALSE;
	}
	return TRUE;
}

BOOL dvd_seek(int seconds)
{
	if (seconds < 0)
		return FALSE;

	dvd_end_still();
	dvd_setspeed(MPEG_SPEED_NORMAL);
	script_speed_callback();
	MSG("DVD: Seek to %d secs...\n", seconds);

	LONGLONG newpts = seconds * 90000;

	if (dvdnav_time_search(dvd, newpts) != DVDNAV_STATUS_OK)
	{
		MSG_ERROR("DVD: seek error.\n");
		return FALSE;
	}

#ifdef MY_SEARCH
	bool simpleseek = false;
	if (!dvdnav_is_domain_vts(dvd))
	{
		MSG_ERROR("DVD: Cannot seek - not in play!\n");
		return FALSE;
		//simpleseek = true;
	}

	bool needs_restore = false;
	LONGLONG cur_vobu_pts0 = 0;
	int base_titleid = 1, base_chapid = 1;
	int num_parts = 0;
	int raf = 0;

	dvdnav_current_title_info(dvd, &cur_titleid, &cur_chapid);
	base_titleid = cur_titleid;
	base_chapid = cur_chapid;
	
	/// \TODO: don't start from beginning in all cases
	if (dvdnav_title_play(dvd, cur_titleid) != DVDNAV_STATUS_OK)
		goto err;
	dvdnav_current_title_info(dvd, &cur_titleid, &cur_chapid);
	dvdnav_get_number_of_parts(dvd, cur_titleid, &num_parts);

	old_cell_pts = 0;
	old_vob_id = -1;

	// use navigation info
	dvd_scan = true;
	
	dvdnav_get_readahead_flag(dvd, &raf);
	dvdnav_set_readahead_flag(dvd, 0);

	//if (newpts < cur_vobu_pts || !simpleseek)
	{
		cur_vobu_pts = -1;
		while (cur_vobu_pts == -1)
			dvd_player_loop_internal(false);
	}
	cur_vobu_pts0 = cur_vobu_pts;

	//while (true)
	{
		// search start chapter
		LONGLONG last_ptm = cur_cell_pts;
		while (cur_cell_pts < newpts)
		{
			if (last_ptm > cur_cell_pts)
			{
				needs_restore = true;
				goto err;
			}
			last_ptm = cur_cell_pts;
			MSG("DVD: jump to part %d...\n", cur_chapid);
			dvdnav_part_play(dvd, cur_titleid, ++cur_chapid);
			cur_cell_pts = -1;
			while (cur_cell_pts == -1)
				dvd_player_loop_internal(false);
			if (cur_chapid >= num_parts)
				break;
		}
		if (cur_chapid > 0)
		{
			dvdnav_part_play(dvd, cur_titleid, --cur_chapid);
			cur_cell_pts = -1;
			while (cur_cell_pts == -1)
				dvd_player_loop_internal(false);
		}
		// now we know the chapter!
		const int numdirs = 3;
		int dir = 0, sks[3] = { 18-16, 12, 18-4 };
		int lastskip = 400;
		// zig-zag iterations
		while (true)
		{
			int from = SEEK_CUR, skip = 0, sk = sks[dir];
			if (dir % 2 == 0)
			{
				while (sk < 19 && !VALID_XWDA(cur_dsi->vobu_sri.fwda[sk]))
					sk++;
				if (sk < 19)
					skip = cur_dsi->vobu_sri.fwda[sk] & SRI_END_OF_CELL;
				else
					skip = lastskip;
				lastskip = skip;
				if (skip == 0)
					break;
			} else
			{
				while (sk > 0 && !VALID_XWDA(cur_dsi->vobu_sri.bwda[sk]))
					sk--;
				if (sk > 0)
					skip = cur_dsi->vobu_sri.bwda[sk] & SRI_END_OF_CELL;
				else
					skip = lastskip;
				lastskip = skip;
				if (skip == 0)
					break;
				
				unsigned int curpos = 0, curlen = 0;
				if (dvdnav_get_position(dvd, &curpos, &curlen) != DVDNAV_STATUS_OK)
					goto err;
				skip = curpos - skip;
				from = SEEK_SET;
			}
			
			if (media_seek(dvd, skip, from) == -1)
				goto err;
			cur_vobu_pts = -1;
			while (cur_vobu_pts == -1)
				dvd_player_loop_internal(false);
			
			if (dir == numdirs - 1 && cur_vobu_pts >= newpts)
				break;
			else if ((dir % 2) == 0 && cur_vobu_pts >= newpts)
				dir++;
			else if ((dir % 2) == 1 && cur_vobu_pts < newpts)
				dir++;
			
		}
	}
err:
	if (needs_restore)
	{
		MSG_ERROR("DVD: Cannot seek - restore pos!\n");
		dvdnav_part_play(dvd, base_titleid, base_chapid);
		//media_rewind(dvd);
	}
	dvdnav_set_readahead_flag(dvd, raf);
	dvd_scan = false;
#endif
	
	MSG("DVD: seek DONE!\n");
	
	khwl_stop();
	dvd_buttons_enable(false);
	//need_to_set_pts = true;
	mpeg_start();

	dvd_seamless = false;
	
	return TRUE;

#if 0
	dvd_scan = true;
	while (mpeg_getrate() == 0)
		dvd_player_loop_internal(false);
	dvd_scan = false;
	
	int rate = mpeg_getrate();
	MSG("DVD: mpeg_rate = %d\n", rate);

	int delta, from;
	LONGLONG old_pts = dvd_vobu_start_ptm;//= pts - pts_base;
	if (old_pts != 0 && simpleseek)
	{
		LONGLONG d = (newpts - old_pts) * (rate * 50) / 90000;
		delta = (int)(d / 2048);	// align to packet's boundary

		delta += dvd_vobu_start_pos;

		from = SEEK_SET;//SEEK_CUR;
		
	} else
	{
		LONGLONG d = newpts * (rate * 50) / 90000;
		delta = (int)(d / 2048);	// align to packet's boundary
		from = SEEK_SET;
	}
	if (media_seek(dvd, delta, from) == -1)
		return FALSE;
#endif

	return TRUE;
}


void dvd_button_press()
{
	dvd_end_still(false);
	if (cur_pci != NULL)
	{
		MSG("DVD: Activating current button...\n");
		// handle mode2 updates
		int button;
		dvdnav_get_current_highlight(dvd, &button);
		dvd_update_button(button, 2);

		dvdnav_button_activate(dvd, dvdnav_get_current_nav_pci(dvd));
		
	}
}

void dvd_button_up()
{
	dvd_end_still(false);
	if (cur_pci != NULL)
	{
		MSG("DVD: Selecting upper button...\n");
		dvdnav_upper_button_select(dvd, dvdnav_get_current_nav_pci(dvd));
	}
}

void dvd_button_down()
{
	dvd_end_still(false);
	if (cur_pci != NULL)
	{
		MSG("DVD: Selecting lower button...\n");
		dvdnav_lower_button_select(dvd, dvdnav_get_current_nav_pci(dvd));
	}
}

void dvd_button_left()
{
	dvd_end_still(false);
	if (cur_pci != NULL)
	{
		MSG("DVD: Selecting left button...\n");
		dvdnav_left_button_select(dvd, dvdnav_get_current_nav_pci(dvd));
	}
}

void dvd_button_right()
{
	dvd_end_still(false);
	if (cur_pci != NULL)
	{
		MSG("DVD: Selecting right button...\n");
		dvdnav_right_button_select(dvd, dvdnav_get_current_nav_pci(dvd));
	}
}

void dvd_button_menu(DVD_MENU_TYPE menu)
{
	dvd_end_still(false);
	if (menu == DVD_MENU_DEFAULT)
	{
		if (dvdnav_is_domain_vts(dvd))
		{
			dvdnav_menu_call(dvd, DVD_MENU_Root/*DVD_MENU_Part*/);	// we want VTSM, not VMGM !
		} else
			dvdnav_menu_call(dvd, DVD_MENU_Escape);
	} else
		dvdnav_menu_call(dvd, (DVDMenuID_t)menu);

}

bool dvd_ismenu()
{
	return dvdnav_is_domain_vts(dvd) != 1;
}

void dvd_button_angle(int setangl)
{
	int num = 0, current = 0;
	dvdnav_get_angle_info(dvd, &current, &num);

	if (num != 0) 
	{
		if (setangl >= 0)
			current = setangl;
		else
		{
			current++;
			if (current > num)
				current = 1;
		}
		if (dvdnav_angle_change(dvd, current))
			return;
	}
	dvd_invalid();
}

int dvd_getangle()
{
	int num = 0, current = 0;
	dvdnav_get_angle_info(dvd, &current, &num);
	return current;
}

void dvd_button_audio(char *setlang, int startfrom)
{
	if (dvdnav_is_domain_vts(dvd))
	{
		WORD setl = 0xffff;
		if (setlang == NULL)
		{
			MSG("DVD: dvd_audio (%d)\n", startfrom);
			setl = 0xffff;
		}
		else
		{
			MSG("DVD: dvd_audio %s\n", setlang);
			setl = dvd_getintlang((BYTE *)setlang);
		}
		int audiostream = -1, logstream = -1;
		WORD lang = 0xffff, firstlang = 0xffff;
		bool wasany = false, found = false;
		int firstaudiostream = -1, firstlogstream = -1;
		int curaudiostream = mpeg_getaudiostream();
		for (audiostream = startfrom; audiostream < 8; audiostream++)
		{
			logstream = dvd_get_audio_logical_stream(dvd, (BYTE)audiostream);
			lang = logstream >= 0 ? dvdnav_audio_stream_to_lang(dvd, (BYTE)logstream) : (WORD)0xffff;
			if (logstream >= 0 /*&& lang != 0xffff*/)
			{
				wasany = true;
				if (firstaudiostream == -1)
				{
					firstaudiostream = audiostream;
					firstlogstream = logstream;
					firstlang = lang;
				}
				if ((setl == 0xffff && audiostream > curaudiostream) || 
					(setl != 0xffff && lang == setl))
				{
					found = true;
					break;
				}
			}
		}
		if (!wasany)
		{
			MSG("DVD: No audio streams found.\n");
			dvd_invalid();
			return;
		}
		// set to the first if we didn't found any valid stream after the current
		if (!found)
		{
			audiostream = firstaudiostream;
			logstream = firstlogstream;
			lang = firstlang;
		}
/*
		// -2 means that we don't want any audio
		forced_audio = audiostream == -1 ? -2 : audiostream;
		forced_audio_lang = lang;
		mpeg_setaudiostream(audiostream);
*/
		dvd_set_audio_stream(dvd, audiostream);

		BYTE strlang[3];
		dvd_getcharlang(strlang, lang);
		MSG("DVD: setaudiostream = %d, lang='%c%c'\n", audiostream, strlang[0], strlang[1]);
		script_audio_stream_callback(logstream >= 0 ? logstream+1 : 0);
		script_audio_lang_callback(dvdlangs[dvdlang_lut[lang]].str);
		
	} else
		dvd_invalid();
}

void dvd_button_subtitle(char *setlang, int startfrom)
{
	if (dvdnav_is_domain_vts(dvd))
	{
		WORD setl = 0xffff;
		int spustream = -1, logstream = -1, curstream = -1;
		if (setlang == NULL)
		{
			MSG("DVD: dvd_subtitle (%d)\n", startfrom);
			setl = 0xffff;

			curstream = mpeg_getspustream();
			logstream = dvd_get_spu_logical_stream(dvd, (BYTE)curstream, dvd_get_spu_mode(dvd_vmode));
			if (logstream < 0)
				curstream = -1;
		}
		else
		{
			MSG("DVD: dvd_subtitle %s\n", setlang);
			setl = dvd_getintlang((BYTE *)setlang);
		}
		WORD lang = 0xffff;
		bool wasany = false, found = false;
		for (spustream = startfrom; spustream < 32; spustream++)
		{
			logstream = dvd_get_spu_logical_stream(dvd, (BYTE)spustream, dvd_get_spu_mode(dvd_vmode));
			lang = logstream >= 0 ? dvdnav_spu_stream_to_lang(dvd, (BYTE)logstream) : (WORD)0xffff;
			if (logstream >= 0/* && lang != 0xffff*/)
			{
				wasany = true;
				if ((setl == 0xffff && spustream > curstream) || 
					(setl != 0xffff && lang == setl))
				{
					found = true;
					break;
				}
			}
		}
		if (!wasany)
		{
			MSG("DVD: No subtitle streams found.\n");
			dvd_invalid();
			return;
		}
		// switch off if we didn't found any valid stream after the current
		if (!found)
		{
			spustream = -1;
			logstream = -1;
			lang = 0xffff;
		}
/*
		// -2 means that we don't want any subtitles
		forced_spu = spustream == -1 ? -2 : spustream;
		forced_spu_lang = lang;
		mpeg_setspustream(spustream);
*/
		dvd_set_spu_stream(dvd, spustream, dvd_get_spu_mode(dvd_vmode));

		BYTE strlang[3];
		dvd_getcharlang(strlang, lang);
		MSG("DVD: setspustream = %d, lang='%c%c'\n", spustream, strlang[0], strlang[1]);
		script_spu_stream_callback(logstream >= 0 ? logstream+1 : 0);
		script_spu_lang_callback(dvdlangs[dvdlang_lut[lang]].str);
		
	} else
		dvd_invalid();
}

void dvd_button_return()
{
	MSG("DVD: dvd_go_up\n");
	if (!dvdnav_is_domain_vts(dvd))
	{
		dvd_end_still(); // true
		if (!dvdnav_go_up(dvd))
			dvd_invalid();
	} else
	{
		dvd_invalid();
	}
}


void dvd_button_prev()
{
	if (!dvd_end_still())
	{
		if (dvdnav_current_title_info(dvd, &cur_titleid, &cur_chapid) && cur_titleid > 0)
		{
			dvdnav_part_play(dvd, cur_titleid, --cur_chapid);
			MSG("DVD: dvd_jump_prev\n");
			return;
		}
		dvd_invalid();
	}
}

void dvd_button_next()
{
	if (!dvd_end_still())
	{
		MSG("DVD: dvd_jump_next\n");
		if (dvdnav_current_title_info(dvd, &cur_titleid, &cur_chapid) && cur_titleid > 0)
		{
			if (!dvdnav_part_play(dvd, cur_titleid, ++cur_chapid))
				dvd_invalid();
		} else
		{
			if (!dvdnav_next_pg_search(dvd))
				dvd_invalid();
		}
	}
}

static void dvd_st_spd(MPEG_SPEED_TYPE spd, int mode)
{
	int sp = (int)spd;
	if (mode > 0)
		sp++;
	else if (mode < 0)
		sp--;
	if (dvd_setspeed((MPEG_SPEED_TYPE)sp))
	{
		MSG("DVD: dvd_speed = 0x%3x\n", sp);
	} else
		dvd_invalid();
}

void dvd_button_slow()
{
	if (dvdnav_is_domain_vts(dvd))
	{
		MSG("DVD: dvd_button_slow\n");
		if (dvd_speed == MPEG_SPEED_SLOW_FWD_8X || dvd_speed == MPEG_SPEED_NORMAL)
			dvd_st_spd(MPEG_SPEED_SLOW_FWD_2X, 0);
		else if (dvd_speed == MPEG_SPEED_SLOW_REV_8X)
			dvd_st_spd(MPEG_SPEED_SLOW_REV_2X, 0);
		else if ((dvd_speed & MPEG_SPEED_SLOW_FWD_MASK) == MPEG_SPEED_SLOW_FWD_MASK)
			dvd_button_rew();
		else if ((dvd_speed & MPEG_SPEED_SLOW_REV_MASK) == MPEG_SPEED_SLOW_REV_MASK)
			dvd_button_fwd();
		else
			dvd_invalid();
	}
	else
		dvd_invalid();
}

void dvd_button_fwd()
{
	if (dvdnav_is_domain_vts(dvd))
	{
		if (dvd_speed == MPEG_SPEED_REV_8X || dvd_speed == MPEG_SPEED_NORMAL || dvd_speed == MPEG_SPEED_PAUSE)
			dvd_st_spd(MPEG_SPEED_FWD_8X, 0);
		else if (dvd_speed >= MPEG_SPEED_FWD_8X && dvd_speed < MPEG_SPEED_FWD_48X)
			dvd_st_spd(dvd_speed, 1);
		else if (dvd_speed > MPEG_SPEED_REV_8X && dvd_speed <= MPEG_SPEED_REV_48X)
			dvd_st_spd(dvd_speed, -1);
		// use it in slow mode too
		else if (dvd_speed == MPEG_SPEED_SLOW_REV_2X)
			dvd_st_spd(MPEG_SPEED_SLOW_FWD_2X, 0);
		else if (dvd_speed > MPEG_SPEED_SLOW_FWD_2X && dvd_speed <= MPEG_SPEED_SLOW_FWD_8X)
			dvd_st_spd(dvd_speed, -1);
		else if (dvd_speed >= MPEG_SPEED_SLOW_REV_2X && dvd_speed < MPEG_SPEED_SLOW_REV_8X)
			dvd_st_spd(dvd_speed, 1);
		else
			dvd_invalid();
	} else
		dvd_invalid();
}

void dvd_button_rew()
{
	if (dvdnav_is_domain_vts(dvd))
	{
		if (dvd_speed == MPEG_SPEED_FWD_8X || dvd_speed == MPEG_SPEED_NORMAL || dvd_speed == MPEG_SPEED_PAUSE)
			dvd_st_spd(MPEG_SPEED_REV_8X, 0);
		else if (dvd_speed >= MPEG_SPEED_REV_8X && dvd_speed < MPEG_SPEED_REV_48X)
			dvd_st_spd(dvd_speed, 1);
		else if (dvd_speed > MPEG_SPEED_FWD_8X && dvd_speed <= MPEG_SPEED_FWD_48X)
			dvd_st_spd(dvd_speed, -1);
		// use it in slow mode too
		/*
		// don't use slow rev mode - it's not ready?..
		else if (dvd_speed == MPEG_SPEED_SLOW_FWD_2X)
			dvd_st_spd(MPEG_SPEED_SLOW_REV_2X, 0);
		*/
		else if (dvd_speed > MPEG_SPEED_SLOW_REV_2X && dvd_speed <= MPEG_SPEED_SLOW_REV_8X)
			dvd_st_spd(dvd_speed, -1);
		else if (dvd_speed >= MPEG_SPEED_SLOW_FWD_2X && dvd_speed < MPEG_SPEED_SLOW_FWD_8X)
			dvd_st_spd(dvd_speed, 1);
		else
			dvd_invalid();
	} else
		dvd_invalid();
}

void dvd_button_play()
{
	if (dvd_waiting_to_play)
	{
		dvd_waiting_to_play = false;
		dvd_setspeed(MPEG_SPEED_NORMAL);
		MSG("DVD: dvd_waiting_to_play = false\n");
	}
	if (dvdnav_is_domain_vts(dvd))
	{
		dvd_setspeed(MPEG_SPEED_NORMAL);
		MSG("DVD: dvd_play\n");
	} else
		dvd_invalid();
}

void dvd_button_pause()
{
	if (dvd_not_played_yet)
	{
		dvd_waiting_to_play = true;
		dvd_setspeed(MPEG_SPEED_PAUSE);
		MSG("DVD: dvd_waiting_to_play = true\n");
		return;
	} else
	if (dvdnav_is_domain_vts(dvd))
	{
		if (dvd_speed == MPEG_SPEED_PAUSE || dvd_speed == MPEG_SPEED_STEP)
		{
			dvd_setspeed(MPEG_SPEED_STEP);
			MSG("DVD: dvd_step\n");
		}
		else
		{
			MSG("DVD: dvd_pause\n");
			dvd_setspeed(MPEG_SPEED_PAUSE);
			dvd_savepos(false);
		}
	} else
		dvd_invalid();
}

void dvd_button_step()
{
	if (dvdnav_is_domain_vts(dvd))
	{
		dvd_setspeed(MPEG_SPEED_STEP);
		MSG("DVD: dvd_step\n");
	} else
		dvd_invalid();
}


BOOL dvd_zoom_hor(int scale)
{
	if (dvdnav_is_domain_vts(dvd))
	{
		MSG("DVD: Zoomed set to %d.\n", scale);
		mpeg_zoom_hor(scale);
		return TRUE;
	} else
		dvd_invalid();
	need_user_reset = true;
	return FALSE;
}

BOOL dvd_zoom_ver(int scale)
{
	if (dvdnav_is_domain_vts(dvd))
	{
		MSG("DVD: Zoomed set to %d.\n", scale);
		mpeg_zoom_ver(scale);
		return TRUE;
	} else
		dvd_invalid();
	need_user_reset = true;
	return FALSE;
}

BOOL dvd_scroll(int offsetx, int offsety)
{
	if (dvdnav_is_domain_vts(dvd))
	{
		MSG("DVD: Scroll set to %d,%d.\n", offsetx, offsety);
		mpeg_scroll(offsetx, offsety);
		return TRUE;
	} else
		dvd_invalid();
	need_user_reset = true;
	return FALSE;
}

int dvd_get_total_time(int title, int chapter, int *tim)
{
	uint64_t time;
	int ret = dvd_get_time(dvd, title - 1, chapter - 1, &time);
	*tim = (int)(time / 90000);
	return ret;
}

int dvd_get_chapter_for_time(int title, int time, int *chapter)
{
	int chap;
	int ret = dvd_get_chapter(dvd, title - 1, (uint64_t)time * 90000, &chap);
	*chapter = chap + 1;
	return ret;
}

char *dvd_error_string()
{
	return (char *)dvdnav_err_to_string(dvd);
}

int dvd_savepos(bool reset)
{
	DWORD pos = 0, title = 0, lenbl = 0;
	DWORD data1, data2;
	if (reset)
	{
		pos = 0xffffffff;
		title = 0xffffffff;
		data1 = data2 = 0xffffffff;
		// if already reset
		if (dvd_saved_pos == pos && dvd_saved_title == title)
			return 0;
		MSG("DVD: Resetting saved movie position.\n");
	} else
	{
		if (!dvdnav_is_domain_vts(dvd))
			return -1;
		if (dvdnav_get_position(dvd, (unsigned int *)&pos, (unsigned int *)&lenbl) != DVDNAV_STATUS_OK)
			return -1;
		int ch;
		if (dvdnav_current_title_info(dvd, (int *)&title, &ch) != DVDNAV_STATUS_OK) 
			return FALSE;

		// in menu or somewhere?
		if (pos == 0 || title == 0)
			return -1;

		dvd_saved_vmode = dvd_vmode;
		dvd_saved_audio_stream = mpeg_getaudiostream(); 
		dvd_saved_spu_stream   = mpeg_getspustream(); 
		data1 = dvd_saved_vmode & 0xff;
		data1 |= (((dvd_saved_audio_stream + 128) & 0xff) << 8);
		data1 |= (((dvd_saved_spu_stream + 128) & 0xff) << 16);
	}

	if (!dvd_was_saved && !reset)
	{
		DWORD id = settings_get(SETTING_DVD_ID1);
		DWORD titlepos = settings_get(SETTING_DVD_POS1);
		DWORD ddata1 = settings_get(SETTING_DVD_DATA11);
		DWORD ddata2 = settings_get(SETTING_DVD_DATA12);
		// shift other saved positions
		// we use this loop order to detect saved pos and break - this saves the last item
		for (int i = 1; i < dvd_num_saved; i++)
		{
			if (id == dvd_ID && titlepos != 0xffffffff)
				break;
			DWORD oldid = settings_get((SETTING_SET)(SETTING_DVD_ID1 + i * 4));
			DWORD oldtitlepos = settings_get((SETTING_SET)(SETTING_DVD_POS1 + i * 4));
			DWORD olddata1 = settings_get((SETTING_SET)(SETTING_DVD_DATA11 + i * 4));
			DWORD olddata2 = settings_get((SETTING_SET)(SETTING_DVD_DATA12 + i * 4));
			settings_set((SETTING_SET)(SETTING_DVD_ID1 + i * 4), id);
			settings_set((SETTING_SET)(SETTING_DVD_POS1 + i * 4), titlepos);
			settings_set((SETTING_SET)(SETTING_DVD_DATA11 + i * 4), ddata1);
			settings_set((SETTING_SET)(SETTING_DVD_DATA12 + i * 4), ddata2);
			id = oldid;
			titlepos = oldtitlepos;
			ddata1 = olddata1;
			ddata2 = olddata2;
		}
		settings_set(SETTING_DVD_ID1, dvd_ID);
		dvd_was_saved = true;
	}
	
	dvd_saved_pos = pos;
	dvd_saved_title = title;
	settings_set(SETTING_DVD_POS1, ((title & 0xff) << 24) | (pos & 0xffffff));
	settings_set(SETTING_DVD_DATA11, data1);
	script_player_saved_callback();
	MSG("DVD: Saving pos = %d/%d, vmode=%d, spu=%d, aud=%d\n", dvd_saved_title, dvd_saved_pos, 
		dvd_saved_vmode, dvd_saved_spu_stream, dvd_saved_audio_stream);
	return 0;
}

bool dvd_get_saved()
{
	dvd_saved_pos = 0xffffffff;
	dvd_saved_title = 0xffffffff;
	dvd_saved_vmode = -1; dvd_saved_audio_stream = -1; dvd_saved_spu_stream = -1;
	for (int i = 0; i < dvd_num_saved; i++)
	{
		DWORD id = (DWORD)settings_get((SETTING_SET)(SETTING_DVD_ID1 + i * 4));
		if (dvd_ID == id)
		{
			DWORD titlepos = (DWORD)settings_get((SETTING_SET)(SETTING_DVD_POS1 + i * 4));
			if (titlepos != 0xffffffff)	// not-empty slot
			{
				dvd_saved_pos = titlepos & 0xffffff;
				dvd_saved_title = (titlepos >> 24) & 0xff;
				DWORD data1 = (DWORD)settings_get((SETTING_SET)(SETTING_DVD_DATA11 + i * 4));
				dvd_saved_vmode = data1 & 0xff; 
				dvd_saved_audio_stream = ((data1 >> 8) & 0xff) - 128; 
				dvd_saved_spu_stream   = ((data1 >> 16) & 0xff) - 128; 
				break;
			}
		}
	}
	return dvd_saved_pos != 0xffffffff && dvd_saved_title != 0xffffffff && dvd_saved_title != 0xff;
}

BOOL dvd_continue_play()
{
	if (dvd_saved_pos == 0xffffffff || dvd_saved_title == 0xffffffff || dvd_saved_title == 0xff)
		return FALSE;

	wait_for_user = false;
	wait_pause = 0;
	dvd_reset_vm(dvd);
	dvdnav_title_play(dvd, dvd_saved_title);

	// TODO: find a better solution...
	bool old_dvd_waiting_to_play = dvd_waiting_to_play;
	dvd_waiting_to_play = false;
	for (int i = 0; i < 10; i++)
	{
		dvd_player_loop_internal(false);
	}
	dvd_waiting_to_play = old_dvd_waiting_to_play;

	if (dvdnav_current_title_info(dvd, &cur_titleid, &cur_chapid) != DVDNAV_STATUS_OK) 
		return FALSE;
	if (dvdnav_sector_search(dvd, dvd_saved_pos, SEEK_SET) != DVDNAV_STATUS_OK) 
		return FALSE;

	old_titleid = -1;
	old_chapid = -1;
	saved_pts = -1;
	msg("DVD: Continue play from saved pos = %d, title=%d\n", dvd_saved_pos, dvd_saved_title);
	
	script_dvd_menu_callback(dvdnav_is_domain_vts(dvd) != 1);

	// set other saved params
	msg("DVD: restore vmode = %d, aID=%d, sID=%d\n", dvd_saved_vmode, 
			dvd_saved_audio_stream, dvd_saved_spu_stream);

	// restore aspect ratio & vmode
	dvd_vmode = (KHWL_VIDEOMODE)dvd_saved_vmode;
	khwl_display_clear();
	khwl_setvideomode(dvd_vmode, TRUE);
	new_frame_size = true;

	// restore audio stream
	mpeg_setaudiostream(-1);
	if (dvd_saved_audio_stream >= 0)
		dvd_button_audio(NULL, dvd_saved_audio_stream);
	// sanity check
	if (mpeg_getaudiostream() < 0)
		mpeg_setaudiostream(0);
	
	// restore SPU stream
	mpeg_setspustream(-1);
	if (dvd_saved_spu_stream >= 0)
		dvd_button_subtitle(NULL, dvd_saved_spu_stream);

	return TRUE;
}
