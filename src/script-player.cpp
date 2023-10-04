//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - media player wrapper impl.
 *  \file       script-player.cpp
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
#include <ctype.h>
#include <dirent.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_cdrom.h>
#include <libsp/sp_flash.h>
#include <libsp/sp_fip.h>
#include <libsp/sp_khwl.h>

#include <gui/rect.h>
#include <gui/image.h>
#include <gui/text.h>
#include <gui/console.h>
#include <mmsl/mmsl.h>

#include <module.h>
#include <script-internal.h>


static const StringPair player_repeat_pairs[] = 
{ 
	{ "none", PLAYER_REPEAT_NONE },
	{ "selection", PLAYER_REPEAT_SELECTION },
	{ "track", PLAYER_REPEAT_TRACK },
	{ "all", PLAYER_REPEAT_ALL },
	{ "random", PLAYER_REPEAT_RANDOM },
	{ NULL, -1 },
};

static const StringPair player_menu_pairs[] = 
{ 
	{ "0", DVD_MENU_ESCAPE },
	{ "1", DVD_MENU_ROOT },
	{ "escape", DVD_MENU_ESCAPE },
	{ "title", DVD_MENU_TITLE },
	{ "root", DVD_MENU_ROOT },
	{ "subtitle", DVD_MENU_SUBTITLE },
	{ "audio", DVD_MENU_AUDIO },
	{ "angle", DVD_MENU_ANGLE },
	{ "chapters", DVD_MENU_CHAPTERS },
	{ NULL, -1 },
};

static const StringPair player_clrs_pairs[] = 
{ 
	{ "", PLAYER_COLOR_SPACE_UNKNOWN },
	{ "YCrCb", PLAYER_COLOR_SPACE_YCRCB },
	{ "Grayscale", PLAYER_COLOR_SPACE_GRAYSCALE },
	{ NULL, -1 },
};

static const StringPair player_charset_pairs[] = 
{ 
	{ "", SUBTITLE_CHARSET_DEFAULT },
	{ "cp1251", SUBTITLE_CHARSET_CP1251 },
	{ "iso8859-1", SUBTITLE_CHARSET_ISO8859_1 },
	{ "iso8859-2", SUBTITLE_CHARSET_ISO8859_2 },
	{ "koi8-r", SUBTITLE_CHARSET_KOI8R },
	{ NULL, -1 },
};

static const StringPair player_speed_pairs[] = 
{ 
	{ "0", MPEG_SPEED_PAUSE, },
	{ "4", MPEG_SPEED_FWD_4X, },
	{ "8", MPEG_SPEED_FWD_8X, },
	{ "16", MPEG_SPEED_FWD_16X, },
	{ "32", MPEG_SPEED_FWD_32X, },
	{ "48", MPEG_SPEED_FWD_48X, },
	{ "100", MPEG_SPEED_FWD_MAX, },
	{ "-4", MPEG_SPEED_REV_4X, },
	{ "-8", MPEG_SPEED_REV_8X, },
	{ "-16", MPEG_SPEED_REV_16X, },
	{ "-32", MPEG_SPEED_REV_32X, },
	{ "-48", MPEG_SPEED_REV_48X, },
	{ "-100", MPEG_SPEED_REV_MAX, },
	{ "1/2", MPEG_SPEED_SLOW_FWD_2X, },
	{ "1/4", MPEG_SPEED_SLOW_FWD_4X, },
	{ "1/8", MPEG_SPEED_SLOW_FWD_8X, },
	{ "-1/2", MPEG_SPEED_SLOW_REV_2X, },
	{ "-1/4", MPEG_SPEED_SLOW_REV_4X, },
	{ "-1/8", MPEG_SPEED_SLOW_REV_8X, },
	{ NULL, -1 },
};

////////////////////////////////////////////////

int get_volume()
{
	DWORD lvol = 0, rvol = 0;
	khwl_getproperty(KHWL_AUDIO_SET, eaVolumeLeft, sizeof(lvol), &lvol);
	khwl_getproperty(KHWL_AUDIO_SET, eaVolumeRight, sizeof(rvol), &rvol);
	return MAX(lvol, rvol);
}

int get_balance()
{
	DWORD lvol = 0, rvol = 0;
	khwl_getproperty(KHWL_AUDIO_SET, eaVolumeLeft, sizeof(lvol), &lvol);
	khwl_getproperty(KHWL_AUDIO_SET, eaVolumeRight, sizeof(rvol), &rvol);
	int volume = MAX(lvol, rvol);
	if (volume == 0)
		return 0;
	int balance = 100 * MIN(lvol, rvol) / volume - 100;
	return (lvol < rvol) ? balance : -balance;
}

int get_audio_offset()
{
	int audio_offs;
	//khwl_getproperty(KHWL_COMMON_SET, eDoAudioLater, sizeof(audio_offs), &audio_offs);
	audio_offs = (int)(video_get_audio_offset() / INT64(90));
	return audio_offs;
}


void set_volume(int vol)
{
	if (vol < 0)
		vol = 0;
	if (vol > 100)
		vol = 100;
	int balance = get_balance();
	int lvol = balance <= 0 ? vol : (100-vol) * balance / 100;
	int rvol = balance >= 0 ? vol : (vol-100) * balance / 100;
	khwl_setproperty(KHWL_AUDIO_SET, eaVolumeLeft, sizeof(lvol), &lvol);
	khwl_setproperty(KHWL_AUDIO_SET, eaVolumeRight, sizeof(rvol), &rvol);
	msg("Volume set to (%d,%d).\n", lvol, rvol);
}

