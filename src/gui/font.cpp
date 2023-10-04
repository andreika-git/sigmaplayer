//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - font & font manager class impl.
 *  \file       font.cpp
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

#include <stdio.h>
#include <string.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_memory.h>

#include <gui/font.h>
#include <gui/console.h>

FontManager *guifonts = NULL;

#define DFF_FIXED            0x0001 // font is fixed pitch
#define DFF_PROPORTIONAL     0x0002 // font is proportional pitch
#define DFF_ABCFIXED         0x0004 // font is an ABC fixed font
#define DFF_ABCPROPORTIONAL  0x0008 // font is an ABC pro-portional font
#define DFF_1COLOR           0x0010 // font is one color
#define DFF_16COLOR          0x0020 // font is 16 color
#define DFF_256COLOR         0x0040 // font is 256 color
#define DFF_RGBCOLOR         0x0080 // font is RGB color

#ifdef WIN32
#pragma pack(1)
#endif

typedef struct FntHeader
{
	WORD ver;
	DWORD size;
	BYTE copy[60];
	WORD type;
	WORD points;
	WORD vres, hres;
	WORD ascent;
	WORD ilead, elead;
	BYTE italic, underl, strike;
	WORD weight;
	BYTE charset;
	WORD pixwidth, pixheight;
	BYTE pitchfamily;
	WORD avgwidth;
	WORD maxwidth;
	BYTE first, last, def, brk;
	WORD widthbytes;
	DWORD device, face, bitsptr, bits;
	BYTE flags;
} ATTRIBUTE_PACKED FntHeader;

#ifdef WIN32
#pragma pack()
#endif


const int get_ith_font = 1;

/////////////////////////////////////////////////////////////////////////

Font::Font()
{
	num = 0;
	data = NULL;
	widths = NULL;
	height = 0;
	baseline = 0;

	unloaded = false;
}

Font::~Font()
{
	Unload();
}

