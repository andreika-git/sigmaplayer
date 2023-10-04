//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - rectangle (incl. rounded) class header file
 *  \file       rect.h
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

#ifndef SP_RECT_H
#define SP_RECT_H

#include <gui/window.h>

/// Rectangle (incl. rounded) window class
class Rectangle : public Window
{
public:
	/// ctor
	Rectangle();
	/// dtor
	virtual ~Rectangle() {}

	/// Update part of rect in LOCAL coords
	virtual bool Update(Context *context, int x1, int y1, int x2, int y2);

	void SetLineWidth(int);
	void SetRound(int);
	void SetColor(int);
	void SetBkColor(int);

public:
	int round, linewidth;
	int color, bkcolor;
};

#endif // of SP_RECT_H
