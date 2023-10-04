//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - DVD player INTERNAL header file
 *  \file       dvd/dvd-internal.h
 *  \author     bombur
 *  \version    0.2
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

#ifndef SP_DVD_INTERNAL_H
#define SP_DVD_INTERNAL_H

////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

#include <libsp/sp_mpeg.h>

/// Pause playing
BOOL dvd_pause();

/// Continue playing from saved position
BOOL dvd_continue_play();

/// Set title (used for navigation)
int dvd_loadtitle(int titl);

BOOL dvd_zoom_hor(int scale);
BOOL dvd_zoom_ver(int scale);
BOOL dvd_scroll(int offx, int offy);

char *dvd_getdeflang_menu();
char *dvd_getdeflang_audio();
char *dvd_getdeflang_spu();

/// Menu button controls
void dvd_button_press();
void dvd_button_up();
void dvd_button_down();
void dvd_button_left();
void dvd_button_right();

void dvd_getstringlang(BYTE *strlang, WORD lang);

void dvd_button_return();

void dvd_button_pause();
void dvd_button_step();
void dvd_button_slow();
void dvd_button_prev();
void dvd_button_next();
void dvd_button_fwd();
void dvd_button_rew();

////////////////
/// internal funcs.

// used by dvdnav_get_spu_logical_stream()
int dvd_get_spu_mode(KHWL_VIDEOMODE vmode);

int dvd_savepos(bool reset);

#ifdef __cplusplus
}
#endif

#endif // of SP_DVD_INTERNAL_H