BOOL Font::Load()
{
	Unload();

	if (name == NULL)
		return FALSE;
	FILE *fp = fopen(name, "rb");
	if (fp == FALSE)
	{
		msg_error("Font file '%s' not found.\n", name);
		return FALSE;
	}
	WORD magic;
	if (fread(&magic, sizeof(WORD), 1, fp) != 1)
	{
		msg_error("Cannot read from font file '%s'.\n", name);
		return FALSE;
	}
	int base = 0;
	if (magic == 0x5a4d)	///////////////////// .FON
	{
		fseek(fp, 0, SEEK_END);
		DWORD siz = ftell(fp);
		fseek(fp, 0x3c, SEEK_SET);
		DWORD offs = 0;
		int ret = fread(&offs, sizeof(DWORD), 1, fp);
		if (ret != 1 || offs < 0x40 || offs >= siz)
		{
			msg_error("Cannot read .FON header for '%s'.\n", name);
			return FALSE;
		}
		fseek(fp, offs, SEEK_SET);
		WORD ne;
		fread(&ne, sizeof(WORD), 1, fp);
		if (ne != 0x454e)
		{
			msg_error("Unknown .FON format for '%s'.\n", name);
			return FALSE;
		}
		fseek(fp, offs + 0x24, SEEK_SET);
		WORD res_offs;
		fread(&res_offs, sizeof(WORD), 1, fp);
		if (res_offs >= siz)
		{
			return FALSE;
		}
		fseek(fp, offs + res_offs, SEEK_SET);
		WORD res_shift;
		fread(&res_shift, sizeof(WORD), 1, fp);
		if (res_shift > 32)
		{
			return FALSE;
		}
		bool found = false;
		for (int r = 0, f = 0; ; r++)
		{
			fseek(fp, offs + res_offs + 2 + r * 0x14, SEEK_SET);
			WORD type;
			fread(&type, sizeof(WORD), 1, fp);
			if (type == 0)
				break;
			if (type == 0x8008)	// get first font
			{
				fseek(fp, 6, SEEK_CUR);
				WORD dir_offs;
				fread(&dir_offs, sizeof(WORD), 1, fp);
				base = dir_offs << res_shift;
				found = true;
				if (++f == get_ith_font)
				{
					break;
				}
			}
		}
		if (!found)
		{
			msg_error("No fonts found in .FON file '%s'.\n", name);
			return FALSE;
		}
	}
	///////////////////// .FNT
	FntHeader h;
	fseek(fp, base, SEEK_SET);
	if (fread((void *)&h, sizeof(h), 1, fp) != 1)
	{
		msg_error("Cannot read font header for '%s'\n", name);
		return FALSE;
	}

	int first = 0, last = 0, ver = 3, avgwidth = 0;
	if (h.ver < 0x0100 || h.ver > 0x0300)
	{
		msg_error("Unknown font format for '%s'\n", name);
		return FALSE;
	}
	if ((h.type & 1) == 1)
	{
		msg_error("Vector fonts not supported ('%s')\n", name);
		return FALSE;
	}
	if (h.maxwidth > 64)
	{
		msg_error("Max. width for raster fonts supported is 64. ('%s')\n", name);
		return FALSE;
	}
	/*
	if ((h.flags & DFF_ABCFIXED) || (h.flags & DFF_ABCPROPORTIONAL))
	{
		// skip ABC
		fseek(fp, 3 * 2, SEEK_CUR);
	}
	if ((h.flags & DFF_1COLOR) != DFF_1COLOR)
	{
		// skip color offset
		fseek(fp, 4, SEEK_CUR);
	}
	
	//fseek(fp, 16, SEEK_CUR);
	if ((h.flags & DFF_16COLOR) == DFF_16COLOR ||
		(h.flags & DFF_256COLOR) == DFF_256COLOR ||
		(h.flags & DFF_RGBCOLOR) == DFF_RGBCOLOR)
	{
		msg_error("Only black-white fonts supported.");
		return FALSE;
	}
	*/

	num = h.last + 1;
	if (num < 1)
	{
		msg_error("No characters found in font '%s'\n", name);
		return FALSE;
	}
	if (num > 256)
	{
		msg_error("Font '%s' contains more than 256 characters (%d). Truncating...\n", 
					name, num);
		num = 256;
	}

	if (h.pixheight < 1 || h.pixheight > 255)
	{
		msg_error("Wrong font height %d for '%s'.\n", h.pixheight, name);
		return FALSE;
	}
	height = h.pixheight;
	baseline = h.ascent + 1;		// we add 1 for underline distance!
	if (baseline < 1)
		baseline = 1;
	if (baseline >= height)
		baseline = height - 1;
	first = h.first;
	last = h.last;
	ver = h.ver;
	avgwidth = h.avgwidth;

	// reading chartable
	int i;
	widths = new int [num];
	if (widths == NULL)
		return FALSE;
	data = new ULONGLONG* [num];
	if (data == NULL)
		return FALSE;
	
	data[0] = (ULONGLONG *)SPmalloc(num * height * sizeof(ULONGLONG));
	if (data[0] == NULL)
		return FALSE;

	// temp allocs
	int *off = new int [num];
	if (off == NULL)
		return FALSE;
	BYTE *tmpdata = new BYTE [height * 8];
	if (tmpdata == NULL)
	{
		delete [] off;
		return FALSE;
	}

	int num_allocs = 0;
	for (i = 0; i < num; i++)
	{
		DWORD w = 0;
		if (i >= first && i <= last)
		{
			if (fread(&w, 2, 1, fp) != 1)
				w = 0;
			if (ver == 0x0200)
			{
				WORD of;
				if (fread(&of, 2, 1, fp) != 1)
					off[i] = -1;
				else
					off[i] = of;
			} else
			{
				if (fread(&(off[i]), 4, 1, fp) != 1)
					off[i] = -1;
			}
		} else
			off[i] = -1;
		// skip too wide characters
		if (w > 64 || off[i] <= 0)
			w = 0;
		widths[i] = w;
		if (w > 0)
		{
			data[i] = data[0] + num_allocs * height;
			num_allocs++;

			memset(data[i], 0, sizeof(ULONGLONG) * height);
		} else if (i > 0)
			data[i] = NULL;
	}

	// now read symbols
	for (i = 0; i < num; i++)
	{
		if (widths[i] < 1 || off[i] < 0)
			continue;
		ULONGLONG *d = data[i];
		if (d == NULL)
			continue;
		int numbytes = ((widths[i] + 7) / 8 + 1) / 2 * 2;
		if (fseek(fp, off[i] + base, SEEK_SET) != 0)
			continue;
		if (fread(tmpdata, height * numbytes, 1, fp) != 1)
			continue;
		for (int j = 0; j < numbytes; j++)
		{
			for (int k = 0; k < height; k++)
			{
				ULONGLONG tmpd = tmpdata[j * height + k];
				for (int l = 0; l < 8; l++)
					d[k] |= ((tmpd >> (7 - l)) & 1) << (j * 8 + l);
			}
		}
	}

	delete [] tmpdata;
	delete [] off;

	// fix space symbol
	if (widths[32] == 0)
	{
		widths[32] = avgwidth;
		data[32] = data[0] + num_allocs * height;
		num_allocs++;

		for (i = 0; i < height; i++)
		{
			data[32][i] = 0;
		}
	}

	data[0] = (ULONGLONG *)SPrealloc(data[0], (num_allocs + 1) * height * sizeof(ULONGLONG));
	if (data[0] == NULL)
		return FALSE;
	for (int k = 1, na = 0; k < num; k++)
	{
		if (data[k] != NULL)
			data[k] = data[0] + (na++) * height;
	}


//	msg("FONT %s loaded OK.\n", name);

	unloaded = false;
	return TRUE;
}

