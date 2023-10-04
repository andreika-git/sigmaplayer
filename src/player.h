//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - file player header file
 *  \file       player.h
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

#ifndef SP_PLAYER_H
#define SP_PLAYER_H

// use this for our audio/video players
#define INTERNAL_VIDEO_PLAYER
#define INTERNAL_VIDEO_MPEG_PLAYER
#define INTERNAL_AUDIO_PLAYER

//#define EXTERNAL_PLAYER

int player_mem_init();
int player_mem_deinit();

/// Play file
int player_play(char *filepath = NULL);

/// Advance playing
int player_info_loop();
int player_id3_loop();

/// Pause playing
BOOL player_pause();

/// Stop playing
BOOL player_stop();

/// Seek to given time and play
BOOL player_seek(int seconds);

BOOL player_zoom_hor(int scale);
BOOL player_zoom_ver(int scale);
BOOL player_scroll(int offx, int offy);

int player_forward();
int player_rewind();

/// Start get_info from media file using external program.
void player_startinfo(const char *fname, const char *charset);

void player_setdebug(BOOL ison);
BOOL player_getdebug();

#endif // of SP_PLAYER_H
