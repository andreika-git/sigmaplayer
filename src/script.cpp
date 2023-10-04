//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - script wrapper impl.
 *  \file       script.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       12.10.2006
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
#include <time.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <ctype.h>
#include <dirent.h>

#include <libsp/sp_misc.h>


#ifdef WIN32
#define USE_MMSL_PARSER
#endif


#ifdef USE_MMSL_PARSER
#include <mmsl/mmsl-parser.h>
#endif

#include <script-internal.h>
#include <module.h>
#include <libsp/sp_flash.h>

ScriptParams *params = NULL;

#include <mmsl/mmsl-file.h>

#ifdef USE_MMSL_PARSER
#include <script-pvars.inc.c>
#include <script-pobjs.inc.c>
#else
#include <script-vars.inc.c>
#include <script-objs.inc.c>
#endif

//////////////////////////////////////////////////

static const StringPair tvtype_pairs[] = 
{ 
	{ "letterbox", 0 },
	{ "panscan", 1 },
	{ "wide", 2 },
	{ "vcenter", 3 },
	{ NULL, -1 },
};

static const StringPair tvstandard_pairs[] = 
{ 
	{ "ntsc", 0 },
	{ "pal", 1 },
	{ "480p", 2 },
	{ "576p", 3 },
	{ "720p", 4 },
	{ "1080i", 5 },
	{ NULL, -1 },
};

static const StringPair tvout_pairs[] = 
{ 
	{ "composite", 0 },
	{ "ypbpr", 1 },
	{ "rgb", 2 },
	{ NULL, -1 },
};

static const StringPair audioout_pairs[] = 
{ 
	{ "analog", 0 },
	{ "digital", 1 },
	{ NULL, -1 },
};

static const StringPair hddspeed_pairs[] = 
{ 
	{ "fastest", 0 },
	{ "limited", 1 },
	{ "slow", 2 },
	{ NULL, -1 },
};

static const StringPair button_pairs[] = 
{
	{ "", FIP_KEY_NONE },
	{ "power", FIP_KEY_POWER },
	{ "eject", FIP_KEY_EJECT },
	{ "one", FIP_KEY_ONE },
	{ "two",FIP_KEY_TWO },
	{ "three",FIP_KEY_THREE },
	{ "four",FIP_KEY_FOUR },
	{ "five",FIP_KEY_FIVE },
	{ "six",FIP_KEY_SIX },
	{ "seven",FIP_KEY_SEVEN },
	{ "eight",FIP_KEY_EIGHT },
	{ "nine",FIP_KEY_NINE },
	{ "zero",FIP_KEY_ZERO },
	{ "cancel",FIP_KEY_CANCEL },
	{ "search",FIP_KEY_SEARCH },
	{ "enter",FIP_KEY_ENTER },
	{ "osd",FIP_KEY_OSD },
	{ "subtitle",FIP_KEY_SUBTITLE },
	{ "setup",FIP_KEY_SETUP },
	{ "return",FIP_KEY_RETURN },
	{ "title",FIP_KEY_TITLE },
	{ "pn",FIP_KEY_PN },
	{ "menu",FIP_KEY_MENU },
	{ "ab",FIP_KEY_AB },
	{ "repeat",FIP_KEY_REPEAT },
	{ "up",FIP_KEY_UP },
	{ "down",FIP_KEY_DOWN },
	{ "left",FIP_KEY_LEFT },
	{ "right",FIP_KEY_RIGHT },
	{ "volume_down",FIP_KEY_VOLUME_DOWN },
	{ "volume_up",FIP_KEY_VOLUME_UP },
	{ "pause",FIP_KEY_PAUSE },
	{ "rewind",FIP_KEY_REWIND },
	{ "forward",FIP_KEY_FORWARD },
	{ "prev",FIP_KEY_SKIP_PREV },
	{ "next",FIP_KEY_SKIP_NEXT },
	{ "play",FIP_KEY_PLAY },
	{ "stop",FIP_KEY_STOP },
	{ "slow",FIP_KEY_SLOW },
	{ "audio",FIP_KEY_AUDIO },
	{ "vmode",FIP_KEY_VMODE },
	{ "mute",FIP_KEY_MUTE },
	{ "zoom",FIP_KEY_ZOOM },
	{ "program",FIP_KEY_PROGRAM },
	{ "pbc",FIP_KEY_PBC },
	{ "angle",FIP_KEY_ANGLE },

	{ "eject", FIP_KEY_FRONT_EJECT },
	{ "pause",FIP_KEY_FRONT_PAUSE },
	{ "rewind",FIP_KEY_FRONT_REWIND },
	{ "forward",FIP_KEY_FRONT_FORWARD },
	{ "prev",FIP_KEY_FRONT_SKIP_PREV },
	{ "next",FIP_KEY_FRONT_SKIP_NEXT },
	{ "play",FIP_KEY_FRONT_PLAY },
	{ "stop",FIP_KEY_FRONT_STOP },
	{ NULL, -1 },
};

static const StringPair front_button_pairs2[] = 
{
	{ "", FIP_KEY_NONE },
	{ "front_eject", FIP_KEY_FRONT_EJECT },
	{ "front_pause",FIP_KEY_FRONT_PAUSE },
	{ "front_rewind",FIP_KEY_FRONT_REWIND },
	{ "front_forward",FIP_KEY_FRONT_FORWARD },
	{ "front_prev",FIP_KEY_FRONT_SKIP_PREV },
	{ "front_next",FIP_KEY_FRONT_SKIP_NEXT },
	{ "front_play",FIP_KEY_FRONT_PLAY },
	{ "front_stop",FIP_KEY_FRONT_STOP },
	{ NULL, -1 },
};

static const StringPair pad_special_pairs[] = 
{
	{ "play", FIP_SPECIAL_PLAY, },
	{ "pause", FIP_SPECIAL_PAUSE, },
	{ "mp3", FIP_SPECIAL_MP3, },
	{ "dvd", FIP_SPECIAL_DVD, },
	{ "s", FIP_SPECIAL_S, },
	{ "v", FIP_SPECIAL_V,  },
	{ "cd", FIP_SPECIAL_CD, },
	{ "dolby", FIP_SPECIAL_DOLBY,  },
	{ "dts", FIP_SPECIAL_DTS, },
	{ "play_all", FIP_SPECIAL_ALL,  },
	{ "play_repeat", FIP_SPECIAL_REPEAT,  },
	{ "pbc", FIP_SPECIAL_PBC, },
	{ "camera", FIP_SPECIAL_CAMERA, },
	{ "colon1", FIP_SPECIAL_COLON1, },
	{ "colon2", FIP_SPECIAL_COLON2, },
	{ NULL, -1 },
};

static const StringPair mediatype_pairs[] = 
{
	{ "none", CDROM_STATUS_NODISC },
	{ "none", CDROM_STATUS_TRAYOPEN },
    { "dvd", CDROM_STATUS_HAS_DVD },
    { "iso", CDROM_STATUS_HAS_ISO },
    { "audio", CDROM_STATUS_HAS_AUDIO },
    { "mixed", CDROM_STATUS_HAS_MIXED },
	{ NULL, -1 },
};

static int GetScreenAdjustmentSetting(int idx)
{
	SETTING_SET which = SETTING_BRIGHTNESS_CONTRAST_SATURATION_PAL;
	if (gui.GetTvStandard() == WINDOW_TVSTANDARD_NTSC)
		which = SETTING_BRIGHTNESS_CONTRAST_SATURATION_NTSC;
	DWORD v = (settings_get(which) >> (idx * 10)) & 1023;
	return v;
}

static void SetScreenAdjustmentSetting(int idx, int v)
{
	if (v < 0) v = 0;
	if (v > 1000) v = 1000;
	SETTING_SET which = SETTING_BRIGHTNESS_CONTRAST_SATURATION_PAL;
	if (gui.GetTvStandard() == WINDOW_TVSTANDARD_NTSC)
		which = SETTING_BRIGHTNESS_CONTRAST_SATURATION_NTSC;
	DWORD d = settings_get(which) & ~(1023 << (idx * 10));
	settings_set(which, d | ((v & 1023) << (idx * 10)));
}