void set_balance(int balance)
{
	if (balance < -100)
		balance = -100;
	if (balance > 100)
		balance = 100;
	int vol = get_volume();
	int lvol = balance <= 0 ? vol : (100-vol) * balance / 100;
	int rvol = balance >= 0 ? vol : (vol-100) * balance / 100;
	khwl_setproperty(KHWL_AUDIO_SET, eaVolumeLeft, sizeof(lvol), &lvol);
	khwl_setproperty(KHWL_AUDIO_SET, eaVolumeRight, sizeof(rvol), &rvol);
	msg("Volume set to (%d,%d).\n", lvol, rvol);
}

void set_audio_offset(int audio_offs)
{
	if (audio_offs < -5000)
		audio_offs = -5000;
	if (audio_offs > 5000)
		audio_offs = 5000;
	audio_offs *= 90;
	//khwl_setproperty(KHWL_COMMON_SET, eDoAudioLater, sizeof(audio_offs), &audio_offs);
	video_set_audio_offset(audio_offs);
	msg("Audio offset set to %d PTS.\n", audio_offs);
}

///////////////////////////////////////////////////////////

bool player_dvd_command(const SPString & command)
{
	if (command.CompareNoCase("play") == 0)
	{
		params->player_error = "";

		if (module_load("dvd") < 0)
			return false;

		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_ERROR);

		gui_update();
		
		if (params->player_source.FindNoCase("/dvd") == 0 &&
			params->status != CDROM_STATUS_HAS_DVD)
		{
			module_unload("dvd");
			return false;
		}

		if (params->dvdplaying)
		{
			// if user changed time
			if (params->info.cur_time_changed)
			{
				params->info.cur_time_changed = false;
				dvd_seek(params->info.cur_time);
				return true;
			}
			// if user changed title or chapter:
			if (params->info.cur_changed)
			{
				params->info.cur_changed = false;
				if (!dvd_seek_titlepart(params->info.cur_title, params->info.cur_chapter))
				{
					dvd_get_cur(&params->info.cur_title, &params->info.cur_chapter);
					params->info.num_chapters = dvd_getnumchapters(params->info.cur_title);
				}
				return true;
			}
			
			// restore normal play
			dvd_button_play();
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
		// first time - start DVD...
		dvd_setdeflang_menu(params->info.dvd_menu_lang);
		dvd_setdeflang_audio(params->info.dvd_audio_lang);
		dvd_setdeflang_spu(params->info.spu_lang);

		const char *source;
		bool play_from_drive = true;
		if (params->player_source.FindNoCase("/dvd") != 0 && 
			params->player_source.FindNoCase("/cdrom/video_ts/") != 0)
		{
			source = cdrom_getdevicepath(*params->player_folder);
			play_from_drive = false;
		} else
			source = cdrom_getdevicepath(NULL);
		int ret = dvd_play(source, play_from_drive);
		if (ret < 0)
		{
			// Failure - try DVD disc as ISO...
			params->require_next_status = CDROM_STATUS_HAS_ISO;
			return true;
		}
		
		// all went OK!
		params->dvdplaying = TRUE;
		params->info.num_titles = dvd_getnumtitles();
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
		return true;
	}
	else if (command.CompareNoCase("stop") == 0)
	{
		if (params->dvdplaying)
		{
			dvd_stop();
			params->dvdplaying = FALSE;
			if (mmsl != NULL)
			{
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
			}
			module_unload("dvd");
			return true;
		}
	}
	else if (command.CompareNoCase("cancel") == 0)
	{
		if (params->dvdplaying)
		{
			params->info.cur_chapter = params->info.real_cur_chapter;
			params->info.cur_title = params->info.real_cur_title;
			params->info.cur_time = params->info.real_cur_time;
			params->info.cur_changed = false;
			params->info.cur_time_changed = false;
			return true;
		}
	}
	else if (params->dvdplaying)
	{
		return dvd_do_command(command);
	}
	return false;
}
			
