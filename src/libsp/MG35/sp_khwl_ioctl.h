//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - Internal KHWL driver's IOCTL codes.
 * 								For Technosonic-compatible players ('MP')
 *  \file       sp_khwl_ioctl.h
 *  \author     bombur
 *  \version    0.1
 *  \date       4.07.2004
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

#ifndef SP_KHWL_IOCTL_H
#define SP_KHWL_IOCTL_H

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////
/// Driver I/O defines:

#define KHWL_IO_BASE 	0x3cc
#define KHWL_IO_PLAYER 	0x3d0
#define KHWL_IO_PROP 	0x3d4
#define KHWL_IO_OSD 	0x3e1
#define KHWL_IO_DISPLAY	0x3e6


#define KHWL_HARDRESET 		KHWL_IO_BASE+3

#define KHWL_PLAY 			KHWL_IO_PLAYER
#define KHWL_STOP 			KHWL_IO_PLAYER+1
#define KHWL_PAUSE 			KHWL_IO_PLAYER+2
#define KHWL_AUDIOSWITCH 	KHWL_IO_PLAYER+3

#define KHWL_WAIT 			KHWL_IO_PROP+1
#define KHWL_SETPROP		KHWL_IO_PROP+2
#define KHWL_GETPROP		KHWL_IO_PROP+3

#define KHWL_OSDFB_SWITCH	KHWL_IO_OSD
#define KHWL_OSDFB_UPDATE	KHWL_IO_OSD+1
#define KHWL_OSDFB_ALPHA	KHWL_IO_OSD+2	// general alpha apply

#define KHWL_DISPLAY_CLEAR	KHWL_IO_DISPLAY
#define KHWL_DISPLAY_YUV  	KHWL_IO_DISPLAY+1

#ifdef __cplusplus
}
#endif

#endif // of SP_HWL_IOCTL_H