//////////////////////////////////////////////////

int script_init()
{
	int i;
	int buflen;
	BYTE *buf = NULL;

	params = new ScriptParams();
	tmpit = new Item();

	const char *mmso_filename = "mmsl/startup.mmso";

#ifdef USE_MMSL_PARSER
	mmsl_parser = new MmslParser();
	
	for (i = 0; script_vars[i].name != NULL; i++)
	{
		mmsl_parser->RegisterVariable(script_vars[i].name, script_vars[i].ID);
	}
	
	for (i = 0; script_objects[i].name != NULL; i++)
	{
		mmsl_parser->RegisterObject(script_objects[i].name, script_objects[i].ID);
	}

	for (i = 0; script_object_vars[i].name != NULL; i++)
	{
		mmsl_parser->RegisterObjectVariable(script_object_vars[i].obj_ID, 
											script_object_vars[i].name, script_object_vars[i].ID);
	}
	if (!mmsl_parser->ParseFile("startup.mmsl"))
	{
		msg_error("Cannot open/parse script file!\n");
	}
	buf = mmsl_parser->Save(&buflen);

	SPSafeDelete(mmsl_parser);

	if (buf != NULL)
	{
		mmso_save(mmso_filename, buf, buflen);
		SPSafeFree(buf);
	}
#endif

	mmsl = new Mmsl();
	msg("Loading MMSO '%s'\n", mmso_filename);
	buf = mmso_load(mmso_filename, &buflen);

	if (buf != NULL)
	{
		if (!mmsl->Load(buf, buflen))
		{
			msg("Cannot init main script!\n");
		}
		SPSafeFree(buf);

		for (i = 0; script_vars[i].ID >= 0; i++)
		{
			if (mmsl->BindVariable(script_vars[i].ID, script_vars[i].Get, script_vars[i].Set) < 0)
				msg("Script: Cannot bind variable #%d\n", script_vars[i].ID);
		}

		for (i = 0; script_objects[i].ID >= 0; i++)
		{
			if (mmsl->BindObject(script_objects[i].ID, script_objects[i].Create, script_objects[i].Delete, NULL) < 0)
				msg("Script: Cannot bind object #%d\n", script_objects[i].ID);
		}

		for (i = 0; script_object_vars[i].ID >= 0; i++)
		{
			if (mmsl->BindObjectVariable(script_object_vars[i].obj_ID, script_object_vars[i].ID, script_object_vars[i].Get, 
										script_object_vars[i].Set, NULL) < 0)
				msg("Script: Cannot bind object's #%d variable #%d\n", 
									script_object_vars[i].obj_ID, script_object_vars[i].ID);
		}
	} else
	{
		msg("Cannot load the main MMSO script!\n");
	}

	msg("Mmsl: Starting...\n");

	//mmsl->Dump();
	
	SPSafeFree(buf);

	struct timeval tv;
	gettimeofday(&tv, NULL);
	params->start_sec = (int)tv.tv_sec;
	params->curtime = params->old_curtime = tv.tv_usec / 1000;

	params->fw_ver.Printf("%s-%d.%d (" __DATE__ ")", SP_NAME, SP_VERSION / 100, SP_VERSION % 100);
	params->mmsl_ver = MMSL_VERSION;

	return 0;
}

int script_deinit()
{
	stop_all();
	cdda_close();

	SPSafeDelete(params);
	SPSafeDelete(tmpit);

	return 0;
}

int script_update(bool gfx_only)
{
	if (!params)
		return 0;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	
	params->curtime = (int)((tv.tv_sec - params->start_sec) * 1000 + tv.tv_usec / 1000);

	// check if we need to call elapsed timers:
restart:
	ScriptTimerObject *cur = params->timed_objs.GetFirst();
	while (cur != NULL)
	{
		// check timer value
		Window *win = (Window *)cur->obj;
		if (win != NULL)
		{
			// check if object timer elapsed
			if (!gfx_only && cur->type == SCRIPT_OBJECT_TIMER)
			{
				if (win->timer > 0 && win->timer < params->curtime)
				{
					win->timer = 0;
					mmsl->UpdateObjectVariable(win, cur->var_ID);
					if (win->timerobj == NULL || win->timer != 0)
						goto restart;
				}
			}
			// some objects may require updates
			else if (cur->type == SCRIPT_OBJECT_UPDATE)
			{
				win->Update(params->curtime);
			}
		}
		cur = cur->next;
	}

	if (gfx_only)
		return 0;

	// update other stuff

	if (params->need_to_toggle_tray)
	{
		if (toggle_tray())
			params->need_to_toggle_tray = FALSE;
	}

	if (params->flash_progress >= 0 && params->flash_progress < 100)
	{
		params->flash_p = flash_cycle();
		if (params->flash_p != params->flash_progress || params->flash_p < 0)
		{
			params->flash_progress = params->flash_p;

			msg("FLASH %d%%\n", params->flash_progress);
			if (params->flash_progress == 100)
			{
				msg("FLASH OK!\n");
			}
			mmsl->UpdateVariable(SCRIPT_VAR_FLASH_PROGRESS);
		}

		// do nothing more
		return 0;
	}

	if (params->player_do_command)
	{
		params->player_do_command = false;
		player_do_command();
	}

	if (params->dvdplaying)
	{
		if (dvd_player_loop() == 1)
		{
			dvd_stop();
			params->dvdplaying = FALSE;
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
			module_unload("dvd");
		}
	}
#ifdef EXTERNAL_PLAYER
	if (params->fileplaying)
	{
		if (player_info_loop() == 1)
		{
			player_stop();
			params->fileplaying = FALSE;
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
		}
	}
#endif
#ifdef INTERNAL_VIDEO_PLAYER
	if (params->videoplaying)
	{
		if (video_loop() == 1)
		{
			video_stop();
			params->videoplaying = FALSE;
			params->speed = MPEG_SPEED_STOP;
			if (mmsl != NULL)
			{
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
			}
		}
	}
#endif
#ifdef INTERNAL_AUDIO_PLAYER
	if (params->audioplaying)
	{
		if (audio_loop() == 1)
		{
			audio_delete_filelist();
			audio_stop();
			params->audioplaying = FALSE;
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
		}
	}
#endif
	if (params->cddaplaying)
	{
		if (cdda_player_loop() == 1)
		{
			cdda_stop();
			params->cddaplaying = FALSE;
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
		}
	}

	if (params->waitinfo)
	{
		if (player_id3_loop() == 1)
		{
			params->waitinfo = FALSE;
		}
	}
	
	return 0;
}

int script_get_time(struct timeval *tv)
{
	if (tv == NULL)
		return params->curtime;
	else
		return (int)((tv->tv_sec - params->start_sec) * 1000 + tv->tv_usec / 1000 - params->curtime);
}

void script_skiptime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	int next_time = script_get_time(&tv);
	params->start_sec += next_time / 1000;
}

bool script_update_variable(int ID)
{
	return mmsl->UpdateVariable(ID) != 0;
}

///////////////////////////////////////////////////////

void script_drive_callback(CDROM_STATUS status)
{
	bool status_changed = (status != params->status);
	if ((status == CDROM_STATUS_TRAYOPEN && params->status != CDROM_STATUS_TRAYOPEN) ||
		(params->status == CDROM_STATUS_TRAYOPEN && status != CDROM_STATUS_TRAYOPEN) ||
		(params->status == CDROM_STATUS_UNKNOWN && status != CDROM_STATUS_UNKNOWN))
	{
		params->status = status;
#if 1
		msg("* DRIVE.TRAY = %d\n", params->status);
#endif
		mmsl->UpdateVariable(SCRIPT_VAR_DRIVE_TRAY);
	}
	if (status_changed)
	{
		params->status = status;
#if 1
		msg("* DRIVE.MEDIATYPE = %d\n", params->status);
#endif
		mmsl->UpdateVariable(SCRIPT_VAR_DRIVE_MEDIATYPE);
	}
}