#ifdef EXTERNAL_PLAYER
bool player_file_command(const SPString & command)
{
	if (command.CompareNoCase("play") == 0)
	{
		params->player_error = "";
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_ERROR);
		gui_update();
		
		if (params->status != CDROM_STATUS_HAS_ISO && params->status != CDROM_STATUS_HAS_MIXED)
			return false;
		if (params->fileplaying)
		{
			// if user changed time
			if (params->info.cur_time_changed)
			{
				params->info.cur_time_changed = false;
				player_seek(params->info.cur_time);
				params->speed = MPEG_SPEED_NORMAL;
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
				return true;
			}
			
			// restore normal play
			params->speed = MPEG_SPEED_NORMAL;
			player_play();
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
		params->fileplaying = FALSE;
		if (player_play(params->player_source) >= 0)
		{
			// all went OK!
			params->fileplaying = TRUE;
			params->speed = MPEG_SPEED_NORMAL;
		}
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
		return true;
	}
	else if (command.CompareNoCase("info") == 0)
	{
		params->waitinfo = TRUE;
		player_startinfo(params->player_source, params->iso_lang);
	}
	else if (command.CompareNoCase("stop") == 0)
	{
		if (params->fileplaying)
		{
			player_stop();
			params->fileplaying = FALSE;
			params->speed = MPEG_SPEED_STOP;
			if (mmsl != NULL)
			{
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
			}
			return true;
		}
	}
  	else if (command.CompareNoCase("pause") == 0)
	{
		if (params->fileplaying)
		{
			player_pause();
			params->speed = MPEG_SPEED_PAUSE;
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
	}
  	else if (command.CompareNoCase("forward") == 0)
	{
		if (params->fileplaying)
		{
			if (player_forward() == 1)
			{
				params->speed = MPEG_SPEED_FWD_4X;
			} else
				params->speed = MPEG_SPEED_NORMAL;
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
	}
  	else if (command.CompareNoCase("rewind") == 0)
	{
		if (params->fileplaying)
		{
			if (player_rewind() == 1)
			{
				params->speed = MPEG_SPEED_REV_4X;
			} else
				params->speed = MPEG_SPEED_NORMAL;
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
	}
	return false;
}
#endif

#ifdef INTERNAL_VIDEO_PLAYER
bool player_video_command(const SPString & command)
{
	if (command.CompareNoCase("play") == 0)
	{
		params->player_error = "";
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_ERROR);
		gui_update();
		
		if (params->status != CDROM_STATUS_HAS_ISO && params->status != CDROM_STATUS_HAS_MIXED)
			return false;
		if (params->videoplaying)
		{
			// if user changed time
			if (params->info.cur_time_changed)
			{
				params->info.cur_time_changed = false;
				video_seek(params->info.cur_time);
				//return true;
			}
			
			// restore normal play
			params->speed = MPEG_SPEED_NORMAL;
			video_play();
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
		params->videoplaying = FALSE;
		if (video_play(params->player_source) >= 0)
		{
			// all went OK!
			params->videoplaying = TRUE;
			params->speed = MPEG_SPEED_NORMAL;
		}
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
		return true;
	}
	else if (command.CompareNoCase("stop") == 0)
	{
		if (params->videoplaying)
		{
			if (video_stop())
			{
				params->videoplaying = FALSE;
				params->speed = MPEG_SPEED_STOP;
				if (mmsl != NULL)
				{
					mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
					mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
				}
				return true;
			}
			return false;
		}
	}
  	else if (command.CompareNoCase("pause") == 0)
	{
		if (params->videoplaying)
		{
			video_pause();
			params->speed = MPEG_SPEED_PAUSE;
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
	}
  	else if (command.CompareNoCase("forward") == 0 || command.CompareNoCase("rewind") == 0)
	{
		if (params->videoplaying)
		{
			bool fwd = command.CompareNoCase("forward") == 0;
			if (params->speed == MPEG_SPEED_FWD_4X)
				params->speed = fwd ? MPEG_SPEED_FWD_MAX : MPEG_SPEED_REV_4X;
			else if (params->speed == MPEG_SPEED_REV_4X)
				params->speed = fwd ? MPEG_SPEED_FWD_4X : MPEG_SPEED_REV_MAX;
			else if (params->speed == MPEG_SPEED_FWD_MAX)
				params->speed = fwd ? MPEG_SPEED_FWD_MAX : MPEG_SPEED_FWD_4X;
			else if (params->speed == MPEG_SPEED_REV_MAX)
				params->speed = fwd ? MPEG_SPEED_REV_4X : MPEG_SPEED_REV_MAX;
			else if (params->speed == MPEG_SPEED_NORMAL)
				params->speed = fwd ? MPEG_SPEED_FWD_4X : MPEG_SPEED_REV_4X;
			
			if (params->speed & MPEG_SPEED_FWD_MASK)
			{
				if (video_forward(params->speed == MPEG_SPEED_FWD_MAX) != 1)
					params->speed = MPEG_SPEED_NORMAL;
			} else
			{
				if (video_rewind(params->speed == MPEG_SPEED_REV_MAX) != 1)
					params->speed = MPEG_SPEED_NORMAL;
			}
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
	}
	else if (command.CompareNoCase("audio") == 0)
	{
		if (params->videoplaying)
		{
			video_set_audio_track(-1); 
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_LANGUAGE_AUDIO);
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_AUDIO_STREAM);
			return true;
		}
	}
	else if (command.CompareNoCase("subtitle") == 0)
	{
		if (params->videoplaying)
		{
			if (!subtitle_next())
				script_error_callback(SCRIPT_ERROR_INVALID);
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_LANGUAGE_SUBTITLE);
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SUBTITLE_STREAM);
			return true;
		} else
			script_error_callback(SCRIPT_ERROR_INVALID);
		return true;
	}
	else if (command.CompareNoCase("angle") == 0)
	{
		script_error_callback(SCRIPT_ERROR_INVALID);
		return true;
	}
	return false;
}
#endif

#ifdef INTERNAL_AUDIO_PLAYER
bool player_audio_command(const SPString & command)
{
	if (command.CompareNoCase("play") == 0)
	{
		params->player_error = "";
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_ERROR);
		gui_update();
		
		if (params->status != CDROM_STATUS_HAS_ISO && params->status != CDROM_STATUS_HAS_MIXED)
			return false;
		if (params->audioplaying)
		{
			// if user changed time
			if (params->info.cur_time_changed)
			{
				params->info.cur_time_changed = false;
				audio_seek(params->info.cur_time, SEEK_SET);
				params->speed = MPEG_SPEED_NORMAL;
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
				return true;
			}
			
			// restore normal play
			params->speed = MPEG_SPEED_NORMAL;
			audio_play();
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
		params->audioplaying = FALSE;
		if (audio_play(params->player_source, params->player_type == PLAYER_TYPE_FOLDER) >= 0)
		{
			// all went OK!
			params->audioplaying = TRUE;
			params->speed = MPEG_SPEED_NORMAL;
		}
		params->player_type = PLAYER_TYPE_AUDIO;
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
		return true;
	}
	else if (command.CompareNoCase("info") == 0)
	{
		params->waitinfo = TRUE;
		if (params->player_type != PLAYER_TYPE_FOLDER)
			player_startinfo(params->player_source, params->iso_lang);
	}
	else if (command.CompareNoCase("stop") == 0)
	{
		if (params->audioplaying)
		{
			audio_delete_filelist();
			audio_stop();
			params->audioplaying = FALSE;
			params->speed = MPEG_SPEED_STOP;
			if (mmsl != NULL)
			{
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
			}
			return true;
		}
	}
	else if (command.CompareNoCase("next") == 0)
	{
		if (params->audioplaying)
		{
			if (audio_stop())
			{
				audio_delete_filelist();

				params->audioplaying = FALSE;
				params->speed = MPEG_SPEED_STOP;
				if (mmsl != NULL)
				{
					mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
					mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
				}
			}
			return true;
		}
	}
	else if (command.CompareNoCase("prev") == 0)
	{
		if (params->audioplaying)
		{
			if (audio_stop(FALSE))	// 'prev'
			{
				audio_delete_filelist();

				params->audioplaying = FALSE;
				params->speed = MPEG_SPEED_STOP;
				if (mmsl != NULL)
				{
					mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
					mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
				}
			}
			return true;
		}
	}
  	else if (command.CompareNoCase("pause") == 0)
	{
		if (params->audioplaying)
		{
			audio_pause();
			params->speed = MPEG_SPEED_PAUSE;
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
	}
  	else if (command.CompareNoCase("forward") == 0)
	{
		if (params->audioplaying)
		{
			if (audio_forward() == 1)
			{
				params->speed = MPEG_SPEED_FWD_4X;
			} else
				params->speed = MPEG_SPEED_NORMAL;
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
	}
  	else if (command.CompareNoCase("rewind") == 0)
	{
		if (params->audioplaying)
		{
			if (audio_rewind() == 1)
			{
				params->speed = MPEG_SPEED_REV_4X;
			} else
				params->speed = MPEG_SPEED_NORMAL;
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
	}
	return false;
}
#endif

bool player_cdda_command(const SPString & command)
{
	if (command.CompareNoCase("play") == 0)
	{
		params->player_error = "";
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_ERROR);
		gui_update();
		
		if (params->status != CDROM_STATUS_HAS_AUDIO && params->status != CDROM_STATUS_HAS_MIXED)
			return false;
		if (params->cddaplaying)
		{
			// if user changed time
			if (params->info.cur_time_changed)
			{
				params->info.cur_time_changed = false;
				cdda_seek(params->info.cur_time);
				//return true;
			}
			
			// restore normal play
			params->speed = MPEG_SPEED_NORMAL;
			cdda_play(NULL);
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
		params->cddaplaying = FALSE;
		if (cdda_play(params->player_source) >= 0)
		{
			// all went OK!
			params->cddaplaying = TRUE;
			params->speed = MPEG_SPEED_NORMAL;
		}
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
		return true;
	}
	else if (command.CompareNoCase("info") == 0)
	{
		params->info.length = cdda_get_length(params->player_source);
		params->info.num_titles = cdda_get_numtracks();
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_LENGTH);
		mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_NUM_TITLES);
	}
	else if (command.CompareNoCase("stop") == 0)
	{
		if (params->cddaplaying)
		{
			cdda_stop();
			params->cddaplaying = FALSE;
			params->speed = MPEG_SPEED_STOP;
			if (mmsl != NULL)
			{
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
			}
			return true;
		}
	}
  	else if (command.CompareNoCase("pause") == 0)
	{
		if (params->cddaplaying)
		{
			cdda_pause();
			params->speed = MPEG_SPEED_PAUSE;
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
	}
  	else if (command.CompareNoCase("forward") == 0)
	{
		if (params->cddaplaying)
		{
			cdda_forward();
			params->speed = MPEG_SPEED_NORMAL;
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
	}
  	else if (command.CompareNoCase("rewind") == 0)
	{
		if (params->cddaplaying)
		{
			cdda_rewind();
			params->speed = MPEG_SPEED_NORMAL;
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SPEED);
			return true;
		}
	}
	return false;
}


bool is_internal_playing()
{
	return params->dvdplaying == TRUE || params->cddaplaying == TRUE 
		|| params->videoplaying == TRUE || params->audioplaying == TRUE;
}

bool is_playing()
{
	return params->dvdplaying == TRUE || params->fileplaying == TRUE 
		|| params->videoplaying == TRUE || params->audioplaying == TRUE 
		|| params->cddaplaying == TRUE;
}

void player_update_source(char *filepath)
{
	params->player_source = filepath;
	mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_SOURCE);
}

void player_do_command()
{
	SPString & cmd = params->player_command;

	if (params->player_type == PLAYER_TYPE_DVD)
	{
		player_dvd_command(cmd);
	}
#ifdef EXTERNAL_PLAYER
	else if (params->player_type == PLAYER_TYPE_FILE)
	{
		if (!cdrom_ismounted())
			cdrom_mount(params->iso_lang, FALSE);
		player_file_command(cmd);
	}
#else
	else if (params->player_type == PLAYER_TYPE_FILE)
	{
		if (cmd.CompareNoCase("info") == 0)
		{
			params->waitinfo = TRUE;
			player_startinfo(params->player_source, params->iso_lang);
		}
	}
#endif
#ifdef INTERNAL_VIDEO_PLAYER
	else if (params->player_type == PLAYER_TYPE_VIDEO)
	{
		if (!cdrom_ismounted())
			cdrom_mount(params->iso_lang, FALSE);
		player_video_command(cmd);
	}
#endif
#ifdef INTERNAL_AUDIO_PLAYER
	else if (params->player_type == PLAYER_TYPE_AUDIO || 
		params->player_type == PLAYER_TYPE_FOLDER)
	{
		if (!cdrom_ismounted())
			cdrom_mount(params->iso_lang, FALSE);
		player_audio_command(cmd);
	}
#endif
	else if (params->player_type == PLAYER_TYPE_AUDIOCD)
	{
		player_cdda_command(cmd);
	}
	else
	{
		// undo playing
		if (cmd == "play")
		{
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_PLAYING);
		}
	}
}


////////////////////////////////////////////////////////////////////////

void on_player_get(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	if (var == NULL)
		return;
	switch (var_id)
	{
	case SCRIPT_VAR_PLAYER_SOURCE:
		var->Set(params->player_source);
		break;
    case SCRIPT_VAR_PLAYER_COMMAND:
	case SCRIPT_VAR_PLAYER_SELECT:
		var->Set("");
		break;
	case SCRIPT_VAR_PLAYER_PLAYING:
		{
			if (params->player_type == PLAYER_TYPE_DVD)
				var->Set(params->dvdplaying);
			else if (params->player_type == PLAYER_TYPE_FILE)
				var->Set(params->fileplaying);
			else if (params->player_type == PLAYER_TYPE_VIDEO)
				var->Set(params->videoplaying);
			else if (params->player_type == PLAYER_TYPE_AUDIO)
				var->Set(params->audioplaying);
			else if (params->player_type == PLAYER_TYPE_AUDIOCD)
				var->Set(params->cddaplaying);
			else
				var->Set(0);
		}
		break;
	case SCRIPT_VAR_PLAYER_SAVED:
		if (params->player_type == PLAYER_TYPE_DVD)
		{
			if (module_load("dvd") < 0)
			{
				var->Set(0);
				return;
			}
			var->Set(dvd_get_saved() ? 1 : 0);
		}
		else
			var->Set(0);
		break;
    case SCRIPT_VAR_PLAYER_REPEAT:
		StringPair::Set(var, player_repeat_pairs, params->player_repeat);
		break;
    case SCRIPT_VAR_PLAYER_SPEED:
		{
			MPEG_SPEED_TYPE speed = MPEG_SPEED_STOP;
			if (params->player_type == PLAYER_TYPE_DVD)
			{
				if (!params->dvdplaying)
					speed = MPEG_SPEED_STOP;
				else
					speed = dvd_getspeed();
			}
			else if (params->player_type == PLAYER_TYPE_FILE)
				speed = params->speed;
			else if (params->player_type == PLAYER_TYPE_VIDEO)
				speed = params->speed;
			else if (params->player_type == PLAYER_TYPE_AUDIO)
				speed = params->speed;
			else if (params->player_type == PLAYER_TYPE_AUDIOCD)
				speed = params->speed;
			switch (speed)
			{
			case MPEG_SPEED_NORMAL:
				var->Set(1); break;
			case MPEG_SPEED_PAUSE:
			case MPEG_SPEED_STEP:
			case MPEG_SPEED_STOP:
				var->Set(0); break;
			case MPEG_SPEED_FWD_4X:
				var->Set(4); break;
			case MPEG_SPEED_FWD_8X:
				var->Set(8); break;
			case MPEG_SPEED_FWD_16X:
				var->Set(16); break;
			case MPEG_SPEED_FWD_32X:
				var->Set(32); break;
			case MPEG_SPEED_FWD_48X:
				var->Set(48); break;
			case MPEG_SPEED_FWD_MAX:
				var->Set(100); break;
			case MPEG_SPEED_REV_4X:
				var->Set(-4); break;
			case MPEG_SPEED_REV_8X:
				var->Set(-8); break;
			case MPEG_SPEED_REV_16X:
				var->Set(-16); break;
			case MPEG_SPEED_REV_32X:
				var->Set(-32); break;
			case MPEG_SPEED_REV_48X:
				var->Set(-48); break;
			case MPEG_SPEED_REV_MAX:
				var->Set(-100); break;
			case MPEG_SPEED_SLOW_FWD_2X:
				var->Set("1/2"); break;
			case MPEG_SPEED_SLOW_FWD_4X:
				var->Set("1/4"); break;
			case MPEG_SPEED_SLOW_FWD_8X:
				var->Set("1/8"); break;
			case MPEG_SPEED_SLOW_REV_2X:
				var->Set("-1/2"); break;
			case MPEG_SPEED_SLOW_REV_4X:
				var->Set("-1/4"); break;
			case MPEG_SPEED_SLOW_REV_8X:
				var->Set("-1/8"); break;
			default:
				var->Set(0); break;
			}
		}
		break;
	case SCRIPT_VAR_PLAYER_ANGLE:
		if (params->player_type == PLAYER_TYPE_DVD)
			var->Set(dvd_getangle());
		else
			var->Set(0);
		break;
	case SCRIPT_VAR_PLAYER_MENU:
		if (params->player_type == PLAYER_TYPE_DVD)
		{
			if (module_load("dvd") < 0)
			{
				var->Set(0);
				return;
			}
			var->Set(dvd_ismenu() ? 1 : 0);
		}
		else
			var->Set(0);
		break;
    case SCRIPT_VAR_PLAYER_LANGUAGE_MENU:
		if (params->player_type == PLAYER_TYPE_DVD)
			var->Set(params->info.dvd_menu_lang);
		else
			var->Set("");
		break;
    case SCRIPT_VAR_PLAYER_LANGUAGE_AUDIO:
		if (params->player_type == PLAYER_TYPE_DVD)
			var->Set(params->info.dvd_audio_lang);
		else
			var->Set("");
		break;
    case SCRIPT_VAR_PLAYER_LANGUAGE_SUBTITLE:
		if (params->player_type == PLAYER_TYPE_DVD || params->player_type == PLAYER_TYPE_VIDEO)
			var->Set(params->info.spu_lang);
		else
			var->Set("");
		break;
	case SCRIPT_VAR_PLAYER_SUBTITLE:
		if (params->player_type == PLAYER_TYPE_VIDEO)
			var->Set(params->info.subtitle);
		else
			var->Set("");
		break;
	case SCRIPT_VAR_PLAYER_SUBTITLE_CHARSET:
		StringPair::Set(var, player_charset_pairs, params->info.subtitle_charset);
		break;
	case SCRIPT_VAR_PLAYER_SUBTITLE_WRAP:
		var->Set(params->info.subtitle_wrap);
		break;
    case SCRIPT_VAR_PLAYER_VOLUME:
		var->Set(get_volume());
		break;
	case SCRIPT_VAR_PLAYER_BALANCE:
		var->Set(get_balance());
		break;
	case SCRIPT_VAR_PLAYER_AUDIO_OFFSET:
		var->Set(get_audio_offset());
		break;
    case SCRIPT_VAR_PLAYER_TIME:
		var->Set(params->info.cur_time);
		break;
	case SCRIPT_VAR_PLAYER_TITLE:
		var->Set(params->info.cur_title);
		break;
	case SCRIPT_VAR_PLAYER_CHAPTER:
		var->Set(params->info.cur_chapter);
		break;
	case SCRIPT_VAR_PLAYER_NUM_TITLES:
		var->Set(params->info.num_titles);
		break;
	case SCRIPT_VAR_PLAYER_NUM_CHAPTERS:
		var->Set(params->info.num_chapters);
		break;
    case SCRIPT_VAR_PLAYER_NAME:
		var->Set(params->info.name);
		break;
    case SCRIPT_VAR_PLAYER_ARTIST:
		var->Set(params->info.artist);
		break;
    case SCRIPT_VAR_PLAYER_AUDIO_INFO:
		var->Set(params->info.audio_info);
		break;
    case SCRIPT_VAR_PLAYER_VIDEO_INFO:
		var->Set(params->info.video_info);
		break;
	case SCRIPT_VAR_PLAYER_AUDIO_STREAM:
		var->Set(params->info.audio_stream);
		break;
    case SCRIPT_VAR_PLAYER_SUBTITLE_STREAM:
		var->Set(params->info.spu_stream);
		break;
    case SCRIPT_VAR_PLAYER_LENGTH:
		var->Set(params->info.length);
		break;
    case SCRIPT_VAR_PLAYER_WIDTH:
		var->Set(params->info.width);
		break;
    case SCRIPT_VAR_PLAYER_HEIGHT:
		var->Set(params->info.height);
		break;
	case SCRIPT_VAR_PLAYER_FRAME_RATE:
		var->Set(params->info.frame_rate);
		break;
	case SCRIPT_VAR_PLAYER_COLOR_SPACE:
		StringPair::Set(var, player_clrs_pairs, params->info.clrs);
		break;
    case SCRIPT_VAR_PLAYER_DEBUG:
		if (params->player_type == PLAYER_TYPE_DVD)
			var->Set(dvd_getdebug());
#ifdef EXTERNAL_PLAYER
		else if (params->player_type == PLAYER_TYPE_FILE)
			var->Set(player_getdebug());
#endif
#ifdef INTERNAL_VIDEO_PLAYER
		else if (params->player_type == PLAYER_TYPE_VIDEO)
			var->Set(video_getdebug());
#endif
#ifdef INTERNAL_AUDIO_PLAYER
		else if (params->player_type == PLAYER_TYPE_AUDIO)
			var->Set(audio_getdebug());
#endif
		else if (params->player_type == PLAYER_TYPE_AUDIOCD)
			var->Set(cdda_getdebug());
		else
			var->Set(0);
		break;
    case SCRIPT_VAR_PLAYER_ERROR:
		var->Set(params->player_error);
		break;
	}
}

void on_player_set(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	if (var == NULL)
		return;
	
	params->player_error = "";

	switch (var_id)
	{
	case SCRIPT_VAR_PLAYER_SOURCE:
		{
			params->player_source = var->GetString();
			params->player_source.Replace('\\', '/');
			if (params->player_source.Find('/') != 0)
				params->player_source = "/" + params->player_source;
			params->player_folder = params->player_source;
			int rf = params->player_folder.ReverseFind('/');
			if (rf > 0)
				params->player_folder = params->player_folder.Left(rf);
			// detect media type
			params->player_type = PLAYER_TYPE_UNKNOWN;
			if (params->player_source.FindNoCase("/dvd") == 0)
			{
				params->player_type = PLAYER_TYPE_DVD;
			}
			else if (params->player_source.FindNoCase("/cdrom") == 0 || 
					params->player_source.FindNoCase("/hdd") == 0)
			{
				// test for folder 
				struct stat64 statbuf;
				if (cdrom_stat(params->player_source, &statbuf) >= 0
					&& S_ISDIR(statbuf.st_mode))
				{
					params->player_type = PLAYER_TYPE_FOLDER;
				}
				else if (params->player_source.FindNoCase(".cda") > 0)
					params->player_type = PLAYER_TYPE_AUDIOCD;
				else if (params->player_source.FindNoCase(".ifo") > 0 ||
					params->player_source.FindNoCase(".bup") > 0)
					params->player_type = PLAYER_TYPE_DVD;
#ifdef INTERNAL_VIDEO_PLAYER
				else if (params->player_source.FindNoCase(".avi") > 0
						|| params->player_source.FindNoCase(".divx") > 0
						|| params->player_source.FindNoCase(".mp4") > 0
						|| params->player_source.FindNoCase(".3gp") > 0
						|| params->player_source.FindNoCase(".mov") > 0
#ifdef INTERNAL_VIDEO_MPEG_PLAYER
						|| params->player_source.FindNoCase(".mpg") > 0
						|| params->player_source.FindNoCase(".m1v") > 0
						|| params->player_source.FindNoCase(".m2v") > 0
						|| params->player_source.FindNoCase(".mpeg") > 0
						|| params->player_source.FindNoCase(".vob") > 0
						|| params->player_source.FindNoCase(".dat") > 0
#endif
						)
					params->player_type = PLAYER_TYPE_VIDEO;
#endif
#ifdef INTERNAL_AUDIO_PLAYER
				else if (params->player_source.FindNoCase(".mp3") > 0
						|| params->player_source.FindNoCase(".mp2") > 0
						|| params->player_source.FindNoCase(".mp1") > 0
						|| params->player_source.FindNoCase(".mpa") > 0
						|| params->player_source.FindNoCase(".ac3") > 0
						|| params->player_source.FindNoCase(".wav") > 0
						|| params->player_source.FindNoCase(".ogg") > 0)
					params->player_type = PLAYER_TYPE_AUDIO;
#endif
				else
					params->player_type = PLAYER_TYPE_FILE;
			}
			
			params->info.name = "";
			params->info.artist = "";
			params->info.width = 0;
			params->info.height = 0;
			params->info.frame_rate = 0;
			params->info.clrs = PLAYER_COLOR_SPACE_UNKNOWN;
			params->info.length = 0;
			
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_NAME);
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_ARTIST);
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_WIDTH);
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_HEIGHT);
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_FRAME_RATE);
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_COLOR_SPACE);
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_LENGTH);
			
		}
		break;
    case SCRIPT_VAR_PLAYER_COMMAND:
		params->player_command = var->GetString();
		//if (params->player_type == PLAYER_TYPE_DVD || params->player_command.CompareNoCase("stop") == 0)
			player_do_command();
		/*else
			params->player_do_command = true;
		*/
		break;
	case SCRIPT_VAR_PLAYER_PLAYING:
		{
			switch (var->GetInteger())
			{
			case 0:
				if (params->player_type == PLAYER_TYPE_DVD)
					player_dvd_command("stop");
#ifdef EXTERNAL_PLAYER
				else if (params->player_type == PLAYER_TYPE_FILE)
					player_file_command("stop");
#endif
#ifdef INTERNAL_VIDEO_PLAYER
				else if (params->player_type == PLAYER_TYPE_VIDEO)
					player_video_command("stop");
#endif
#ifdef INTERNAL_AUDIO_PLAYER
				else if (params->player_type == PLAYER_TYPE_AUDIO)
					player_audio_command("stop");
#endif
				else if (params->player_type == PLAYER_TYPE_AUDIOCD)
					player_cdda_command("stop");
				break;
			case 1:
				if (params->player_type == PLAYER_TYPE_DVD)
					player_dvd_command("play");
#ifdef EXTERNAL_PLAYER
				else if (params->player_type == PLAYER_TYPE_FILE)
					player_file_command("play");
#endif
#ifdef INTERNAL_VIDEO_PLAYER
				else if (params->player_type == PLAYER_TYPE_VIDEO)
					player_video_command("play");
#endif
#ifdef INTERNAL_AUDIO_PLAYER
				else if (params->player_type == PLAYER_TYPE_AUDIO)
					player_audio_command("play");
#endif
				else if (params->player_type == PLAYER_TYPE_AUDIOCD)
					player_cdda_command("play");
				break;
			default:
				;
			}
		}
		break;
    case SCRIPT_VAR_PLAYER_SELECT:
		break;
    case SCRIPT_VAR_PLAYER_REPEAT:
		params->player_repeat = (PLAYER_REPEAT)StringPair::Get(var, player_repeat_pairs, PLAYER_REPEAT_NONE);
		break;
    case SCRIPT_VAR_PLAYER_SPEED:
		if (params->player_type == PLAYER_TYPE_DVD)
		{
			MPEG_SPEED_TYPE speed = (MPEG_SPEED_TYPE)StringPair::Get(var, player_speed_pairs, MPEG_SPEED_NORMAL);
			dvd_setspeed(speed);
		}
		break;
	case SCRIPT_VAR_PLAYER_ANGLE:
		if (params->player_type == PLAYER_TYPE_DVD)
		{
			dvd_button_angle(var->GetInteger());
		}
		break;
	case SCRIPT_VAR_PLAYER_MENU:
		if (params->player_type == PLAYER_TYPE_DVD)
		{
			DVD_MENU_TYPE menu = (DVD_MENU_TYPE)StringPair::Get(var, player_menu_pairs, DVD_MENU_DEFAULT);
			dvd_button_menu(menu);
		}
		break;
    case SCRIPT_VAR_PLAYER_LANGUAGE_MENU:
		params->info.dvd_menu_lang = var->GetString();
		break;
    case SCRIPT_VAR_PLAYER_LANGUAGE_AUDIO:
		if (params->player_type == PLAYER_TYPE_DVD)
		{
			params->info.dvd_audio_lang = var->GetString();
			if (params->dvdplaying)
				dvd_button_audio(params->info.dvd_audio_lang);
		}
		break;
    case SCRIPT_VAR_PLAYER_LANGUAGE_SUBTITLE:
		if (params->player_type == PLAYER_TYPE_DVD)	// only for DVDs
		{
			params->info.spu_lang = var->GetString();
			if (params->dvdplaying)
				dvd_button_subtitle(params->info.spu_lang);
		}
		break;
	case SCRIPT_VAR_PLAYER_SUBTITLE_CHARSET:
		params->info.subtitle_charset = (SUBTITLE_CHARSET)StringPair::Get(var, player_charset_pairs, SUBTITLE_CHARSET_DEFAULT);
		break;
	case SCRIPT_VAR_PLAYER_SUBTITLE_WRAP:
			params->info.subtitle_wrap = MAX(0, var->GetInteger());
		break;
    case SCRIPT_VAR_PLAYER_VOLUME:
		set_volume(var->GetInteger());
		break;
	case SCRIPT_VAR_PLAYER_BALANCE:
		set_balance(var->GetInteger());
		break;
	case SCRIPT_VAR_PLAYER_AUDIO_OFFSET:
		set_audio_offset(var->GetInteger());
		break;
    case SCRIPT_VAR_PLAYER_TIME:
		if (params->player_type == PLAYER_TYPE_DVD)
		{
			params->info.cur_time = var->GetInteger();
			params->info.cur_time_changed = true;
			dvd_get_chapter_for_time(params->info.cur_title, params->info.cur_time,
							&params->info.cur_chapter);
			mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_CHAPTER);
		}
		else if (params->player_type == PLAYER_TYPE_FILE)
		{
			params->info.cur_time = var->GetInteger();
			params->info.cur_time_changed = true;
		}
		else if (params->player_type == PLAYER_TYPE_VIDEO)
		{
			params->info.cur_time = var->GetInteger();
			params->info.cur_time_changed = true;
		}
		else if (params->player_type == PLAYER_TYPE_AUDIO)
		{
			params->info.cur_time = var->GetInteger();
			params->info.cur_time_changed = true;
		}
		else if (params->player_type == PLAYER_TYPE_AUDIOCD)
		{
			params->info.cur_time = var->GetInteger();
			params->info.cur_time_changed = true;
		}
		break;
	case SCRIPT_VAR_PLAYER_CHAPTER:
		if (params->player_type == PLAYER_TYPE_DVD)
		{
			int ch = var->GetInteger();
			if (ch > 0)
			{
				msg("Player chapter set to %d.\n", ch);
				params->info.cur_chapter = ch;
				params->info.cur_changed = true;
				dvd_get_total_time(params->info.cur_title, params->info.cur_chapter, 
					&params->info.cur_time);
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_TIME);
			}
		}
		break;
	case SCRIPT_VAR_PLAYER_TITLE:
		if (params->player_type == PLAYER_TYPE_DVD)
		{
			int titl = var->GetInteger();
			if (titl > 0)
			{
				msg("Player title set to %d.\n", titl);
				params->info.cur_title = titl;
				params->info.cur_chapter = 1;
				params->info.cur_changed = true;
				params->info.num_chapters = dvd_getnumchapters(params->info.cur_title);
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_NUM_CHAPTERS);
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_CHAPTER);
				dvd_get_total_time(params->info.cur_title, params->info.cur_chapter, 
					&params->info.cur_time);
				mmsl->UpdateVariable(SCRIPT_VAR_PLAYER_TIME);
			}
		}
		break;
    case SCRIPT_VAR_PLAYER_DEBUG:
		if (params->player_type == PLAYER_TYPE_DVD)
		{
			dvd_setdebug(var->GetInteger());
		}
#ifdef EXTERNAL_PLAYER
		else if (params->player_type == PLAYER_TYPE_FILE)
		{
			player_setdebug(var->GetInteger());
		}
#endif
		else if (params->player_type == PLAYER_TYPE_VIDEO)
		{
#ifdef INTERNAL_VIDEO_PLAYER
			video_setdebug(var->GetInteger());
#endif
		}
		else if (params->player_type == PLAYER_TYPE_AUDIO)
		{
#ifdef INTERNAL_AUDIO_PLAYER
			audio_setdebug(var->GetInteger());
#endif
		}
		else if (params->player_type == PLAYER_TYPE_AUDIOCD)
		{
			cdda_setdebug(var->GetInteger());
		}
		break;
	}
}
