//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - text object class impl.
 *  \file       rect.cpp
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

#include <string.h>
#include <libsp/sp_misc.h>
#include <libsp/sp_memory.h>

#include <gui/font.h>
#include <gui/text.h>


static BYTE row_buf[720];

/////////////////////////////////////////////////////////////////////////

#define TEXT_OUTLINE_IF(rdd, c, wdd, bc)  { if (*(rdd) != c) *(wdd) = bc; }
#define TEXT_OUTLINE(wdd, bc)  { *(wdd) = bc; }

Text::Text()
{
	font = NULL;
	style = TEXT_STYLE_NORMAL;
	color = gui.GetColor();
	bkcolor = gui.GetBkColor();
	text_align = TEXT_ALIGN_LEFT;
	transparent = true;
}

bool Text::Update(Context *context, int x1, int y1, int x2, int y2)
{
	if (context == NULL || font == NULL)
		return false;

	if (font->unloaded)
	{
		if (!font->Load())
			return false;
	}
	
	BYTE *d, *pd, *nd, *rd;
	int clrs[2];
	BYTE clrs2;
	if (style == TEXT_STYLE_OUTLINE)
	{
		if (x1 > 0)
			x1--;
		if (y1 > 0)
			y1--;
		x2--;
		y2--;
		d = context->data + (y1 + 1) * context->pitch + (x1 + 1);
		
		pd = d - context->pitch;
		nd = d + context->pitch;
		rd = row_buf;

		clrs[0] = -1;
		clrs2 = (BYTE)bkcolor;
	}
	else
	{
		d = context->data + y1 * context->pitch + x1;

		clrs[0] = bkcolor;
		clrs2 = '\0';
	}
	clrs[1] = color;
	
	for (int iy = y1; iy <= y2; iy++)
	{
		int row = iy / font->height;
		if (row < 0 || row >= rows.GetN())
			continue;
		int offy = iy % font->height;
		int ix, w = 0, cw = 0, ich = -1;
		BYTE ch = 0;
		ULONGLONG *s = NULL;
		int row_offs, xx1 = x1;
		if (text_align == TEXT_ALIGN_LEFT)
			row_offs = 0;
		else if (text_align == TEXT_ALIGN_CENTER)
		{
			row_offs = MAX(((x2 - x1 + 1) - rows[row].width) / 2, 0);
			if (clrs[0] >= 0)
				memset(d, clrs[0], row_offs);
			xx1 = MAX(x1 - row_offs, 0);
		}
		else
		{
			row_offs = MAX(((x2 - x1 + 1) - rows[row].width), 0);
			if (clrs[0] >= 0)
				memset(d, clrs[0], row_offs);
			xx1 = MAX(x1 - row_offs, 0);
		}
		int xx2 = x2 - row_offs;
		BYTE *dd, *pdd, *ndd, *rdd;
		BYTE prev_cc, prev_rdd;
		dd = d + row_offs;
		if (style == TEXT_STYLE_OUTLINE)
		{
			pdd = pd + row_offs;
			ndd = nd + row_offs;
			rdd = rd + row_offs;
			prev_cc = 0;
			prev_rdd = 0;
		}
		for (ix = 0; ix <= xx2; ix++)
		{
			if (ix >= w)
			{
onemore:
				ch = text[rows[row].off + (++ich)];
				if (ch == '\0' || ch == '\n')
					break;
				cw = font->widths[ch];
				if (cw < 1)
					goto onemore;
				w += cw;
				s = font->data[ch];
			}
			
			if (ix >= xx1)
			{
				int c;
				if (style == TEXT_STYLE_UNDERLINE && offy == font->baseline)
					c = clrs[1];
				else
					c = clrs[((s[offy] >> (cw - w + ix)) & 1)];
				if (c >= 0)
				{
					BYTE cc = (BYTE)c;
					*dd = cc;
					if (clrs2 > 0)	// outline
					{
						if (prev_rdd != cc)
							TEXT_OUTLINE(pdd-1, clrs2);
						TEXT_OUTLINE_IF(rdd,   cc, pdd,   clrs2);
						TEXT_OUTLINE_IF(rdd+1, cc, pdd+1, clrs2);
						
						TEXT_OUTLINE(ndd-1, clrs2);
						TEXT_OUTLINE(ndd,   clrs2);
						TEXT_OUTLINE(ndd+1, clrs2);
						
						if (prev_cc != cc)
							TEXT_OUTLINE(dd-1,  clrs2);
						TEXT_OUTLINE(dd+1,  clrs2);
						prev_cc = cc;
						prev_rdd = *rdd;
						*rdd = cc;
					}
				}
				dd++;
				if (clrs2 > 0)
				{
					if (c < 0)
					{
						prev_cc = 0;
						prev_rdd = *rdd;
						*rdd = 0;
					}
					rdd++;
					pdd++;
					ndd++;
				}
			}
		}
		if (ix < xx1)
			ix = xx1;
		if (clrs[0] >= 0 && ix < xx2)
			memset(dd, clrs[0], xx2 - ix + 1);

		d += context->pitch;
		if (clrs2 > 0)
		{
			pd += context->pitch;
			nd += context->pitch;
		}
	}
	
	return true;
}

bool Text::SetFont(char *fname)
{
	font = guifonts->GetFont(fname);
	if (font == NULL)
		return false;
	return UpdateFormat();
}

bool Text::SetText(const SPString & txt)
{
	text = txt;
	return UpdateFormat();
}

bool Text::SetStyle(TEXT_STYLE st)
{
	style = st;
	return UpdateFormat();
}

void Text::SetTextAlign(TEXT_ALIGN ta)
{
	text_align = ta;
	dirty = true;
}

void Text::SetColor(int c)
{
	if (c >= 0 && c < 256)
	{
		color = c;
		dirty = true;
	}
}

void Text::SetBkColor(int bc)
{
	if (bc > 255)
		bc = -1;
	bkcolor = bc;
	dirty = true;
}

bool Text::UpdateFormat()
{
	int new_width = 0;
	int new_height = 0;
	rows.Clear();

	if (font != NULL)
	{
		if (font->unloaded)
		{
			if (!font->Load())
				return false;
		}

		int rowidx = -1;
		for (int i = 0; i < text.GetLength(); i++)
		{
			if (rowidx < 0)
			{
				TextRow r;
				r.off = i;
				r.len = 0;
				r.width = 0;
				rowidx = rows.Add(r);
			}
			if (text[i] == '\n')
			{
				if (rows[rowidx].width > new_width)
					new_width = rows[rowidx].width;
				new_height += font->height;
				rowidx = -1;
				continue;
			}
			rows[rowidx].width += font->widths[(BYTE)text[i]];
			rows[rowidx].len++;
		}
		if (rowidx >= 0)
		{
			if (rows[rowidx].width > new_width)
				new_width = rows[rowidx].width;
			new_height += font->height;
		}
	}
	memset(row_buf, 0, MIN(new_width, 720));

	if (style == TEXT_STYLE_OUTLINE)
	{
		new_width += 2;
		new_height += 2;
	}
	
	SetWidth(new_width);
	SetHeight(new_height);
	auto_width = new_width;
	auto_height = new_height;

	dirty = true;
	return true;
}
