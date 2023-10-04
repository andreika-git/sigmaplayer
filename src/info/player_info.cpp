//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - File info source file.
 *  \file       player_info.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       22.10.2006
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <pty.h>
#include <termios.h>
#include <sys/poll.h>
#include <sys/wait.h>

#include <libsp/sp_misc.h>
#include "player_info.h"

#ifdef PLAYER_INFO_EMBED
#include <libsp/sp_cdrom.h>
#include "script.h"
#endif

#include <contrib/libid3tag/id3tag.h>

extern "C"
{
#include <contrib/libid3tag/ucs4.h>
#include <contrib/libid3tag/latin1.h>
#include <contrib/libid3tag/cp1251.h>
}

#if 0
int strcasecmp (char *dst, char *src)
{
	int f, l;
	do 
	{
		f = tolower((unsigned char)(*(dst++)));
		l = tolower((unsigned char)(*(src++)));
	} while (f != 0 && (f == l));
	return(f - l);
}
#endif

///////////////////////////////////////////////////////

#define NetWordRotate(w) w = (WORD)((((w) & 0x00ffU) << 8) | (((w) & 0xff00U) >> 8))
#define NetDwordRotate(dw) dw = ((DWORD)( \
		(((DWORD)(dw) & (DWORD)0x000000ffUL) << 24) | \
		(((DWORD)(dw) & (DWORD)0x0000ff00UL) <<  8) | \
		(((DWORD)(dw) & (DWORD)0x00ff0000UL) >>  8) | \
		(((DWORD)(dw) & (DWORD)0xff000000UL) >> 24) ));

#ifdef WIN32
#pragma pack(1)
#endif

typedef struct JfifMarkerSize
{
	WORD marker;
	WORD size;

} ATTRIBUTE_PACKED JfifMarkerSize;

typedef struct JfifFrameMarker
{
	BYTE precision;
	WORD height;
	WORD width;
	BYTE numc;

} ATTRIBUTE_PACKED JfifFrameSize;

#ifdef WIN32
#pragma pack()
#endif

BOOL player_get_jpginfo(const char *fname)
{
#ifdef WIN32
	if (fname[0] == '/') fname++;
#endif

	FILE *fp = fopen(fname, "rb");
	if (fp == NULL)
	{
#ifndef PLAYER_INFO_EMBED
		printf("Errr\n");
#endif
		return FALSE;
	}

	WORD soi;
	fread(&soi, sizeof(soi), 1, fp);
	JfifMarkerSize marker;

	int width = -1, height = -1;
	int numc = -1;

	if (soi == (WORD)0xd8ff)
	{
		for (;;)
		{
			if (fread(&marker, sizeof(marker), 1, fp) != 1)
				break;
			// corrupted file... give it a chance...
			if ((marker.marker & 0xff) != 0xff)
			{
				fseek(fp, -1, SEEK_CUR);
				continue;
			}
			// SOF
			if ((marker.marker & 0xf0ff) == 0xc0ff 
				&& marker.marker != 0xc4ff && marker.marker != 0xccff)
			{
				JfifFrameMarker frame;
				if (fread(&frame, sizeof(frame), 1, fp) == 1)
				{
					NetWordRotate(frame.width);
					NetWordRotate(frame.height);
					width = frame.width;
					height = frame.height;
					numc = frame.numc;
				}
				break;
			}
			NetWordRotate(marker.size);
			if (marker.size < 2)
				break;
			if (marker.size > 2)
				fseek(fp, marker.size - 2, SEEK_CUR);
		}
	}
	bool wasany = false;
	if (width >= 0 && height >= 0)
	{
#ifdef PLAYER_INFO_EMBED
		script_framesize_callback(width, height);
#else
		printf("Dims\n%d\n%d\n", width, height);
#endif
		wasany = true;
	}
	if (numc == 1 || numc == 3)
	{
#ifdef PLAYER_INFO_EMBED
		script_colorspace_callback(numc);
#else
		printf("Clrs\n%d\n", numc);
#endif
		wasany = true;
	}
	if (!wasany)
	{
#ifdef PLAYER_INFO_EMBED
		printf("None\n");
#endif
	}
	fclose(fp);

	return TRUE;
}

BOOL player_get_id3(const char *fname, const char *charset)
{
	bool wasany = false;
	PLAYER_INFO_CHARSET info_charset = PLAYER_INFO_CHARSET_LATIN1;
	if (strcasecmp(charset, "cp1251") == 0)
		info_charset = PLAYER_INFO_CHARSET_CP1251;

#ifdef WIN32
	if (fname[0] == '/') fname++;
#endif
	struct id3_file *id3 = id3_file_open(fname, ID3_FILE_MODE_READONLY);
	if (id3 == NULL)
	{
#ifndef PLAYER_INFO_EMBED
		printf("Errr\n");
#endif
		return FALSE;
	}
	struct id3_tag *id3tag = id3_file_tag(id3);
	if (id3tag != NULL)
	{
		static char const *frms[] = { ID3_FRAME_TITLE, ID3_FRAME_ARTIST };
		for (int i = 0; i < 2; i++)
		{
			struct id3_frame *id3frame = id3_tag_findframe(id3tag, frms[i], 0);
			if (id3frame != NULL)
			{
				union id3_field field = id3frame->fields[1];
				id3_ucs4_t const *tmpstr = id3_field_getstrings(&field, 0);
				if (tmpstr != NULL)
				{
					int slen = id3_ucs4_latin1size(tmpstr);
					id3_latin1_t *str = new id3_latin1_t[slen + 1];
					if (str != NULL)
					{
						if (info_charset == PLAYER_INFO_CHARSET_CP1251)
							id3_cp1251_encode(str, tmpstr);
						else
							id3_latin1_encode(str, tmpstr);
					
						if (slen > 250)
							str[250] = '\0';
#ifdef PLAYER_INFO_EMBED
						if (i == 0)
							script_name_callback((char *)str);
						else if (i == 1)
							script_artist_callback((char *)str);
#else
						static char const *hdrs[] = { "Titl", "Atst" };
						printf("%s\n%s\n", hdrs[i], str);
#endif
						SPSafeDeleteArray(str);
						wasany = true;
					}
				}
			}
		}
	}
#ifndef PLAYER_INFO_EMBED
	if (!wasany)
		printf("None\n");
#endif
	if (id3 != NULL)
	{
		id3_file_close(id3);
		id3 = NULL;
	}
	
	return FALSE;
}

BOOL player_getinfo(const char *fname, const char *charset)
{
	const char *ext = strrchr(fname, '.');
	if (ext != NULL)
	{
		if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)
		{
			return player_get_jpginfo(fname);
		}
		else if (strcasecmp(ext, ".mp3") == 0 || strcasecmp(ext, ".wav") == 0 || strcasecmp(ext, ".mp2") == 0 || strcasecmp(ext, ".mp1") == 0 || strcasecmp(ext, ".mpa") == 0 || strcasecmp(ext, ".mus") == 0)
		{
			return player_get_id3(fname, charset);
		}
	}
	return FALSE;
}


#ifndef PLAYER_INFO_EMBED

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("Use: fileinfo.bin fname charset.\n");
		return 1;
	}

	player_getinfo(argv[1], argv[2]);

	fflush(stdout);
	return 0;
}

#endif