void script_set_key(int key)
{
	params->key = key;
	
	for (int i = 0; button_pairs[i].str != NULL; i++)
	{
		if (button_pairs[i].value == key)
		{
			params->keystr = button_pairs[i].str;
			break;
		}
	}
}

void script_key_callback(int key)
{
	if (params->flash_progress < 0)
	{
		msg("****** button %s ******\n", get_button_string(key));

		script_set_key(key);
		mmsl->UpdateVariable(SCRIPT_VAR_PAD_KEY);
	}
}

void script_time_callback(int secs)
{
	params->info.cur_time = secs;
	params->info.real_cur_time = secs;
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_TIME);
}

void script_totaltime_callback(int secs)
{
	params->info.length = secs;
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_LENGTH);
}

void script_dvd_menu_callback(bool )
{
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_MENU);
}

void script_speed_callback(int speed)
{
	if (speed >= 0)
		params->speed = (MPEG_SPEED_TYPE)speed;
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
}

void script_dvd_chapter_callback(int chapter)
{
	params->info.real_cur_chapter = params->info.cur_chapter = chapter;
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_CHAPTER);
}

void script_dvd_title_callback(int title, int numchapters)
{
	params->info.real_cur_title = params->info.cur_title = title;
	params->info.num_chapters = numchapters;
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_TITLE);
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_NUM_CHAPTERS);
}

void script_player_saved_callback()
{
	if (mmsl != NULL)
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SAVED);
}

void script_cdda_track_callback(int track)
{
	params->info.real_cur_title = params->info.cur_title = track;
	params->info.num_chapters = 0;
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_TITLE);
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_NUM_CHAPTERS);
}

void script_name_callback(const SPString & n)
{
	params->info.name = n;
	msg("Title: %s\n", *params->info.name);
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_NAME);
}

void script_artist_callback(const SPString & a)
{
	params->info.artist = a;
	msg("Artist: %s\n", *params->info.artist);
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_ARTIST);
}

void script_audio_info_callback(const SPString & ai)
{
	params->info.audio_info = ai;
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_AUDIO_INFO);
}

void script_video_info_callback(const SPString & vi)
{
	params->info.video_info = vi;
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_VIDEO_INFO);
}

void script_player_subtitle_callback(const char *s)
{
	params->info.subtitle = s;
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SUBTITLE);
}

void script_error_callback(SCRIPT_ERROR_CODE err)
{
	switch (err)
	{
	case SCRIPT_ERROR_INVALID:
		params->player_error = "invalid";
		break;
	case SCRIPT_ERROR_BAD_AUDIO:
		params->player_error = "badaudio";
		break;
	case SCRIPT_ERROR_BAD_CODEC:
		params->player_error = "badvideo";
		break;
	case SCRIPT_ERROR_QPEL:
		params->player_error = "qpel";
		break;
	case SCRIPT_ERROR_GMC:
		params->player_error = "gmc";
		break;
	case SCRIPT_ERROR_WAIT:
		params->player_error = "wait";
		break;
	case SCRIPT_ERROR_CORRUPTED:
		params->player_error = "corrupted";
		break;
	default:
		return;
	}
	
	if (params->player_type == PLAYER_TYPE_DVD)
	{
		if (dvd_getdebug())
			msg("DVD Warning!: %s!\n", *params->player_error);
	}
#ifdef EXTERNAL_PLAYER
	else if (params->player_type == PLAYER_TYPE_FILE)
	{
		if (player_getdebug())
			msg("Player Warning!: %s!\n", *params->player_error);
	}
#endif
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_ERROR);
}

void script_framesize_callback(int width, int height)
{
	if ((params->player_type == PLAYER_TYPE_DVD && dvd_getdebug())
#ifdef EXTERNAL_PLAYER
		|| (params->player_type == PLAYER_TYPE_FILE && player_getdebug())
#endif
		)
	{
		msg("* FRAME = %dx%d\n", width, height);
	}
	params->info.width = width;
	params->info.height = height;
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_WIDTH);
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_HEIGHT);
}

void script_framerate_callback(int frame_rate)
{
	if ((params->player_type == PLAYER_TYPE_DVD && dvd_getdebug())
#ifdef EXTERNAL_PLAYER
		|| (params->player_type == PLAYER_TYPE_FILE && player_getdebug())
#endif
		)
	{
		msg("* FRAME_RATE = %d\n", frame_rate);
	}
	params->info.frame_rate = frame_rate;
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_FRAME_RATE);
}

void script_zoom_scroll_reset_callback()
{
	params->hscale = 0;
	params->vscale = 0;
	params->hscroll = 0;
	params->vscroll = 0;
	mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_HZOOM);
	mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_VZOOM);
	mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_HSCROLL);
	mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_VSCROLL);
}

void script_colorspace_callback(int clrs)
{
	if ((params->player_type == PLAYER_TYPE_DVD && dvd_getdebug()) 
#ifdef EXTERNAL_PLAYER
		|| (params->player_type == PLAYER_TYPE_FILE && player_getdebug())
#endif
		)
	{
		msg("* COLORSPACE = %d\n", clrs);
	}
	params->info.clrs = (PLAYER_COLOR_SPACE)clrs;
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_COLOR_SPACE);
}

void script_audio_lang_callback(const char *lang)
{
	params->info.dvd_audio_lang = lang;
}

void script_spu_lang_callback(const char *lang)
{
	params->info.spu_lang = lang;
}

void script_audio_stream_callback(int stream)
{
	params->info.audio_stream = stream;
}

void script_spu_stream_callback(int stream)
{
	params->info.spu_stream = stream;
}

/////////////////////////////////////////////////////

bool stop_all()
{
	if (params->dvdplaying)
		player_dvd_command("stop");
#ifdef EXTERNAL_PLAYER
	if (params->fileplaying)
		player_file_command("stop");
#endif
#ifdef INTERNAL_VIDEO_PLAYER
	if (params->videoplaying)
	{
		return player_video_command("stop");
	}
#endif
#ifdef INTERNAL_AUDIO_PLAYER
	if (params->audioplaying)
		player_audio_command("stop");
#endif
	if (params->cddaplaying)
		player_cdda_command("stop");
	return true;
}


bool toggle_tray()
{
	static int in_toggle = 0;
	if (in_toggle)
		return false;

	in_toggle = 1;
	gui.Update();

	BOOL force_update = FALSE;
	CDROM_STATUS status = cdrom_getstatus(&force_update);
	bool eject = (status != CDROM_STATUS_TRAYOPEN);

	if (eject)
	{
		if (!stop_all())
		{
			params->need_to_toggle_tray = TRUE;
			return false;
		}

		// media is being changed - so close CDDA data
		cdda_close();

		// clear all cached lists and playlists
		script_explorer_reset();

		status = CDROM_STATUS_TRAYOPEN;
		params->require_next_status = CDROM_STATUS_UNKNOWN;
	}

	if (force_update)
		params->status = CDROM_STATUS_UNKNOWN;

	script_drive_callback(status);

	msg("* toggle_tray: eject = %d (%d)!!!\n", eject, params->status);
	cdrom_eject(eject);
	
	params->wasejected = TRUE; // to restore fip
	in_toggle = 0;
	return true;
}

void disc_changed(CDROM_STATUS st)
{
	if (st != CDROM_STATUS_CURRENT)
		params->status = st;
	if (is_playing())
		return;
	BOOL force_update = FALSE;
	CDROM_STATUS status = cdrom_getstatus(&force_update);

	if ((status & CDROM_STATUS_HASDISC) == CDROM_STATUS_HASDISC
		&& params->require_next_status != CDROM_STATUS_UNKNOWN)
		status = params->require_next_status;

	if (params->status != status || force_update)
	{
		if (status == CDROM_STATUS_NODISC || status == CDROM_STATUS_TRAYOPEN)
			params->require_next_status = CDROM_STATUS_UNKNOWN;
		if (force_update)
			params->status = CDROM_STATUS_UNKNOWN;
		params->wasejected = FALSE;
		script_drive_callback(status);
	}
}

