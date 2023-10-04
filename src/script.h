//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - script wrapper header file
 *  \file       script.h
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

#ifndef SP_SCRIPT_H
#define SP_SCRIPT_H

#include <mmsl/mmsl.h>

////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

enum SCRIPT_ERROR_CODE
{
	SCRIPT_ERROR_INVALID = 0,
	SCRIPT_ERROR_BAD_AUDIO,
	SCRIPT_ERROR_BAD_CODEC,
	SCRIPT_ERROR_QPEL,
	SCRIPT_ERROR_GMC,
	SCRIPT_ERROR_WAIT,
	SCRIPT_ERROR_CORRUPTED,
};

/// General program cycle
int cycle(bool gfx_only = false);

/// Initialize script variables & objects
int script_init();

/// Deinit script defs.
int script_deinit();

/// Update script data (call triggers if something changed)
int script_update(bool gfx_only = false);

/// Get the current time elapsed since script was started, in milliseconds.
int get_curtime();

/// Skip timer for all time elapsed since the last call of script_update(). Used for sleeping.
void script_skiptime();

bool script_update_variable(int ID);

/// 'ison' is the next status
bool player_turn_onoff(bool on);
void player_halt();

bool script_next_vmode();

void script_key_callback(int key);
void script_drive_callback(CDROM_STATUS);

void script_time_callback(int secs);
void script_totaltime_callback(int secs);

void script_error_callback(SCRIPT_ERROR_CODE);

void script_name_callback(const SPString &);
void script_artist_callback(const SPString &);
void script_audio_info_callback(const SPString &);
void script_video_info_callback(const SPString &);
void script_framesize_callback(int width, int height);
void script_framerate_callback(int frame_rate);
void script_zoom_scroll_reset_callback();
void script_colorspace_callback(int clrs);
void script_speed_callback(int speed = -1);
void script_player_saved_callback();

void script_player_subtitle_callback(const char *);

void script_dvd_menu_callback(bool ismenu);
void script_dvd_chapter_callback(int chapter);
void script_dvd_title_callback(int title, int numchapters);
void script_cdda_track_callback(int track);
void script_audio_lang_callback(const char *lang);
void script_spu_lang_callback(const char *lang);
void script_audio_stream_callback(int stream);
void script_spu_stream_callback(int stream);

const char *get_button_string(int);
bool stop_all();
bool toggle_tray();
bool start_iso();
bool start_dvd();
bool stop_dvd();
void disc_changed(CDROM_STATUS = CDROM_STATUS_CURRENT);
bool is_internal_playing();
bool is_playing();

/// Get current time (if tv == NULL) or delta-time, in msec.
int script_get_time(struct timeval *tv);

void player_update_source(char *filepath);

////////////////////////////////////////////////////

enum SCRIPT_TIMER_OBJECT_TYPE
{
	SCRIPT_OBJECT_UPDATE = 0,
		SCRIPT_OBJECT_TIMER,
};

class ScriptTimerObject
{
public:
	/// ctor
	ScriptTimerObject()
	{
		prev = next = NULL;
	}
	
public:
	MMSL_OBJECT obj;
	int var_ID;
	SCRIPT_TIMER_OBJECT_TYPE type;
	
	ScriptTimerObject *prev, *next;
};


#ifdef __cplusplus
}
#endif

#endif // of SP_SCRIPT_H
