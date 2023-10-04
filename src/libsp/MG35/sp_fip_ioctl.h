//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - Internal FIP IOCTL codes.
 * 								For Technosonic-compatible players ('MP')
 *  \file       sp_fip_ioctl.h
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

#ifndef SP_FIP_IOCTL_H
#define SP_FIP_IOCTL_H

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////
/// Driver I/O defines:


/// Controls the display. 
/// Used for setting the pulse width and turning the display on and off.
/// The flags you wish to set on the display where flags is one of the 
/// FIP_PULSE_ defines.
#define FIP_DISPLAY_CONTROL         0x45000f

/// Description: Displays the symbol on the display.
/// Sets given symbol data at given position: (data | (pos << 16)).
#define FIP_DISPLAY_SYMBOL          0x45000f

/// Reads a button code (like getchar()). Can be blocked or not.
/// The button code is returned.
#define FIP_BUTTON_READ             0x450001

// #define FIP_???                  0x450004

#ifdef __cplusplus
}
#endif


#endif // of SP_FIP_IOCTL_H
