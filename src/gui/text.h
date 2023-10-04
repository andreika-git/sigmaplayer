//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - text object class header file
 *  \file       gui/text.h
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

#ifndef SP_TEXT_H
#define SP_TEXT_H

#include <gui/font.h>
#include <gui/window.h>

enum TEXT_STYLE
{
	TEXT_STYLE_NORMAL = 0,
	TEXT_STYLE_UNDERLINE,
	TEXT_STYLE_OUTLINE,
};

enum TEXT_ALIGN
{
	TEXT_ALIGN_LEFT = 0,
	TEXT_ALIGN_CENTER,
	TEXT_ALIGN_RIGHT
};

class TextRow
{
public:
	/// row text offset
	int off;
	/// row text length
	int len;
	/// row width, pixels
	int width;
};

/// Text object window class
class Text : public Window
{
public:
	/// ctor
	Text();
	/// dtor
	virtual ~Text() {}

	/// Update part of text in LOCAL coords
	virtual bool Update(Context *context, int x1, int y1, int x2, int y2);

	bool SetText(const SPString & txt);
	bool SetFont(char *fname);
	bool SetStyle(TEXT_STYLE);
	void SetColor(int);
	void SetBkColor(int);
	void SetTextAlign(TEXT_ALIGN);

public:
	/// text
	SPString text;
	/// current font
	Font *font;
	/// formatted rows data
	SPList<TextRow> rows;

	TEXT_STYLE style;
	int color, bkcolor;
	TEXT_ALIGN text_align;

private:
	// Update text dims for new font/text
	bool UpdateFormat();
};

#endif // of SP_TEXT_H
