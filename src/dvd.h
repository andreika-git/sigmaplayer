//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - DVD player header file
 *  \file       dvd.h
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

#ifndef SP_DVD_H
#define SP_DVD_H

// must be equal to DVDMenuID_t from dvdtypes.h
typedef enum 
{
	DVD_MENU_DEFAULT = -1,
	DVD_MENU_ESCAPE     = 0,
	DVD_MENU_TITLE      = 2,
	DVD_MENU_ROOT       = 3,
	DVD_MENU_SUBTITLE   = 4,
	DVD_MENU_AUDIO      = 5,
	DVD_MENU_ANGLE      = 6,
	DVD_MENU_CHAPTERS   = 7
} DVD_MENU_TYPE;


////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

#include <libsp/sp_mpeg.h>

/// Open DVD player
int dvd_open(const char *path);
/// Close DVD player
int dvd_close();
/// Reset DVD player
int dvd_reset();

/// Get next data block
int dvd_get_next_block(BYTE **buf, int *event, int *len);
/// Free data block (if cache used).
int dvd_free_block(BYTE *data);

/// Get DVD error string
char *dvd_error_string();

/// Seek to given time and play
BOOL dvd_seek(int seconds);
BOOL dvd_seek_titlepart(int title, int part);
void dvd_get_cur(int *title, int *chapter);
int dvd_getnumchapters(int title);
int dvd_getnumtitles();

void dvd_button_play();

void dvd_setdeflang_menu(char *);
void dvd_setdeflang_audio(char *);
void dvd_setdeflang_spu(char *);

/// Play DVD disc (from drive or folder)
int dvd_play(const char *dvdpath, bool play_from_drive);

/// Stop playing
BOOL dvd_stop();

bool dvd_do_command(const SPString & command);

bool dvd_get_saved();

/// Change playing speed
BOOL dvd_setspeed(MPEG_SPEED_TYPE speed);
MPEG_SPEED_TYPE dvd_getspeed();

void dvd_setdebug(BOOL ison);
BOOL dvd_getdebug();

/// Advance playing
int dvd_player_loop();

int dvd_getangle();
bool dvd_ismenu();

// title = 1..99, chapter = 1.999
// time in seconds
int dvd_get_total_time(int title, int chapter, int *time);
int dvd_get_chapter_for_time(int title, int time, int *chapter);

void dvd_button_menu(DVD_MENU_TYPE menu = DVD_MENU_DEFAULT);
void dvd_button_angle(int setangl = -1);
void dvd_button_audio(char *setlang = NULL, int startfrom = 0);
void dvd_button_subtitle(char *setlang = NULL, int startfrom = 0);


#ifdef __cplusplus
}
#endif

#endif // of SP_DVD_H
