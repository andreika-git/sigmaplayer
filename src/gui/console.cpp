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

#include <string.h>
#include <libsp/sp_misc.h>
#include <libsp/sp_memory.h>

#include <gui/console.h>
#include <gui/console_font.inc.cpp>

Console *console = NULL;

Console::Console(int cols, int rows)
{
	if (cols < 1)
		cols = 1;
	if (rows < 1)
		rows = 1;
	numrows = rows;
	numcols = cols;
	str = (char **)SPmalloc(numrows * sizeof(char *));
	str[0] = (char *)SPmalloc(numrows * numcols * sizeof(char));
	slen = (int *)SPmalloc(numrows * sizeof(int));
	for (int i = 1; i < numrows; i++)
		str[i] = str[i - 1] + numcols * sizeof(char);
	num = numrows;

	width = 0;
	height = 0;

	font = NULL;
#ifdef CONSOLE_USE_FONT_8x8
	SetFont(CONSOLE_FONT_8x8);
#else
	SetFont(CONSOLE_FONT_8x16);
#endif
}

bool Console::SetFont(CONSOLE_FONT_TYPE type)
{
	switch (type)
	{
	case CONSOLE_FONT_8x8:
		cur_fnt = (BYTE *)font8x8;
		font_width = 8;
		font_height = 8;
		break;
	case CONSOLE_FONT_8x16:
		cur_fnt = (BYTE *)font8x16;
		font_width = 8;
		font_height = 16;
		break;
	default:
		return false;
	}

	if (font != NULL)
		SPfree(font);
	
	font = (BYTE *)SPmalloc(font_width * font_height * 256);
	
	UpdateFont();
	
	// sorry, console size will change if font changes
	width = numcols * font_width;
	height = numrows * font_height;

	// center left-bottom
	x = gui.left;
	y = gui.bottom - height;
	
	dirty = true;

	return true;
}

void Console::UpdateFont()
{
	int siz = font_width * font_height;
	int chsiz = ((font_width + 7) / 8);
	BYTE c1 = (BYTE)gui.GetWhiteColor();
	BYTE c2 = (BYTE)gui.GetTransparentColor();
	for (int i = 0; i < 256; i++)
	{
		BYTE *ff = font + i * siz;
		BYTE *sff = cur_fnt + i * chsiz * font_height;
		for (int j = 0; j < siz; j++)
			*ff++ = ((sff[j / font_width] >> (font_width - 1 - j % font_width)) & 1) != 0 ? c1 : c2;
	}
}

Console::~Console()
{
	if (font != NULL)
		SPfree(font);

	SPfree(str[0]);
	SPfree(str);
	SPfree(slen);
}

bool Console::Printf(char *s)
{
	char *start = s;
	for (; *s != '\0'; s++)
	{
		switch (*s)
		{
		case '\r':
			continue;
		case '\n':
			AddString(start, s - start);
			start = s + 1;
			continue;
		// TODO: add tabs
		}
	}
	if (s != start)
		AddString(start, s - start);
	
	return false;
}

bool Console::AddString(const char *s, int len)
{
	if (num > 0)
		num--;
	else
	{
		// shift pointers
		for (int iy = 0; iy < numrows - 1; iy++)
		{
			strncpy(str[iy], str[iy + 1], slen[iy + 1]);
			slen[iy] = slen[iy + 1];
		}
	}
	int last = numrows - num - 1;
	
	if (len == 0)
		len = 99999;
	int sl, sll;
	for (sl = 0, sll = 0; s[sl] != '\0' && sl < len && sl < numcols; sl++)
	{
		if (s[sl] == '\r' || s[sl] == '\n')
			continue;
		str[last][sll++] = s[sl];
	}
	slen[last] = sll;
	dirty = true;
	return true;
}

bool Console::Update(Context *context, int x1, int y1, int x2, int y2)
{
	if (font == NULL || context == NULL)
		return false;
	if (num == numrows)
		return false;
	int nx = MIN((x2 + font_width - 1) / font_width, numcols), ny = MIN((y2 + font_height - 1) / font_height, numrows);
	int charsize = font_width * font_height;
	int xx = (x1 / font_width) * font_width;
	BYTE *d = context->data + xx;
	BYTE trc = (BYTE)gui.GetTransparentColor();
	for (int iy = MAX(y1 / font_height, num), yy = iy * font_height; iy < ny; iy++, yy += font_height)
	{
		int iiy = iy - num;
		BYTE *dst = d + yy * context->pitch;
		char *s = str[iiy];
		int fy2 = MIN(font_height - 1, y2 - yy);
		int fx = x1 - xx;
		int dx = xx;
		for (int ix = x1 / font_width; ix < MIN(nx, slen[iiy]); ix++)
		{
			int xxi = ix * font_width;
			int fw = x2 - xxi + 1 < font_width ? x2 - xxi + 1 : font_width;
			BYTE *base = font + (BYTE)s[ix] * charsize;
			for (int fy = MAX(y1 - yy, 0); fy <= fy2; fy++)
				memcpy(dst + fy * context->pitch + fx, base + fy * font_width + fx, fw);
			fx = 0;
			dst += fw;
			dx += fw;
		}
		// finish the line
		int pad = x2 + 1 - dx - fx;
		if (pad > 0)
		{
			for (int fy = MAX(0, y1 - yy); fy <= fy2; fy++)
				memset(dst + fy * context->pitch + fx, trc, pad);
		}
	}
	return true;
}

bool Console::TextOut(Context *context, char *str, int nx)
{
	if (font == NULL || context == NULL)
		return false;
	int charsize = font_width * font_height;
	BYTE *dst = context->data;
	int ix, nnx = (nx < 1) ? 9999 : nx;
	for (ix = 0; str[ix] && ix < nnx; ix++)
	{
		BYTE *base = font + str[ix] * charsize;
		for (int fy = 0; fy < font_height; fy++)
			memcpy(dst + fy * context->pitch, base + fy * font_width, font_width);
		dst += font_width;
	}
	// finish the line
	if (nx > ix)
	{
		int pad = (nx - ix) * font_width;
		BYTE trc = (BYTE)gui.GetTransparentColor();
		for (int fy = 0; fy < font_height; fy++)
			memset(dst + fy * context->pitch, trc, pad);
	}
	return true;
}