/////////////////////////////////////////////////////////

void player_halt()
{
	for (;;)
	{
		khwl_osd_update();
	}
}

bool player_turn_onoff(bool on)
{
	// we cannot combine instructions because of different order
	if (!on)	// OFF
	{
		// close CD-ROM tray and prepare for shutdown
		BOOL force_status = FALSE;
		CDROM_STATUS status = cdrom_getstatus(&force_status);
		if (status == CDROM_STATUS_TRAYOPEN || !cdrom_isready())
		{
			fip_clear();
			fip_write_string("CLOSE");
			cdrom_eject(false);
			for (int tries = 0; tries < 80; tries++)
			{
				//msg("try=%d [status=%d]\n", tries, status);
				//gui_update();
				
				usleep(250000);
				if (cdrom_isready())
				{
					status = cdrom_getstatus(&force_status);
					if (status != CDROM_STATUS_TRAYOPEN && status != CDROM_STATUS_UNKNOWN)
						break;
				}
			}
			// if we stop CDROM before it detects a disc, we won't start it again...
			// I don't know a reliable way to check if we can stop - so wait 3 secs...
			//status = cdrom_getstatus();
			//if (status != CDROM_STATUS_TRAYOPEN && status != CDROM_STATUS_NODISC)
			//	usleep(3500000);
		}

		params->status = CDROM_STATUS_NODISC;
		mmsl->UpdateVariable(SCRIPT_VAR_DRIVE_TRAY);
		mmsl->UpdateVariable(SCRIPT_VAR_DRIVE_MEDIATYPE);

		gui.Clear();
		khwl_display_clear();
		fip_clear();

		gui.SwitchDisplay(false);
		khwl_osd_update();

		cdrom_switch(FALSE);
		khwl_ideswitch(FALSE);
	} 
	else	// ON
	{
		fip_write_string("LoAd");
		khwl_ideswitch(TRUE);
		
		usleep(1000000);

		gui.SwitchDisplay(true);
		
		usleep(1000000);
		
		khwl_set_window_frame(params->bigbackleft, params->bigbacktop, params->bigbackright, params->bigbackbottom);
		gui.SetBackground(params->bigback);
		
		usleep(1000000);

		cdrom_switch(TRUE);
	}
	return true;
}

bool script_next_vmode()
{
	int vout_max = settings_getmax(SETTING_TVOUT);
	int vstd_max = 1;//settings_getmax(SETTING_TVSTANDARD);
	int vout = gui.GetTvOut();
	int vstd = gui.GetTvStandard();
	if (vstd >= WINDOW_TVSTANDARD_480P)
	{
		vout = 0;
		vstd = 0;
	}
	else if (++vout > vout_max)
	{
		vout = 0;
		if (++vstd > vstd_max)
		{
			vstd = WINDOW_TVSTANDARD_480P;
			vout = WINDOW_TVOUT_YPBPR;
		}
	}
		
	gui.SetTv((WINDOW_TVOUT)vout, (WINDOW_TVSTANDARD)vstd);
	if (!params->dvdplaying && !params->videoplaying)
	{
		khwl_setvideomode(KHWL_VIDEOMODE_NONE, TRUE);
		khwl_set_window_frame(params->bigbackleft, params->bigbacktop, params->bigbackright, params->bigbackbottom);
		gui.SetBackground(params->bigback);
	}
	const char *vstds[] = { "NTSC", "PAL", "480P", "576P", "720P", "1080I" };
	const char *vouts[] = { "C/S-Video", "C/YPbPr", "C/RGB" };
	msg("VMODE set = %s %s\n", vstds[vstd], vouts[vout]);

	mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_TVSTANDARD);
	mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_TVOUT);
	return true;
}

////////////////////////////////////////////////////////////////////////

void on_kernel_get(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	switch (var_id)
	{
	case SCRIPT_VAR_KERNEL_POWER:
		var->Set(1);
		break;
	case SCRIPT_VAR_KERNEL_PRINT:
		var->Set("");
		break;
	case SCRIPT_VAR_KERNEL_FIRMWARE_VERSION:
		var->Set(params->fw_ver);
		break;
	case SCRIPT_VAR_KERNEL_MMSL_VERSION:
		var->Set(params->mmsl_ver);
		break;
	case SCRIPT_VAR_KERNEL_MMSL_ERRORS:
		{
			const char *errs[] = { "all", "general", "critical", "none" };
			var->Set(errs[params->errors]);
		}
		break;
	case SCRIPT_VAR_KERNEL_RUN:
		var->Set("");
		break;
	case SCRIPT_VAR_KERNEL_RANDOM:
		var->Set(rand() % 32768);
		break;
	case SCRIPT_VAR_KERNEL_FREQUENCY:
		var->Set(khwl_getfrequency());
		break;
	case SCRIPT_VAR_KERNEL_CHIP:
		var->Set(khwl_gethw());
		break;
	case SCRIPT_VAR_KERNEL_FREE_MEMORY:
		{
			struct sysinfo si;
			sysinfo(&si);
			var->Set(si.freeram);
		}
		break;
	case SCRIPT_VAR_KERNEL_FLASH_MEMORY:
		{
			var->Set(flash_get_memory_size());
		}
		break;
	}
}

void on_kernel_set(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	switch (var_id)
	{
	case SCRIPT_VAR_KERNEL_POWER:
		if (var->GetInteger() == 2)
			player_halt();
		else 
		{
			extern bool is_sleeping, do_sleep, need_to_stop;
			is_sleeping = var->GetInteger() == 0;
			do_sleep = 1;
			if (is_sleeping)
			{
				if (!stop_all())
					need_to_stop = true;
			}
			//player_turn_onoff(var->GetInteger() != 0);
		}
		break;
	case SCRIPT_VAR_KERNEL_PRINT:
		msg("User: %s\n", *var->GetString());
		break;
	case SCRIPT_VAR_KERNEL_MMSL_ERRORS:
		{
			SPString str = var->GetString();
			if (str.CompareNoCase("none") == 0)
			{
				params->errors = SCRIPT_ERRORS_NONE;
				msg_set_filter(MSG_FILTER_NONE);
			}
			else if (str.CompareNoCase("general") == 0)
			{
				params->errors = SCRIPT_ERRORS_GENERAL;
				msg_set_filter(MSG_FILTER_ERROR);
			}
			else if (str.CompareNoCase("critical") == 0)
			{
				params->errors = SCRIPT_ERRORS_CRITICAL;
				msg_set_filter(MSG_FILTER_CRITICAL);
			}
			else
			{
				params->errors = SCRIPT_ERRORS_ALL;
				msg_set_filter(MSG_FILTER_ALL);
			}
		}
		break;
	case SCRIPT_VAR_KERNEL_RUN:
		{
			SPString str = var->GetString();
			str.TrimLeft();
			str.TrimRight();
			char *args[20];
			args[0] = &str[0];
			bool in_quotes = false;
			int curarg = 0;
			for (int i = 0; i < str.GetLength(); i++)
			{
				if (str[i] == '\"')
				{
					in_quotes = !in_quotes;
				}
				if (str[i] == ' ' && !in_quotes)
				{
					str[i] = '\0';
					args[++curarg] = &str[i + 1];
					if (curarg >= 20)
						break;
				}
			}
			args[++curarg] = NULL;
			if (vfork() == 0)
			{
    			exec_file(args[0], (const char **)args);
			}
		}
		break;
	case SCRIPT_VAR_KERNEL_FREQUENCY:
		khwl_setfrequency(var->GetInteger());
		break;
	}
}

////////////////////////////////////////////////////////////////////////