BOOL Font::Unload()
{
	if (data != NULL)
	{
		//for (int i = 0; i < num; i++)
		//	SPSafeDeleteArray(data[i]);
		SPSafeFree(data[0]);
	}
	SPSafeDeleteArray(data);
	SPSafeDeleteArray(widths);
	num = 0;

	unloaded = true;
	return TRUE;
}


///////////////////////////////////////////////////////////////////

FontManager::FontManager()
{
	fonthash.SetN(resource_num_hash);
	def = NULL;
}

FontManager::~FontManager()
{
	SPSafeDelete(def);
}

Font *FontManager::GetFont(char *fname)
{
	if (fname == NULL || fname[0] == '\0')
		return GetDefaultFont();

	static Font testfont;
	testfont.SetConstName(fname);
	Font *fnt = fonthash.Get(testfont);
	testfont.name = NULL;
	if (fnt != NULL)
		return fnt;
	
	fnt = new Font();
	fnt->SetName(fname);
	if (!fnt->Load())
	{
		delete fnt;
		return NULL;
	}
	
	fonthash.Add(fnt);
	return fnt;
}

Font *FontManager::GetDefaultFont()
{
	if (def != NULL)
		return def;
	if (console == NULL || console->cur_fnt == NULL)
	{
		msg_error("Default font not found.\n");
		return NULL;
	}
	def = new Font();
	if (def == NULL)
		return NULL;

	def->num = 256;
	def->height = console->font_height;
	def->widths = new int [def->num];
	def->data = new ULONGLONG* [def->num];
	int chsiz = ((console->font_width + 7) / 8);
	for (int i = 0; i < def->num; i++)
	{
		BYTE *d = console->cur_fnt + i * chsiz * def->height;
		def->data[i] = new ULONGLONG [def->height];
		def->widths[i] = console->font_width;
		int fw = console->font_width - 1;
		for (int x = 0; x < def->height; x++)
		{
			def->data[i][x] = 0;
			for (int k = 0; k < console->font_width; k++)
				def->data[i][x] |= ((d[x] >> (fw - k)) & 1) << k;
		}
	}
	return def;
}

BOOL FontManager::ClearFontData()
{
	Font *cur = fonthash.GetFirst();
	while (cur != NULL)
	{
		cur->Unload();
		cur = fonthash.GetNext(*cur);
	}
	return TRUE;
}
