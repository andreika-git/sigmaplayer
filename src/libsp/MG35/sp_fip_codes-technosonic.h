//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - FIP button internal codes header.
 * 								For Technosonic-compatible players ('MP')
 *  \file       sp_fip_codes-technosonic.h
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
	0x08000008, // FIP_KEY_FRONT_EJECT, 
	0x00000080, // FIP_KEY_FRONT_PLAY, 
	0x40000040, // FIP_KEY_FRONT_STOP, 
	0x04000004, // FIP_KEY_FRONT_PAUSE, 
	0x02000002, // FIP_KEY_FRONT_SKIP_PREV, 
	0x20000020, // FIP_KEY_FRONT_SKIP_NEXT, 
	0x10000010, // FIP_KEY_FRONT_REWIND, 
	0x01000001, // FIP_KEY_FRONT_FORWARD, 

	0xffffffff
};

/// 2) definitions for the keys on the remote control:
static DWORD fip_remote_codes[] = 
{
	0x00FF30CF, // FIP_KEY_POWER, 
	0x00FFB04F, // FIP_KEY_EJECT, 

	0x00FF00FF, // FIP_KEY_ONE, 
	0x00FF807F, // FIP_KEY_TWO, 
	0x00FF40BF, // FIP_KEY_THREE, 
	0x00FFC03F, // FIP_KEY_FOUR, 
	0x00FF20DF, // FIP_KEY_FIVE, 
	0x00FFA05F, // FIP_KEY_SIX, 
	0x00FF609F, // FIP_KEY_SEVEN, 
	0x00FFE01F, // FIP_KEY_EIGHT, 
	0x00FF10EF, // FIP_KEY_NINE, 
	0x00FF906F, // FIP_KEY_ZERO, 

	0x00FF50AF, // FIP_KEY_CANCEL, 
	0x00FFD02F, // FIP_KEY_SEARCH, 
	0x00FF708F, // FIP_KEY_ENTER, 

	0x00FF7887, // FIP_KEY_OSD, 
	0x00FFF807, // FIP_KEY_SUBTITLE, 
	0x00FF38C7, // FIP_KEY_SETUP, 
	0x00FFB847, // FIP_KEY_RETURN, 
	0x00FF28D7, // FIP_KEY_TITLE, 
	0x00FFA857, // FIP_KEY_PN, 
	0x00FF6897, // FIP_KEY_MENU, 
	0x00FFE817, // FIP_KEY_AB, 
	0x00FF18E7, // FIP_KEY_REPEAT, 

	0x00FF08F7, // FIP_KEY_UP, 
	0x00FF8877, // FIP_KEY_DOWN, 
	0x00FF48B7, // FIP_KEY_LEFT, 
	0x00FFC837, // FIP_KEY_RIGHT, 

	0x00FFD827, // FIP_KEY_VOLUME_DOWN, 
	0x00FF58A7, // FIP_KEY_VOLUME_UP, 
	0x00FF9867, // FIP_KEY_PAUSE, 

	0x00FF02FD, // FIP_KEY_REWIND, 
	0x00FF827D, // FIP_KEY_FORWARD, 
	0x00FF42BD, // FIP_KEY_SKIP_PREV, 
	0x00FFC23D, // FIP_KEY_SKIP_NEXT, 

	0x00FF22DD, // FIP_KEY_PLAY, 
	0x00FFA25D, // FIP_KEY_STOP, 

	0x00FF12ED, // FIP_KEY_SLOW, 
	0x00FF926D, // FIP_KEY_AUDIO, 
	0x00FF52AD, // FIP_KEY_VMODE, 
	0x00FFD22D, // FIP_KEY_MUTE, 
	0x00FF32CD, // FIP_KEY_ZOOM, 
	0x00FFB24D, // FIP_KEY_PROGRAM, 
	0x00FF728D, // FIP_KEY_PBC, 
	0x00FFF20D, // FIP_KEY_ANGLE, 

	0xffffffff
};

#ifdef __cplusplus
}
#endif


#endif // of SP_FIP_CODES_H
