//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - file info header file
 *  \file       player_info.h
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

#ifndef SP_PLAYER_INFO_H
#define SP_PLAYER_INFO_H

enum PLAYER_INFO_CHARSET
{
	PLAYER_INFO_CHARSET_LATIN1 = 0,
	PLAYER_INFO_CHARSET_CP1251 = 1,
};

BOOL player_getinfo(const char *fname, const char *charset);

BOOL player_get_id3(const char *fname, const char *charset);
BOOL player_get_jpginfo(const char *fname);

#endif // of SP_PLAYER_INFO_H
