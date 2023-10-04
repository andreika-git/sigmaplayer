//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - FIP button internal codes header.
 * 								For DreamX-108 player
 *  \file       sp_fip_codes-dreamx108.h
 *  \author     bombur
 *  \version    0.2
 *  \date       21.01.2009 4.05.2007
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

#ifndef SP_FIP_CODES_H
#define SP_FIP_CODES_H


#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////
/// Button codes definitions:

static DWORD fip_code_offset[2] = { FIP_KEY_FRONT_EJECT, FIP_KEY_POWER };

/// 1) definitions for the buttons on the front panel:
static DWORD fip_panel_codes[] = 
{
	0x00001000, // FIP_KEY_FRONT_EJECT, 
	0x80000080, // FIP_KEY_FRONT_PLAY, 
	0x00080000, // FIP_KEY_FRONT_STOP, 
	0x08000008, // FIP_KEY_FRONT_PAUSE, 
	0x00010000, // FIP_KEY_FRONT_SKIP_PREV, 
	0x00004000, // FIP_KEY_FRONT_SKIP_NEXT, 
	0x10000010, // FIP_KEY_FRONT_REWIND, 
	0x01000001, // FIP_KEY_FRONT_FORWARD, 

	0xffffffff
};

/// 2) definitions for the keys on the remote control:
static DWORD fip_remote_codes[] = 
{
	0x00FBF00F, // FIP_KEY_POWER, 
	0x00FBB847, // FIP_KEY_EJECT, 

	0x00FBCA35, // FIP_KEY_ONE, 
	0x00FBD02F, // FIP_KEY_TWO, 
	0x00FB7887, // FIP_KEY_THREE, 
	0x00FBC03F, // FIP_KEY_FOUR, 
	0x00FB8A75, // FIP_KEY_FIVE, 
	0x00FB20DF, // FIP_KEY_SIX, 
	0x00FB2AD5, // FIP_KEY_SEVEN, 
	0x00FB807F, // FIP_KEY_EIGHT, 
	0x00FBD827, // FIP_KEY_NINE, 
	0x00FB708F, // FIP_KEY_ZERO, 

	0x00FB6897, // FIP_KEY_CANCEL, 
	0x00FBE21D, // FIP_KEY_SEARCH, 
	0x00FBA05F, // FIP_KEY_ENTER, 

	0x00FB827D, // FIP_KEY_OSD, 
	0x00FBA25D, // FIP_KEY_SUBTITLE, 
	0x00FBB24D, // FIP_KEY_SETUP, 
	0x00FBD22D, // FIP_KEY_RETURN, 
	0x00FBF807, // FIP_KEY_TITLE, 
	0x00000800, // FIP_KEY_PN, 
	0x00FB42BD, // FIP_KEY_MENU, 
	0x00FB28D7, // FIP_KEY_AB, 
	0x00FB8877, // FIP_KEY_REPEAT, 

	0x00FB30CF, // FIP_KEY_UP, 
	0x00FBF20D, // FIP_KEY_DOWN, 
	0x00FB9867, // FIP_KEY_LEFT, 
	0x00FB32CD, // FIP_KEY_RIGHT, 

	0x00FBC23D, // FIP_KEY_VOLUME_DOWN, 
	0x00FB22DD, // FIP_KEY_VOLUME_UP, 
	0x00FB629D, // FIP_KEY_PAUSE, 

	0x00FB926D, // FIP_KEY_REWIND, 
	0x00FB6A95, // FIP_KEY_FORWARD, 
	0x00FB38C7, // FIP_KEY_SKIP_PREV, 
	0x00FB609F, // FIP_KEY_SKIP_NEXT, 

	0x00FBA857, // FIP_KEY_PLAY, 
	0x00FBB04F, // FIP_KEY_STOP, 

	0x00FBE817, // FIP_KEY_SLOW, 
	0x00FB728D, // FIP_KEY_AUDIO, 
	0x00FBAA55, // FIP_KEY_VMODE, 
	0x00FB906F, // FIP_KEY_MUTE, 
	0x00FBC837, // FIP_KEY_ZOOM, 
	0x00FFBABA, // FIP_KEY_PROGRAM, 
	0x00FF728D, // FIP_KEY_PBC, 
	0x00FB4AB5, // FIP_KEY_ANGLE, 

	0xffffffff
};

#ifdef __cplusplus
}
#endif


#endif // of SP_FIP_CODES_H