void on_screen_get(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	if (var == NULL)
		return;
	switch (var_id)
	{
	case SCRIPT_VAR_SCREEN_SWITCH:
		var->Set(gui.IsDisplaySwitchedOn());
		break;
	case SCRIPT_VAR_SCREEN_UPDATE:
		var->Set(gui.IsUpdateEnabled());
		break;
	case SCRIPT_VAR_SCREEN_TVSTANDARD:
		StringPair::Set(var, tvstandard_pairs, gui.GetTvStandard());
		break;
	case SCRIPT_VAR_SCREEN_TVOUT:
		StringPair::Set(var, tvout_pairs, gui.GetTvOut());
		break;
	case SCRIPT_VAR_SCREEN_FULLSCREEN:
		var->Set(gui.IsOsdFullscreen());
		break;
	case SCRIPT_VAR_SCREEN_LEFT:
		var->Set(gui.left);
		break;
	case SCRIPT_VAR_SCREEN_TOP:
		var->Set(gui.top);
		break;
	case SCRIPT_VAR_SCREEN_RIGHT:
		var->Set(gui.right);
		break;
	case SCRIPT_VAR_SCREEN_BOTTOM:
		var->Set(gui.bottom);
		break;
	case SCRIPT_VAR_SCREEN_PALETTE:
		var->Set(params->pal);
		break;
	case SCRIPT_VAR_SCREEN_PALIDX:
		var->Set(params->pal_idx);
		break;
	case SCRIPT_VAR_SCREEN_PALALPHA:
		{
		BYTE *p = gui.GetPaletteEntry(params->pal_idx);
		var->Set((p != NULL) ? p[0] : 0);
		}
		break;
	case SCRIPT_VAR_SCREEN_PALCOLOR:
		{
		BYTE *p = gui.GetPaletteEntry(params->pal_idx);
		BYTE r, g, b;
		khwl_tvyuvtovgargb(p[1], p[2], p[3], &r, &g, &b);
		var->Set((p != NULL) ? ((r << 16) | (g << 8) | b) : 0);
		}
		break;
	case SCRIPT_VAR_SCREEN_FONT:
		var->Set(params->font);
		break;
	case SCRIPT_VAR_SCREEN_COLOR:
		var->Set(gui.GetColor());
		break;
	case SCRIPT_VAR_SCREEN_BACKCOLOR:
		var->Set(gui.GetBkColor());
		break;
	case SCRIPT_VAR_SCREEN_TRCOLOR:
		var->Set(gui.GetTransparentColor());
		break;
	case SCRIPT_VAR_SCREEN_HALIGN:
		{
			const char *ha[] = { "left", "center", "right" };
			var->Set(ha[gui.GetHAlign()]);
		}
		break;
	case SCRIPT_VAR_SCREEN_VALIGN:
		{
			const char *ha[] = { "top", "center", "bottom" };
			var->Set(ha[gui.GetVAlign()]);
		}
		break;
	case SCRIPT_VAR_SCREEN_BACK:
		var->Set(params->back);
		break;
	case SCRIPT_VAR_SCREEN_BACK_LEFT:
		var->Set(params->backleft);
		break;
	case SCRIPT_VAR_SCREEN_BACK_TOP:
		var->Set(params->backtop);
		break;
	case SCRIPT_VAR_SCREEN_BACK_RIGHT:
		var->Set(params->backright);
		break;
	case SCRIPT_VAR_SCREEN_BACK_BOTTOM:
		var->Set(params->backbottom);
		break;
	case SCRIPT_VAR_SCREEN_PRELOAD:
		var->Set("");
		break;
	case SCRIPT_VAR_SCREEN_HZOOM:
		var->Set(params->hscale);
		break;
	case SCRIPT_VAR_SCREEN_VZOOM:
		var->Set(params->vscale);
		break;
    case SCRIPT_VAR_SCREEN_ROTATE:
		var->Set(params->rotate * 90);
		break;
    case SCRIPT_VAR_SCREEN_HSCROLL:
		var->Set(params->hscroll);
		break;
    case SCRIPT_VAR_SCREEN_VSCROLL:
		var->Set(params->vscroll);
		break;
	case SCRIPT_VAR_SCREEN_BRIGHTNESS:
		{
		int val;
		khwl_get_display_settings(&val, NULL, NULL);
		var->Set(val);
		}
		break;
	case SCRIPT_VAR_SCREEN_CONTRAST:
		{
			int val;
			khwl_get_display_settings(NULL, &val, NULL);
			var->Set(val);
		}
		break;
	case SCRIPT_VAR_SCREEN_SATURATION:
		{
			int val;
			khwl_get_display_settings(NULL, NULL, &val);
			var->Set(val);
		}
		break;
	}
}

