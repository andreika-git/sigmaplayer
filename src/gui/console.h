//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - console class header file
 *  \file       console.h
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

#ifndef SP_CONSOLE_H
#define SP_CONSOLE_H

#include <gui/window.h>

enum CONSOLE_FONT_TYPE
{
	CONSOLE_FONT_8x8 = 0,
	CONSOLE_FONT_8x16
};

/// Console class
class Console : public Window
{
public:
	/// ctor
	Console(int cols = 80, int rows = 20);

	/// dtor
	virtual ~Console();

	/// Formatted string output
	bool Printf(char *str);

	/// add one string ('\n' are ignored)
	bool AddString(const char *str, int len = 0);

	/// change font
	bool SetFont(CONSOLE_FONT_TYPE type);
	/// call this if palette changed
	void UpdateFont();

	/// Update part of window in LOCAL coords
	virtual bool Update(Context *context, int x1, int y1, int x2, int y2);

	/// Debug output (called from window)
	bool TextOut(Context *context, char *str, int nx = 0);

public:
	/// string data
	char **str;
	/// cached string lengths
	int *slen;
	/// console dims in chars
	int numrows, numcols;
	/// number of non-filled strings (from top to the first string)
	int num;
	/// rastered font
	BYTE *font;
	/// bitfont
	BYTE *cur_fnt;
	int font_width, font_height;
};

extern Console *console;

#endif // of SP_CONSOLE_H
