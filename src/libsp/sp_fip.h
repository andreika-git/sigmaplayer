//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - FIP (front panel & remote) driver's header.
 *  \file       sp_fip.h
 *  \author     bombur
 *  \version    0.1
 *  \date       4.05.2004
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

#ifndef SP_FIP_H
#define SP_FIP_H


#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////
/// Functions:

/// Loads driver module and opens it.
BOOL fip_init(BOOL applymodule);

/// Closes driver handle and unloads module
BOOL fip_deinit();

/// Clears the FIP display.
BOOL fip_clear();

/// Writes character to FIP display at given position.
BOOL fip_write_char(int ch, int pos);

/// Writes right-aligned string to display (up to 7 symbols)
BOOL fip_write_string(const char *str);

/// Writes special display indicators (see FIP_SPECIAL_*)
BOOL fip_write_special(int id, BOOL onoff);

BOOL fip_get_special(int id);

/// Gets pressed button code (see FIP_KEY_*) if available or waits for it
int fip_read_button(BOOL blocked = TRUE);

////////////////////////////////////
/// Button codes definitions:
enum
{
	/// 1) definitions for the buttons on the front panel:
	FIP_KEY_NONE = 0,
	FIP_KEY_FRONT_EJECT,
	FIP_KEY_FRONT_PLAY,
	FIP_KEY_FRONT_STOP,
	FIP_KEY_FRONT_PAUSE,
	FIP_KEY_FRONT_SKIP_PREV,
	FIP_KEY_FRONT_SKIP_NEXT,
	FIP_KEY_FRONT_REWIND,
	FIP_KEY_FRONT_FORWARD,

	/// 2) definitions for the keys on the remote control:
	FIP_KEY_POWER,
	FIP_KEY_EJECT,

	FIP_KEY_ONE,
	FIP_KEY_TWO,
	FIP_KEY_THREE,
	FIP_KEY_FOUR,
	FIP_KEY_FIVE,
	FIP_KEY_SIX,
	FIP_KEY_SEVEN,
	FIP_KEY_EIGHT,
	FIP_KEY_NINE,
	FIP_KEY_ZERO,

	FIP_KEY_CANCEL,
	FIP_KEY_SEARCH,
	FIP_KEY_ENTER,

	FIP_KEY_OSD,
	FIP_KEY_SUBTITLE,
	FIP_KEY_SETUP,
	FIP_KEY_RETURN,
	FIP_KEY_TITLE,
	FIP_KEY_PN,
	FIP_KEY_MENU,
	FIP_KEY_AB,
	FIP_KEY_REPEAT,

	FIP_KEY_UP,
	FIP_KEY_DOWN,
	FIP_KEY_LEFT,
	FIP_KEY_RIGHT,

	FIP_KEY_VOLUME_DOWN,
	FIP_KEY_VOLUME_UP,
	FIP_KEY_PAUSE,

	FIP_KEY_REWIND,
	FIP_KEY_FORWARD,
	FIP_KEY_SKIP_PREV,
	FIP_KEY_SKIP_NEXT,

	FIP_KEY_PLAY,
	FIP_KEY_STOP,
 
	FIP_KEY_SLOW,
	FIP_KEY_AUDIO,
	FIP_KEY_VMODE,
	FIP_KEY_MUTE,
	FIP_KEY_ZOOM,
	FIP_KEY_PROGRAM,
	FIP_KEY_PBC,
	FIP_KEY_ANGLE,
};

enum
{
	FIP_SPECIAL_PBC = 0,
	FIP_SPECIAL_MP3 = 1,
	FIP_SPECIAL_CAMERA = 2,
	FIP_SPECIAL_COLON1 = 3,
	FIP_SPECIAL_DOLBY = 4,
	FIP_SPECIAL_COLON2 = 5,
	FIP_SPECIAL_DTS = 6,

	FIP_SPECIAL_S = 7,
	FIP_SPECIAL_V = 8,
	FIP_SPECIAL_CD = 9,

	FIP_SPECIAL_PLAY = 10,
	FIP_SPECIAL_PAUSE = 11,
	FIP_SPECIAL_ALL = 12,
	FIP_SPECIAL_REPEAT = 13,

	FIP_SPECIAL_DVD = 14,

	FIP_SPECIAL_CIRCLE_1 = 15,
	FIP_SPECIAL_CIRCLE_2,
	FIP_SPECIAL_CIRCLE_3,
	FIP_SPECIAL_CIRCLE_4,
	FIP_SPECIAL_CIRCLE_5,
	FIP_SPECIAL_CIRCLE_6,
	FIP_SPECIAL_CIRCLE_7,
	FIP_SPECIAL_CIRCLE_8,
	FIP_SPECIAL_CIRCLE_9,
	FIP_SPECIAL_CIRCLE_10,
	FIP_SPECIAL_CIRCLE_11,
	FIP_SPECIAL_CIRCLE_12,
};

#ifdef __cplusplus
}
#endif


#endif // of SP_FIP_H