void on_screen_set(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	if (var == NULL)
		return;
	switch (var_id)
	{
	case SCRIPT_VAR_SCREEN_SWITCH:
		if (var->GetString().CompareNoCase("next") == 0)
			script_next_vmode();
		else
		{
			bool onoff = var->GetInteger() != 0;
			gui.SwitchDisplay(onoff);
			if (onoff)
			{
				khwl_set_window_frame(params->bigbackleft, params->bigbacktop, params->bigbackright, params->bigbackbottom);
				gui.SetBackground(params->bigback);
			}
		}
		break;
	case SCRIPT_VAR_SCREEN_UPDATE:
		{
			bool upd = false;
			if (var->GetString().CompareNoCase("now") == 0)
			{
				upd = true;
				gui.Update();
			}
			else 
			{
				upd = var->GetInteger() != 0;
				gui.UpdateEnable(upd);
			}
			if (upd && params->need_setwindow)
			{
				khwl_set_window(-1, -1, -1, -1, params->hscale, params->vscale, params->hscroll, params->vscroll);
				params->need_setwindow = false;
			}
		}
		break;
	case SCRIPT_VAR_SCREEN_TVSTANDARD:
		gui.SetTv(WINDOW_TVOUT_UNKNOWN, (WINDOW_TVSTANDARD)StringPair::Get(var, tvstandard_pairs, -1));
		khwl_set_window_frame(params->bigbackleft, params->bigbacktop, params->bigbackright, params->bigbackbottom);
		gui.SetBackground(params->bigback);
		break;
	case SCRIPT_VAR_SCREEN_TVOUT:
		gui.SetTv((WINDOW_TVOUT)StringPair::Get(var, tvout_pairs, -1), WINDOW_TVSTANDARD_UNKNOWN);
		khwl_set_window_frame(params->bigbackleft, params->bigbacktop, params->bigbackright, params->bigbackbottom);
		gui.SetBackground(params->bigback);
		break;
	case SCRIPT_VAR_SCREEN_FULLSCREEN:
		gui.SetOsdFullscreen(var->GetInteger() != 0);
		break;
	case SCRIPT_VAR_SCREEN_PALETTE:
		{
			SPString pal = var->GetString();
			if (gui.LoadPalette(pal))
			{
				params->pal = pal;
				console->UpdateFont();
			}
		}
		break;
	case SCRIPT_VAR_SCREEN_PALIDX:
		params->pal_idx = var->GetInteger();
		break;
	case SCRIPT_VAR_SCREEN_PALALPHA:
		{
		BYTE *p = gui.GetPaletteEntry(params->pal_idx);
		if (p != NULL)
			p[0] = (BYTE)var->GetInteger();
		}
		break;
	case SCRIPT_VAR_SCREEN_PALCOLOR:
		{
		BYTE *p = gui.GetPaletteEntry(params->pal_idx);
		if (p != NULL)
		{
			SPString str = var->GetString();
			str.TrimLeft();
			BYTE r, g, b;
			if (str.GetLength() >= 7 && str[0] == '#')
			{
				r = (BYTE)(hex2char(str[1]) * 16 + hex2char(str[2]));
				g = (BYTE)(hex2char(str[3]) * 16 + hex2char(str[4]));
				b = (BYTE)(hex2char(str[5]) * 16 + hex2char(str[6]));
			} else
			{
				int c24 = var->GetInteger();
				r = (BYTE)((c24 >> 16) & 0xff);
				g = (BYTE)((c24 >> 8) & 0xff);
				b = (BYTE)((c24) & 0xff);
			}
			khwl_vgargbtotvyuv(r, g, b, p + 1, p + 2, p + 3);
		}
		}
		break;
	case SCRIPT_VAR_SCREEN_FONT:
		params->font = var->GetString();
		break;
	case SCRIPT_VAR_SCREEN_COLOR:
		gui.SetColor(var->GetInteger());
		break;
	case SCRIPT_VAR_SCREEN_BACKCOLOR:
		gui.SetBkColor(var->GetInteger());
		break;
	case SCRIPT_VAR_SCREEN_TRCOLOR:
		gui.SetTransparentColor(var->GetInteger());
		console->UpdateFont();
		break;
	case SCRIPT_VAR_SCREEN_HALIGN:
		{
			SPString str = var->GetString();
			if (str.CompareNoCase("center") == 0)
				gui.SetHAlign(WINDOW_ALIGN_CENTER);
			else if (str.CompareNoCase("right") == 0)
				gui.SetHAlign(WINDOW_ALIGN_RIGHT);
			else 
				gui.SetHAlign(WINDOW_ALIGN_LEFT);
		}
		break;
	case SCRIPT_VAR_SCREEN_VALIGN:
		{
			SPString str = var->GetString();
			if (str.CompareNoCase("center") == 0)
				gui.SetHAlign(WINDOW_ALIGN_CENTER);
			else if (str.CompareNoCase("right") == 0)
				gui.SetHAlign(WINDOW_ALIGN_RIGHT);
			else 
				gui.SetHAlign(WINDOW_ALIGN_LEFT);
		}
		break;
	case SCRIPT_VAR_SCREEN_BACK:
		{
			SPString back = var->GetString();
			if (back.CompareNoCase(params->back) != 0)
			{
				params->back = back;
				params->hscale = 100;
				params->vscale = 100;
				params->hscroll = 0;
				params->vscroll = 0;
				params->rotate = -1;

				int bw = params->backright - params->backleft + 1;
				int bh = params->backbottom - params->backtop + 1;
				if (bw >= params->bigback_width || bh >= params->bigback_height || back.GetLength() == 0)
				{
					params->bigback_width = bw;
					params->bigback_height = bh;
					params->bigback = params->back;
					params->bigbackleft = params->backleft;
					params->bigbacktop = params->backtop;
					params->bigbackright = params->backright;
					params->bigbackbottom = params->backbottom;
				}
				
				khwl_set_window_frame(params->backleft, params->backtop, params->backright, params->backbottom);

				int old_rotate = params->rotate;
				if (!gui.SetBackground(params->back, params->hscale, params->vscale, 
								params->hscroll, params->vscroll, &params->rotate))
				{
					params->back = "";
					mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_BACK);
					if (params->rotate != old_rotate)
						mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_ROTATE);
				}
				
				params->need_setwindow = false;
			}
		}
		break;
	case SCRIPT_VAR_SCREEN_BACK_LEFT:
		params->backleft = var->GetInteger();
		if (params->backleft < 0)
		{
			params->backleft = 0;
			mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_BACK_LEFT);
		}
		if (params->backleft > 719)
		{
			params->backleft = 719;
			mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_BACK_LEFT);
		}
		break;
	case SCRIPT_VAR_SCREEN_BACK_TOP:
		params->backtop = var->GetInteger();
		if (params->backtop < 0)
		{
			params->backtop = 0;
			mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_BACK_TOP);
		}
		if (params->backtop > 479)
		{
			params->backtop = 479;
			mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_BACK_TOP);
		}
		break;
	case SCRIPT_VAR_SCREEN_BACK_RIGHT:
		params->backright = var->GetInteger();
		if (params->backright < 0)
		{
			params->backright = 0;
			mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_BACK_RIGHT);
		}
		if (params->backright > 719)
		{
			params->backright = 719;
			mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_BACK_RIGHT);
		}
		break;
	case SCRIPT_VAR_SCREEN_BACK_BOTTOM:
		params->backbottom = var->GetInteger();
		if (params->backbottom < 0)
		{
			params->backbottom = 0;
			mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_BACK_BOTTOM);
		}
		if (params->backbottom > 479)
		{
			params->backbottom = 479;
			mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_BACK_BOTTOM);
		}
		break;
	case SCRIPT_VAR_SCREEN_PRELOAD:
		{
			SPString fname = var->GetString();
			if (fname == "")
			{
				guiimg->ClearImageData();
#if 0 // !!!!!!!!!!!!!!!!!!!
				guifonts->ClearFontData();
#endif
			}
			else
			{
				if (fname.FindNoCase(".fon") > 0 || fname.FindNoCase(".fnt") > 0)
					guifonts->GetFont(fname);
				else if (fname.FindNoCase(".gif") > 0)
					guiimg->GetImageData(fname);
			}
		}
		break;
	case SCRIPT_VAR_SCREEN_HZOOM:
		{
			SPString z = var->GetString();
			if (z.CompareNoCase("in") == 0)
				params->hscale += 5;
			else if (z.CompareNoCase("out") == 0)
				params->hscale -= 5;
			else
				params->hscale = var->GetInteger();
			if (params->hscale < 25)
				params->hscale = 25;
			if (params->hscale > 400)
				params->hscale = 400;
			params->hscroll = 0;
			params->vscroll = 0;
			if (gui.IsUpdateEnabled())
			{
				khwl_set_window(-1, -1, -1, -1, params->hscale, params->vscale, params->hscroll, params->vscroll);
				params->need_setwindow = false;
			}
			else
				params->need_setwindow = true;
		}
		break;
	case SCRIPT_VAR_SCREEN_VZOOM:
		{
			SPString z = var->GetString();
			if (z.CompareNoCase("in") == 0)
				params->vscale += 5;
			else if (z.CompareNoCase("out") == 0)
				params->vscale -= 5;
			else
				params->vscale = var->GetInteger();
			if (params->vscale < 25)
				params->vscale = 25;
			if (params->vscale > 400)
				params->vscale = 400;
			params->hscroll = 0;
			params->vscroll = 0;
			if (gui.IsUpdateEnabled())
			{
				khwl_set_window(-1, -1, -1, -1, params->hscale, params->vscale, params->hscroll, params->vscroll);
				params->need_setwindow = false;
			}
			else
				params->need_setwindow = true;
		}
		break;
    case SCRIPT_VAR_SCREEN_ROTATE:
		{
		if (var->GetString().CompareNoCase("auto") == 0)
			params->rotate = -1;
		else
		{
			params->rotate = var->GetInteger() / 90;
			if (params->rotate < 0) 
				params->rotate = (-params->rotate/4+1)*4+params->rotate;
			params->rotate %= 4;
		}
		params->hscroll = 0;
		params->vscroll = 0;
		int old_rotate = params->rotate;
		gui.SetBackground(params->back, params->hscale, params->vscale, params->hscroll, params->vscroll, &params->rotate);
		if (params->rotate != old_rotate)
			mmsl->UpdateVariable(SCRIPT_VAR_SCREEN_ROTATE);
		}
		break;
    case SCRIPT_VAR_SCREEN_HSCROLL:
		params->hscroll = var->GetInteger();
		if (gui.IsUpdateEnabled())
		{
			khwl_set_window(-1, -1, -1, -1, params->hscale, params->vscale, params->hscroll, params->vscroll);
			params->need_setwindow = false;
		}
		else
			params->need_setwindow = true;
		break;
    case SCRIPT_VAR_SCREEN_VSCROLL:
		params->vscroll = var->GetInteger();
		if (gui.IsUpdateEnabled())
		{
			khwl_set_window(-1, -1, -1, -1, params->hscale, params->vscale, params->hscroll, params->vscroll);
			params->need_setwindow = false;
		}
		else
			params->need_setwindow = true;
		break;
	case SCRIPT_VAR_SCREEN_BRIGHTNESS:
		khwl_set_display_settings(var->GetInteger(), -1, -1);
		break;
	case SCRIPT_VAR_SCREEN_CONTRAST:
		khwl_set_display_settings(-1, var->GetInteger(), -1);
		break;
	case SCRIPT_VAR_SCREEN_SATURATION:
		khwl_set_display_settings(-1, -1, var->GetInteger());
		break;
	}
}

