//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - font & font manager class header file
 *  \file       gui/font.h
 *  \author     bombur
 *  \version    0.1
 *  \date       4.10.2006
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

#ifndef SP_FONT_H
#define SP_FONT_H

#include <gui/res.h>

/// Font object 
class Font : public Resource
{
public:
	/// ctor
	Font();
	/// dtor
	~Font();

	BOOL Load();
	BOOL Unload();

public:
	/// number of characters
	int num;
	/// bit data (width=64 max.) [num][height]
	ULONGLONG **data;
	/// font char widths array
	int *widths;
	/// font height
	int height;
	/// font baseline
	int baseline;

	bool unloaded;
};

/// Font manager
class FontManager
{
public:
	/// ctor
	FontManager();
	/// dtor
	~FontManager();

	Font *GetDefaultFont();
	
	Font *GetFont(char *fname);

	// Clear font cache
	BOOL ClearFontData();

public:
	SPHashListAbstract<Font, Font> fonthash;
	Font *def;

protected:
	Font *LoadFont(char *fname);
};


extern FontManager *guifonts;


#endif // of SP_FONT_H