////////////////////////////////////////////////////////////////////////


const char *get_button_string(int but)
{
	static const StringPair *pairs[2] = { front_button_pairs2, button_pairs };
	for (int t = 0; t < 2; t++)
	{
		for (int i = 0; pairs[t][i].str != NULL; i++)
		{
			if (pairs[t][i].value == but)
				return pairs[t][i].str;
		}
	}
	return "?";
}

void on_pad_get(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	if (var == NULL)
		return;
	switch (var_id)
	{
    case SCRIPT_VAR_PAD_KEY:
		var->Set(params->keystr);
		break;
	case SCRIPT_VAR_PAD_DISPLAY:
	case SCRIPT_VAR_PAD_CLEAR:
	case SCRIPT_VAR_PAD_SET:
		var->Set("");
		break;
	}
}

void on_pad_set(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	if (var == NULL)
		return;
	switch (var_id)
	{
    case SCRIPT_VAR_PAD_DISPLAY:
		fip_write_string(var->GetString());
		break;
	case SCRIPT_VAR_PAD_SET:
		fip_write_special(StringPair::Get(var, pad_special_pairs, -1), TRUE);
		break;
	case SCRIPT_VAR_PAD_KEY:
		script_set_key(StringPair::Get(var, button_pairs, 0));
		break;
	case SCRIPT_VAR_PAD_CLEAR:
		{
			SPString str = var->GetString();
			if (str.CompareNoCase("all") == 0)
				fip_clear();
			else
				fip_write_special(StringPair::Get(var, pad_special_pairs, -1), FALSE);
		}
		break;
	}
}

////////////////////////////////////////////////////////////////////////

void on_drive_get(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	if (var == NULL)
		return;
	switch (var_id)
	{
    case SCRIPT_VAR_DRIVE_MEDIATYPE:
		StringPair::Set(var, mediatype_pairs, params->status);
		break;
	case SCRIPT_VAR_DRIVE_TRAY:
		if (params->status == CDROM_STATUS_NODISC || 
				(params->status & CDROM_STATUS_HASDISC) == CDROM_STATUS_HASDISC)
			var->Set("close");
		else
			var->Set("open");
		break;
	}
}

void on_drive_set(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	if (var == NULL)
		return;
	switch (var_id)
	{
    case SCRIPT_VAR_DRIVE_TRAY:
		{
			BOOL force_status = FALSE;
			CDROM_STATUS status = cdrom_getstatus(&force_status);
			SPString str = var->GetString();
			if (str.CompareNoCase("toggle") == 0 ||
				(str.CompareNoCase("open") == 0 && status != CDROM_STATUS_TRAYOPEN) ||
				(str.CompareNoCase("close") == 0 && status == CDROM_STATUS_TRAYOPEN)
				) 
			{
				toggle_tray();
			}
		}
		break;
	case SCRIPT_VAR_DRIVE_MEDIATYPE:
		{
			SPString str = var->GetString();
			if (params->status == CDROM_STATUS_HAS_DVD && str.CompareNoCase("iso") == 0)
			{
				params->require_next_status = (params->saved_iso_status != CDROM_STATUS_UNKNOWN) ? 
												params->saved_iso_status : CDROM_STATUS_HAS_ISO;
				disc_changed(CDROM_STATUS_CURRENT);
			}
			else if ((params->status == CDROM_STATUS_HAS_ISO || params->status == CDROM_STATUS_HAS_MIXED) && str.CompareNoCase("dvd") == 0)
			{
				params->saved_iso_status = params->status;
				params->require_next_status = CDROM_STATUS_HAS_DVD;
				disc_changed(CDROM_STATUS_CURRENT);
			}
			else
			{
				params->saved_iso_status = params->status;
				params->require_next_status = CDROM_STATUS_UNKNOWN;
				disc_changed(CDROM_STATUS_UNKNOWN);
			}
		}
		break;
	}
}

////////////////////////////////////////////////////////////////////////

void on_settings_get(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	if (var == NULL)
		return;
	switch (var_id)
	{
	case SCRIPT_VAR_SETTINGS_COMMAND:
		var->Set("");
		break;
	case SCRIPT_VAR_SETTINGS_AUDIOOUT:
		StringPair::Set(var, audioout_pairs, settings_get(SETTING_AUDIOOUT));
		break;
    case SCRIPT_VAR_SETTINGS_TVTYPE:
		StringPair::Set(var, tvtype_pairs, settings_get(SETTING_TVTYPE));
		break;
    case SCRIPT_VAR_SETTINGS_TVSTANDARD:
		StringPair::Set(var, tvstandard_pairs, settings_get(SETTING_TVSTANDARD));
		break;
    case SCRIPT_VAR_SETTINGS_TVOUT:
		StringPair::Set(var, tvout_pairs, settings_get(SETTING_TVOUT));
		break;
	case SCRIPT_VAR_SETTINGS_DVI:
		var->Set(settings_get(SETTING_DVI));
		break;
	case SCRIPT_VAR_SETTINGS_HQ_JPEG:
		var->Set(settings_get(SETTING_HQ_JPEG));
		break;
	case SCRIPT_VAR_SETTINGS_HDD_SPEED:
		StringPair::Set(var, hddspeed_pairs, settings_get(SETTING_HDD_SPEED));
		break;
	case SCRIPT_VAR_SETTINGS_DVD_PARENTAL:
		var->Set(settings_get(SETTING_DVD_PARENTAL));
		break;
	case SCRIPT_VAR_SETTINGS_DVD_MV:
		var->Set(settings_get(SETTING_DVD_MV));
		break;
	case SCRIPT_VAR_SETTINGS_DVD_LANG_MENU:
	case SCRIPT_VAR_SETTINGS_DVD_LANG_AUDIO:
	case SCRIPT_VAR_SETTINGS_DVD_LANG_SPU:
		{
			char str[3] = { 0 };
			SETTING_SET sset = SETTING_VERSION;
			switch (var_id)
			{
			case SCRIPT_VAR_SETTINGS_DVD_LANG_MENU:
				sset = SETTING_DVD_LANG_MENU;
				break;
			case SCRIPT_VAR_SETTINGS_DVD_LANG_AUDIO:
				sset = SETTING_DVD_LANG_AUDIO;
				break;
			default:
				sset = SETTING_DVD_LANG_SPU;
			}
			int v = settings_get(sset);
			if (v > 0)
			{
				str[0] = (char)((v >> 8) & 0xff);
				str[1] = (char)((v) & 0xff);
				var->Set(str);
			} else
				var->Set("");
		}
		break;
	case SCRIPT_VAR_SETTINGS_VOLUME:
		var->Set(settings_get(SETTING_VOLUME));
		break;
	case SCRIPT_VAR_SETTINGS_BALANCE:
		var->Set(settings_get(SETTING_BALANCE) - 100);
		break;
	case SCRIPT_VAR_SETTINGS_BRIGHTNESS:
		var->Set(GetScreenAdjustmentSetting(0));
		break;
	case SCRIPT_VAR_SETTINGS_CONTRAST:
		var->Set(GetScreenAdjustmentSetting(1));
		break;
	case SCRIPT_VAR_SETTINGS_SATURATION:
		var->Set(GetScreenAdjustmentSetting(2));
		break;
	case SCRIPT_VAR_SETTINGS_USER1:
	case SCRIPT_VAR_SETTINGS_USER2:
	case SCRIPT_VAR_SETTINGS_USER3:
	case SCRIPT_VAR_SETTINGS_USER4:
	case SCRIPT_VAR_SETTINGS_USER5:
	case SCRIPT_VAR_SETTINGS_USER6:
	case SCRIPT_VAR_SETTINGS_USER7:
	case SCRIPT_VAR_SETTINGS_USER8:
	case SCRIPT_VAR_SETTINGS_USER9:
	case SCRIPT_VAR_SETTINGS_USER10:
	case SCRIPT_VAR_SETTINGS_USER11:
	case SCRIPT_VAR_SETTINGS_USER12:
	case SCRIPT_VAR_SETTINGS_USER13:
	case SCRIPT_VAR_SETTINGS_USER14:
	case SCRIPT_VAR_SETTINGS_USER15:
	case SCRIPT_VAR_SETTINGS_USER16:
		{
			SETTING_SET idx = (SETTING_SET)(SETTING_GUI_0 + var_id - SCRIPT_VAR_SETTINGS_USER1);
			var->Set(settings_get(idx));
		}
		break;
	}
}

void on_settings_set(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	if (var == NULL)
		return;
	switch (var_id)
	{
	case SCRIPT_VAR_SETTINGS_COMMAND:
		{
			SPString cmd = var->GetString();
			if (cmd.CompareNoCase("defaults") == 0)
			{
				settings_init(true);
				for (int i = SCRIPT_VAR_SETTINGS_AUDIOOUT; i <= SCRIPT_VAR_SETTINGS_USER16; i++)
					mmsl->UpdateVariable(i);
			}
		}
		break;
	case SCRIPT_VAR_SETTINGS_AUDIOOUT:
		settings_set(SETTING_AUDIOOUT, StringPair::Get(var, audioout_pairs, 0));
		break;
	case SCRIPT_VAR_SETTINGS_TVTYPE:
		settings_set(SETTING_TVTYPE, StringPair::Get(var, tvtype_pairs, 0));
		break;
    case SCRIPT_VAR_SETTINGS_TVSTANDARD:
		settings_set(SETTING_TVSTANDARD, StringPair::Get(var, tvstandard_pairs, 1));
		break;
    case SCRIPT_VAR_SETTINGS_TVOUT:
		settings_set(SETTING_TVOUT, StringPair::Get(var, tvout_pairs, 0));
		break;
	case SCRIPT_VAR_SETTINGS_DVI:
		settings_set(SETTING_DVI, var->GetInteger());
		break;
	case SCRIPT_VAR_SETTINGS_HQ_JPEG:
		{
			int val = var->GetInteger();
			val %= 2;
			settings_set(SETTING_HQ_JPEG, val);
		}
		break;
	case SCRIPT_VAR_SETTINGS_HDD_SPEED:
		if (var->GetString().CompareNoCase("next") == 0)
		{
			int val = settings_get(SETTING_HDD_SPEED);
			val = (val + 1) % (settings_getmax(SETTING_HDD_SPEED) + 1);
			settings_set(SETTING_HDD_SPEED, val);
			break;
		}
		settings_set(SETTING_HDD_SPEED, StringPair::Get(var, hddspeed_pairs, 0));
		break;
	case SCRIPT_VAR_SETTINGS_DVD_PARENTAL:
		settings_set(SETTING_DVD_PARENTAL, var->GetInteger());
		break;
	case SCRIPT_VAR_SETTINGS_DVD_MV:
		settings_set(SETTING_DVD_MV, var->GetInteger());
		break;
	case SCRIPT_VAR_SETTINGS_DVD_LANG_MENU:
	case SCRIPT_VAR_SETTINGS_DVD_LANG_AUDIO:
	case SCRIPT_VAR_SETTINGS_DVD_LANG_SPU:
		{
			SETTING_SET sset = SETTING_VERSION;
			switch (var_id)
			{
			case SCRIPT_VAR_SETTINGS_DVD_LANG_MENU:
				sset = SETTING_DVD_LANG_MENU;
				break;
			case SCRIPT_VAR_SETTINGS_DVD_LANG_AUDIO:
				sset = SETTING_DVD_LANG_AUDIO;
				break;
			default:
				sset = SETTING_DVD_LANG_SPU;
			}
			SPString lang = var->GetString();
			if (isalpha(lang[0]) && isalpha(lang[1]))
			{
				settings_set(sset, (lang[0] << 8) | lang[1]);
			}
		}
		break;
	case SCRIPT_VAR_SETTINGS_VOLUME:
		settings_set(SETTING_VOLUME, var->GetInteger());
		break;
	case SCRIPT_VAR_SETTINGS_BALANCE:
		settings_set(SETTING_BALANCE, var->GetInteger() + 100);
		break;
	case SCRIPT_VAR_SETTINGS_BRIGHTNESS:
		SetScreenAdjustmentSetting(0, var->GetInteger());
		break;
	case SCRIPT_VAR_SETTINGS_CONTRAST:
		SetScreenAdjustmentSetting(1, var->GetInteger());
		break;
	case SCRIPT_VAR_SETTINGS_SATURATION:
		SetScreenAdjustmentSetting(2, var->GetInteger());
		break;
	case SCRIPT_VAR_SETTINGS_USER1:
	case SCRIPT_VAR_SETTINGS_USER2:
	case SCRIPT_VAR_SETTINGS_USER3:
	case SCRIPT_VAR_SETTINGS_USER4:
	case SCRIPT_VAR_SETTINGS_USER5:
	case SCRIPT_VAR_SETTINGS_USER6:
	case SCRIPT_VAR_SETTINGS_USER7:
	case SCRIPT_VAR_SETTINGS_USER8:
	case SCRIPT_VAR_SETTINGS_USER9:
	case SCRIPT_VAR_SETTINGS_USER10:
	case SCRIPT_VAR_SETTINGS_USER11:
	case SCRIPT_VAR_SETTINGS_USER12:
	case SCRIPT_VAR_SETTINGS_USER13:
	case SCRIPT_VAR_SETTINGS_USER14:
	case SCRIPT_VAR_SETTINGS_USER15:
	case SCRIPT_VAR_SETTINGS_USER16:
		{
			SETTING_SET idx = (SETTING_SET)(SETTING_GUI_0 + var_id - SCRIPT_VAR_SETTINGS_USER1);
			settings_set(idx, var->GetInteger());
		}
		break;

	}
}

////////////////////////////////////////////////////////////////////////

void on_flash_get(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	if (var == NULL)
		return;
	switch (var_id)
	{
	case SCRIPT_VAR_FLASH_FILE:
		var->Set(params->flash_file);
		break;
    case SCRIPT_VAR_FLASH_ADDRESS:
		var->Set(params->flash_address < 0 ? 0 : params->flash_address);
		break;
    case SCRIPT_VAR_FLASH_PROGRESS:
		var->Set(params->flash_progress);
		break;
	}
}

void on_flash_set(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	if (var == NULL)
		return;
	switch (var_id)
	{
	case SCRIPT_VAR_FLASH_FILE:
		params->flash_file = var->GetString();
		break;
    case SCRIPT_VAR_FLASH_ADDRESS:
		params->flash_address = var->GetInteger();
		break;
	}

	if (var_id == SCRIPT_VAR_FLASH_FILE || var_id == SCRIPT_VAR_FLASH_ADDRESS)
	{
		if (params->flash_file != "" && params->flash_address <= 0)
		{
			params->flash_address = get_firmware_address();
		}
		if (params->flash_file != "" && params->flash_address > 0)
		{
			params->flash_progress = -1;
			params->flash_p = -1;
			msg("FLASH %s at 0x%x...\n", *params->flash_file, params->flash_address);
			
			stop_all();
			cdda_close();

			int ret = flash_file(params->flash_file, params->flash_address);
			if (ret < 0)
			{
				msg_error("Cannot flash file %s at address 0x%x (err=%d).\n", *params->flash_file,
							params->flash_address, ret);
				mmsl->UpdateVariable(SCRIPT_VAR_FLASH_PROGRESS);
			}
			else
				params->flash_progress = 0;
		}

	}
}

