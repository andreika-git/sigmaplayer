//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - Subtitles for video player source file.
 *  \file       subtitle.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       01.10.2008
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
#include <errno.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_fip.h>
#include <libsp/sp_khwl.h>
#include <libsp/sp_mpeg.h>
#include <libsp/sp_cdrom.h>

#include "script.h"
#include "script-internal.h"
#include "subtitle.h"


static const int max_allowed_playlist_subs = 2;

static Subtitles *sub = NULL;

static const SubtitleFormat sub_formats[] = 
{
	{	/* 0 */
		"SubRip",
		{	SUB(SUB_TOKEN_SKIP_DIGITS), "\n", SUB(SUB_TOKEN_HOUR1), ":", SUB(SUB_TOKEN_MIN1), ":", SUB(SUB_TOKEN_SEC1), SUB(SUB_TOKEN_OPTIONAL_NEXT), ",", SUB(SUB_TOKEN_OPTIONAL_NEXT), SUB(SUB_TOKEN_MSEC1), " --> ",
			SUB(SUB_TOKEN_HOUR2), ":", SUB(SUB_TOKEN_MIN2), ":", SUB(SUB_TOKEN_SEC2), SUB(SUB_TOKEN_OPTIONAL_NEXT), ",", SUB(SUB_TOKEN_OPTIONAL_NEXT), SUB(SUB_TOKEN_MSEC2), 
			NULL
		},
		"\n\n"
	},
	{	/* 1 */
		"SubViewer 1.0",
		{	"[", SUB(SUB_TOKEN_HOUR1), ":", SUB(SUB_TOKEN_MIN1), ":", SUB(SUB_TOKEN_SEC1), "]\n", 
			NULL
		},
		"\n"
	},
	{	/* 2 */
		"SubViewer 2.0",
		{	SUB(SUB_TOKEN_HOUR1), ":", SUB(SUB_TOKEN_MIN1), ":", SUB(SUB_TOKEN_SEC1), ".", SUB(SUB_TOKEN_DSEC1), ",",
			SUB(SUB_TOKEN_HOUR2), ":", SUB(SUB_TOKEN_MIN2), ":", SUB(SUB_TOKEN_SEC2), ".", SUB(SUB_TOKEN_DSEC2), "\n",
			NULL
		},
		"\n\n"
	},
	{	/* 3 */
		"DVDSubtitle",
		{	"{T ", SUB(SUB_TOKEN_HOUR1), ":", SUB(SUB_TOKEN_MIN1), ":", SUB(SUB_TOKEN_SEC1), ":", SUB(SUB_TOKEN_DSEC1), "\n",
			NULL
		},
		"\n}\n"
	},
	{	/* 4 */
		"DVD Architect",
		{	SUB(SUB_TOKEN_SKIP_DIGITS_4), "\t", SUB(SUB_TOKEN_HOUR1), ":", SUB(SUB_TOKEN_MIN1), ":", SUB(SUB_TOKEN_SEC1), ":", SUB(SUB_TOKEN_DSEC1), "\t",
			SUB(SUB_TOKEN_HOUR2), ":", SUB(SUB_TOKEN_MIN2), ":", SUB(SUB_TOKEN_SEC2), ":", SUB(SUB_TOKEN_DSEC2), "\t",
			NULL
		},
		"\n\n"
	},
	{	/* 5 */
		"MicroDVD",
		{	"{", SUB(SUB_TOKEN_FRAMES1), "}{", SUB(SUB_TOKEN_FRAMES2), "}",
			NULL
		},
		"\n"
	},
	{	/* 6 */
		"MPSub",
		{	SUB(SUB_TOKEN_DELTA_SECS1), SUB(SUB_TOKEN_OPTIONAL_NEXT), ".", SUB(SUB_TOKEN_OPTIONAL_NEXT), SUB(SUB_TOKEN_DELTA_MSECS1), " ", 
			SUB(SUB_TOKEN_DELTA_SECS2), SUB(SUB_TOKEN_OPTIONAL_NEXT), ".", SUB(SUB_TOKEN_OPTIONAL_NEXT), SUB(SUB_TOKEN_DELTA_MSECS2), "\n",
			NULL 
		},
		"\n\n"
	},
	{	/* 7 */
		"TMPlayer",
		{	SUB(SUB_TOKEN_HOUR1), ":", SUB(SUB_TOKEN_MIN1), ":", SUB(SUB_TOKEN_SEC1), ",", SUB(SUB_TOKEN_LINE_NUMBER), "=",
			NULL
		},
		"\n"
	},
	{	/* 8 */
		"SubSonic",
		{	"1 ", SUB(SUB_TOKEN_SEC1_256), ".", SUB(SUB_TOKEN_DSEC1_256), SUB(SUB_TOKEN_OPTIONAL_NEXT_2), " \\ ~:\\",
			NULL
		},
		"\n"
	},
	{	/* 9 */
		"SubStation Alpha",
		{	SUB(SUB_TOKEN_SKIP_TO_NEXT), ",", SUB(SUB_TOKEN_HOUR1_1), ":", SUB(SUB_TOKEN_MIN1), ":", SUB(SUB_TOKEN_SEC1), ".", SUB(SUB_TOKEN_DSEC1), ",",
			SUB(SUB_TOKEN_HOUR2_1), ":", SUB(SUB_TOKEN_MIN2), ":", SUB(SUB_TOKEN_SEC2), ".", SUB(SUB_TOKEN_DSEC2), ",",
			SUB(SUB_TOKEN_SKIP_TO_NEXT), ",", SUB(SUB_TOKEN_SKIP_TO_NEXT), ",", SUB(SUB_TOKEN_SKIP_TO_NEXT), ",", 
			SUB(SUB_TOKEN_SKIP_TO_NEXT), ",", SUB(SUB_TOKEN_SKIP_TO_NEXT), ",", SUB(SUB_TOKEN_SKIP_TO_NEXT), ",",
			NULL
		},
		"\n"
	},
	{
		NULL,
		{ NULL },
		NULL
	}
};

/////////////////////////////////////////////////////////////////////

BOOL SubtitleData::Alloc(int add_size)
{
	if (add_size > 0 && ptr != NULL)
	{
		ptr = (char *)SPrealloc((void *)ptr, max_string_length + add_size);
		left += add_size;
	}
	else
		ptr = cur = (char *)SPmalloc(max_string_length);
	if (ptr == NULL)
		return FALSE;
	cur[0] = '\0';
	return TRUE;
}

/////////////////////////////////////////////////////////////////////

Subtitles::Subtitles()
{
	Reset();

	saved_left[0] = saved_left[1] = 0;

	max_line_letters_cnt = 35;

	cur_sub = -1;
}

Subtitles::~Subtitles()
{
	data.DeleteObjects();
	subs.DeleteObjects();
}

void Subtitles::Reset()
{
	fd = -1;
	cur_method = -1;
	buf_idx = 0;
	buf_cnt[0] = buf_cnt[1] = 0;
	buf_read_left[0] = buf_read_left[1] = 0;
	buf_read_cnt = 1;
	
	is_start_of_file = true;
	is_utf8 = false; is_utf16 = false; is_utf16be = false;
}

void Subtitles::InitTranslationTable(SUBTITLE_CHARSET charset)
{
	static const BYTE koi_win[][2] = 
	{ 
		{ 0xe1, 0xc0 },		{ 0xe2, 0xc1 },		{ 0xf7, 0xc2 },		{ 0xe7, 0xc3 },
		{ 0xe4, 0xc4 },		{ 0xe5, 0xc5 },		{ 0xf6, 0xc6 },		{ 0xfa, 0xc7 },
		{ 0xe9, 0xc8 },		{ 0xea, 0xc9 },		{ 0xeb, 0xca },		{ 0xec, 0xcb },
		{ 0xed, 0xcc },		{ 0xee, 0xcd },		{ 0xef, 0xce },		{ 0xf0, 0xcf },
		{ 0xf2, 0xd0 },		{ 0xf3, 0xd1 },		{ 0xf4, 0xd2 },		{ 0xf5, 0xd3 },
		{ 0xe6, 0xd4 },		{ 0xe8, 0xd5 },		{ 0xe3, 0xd6 },		{ 0xfe, 0xd7 },
		{ 0xfb, 0xd8 },		{ 0xfd, 0xd9 },		{ 0xff, 0xda },		{ 0xf9, 0xdb },
		{ 0xf8, 0xdc },		{ 0xfc, 0xdd },		{ 0xe0, 0xde },		{ 0xf1, 0xdf },
		{ 0xc1, 0xe0 },		{ 0xc2, 0xe1 },		{ 0xd7, 0xe2 },		{ 0xc7, 0xe3 },
		{ 0xc4, 0xe4 },		{ 0xc5, 0xe5 },		{ 0xd6, 0xe6 },		{ 0xda, 0xe7 },
		{ 0xc9, 0xe8 },		{ 0xca, 0xe9 },		{ 0xcb, 0xea },		{ 0xcc, 0xeb },
		{ 0xcd, 0xec },		{ 0xce, 0xed },		{ 0xcf, 0xee },		{ 0xd0, 0xef },
		{ 0xd2, 0xf0 },		{ 0xd3, 0xf1 },		{ 0xd4, 0xf2 },		{ 0xd5, 0xf3 },
		{ 0xc6, 0xf4 },		{ 0xc8, 0xf5 },		{ 0xc3, 0xf6 },		{ 0xde, 0xf7 },
		{ 0xdb, 0xf8 },		{ 0xdd, 0xf9 },		{ 0xdf, 0xfa },		{ 0xd9, 0xfb },
		{ 0xd8, 0xfc },		{ 0xdc, 0xfd },		{ 0xc0, 0xfe },		{ 0xd1, 0xff },
		{ 0xb3, 0xa8 },		{ 0xa3, 0xb8 },		{ 0, 0 },
	};

	for (int i = 0; i < 256; i++)
		charset_translation_table[i] = (BYTE)i;

	if (charset == SUBTITLE_CHARSET_KOI8R)	// KOI8 to CP1251
	{
		for (int i = 0; koi_win[i][0] != '\0'; i++)
			charset_translation_table[koi_win[i][0]] = koi_win[i][1];
	}
}

void Subtitles::FillUTFTable(SUBTITLE_CHARSET charset)
{
	const static WORD utf_win1251[128] = 
	{
		0x0402,0x0403,0x201A,0x0453,0x201E,0x2026,0x2020,0x2021,0x20AC,0x2030,0x0409,0x2039,0x040A,0x040C,0x040B,0x040F,
		0x0452,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,0x0000,0x2122,0x0459,0x203A,0x045A,0x045C,0x045B,0x045F,
		0x00A0,0x040E,0x045E,0x0408,0x00A4,0x0490,0x00A6,0x00A7,0x0401,0x00A9,0x0404,0x00AB,0x00AC,0x00AD,0x00AE,0x0407,
		0x00B0,0x00B1,0x0406,0x0456,0x0491,0x00B5,0x00B6,0x00B7,0x0451,0x2116,0x0454,0x00BB,0x0458,0x0405,0x0455,0x0457,
		0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,0x041F,
		0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,0x0428,0x0429,0x042A,0x042B,0x042C,0x042D,0x042E,0x042F,
		0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,0x043F,
		0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,0x0448,0x0449,0x044A,0x044B,0x044C,0x044D,0x044E,0x044F,
	};

	const static WORD utf_iso8859_2[128] = 
	{
		0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,
		0x0090,0x0091,0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,0x009B,0x009C,0x009D,0x009E,0x009F,
		0x00A0,0x0104,0x02D8,0x0141,0x00A4,0x013D,0x015A,0x00A7,0x00A8,0x0160,0x015E,0x0164,0x0179,0x00AD,0x017D,0x017B,
		0x00B0,0x0105,0x02DB,0x0142,0x00B4,0x013E,0x015B,0x02C7,0x00B8,0x0161,0x015F,0x0165,0x017A,0x02DD,0x017E,0x017C,
		0x0154,0x00C1,0x00C2,0x0102,0x00C4,0x0139,0x0106,0x00C7,0x010C,0x00C9,0x0118,0x00CB,0x011A,0x00CD,0x00CE,0x010E,
		0x0110,0x0143,0x0147,0x00D3,0x00D4,0x0150,0x00D6,0x00D7,0x0158,0x016E,0x00DA,0x0170,0x00DC,0x00DD,0x0162,0x00DF,
		0x0155,0x00E1,0x00E2,0x0103,0x00E4,0x013A,0x0107,0x00E7,0x010D,0x00E9,0x0119,0x00EB,0x011B,0x00ED,0x00EE,0x010F,
		0x0111,0x0144,0x0148,0x00F3,0x00F4,0x0151,0x00F6,0x00F7,0x0159,0x016F,0x00FA,0x0171,0x00FC,0x00FD,0x0163,0x02D9,
	};

	DWORD i;
	for (i = 0; i < 127; i++)
		utf16_to_ansi_table[i] = (BYTE)i;
	for (i = 127; i < sizeof(utf16_to_ansi_table); i++)
		utf16_to_ansi_table[i] = ' ';
	if (charset == SUBTITLE_CHARSET_ISO8859_1)
	{
		for (i = 127; i < 256; i++)
			utf16_to_ansi_table[i] = (BYTE)i;
	}
	else if (charset == SUBTITLE_CHARSET_ISO8859_2)
	{
		for (i = 0; i < 128; i++)
		{
			int j = Min((size_t)utf_iso8859_2[i], sizeof(utf16_to_ansi_table));
			if (j > 0)
				utf16_to_ansi_table[j] = (BYTE)(i + 128);
		}
	}
	else
	{
		for (i = 0; i < 128; i++)
		{
			int j = Min((size_t)utf_win1251[i], sizeof(utf16_to_ansi_table));
			if (j > 0)
				utf16_to_ansi_table[j] = (BYTE)(i + 128);
		}
	}

	if (is_utf8 || is_utf16)
	{
		for (i = 0; i < 256; i++)
			charset_translation_table[i] = (BYTE)i;
	}
}

int Subtitles::AddString(BYTE *str, int &length, bool attach)
{
	int l = length + 1;
	if (l > max_string_length)
		return -1;

	int n = data.GetN();
	SubtitleData *d = data[n - 1];
	if (n == 0 || d->left < l)
	{
		if (!attach || n == 0 || !d->Alloc(l))
		{
			d = new SubtitleData();
			if (!d->Alloc(0))
				return -1;
			if (data.Add(d) < 0)
				return -1;
			n = data.GetN();
		}
	}

	char last_ds;
	if (attach && d->cur > d->ptr)
	{
		*(d->cur - 1) = last_ds = '\n';
	}
	else
		last_ds = 0;

	// replace all newlines now:
	char *dst = d->cur;
	int i = 0;
	int line_letters_cnt = 0, last_space_cnt = 0;
	for (BYTE *s = str; i < length; i++, s++)
	{
		char ds;
		if (*s == '|')
			ds = '\n';
		else if (*s == '\\' && s[1] == 'N')
		{
			ds = '\n';
			s++;
			i++;
		}
		else if (*s == '[' && s[1] == 'b' && s[2] == 'r' && s[3] == ']')
		{
			ds = '\n';
			s += 3;
			i += 3;
		}
		else
		{
			ds = charset_translation_table[(BYTE)*s];
			if (ds == ' ')
				last_space_cnt = line_letters_cnt;
		}
		if (line_letters_cnt == 0)
		{
			if (ds == ' ' || ds == '\n' || ds == '%')
			{
				continue;
			}
		} 
		else if (line_letters_cnt > max_line_letters_cnt)
		{
			if (last_space_cnt > 0)
			{
				int delta = line_letters_cnt - last_space_cnt;
				s -= delta;
				i -= delta;
				dst -= delta;
				line_letters_cnt = last_space_cnt;
				last_space_cnt = 0;
				ds = '\n';
			}
		}
		if (ds == '\n')
		{
			line_letters_cnt = 0;
			if (last_ds == '\n')
			{
				continue;
			}
		}
		else
			line_letters_cnt++;
		*dst++ = last_ds = ds;
	}

	if (last_ds == '\n' || last_ds == '%')
	{
		dst--;
	}
	*dst = '\0';
	int pos = d->cur - d->ptr;
	length = dst - d->cur + 1;
	d->cur += length;
	d->len += length;
	d->left -= length;
	length--;
	return pos;
}

BOOL Subtitles::Scan(BYTE *&buf, int &left, int line_num, int method, SubtitleFields fields[2])
{
	memset(fields, 0xff, sizeof(SubtitleFields) * 2);
	fields[1].is_set = false;

	SUB_CUR_MODE cur_mode = SUB_CUR_MODE_NORMAL;
	SUB_CUR_FORMAT cur_format = SUB_CUR_FORMAT_STRING;
	SUB_CUR_FIELD cur_field = SUB_CUR_FIELD_NONE;
	int cur_num_digits = 0;

	int field_idx = 0;
	const SubtitleFormat &fmt = sub_formats[method];
	for (int i = 0; fmt.start[i] != NULL; i++)
	{
		const char *str = fmt.start[i];
		if ((int)str < 0x1000)
		{
			cur_field = SUB_CUR_FIELD_NONE;
			
			if (str == SUB(SUB_TOKEN_SKIP_TO_NEXT))
			{
				cur_mode = SUB_CUR_MODE_SEARCH_NEXT;
				continue;
			}
			else if (str == SUB(SUB_TOKEN_OPTIONAL_NEXT))
			{
				cur_mode = SUB_CUR_MODE_OPTIONAL;
				continue;
			}
			else if (str == SUB(SUB_TOKEN_OPTIONAL_NEXT_2))
			{
				if (line_num > 0)
					cur_mode = SUB_CUR_MODE_OPTIONAL;
				continue;
			}
			if (str >= SUB(SUB_TOKEN_SKIP_DIGITS) && str <= SUB(SUB_TOKEN_SKIP_DIGITS_4))
			{
				cur_format = SUB_CUR_FORMAT_DIGITS;
				cur_num_digits = str - SUB(SUB_TOKEN_SKIP_DIGITS);
			}

			field_idx = ((int)str > 0x100) ? 1 : 0;
			switch ((int)str & 0xff)
			{
			case SUB_TOKEN_HOUR1:
				cur_format = SUB_CUR_FORMAT_DIGITS;
				cur_num_digits = 2;
				cur_field = SUB_CUR_FIELD_HOUR;
				break;
			case SUB_TOKEN_HOUR1_1:
				cur_format = SUB_CUR_FORMAT_DIGITS;
				cur_num_digits = 1;
				cur_field = SUB_CUR_FIELD_HOUR;
				break;
			case SUB_TOKEN_MIN1:
				cur_format = SUB_CUR_FORMAT_DIGITS;
				cur_num_digits = 2;
				cur_field = SUB_CUR_FIELD_MIN;
				break;
			case SUB_TOKEN_SEC1:
				cur_format = SUB_CUR_FORMAT_DIGITS;
				cur_num_digits = 2;
				cur_field = SUB_CUR_FIELD_SEC;
				break;
			case SUB_TOKEN_DSEC1:
				cur_format = SUB_CUR_FORMAT_DIGITS;
				cur_num_digits = 2;
				cur_field = SUB_CUR_FIELD_MSEC;
				break;
			case SUB_TOKEN_MSEC1:
				cur_format = SUB_CUR_FORMAT_DIGITS;
				cur_num_digits = 3;
				cur_field = SUB_CUR_FIELD_MSEC;
				break;
			case SUB_TOKEN_SEC1_256:
				cur_format = SUB_CUR_FORMAT_DIGITS;
				cur_num_digits = 0;
				cur_field = SUB_CUR_FIELD_SEC256;
				break;
			case SUB_TOKEN_DSEC1_256:
				cur_format = SUB_CUR_FORMAT_DIGITS;
				cur_num_digits = 2;
				cur_field = SUB_CUR_FIELD_MSEC256;
				break;
			case SUB_TOKEN_DELTA_SECS1:
				cur_format = SUB_CUR_FORMAT_DIGITS_SIGN;
				cur_num_digits = 0;
				cur_field = SUB_CUR_FIELD_DELTA_SEC;
				break;
			case SUB_TOKEN_DELTA_MSECS1:
				cur_format = SUB_CUR_FORMAT_DIGITS;
				cur_num_digits = 0;
				cur_field = SUB_CUR_FIELD_DELTA_MSEC;
				break;
			case SUB_TOKEN_FRAMES1:
				cur_format = SUB_CUR_FORMAT_DIGITS;
				cur_num_digits = 0;
				cur_field = SUB_CUR_FIELD_FRAMES;
				break;
			case SUB_TOKEN_LINE_NUMBER:
				cur_format = SUB_CUR_FORMAT_DIGITS;
				cur_num_digits = 1;
				cur_field = SUB_CUR_FIELD_LINE_NUMBER;
				break;
			}
				
		} else
		{
			cur_format = SUB_CUR_FORMAT_STRING;
			cur_field = SUB_CUR_FIELD_NONE;
		}
		
		int value = 0, sign = 1;
		while (left > 0)
		{
			int num_read = 0;
			SavePos(1, buf, left);
			switch (cur_format)
			{
			case SUB_CUR_FORMAT_STRING:
				{
					const char *ss = str;
					for (; *ss != '\0'; buf++, left--, num_read++)
					{
						if (left < 1)
						{
							if ((buf = ReadMore(&left)) == NULL)
							{
								num_read = 0;
								return FALSE;
							}
						}
						if (*buf == '\r')
							continue;
						if (*buf != *ss)
						{
							num_read = 0;
							if (!LoadPos(1, buf, left))
								return FALSE;
							break;
						}
						ss++;
					}
				}
				break;
			case SUB_CUR_FORMAT_DIGITS_SIGN:
				if (*buf == '-')
				{
					num_read++;
					buf++;
					left--;
					sign = -1;
					if (left < 1)
					{
						if ((buf = ReadMore(&left)) == NULL)
						{
							num_read = 0;
							return FALSE;
						}
					}
				}
			case SUB_CUR_FORMAT_DIGITS:
				value = 0;
				while (left > 0)
				{
					int max_num_read = cur_num_digits == 0 ? left : cur_num_digits;
					for ( ; num_read < max_num_read && left > 0 && isdigit(*buf); num_read++, buf++, left--)
					{
						value = value * 10 + (*buf - '0');
					}
					//if ((cur_num_digits > 0 && num_read == cur_num_digits) || (cur_num_digits == 0 && num_read > 0))
					if (left > 0)
						break;
					if ((buf = ReadMore(&left)) == NULL)
						return FALSE;
				}
				if (cur_num_digits > 0 && num_read != cur_num_digits)
				{
					num_read = 0;
					value = 0;
				}
				break;
			}
			
			if (num_read > 0)
			{
				value *= sign;

				switch (cur_field)
				{
				case SUB_CUR_FIELD_HOUR:
					fields[field_idx].hour = value;
					break;
				case SUB_CUR_FIELD_MIN:
					fields[field_idx].min = value;
					break;
				case SUB_CUR_FIELD_SEC:
					fields[field_idx].sec = value;
					break;
				case SUB_CUR_FIELD_SEC256:
					fields[field_idx].sec256 = value;
					break;
				case SUB_CUR_FIELD_MSEC:
					fields[field_idx].msec = (num_read == 2) ? value * 10 : value;
					break;
				case SUB_CUR_FIELD_MSEC256:
					fields[field_idx].msec256 = (num_read == 2) ? value * 10 : value;
					break;
				case SUB_CUR_FIELD_DELTA_SEC:
					fields[field_idx].delta_sec = value - 1;
					break;
				case SUB_CUR_FIELD_DELTA_MSEC:
					fields[field_idx].delta_msec = ((num_read == 1) ? value * 100 : value * 10) - 1;
					break;
				case SUB_CUR_FIELD_FRAMES:
					fields[field_idx].frames = value;
					break;
				case SUB_CUR_FIELD_LINE_NUMBER:
					fields[field_idx].line_number = value;
					break;
				case SUB_CUR_FIELD_NONE:
					break;
				}
				if (cur_field != SUB_CUR_FIELD_NONE)
					fields[field_idx].is_set = true;

				cur_mode = SUB_CUR_MODE_NORMAL;
			}
			else if (cur_mode == SUB_CUR_MODE_SEARCH_NEXT)
			{
				// skip only at current line
				buf++;
				left--;
				if (left < 1)
				{
					if ((buf = ReadMore(&left)) == NULL)
						return FALSE;
				}
				if (*buf == '\n' || left < 1)
					return FALSE;
				continue;
			}
			else if (cur_mode == SUB_CUR_MODE_OPTIONAL)
			{
				LoadPos(1, buf, left);
				cur_mode = SUB_CUR_MODE_NORMAL;
				break;
			}
			else
			{
				// failure - didn't match the template
				return FALSE;
			}

			cur_mode = SUB_CUR_MODE_NORMAL;
			break;
		}
		if (left < 1)
		{
			buf = sub->ReadMore(&left);
			if (buf == NULL)
				return FALSE;
		}
	}
	return TRUE;
}

void Subtitles::ConvertUTF16(BYTE *buf, int *left)
{
	int l = *left;
	BYTE *wb = buf;
	BYTE *rb = wb;
	for (; l > 0; l-=2, wb++, rb+=2)
	{
		WORD u = rb[0] | (rb[1] << 8);
		
		*wb = (u < sizeof(utf16_to_ansi_table)) ? utf16_to_ansi_table[u] : ' ';
	}

	*left = wb - buf;
}

void Subtitles::ConvertUTF16BE(BYTE *buf, int *left)
{
	int l = *left;
	BYTE *wb = buf;
	BYTE *rb = wb;
	for (; l > 0; l-=2, wb++, rb+=2)
	{
		WORD u = rb[1] | (rb[0] << 8);

		*wb = (u < sizeof(utf16_to_ansi_table)) ? utf16_to_ansi_table[u] : ' ';
	}

	*left = wb - buf;
}

int Subtitles::ConvertUTF8(BYTE *buf, int *left)
{
	int l = *left - 6;
	int d = 1;
	BYTE *wb = buf;
	BYTE *rb = wb;
	
	for (; l > 0; wb++, rb += d, l -= d)
	{
		WORD u = 0xffff;
		d = 1;
		if ((rb[0] & 0x80) == 0x00) 
			u = rb[0];
		else if ((rb[0] & 0xe0) == 0xc0 && (rb[1] & 0xc0) == 0x80) 
		{
			u = ((rb[0] & 0x1fL) << 6) | ((rb[1] & 0x3fL) << 0);
			if (u >= 0x00000080L)
				d = 2;
		}
		else if ((rb[0] & 0xf0) == 0xe0 && (rb[1] & 0xc0) == 0x80 && (rb[2] & 0xc0) == 0x80) 
		{
			u = ((rb[0] & 0x0fL) << 12) | ((rb[1] & 0x3fL) <<  6) | ((rb[2] & 0x3fL) <<  0);
			if (u >= 0x00000800L)
				d = 3;
		}
		else if ((rb[0] & 0xf8) == 0xf0 && (rb[1] & 0xc0) == 0x80 && (rb[2] & 0xc0) == 0x80 &&
			(rb[3] & 0xc0) == 0x80) 
		{
			u = ((rb[0] & 0x07L) << 18) | ((rb[1] & 0x3fL) << 12) |
				((rb[2] & 0x3fL) <<  6) | ((rb[3] & 0x3fL) <<  0);
			if (u >= 0x00010000L)
				d = 4;
		}
		else if ((rb[0] & 0xfc) == 0xf8 && (rb[1] & 0xc0) == 0x80 && (rb[2] & 0xc0) == 0x80 &&
			(rb[3] & 0xc0) == 0x80 && (rb[4] & 0xc0) == 0x80) 
		{
			u = ((rb[0] & 0x03L) << 24) | ((rb[1] & 0x3fL) << 18) |
				((rb[2] & 0x3fL) << 12) | ((rb[3] & 0x3fL) <<  6) | ((rb[4] & 0x3fL) <<  0);
			if (u >= 0x00200000L)
				d = 5;
		}
		else if ((rb[0] & 0xfe) == 0xfc && (rb[1] & 0xc0) == 0x80 && (rb[2] & 0xc0) == 0x80 &&
			(rb[3] & 0xc0) == 0x80 && (rb[4] & 0xc0) == 0x80 && (rb[5] & 0xc0) == 0x80) 
		{
			u = ((rb[0] & 0x01L) << 30) | ((rb[1] & 0x3fL) << 24) | ((rb[2] & 0x3fL) << 18) |
				((rb[3] & 0x3fL) << 12) | ((rb[4] & 0x3fL) <<  6) | ((rb[5] & 0x3fL) <<  0);
			if (u >= 0x04000000L)
				d = 6;
		}

		*wb = (u < sizeof(utf16_to_ansi_table)) ? utf16_to_ansi_table[u] : ' ';
	}

	*left = wb - buf;

	return rb - buf;
}

BYTE *Subtitles::ReadMore(int *left, bool force_read)
{
	// we don't want to read more data if subtitle format is unknown
	if (cur_method < 0 && !force_read)
		return NULL;
	int new_bufidx = (++buf_idx) & 1;
	BYTE *b = buf[new_bufidx];
	if (buf_read_cnt <= buf_cnt[new_bufidx])
	{
		*left = buf_read_left[new_bufidx];
	} else
	{
		int left0 = read(fd, b, read_buf_length);
		if (is_start_of_file)
		{
			if (b[0] == 0xff && b[1] == 0xfe)
			{
				sub->is_utf16 = true;
				msg("Subtitle: UTF-16 Detected!\n");
				FillUTFTable(params->info.subtitle_charset);
				b += 2;
				left0 -= 2;
			}
			if (b[0] == 0xfe && b[1] == 0xff)
			{
				sub->is_utf16be = true;
				msg("Subtitle: UTF-16BE Detected!\n");
				FillUTFTable(params->info.subtitle_charset);
				b += 2;
				left0 -= 2;
			}
			else if (b[0] == 0xef && b[1] == 0xbb && b[2] == 0xbf)
			{
				sub->is_utf8 = true;
				msg("Subtitle: UTF-8 Detected!\n");
				FillUTFTable(params->info.subtitle_charset);
				b += 3;
				left0 -= 3;
			}
			is_start_of_file = false;
		}
		*left = left0;

		// UTF8/16 conversion
		if (sub->is_utf16)
		{
			ConvertUTF16(b, left);
		}
		if (sub->is_utf16be)
		{
			ConvertUTF16BE(b, left);
		}
		else if (sub->is_utf8)
		{
			left0 -= ConvertUTF8(b, left);
			lseek(fd, -left0, SEEK_CUR);
		}

		buf_read_cnt++;
	}
	if (*left < 1)
	{
		if (*left < 0)
		{
			msg("Subtitle: cannot read file!\n");
		}
		return NULL;
	}
	buf_read_left[new_bufidx] = *left;
	buf_cnt[new_bufidx] = buf_read_cnt;
	buf_idx = new_bufidx;
	return b;
}

void Subtitles::SavePos(int saveidx, BYTE *buf, int left)
{
	saved_bufidx[saveidx] = buf_idx;
	saved_buf[saveidx] = buf;
	saved_left[saveidx] = left;
	saved_bufcnt[saveidx] = buf_cnt[buf_idx];
}

BOOL Subtitles::LoadPos(int saveidx, BYTE *&buf, int &left)
{
	if (buf_cnt[saved_bufidx[saveidx]] != saved_bufcnt[saveidx] || saved_buf[saveidx] == NULL)
		return FALSE;
	buf_idx = saved_bufidx[saveidx];
	buf = saved_buf[saveidx];
	left = saved_left[saveidx];
	return TRUE;
}

///////////////////////////////////////////////////////////////

void Subtitle::Add(BYTE *str, int str_length, SubtitleFields result[2])
{
	if (str_length > 0)
		sub->AddString(str, str_length, false);
	
	SubtitleRecord rec;
	rec.string_length = (WORD)str_length;
	rec.delta_time = GetTime(result[0]);
	records.Add(rec);
	
	if (result[1].is_set)
	{
		SubtitleRecord rec;
		rec.string_length = 0;
		rec.delta_time = GetTime(result[1]);
		records.Add(rec);
	}
}

short Subtitle::GetTime(const SubtitleFields & f)
{
	int new_time = cur_time;
	if (f.frames >= 0)
	{
		new_time = (int)(INT64(100000) * (LONGLONG)f.frames / fps);
	}
	else 
	{
		if (f.sec256 >= 0 && f.msec256 >= 0)
		{
			new_time = f.sec256 * 100 + f.msec256 / 10;
			if (new_time < last_256 || new_time > last_256 + 25600)
				last_256_add += 25600;
			last_256 = new_time;
			new_time += last_256_add;
		}
		if (f.delta_sec != -1)
		{
			new_time += (f.delta_sec + 1) * 100;
		}
		if (f.delta_msec != -1)
		{
			new_time += (f.delta_msec + 1) / 10;
		}

		if (f.sec >= 0)
		{
			new_time = f.hour * 360000 + f.min * 6000 + f.sec * 100 + f.msec / 10;
		}
	}
	int delta = new_time - cur_time;
	cur_time = new_time;
	return (short)delta;
}


/////////////////////////////////////////////////////////////////////

int SimilarityCompare(SPString *str1, SPString *str2)
{
	// this is a little bit slow
	SPString s[2];
	s[0] = *str1;
	s[1] = *str2;
	for (int i = 0; i < 2; i++)
	{
		int name_pos = s[i].ReverseFind('/');
		if (name_pos > 0)
			s[i] = s[i].Mid(name_pos + 1);
		name_pos = s[i].ReverseFind('.');
		if (name_pos >= 0)
			s[i] = s[i].Left(name_pos);
	}

	return sub->vid_file.Similar(s[1]) - sub->vid_file.Similar(s[0]);
}

BOOL read_subtitles(char *video_fname)
{
	static const char *subt_ext[] = { ".sub", ".srt", ".ssa", ".txt", NULL };
	int i;
	SPSafeDelete(sub);
	sub = new Subtitles;

	if (params == NULL)
		return FALSE;

	fip_write_string("LoAd");

	sub->InitTranslationTable(params->info.subtitle_charset);
	sub->max_line_letters_cnt = params->info.subtitle_wrap;

	// now add subtitle files
	for (i = 0; i < params->playlists.GetN(); i++)
	{
		if (params->playlists[i] == NULL)
			continue;
		SPList<Item *> &il = params->playlists[i]->items;
		
		for (int j = 0; j < il.GetN(); j++)
		{
			SPString &name = il[j]->name;
			// check extensions
			bool found = false;
			for (int k = 0; subt_ext[k] != NULL; k++)
			{
				if (name.FindNoCase(subt_ext[k]) == name.GetLength() - 4)
				{
					found = true;
					break;
				}
			}
			if (found)
			{
				if (sub->subs.GetN() < max_allowed_playlist_subs)
				{
					add_subtitle_file(name);
					// enable first user-selected subtitle file by default
					if (sub->subs.GetN() > 0)
						sub->cur_sub = 0;
				}

				// now remove item from the playlist
				if (params->playlists[i]->itemhash != NULL)
					params->playlists[i]->itemhash->Remove(*il[j]);
				SPSafeDelete(il[j]);
				il.Remove(j);
				j--;
			}
		}
	}

//!!!!!!!!!!!!!!!!!!!!
//add_subtitle_file("/cdrom/SUB/MicroDVD.sub");
//add_subtitle_file("/cdrom/SUB/MPSub.sub");
//add_subtitle_file("/cdrom/SUB/DVDSubtitle.sub");
//add_subtitle_file("/cdrom/SUB/SubViewer 1.0.sub");
//add_subtitle_file("/cdrom/SUB/SubSonic.sub");
//add_subtitle_file("/cdrom/SUB/SubRip.srt");

//add_subtitle_file("/cdrom/SUB/LOTR/DVD Architect.sub");
//add_subtitle_file("/cdrom/SUB/LOTR/DVDSubtitle.sub");
//add_subtitle_file("/cdrom/SUB/LOTR/MicroDVD.sub");
//add_subtitle_file("/cdrom/SUB/LOTR/MPSub.sub");
//add_subtitle_file("/cdrom/SUB/LOTR/SubRip.srt");
//add_subtitle_file("/cdrom/SUB/LOTR/SubSonic.sub");
//add_subtitle_file("/cdrom/SUB/LOTR/SubStation Alpha.ssa");
//add_subtitle_file("/cdrom/SUB/LOTR/SubViewer1.sub");
//add_subtitle_file("/cdrom/SUB/LOTR/SubViewer2.sub");
//add_subtitle_file("/cdrom/SUB/LOTR/TMPlayer.sub");
//add_subtitle_file("/cdrom/Scary_movie_RUS_2000.srt");

	// try adding curdir subtitle, if there's only one (and it wasn't added)
	if (sub->subs.GetN() == 0)
	{
		SPClassicList<SPString> files;
		DIR *dir = cdrom_opendir(params->folder);
		SPString name, path = params->folder;
		if (path.ReverseFind('/') != path.GetLength() - 1)
			path += "/";
		while (dir != NULL)
		{
			struct dirent *d = cdrom_readdir(dir);
			if (d == NULL)
				break;
			if ((d->d_name[0] == '.' && d->d_name[1] == '\0') ||
				(d->d_name[0] == '.' && d->d_name[1] == '.' && d->d_name[2] == '\0'))
				continue;
			struct stat64 statbuf;
			name = path + d->d_name;
			if (cdrom_stat(*name, &statbuf) < 0) 
			{
				msg("Subtitle: Cannot stat %s\n", *name);
				statbuf.st_mode = 0;
				//continue;
			}
			if (S_ISDIR(statbuf.st_mode))
				continue;
			
			// is it subtitle file?
			bool found = false;
			for (int k = 0; subt_ext[k] != NULL; k++)
			{
				if (name.FindNoCase(subt_ext[k]) == name.GetLength() - 4)
				{
					found = true;
					break;
				}
			}
			if (found)
			{
				found = false;
				for (int i = 0; i < sub->subs.GetN(); i++)
				{
					if (sub->subs[i]->fname.CompareNoCase(name) == 0)
					{
						found = true;
						break;
					}
				}
				if (!found)
					files.Add(name);
			}
		}
		cdrom_closedir(dir);

		sub->vid_file = video_fname;
		int name_pos = sub->vid_file.ReverseFind('/');
		if (name_pos > 0)
			sub->vid_file = sub->vid_file.Mid(name_pos + 1);
		name_pos = sub->vid_file.ReverseFind('.');
		if (name_pos >= 0)
			sub->vid_file = sub->vid_file.Left(name_pos);

		// now sort and cut the list
		files.Sort(SimilarityCompare);
		for (int i = 0; i < files.GetN() && sub->subs.GetN() < max_allowed_playlist_subs; i++)
		{
			add_subtitle_file(files[i]);
		}
	}

	return TRUE;
}

BOOL delete_subtitles()
{
	SPSafeDelete(sub);
	return TRUE;
}

BOOL add_subtitle_file(char *fname)
{
	if (sub == NULL)
		return FALSE;

	msg("Subtitle: Reading %s...\n", fname);

	sub->Reset();

	sub->fd = cdrom_open(fname, O_RDONLY);
	if (sub->fd < 0)
	{
		msg("Subtitle: cannot open file %s\n", fname);
		return FALSE;
	}

	int line_num = 0;
	Subtitle *news = NULL;
	
	sub->cur_method = -1;
	bool detected = false;

	BYTE str_buf[4096];

	for (;;)
	{
		int left = 0;
		BYTE *buf = sub->ReadMore(&left, !detected);
		if (buf == NULL)
			break;

		while (left > 0)
		{
			int method = MAX(sub->cur_method, 0);
			SubtitleFields result[2];
			bool found = false;
			do
			{
				sub->SavePos(0, buf, left);
				if (sub->Scan(buf, left, line_num, method, result))
				{
					found = true;
					break;
				}
				// try again
				if (sub->cur_method < 0)
				{
					if (!sub->LoadPos(0, buf, left))
					{
						close(sub->fd);
						return FALSE;
					}
				}
				method++;
			} while (sub->cur_method < 0 && sub_formats[method].name != NULL);
			// skip or need more data?
			if (!found)
			{
				do
				{
					while (left > 0 && *buf != '\n')
					{
						buf++;
						left--;
					}
				} while (left < 1 && (buf = sub->ReadMore(&left)) != NULL);
				do
				{
					while (left > 0 && (*buf == '\n' || *buf == '\r'))
					{
						buf++;
						left--;
					}
				} while (left < 1 && (buf = sub->ReadMore(&left)) != NULL);
				continue;
			}
			
			sub->cur_method = method;

			// add subtitle to the list
			if (!detected)
			{
				news = new Subtitle();
				news->fname = fname;
				news->name = fname;
				int name_pos = news->name.ReverseFind('/');
				if (name_pos > 0)
					news->name = news->name.Mid(name_pos + 1);
				name_pos = news->name.ReverseFind('.');
				if (name_pos >= 0)
					news->name = news->name.Left(name_pos);
				news->name = news->name.Left(10);

				if (sub->data.GetN() > 0)
				{
					news->data_idx = sub->data.GetN() - 1;
					SubtitleData *d = sub->data[news->data_idx];
					news->data_start_pos = d->cur - d->ptr;
				} else
				{
					news->data_idx = 0;
					news->data_start_pos = 0;
				}
				news->cur_data_idx = news->data_idx;
				news->cur_data_pos = news->data_start_pos;
				sub->subs.Add(news);
				detected = true;

				msg("Subtitle: * %s format detected!\n", sub_formats[sub->cur_method].name);
			}

			// now read the string
			BYTE *str = str_buf;
			const char *end = sub_formats[method].end;
			int str_length = 0;
			found = false;
			
			while (left > 0 && str_length < 4096)
			{
				found = true;
				sub->SavePos(1, buf, left);
				const char *e = end;
				do
				{
					for (; *e != '\0' && left > 0; buf++, left--)
					{
						if (*buf == '\r')
							continue;
						if (*buf != *e)
						{
							found = false;
							break;
						}
						e++;
					}
				} while (left < 1 && (buf = sub->ReadMore(&left)) != NULL);
				if (buf == NULL)
					break;
				if (found)
					break;
				sub->LoadPos(1, buf, left);
				if (*buf != '\r')
				{
					*str++ = *buf;
					str_length++;
				}
				
				buf++;
				left--;
				
				if (left < 1)
				{
					if ((buf = sub->ReadMore(&left)) == NULL)
					{
						close(sub->fd);
						return FALSE;
					}
				}
			}
			

			if (news->fps == 0 && result[0].frames > 0)
			{
				char buf[128];
				int l = MIN(str_length, 100);
				strncpy(buf, (const char *)str_buf, l);
				buf[l] = '\0';
				int f1 = 0, f2 = 0;
				sscanf(buf, "%d.%d", &f1, &f2);
				news->fps = f1 * 1000 + f2;
				continue;
			}

			if (result[0].line_number > 1)
			{
				// attach to previous string
				if (str_length > 0)
				{
					sub->AddString(str_buf, str_length, true);
					SubtitleRecord &rec = news->records[news->records.GetN() - 1];
					rec.string_length = (WORD)(rec.string_length + str_length + 1);
				}
			} else
			{
				news->Add(str_buf, str_length, result);
				line_num = (line_num + 1) & 1;
			}
		}

		// if didn't guess, no need to read more...
		if (sub->cur_method < 0)
		{
			msg("Subtitle: Error! Unknown subtitle file format.\n");
			close(sub->fd);
			return FALSE;
		}
	}

	news->is_ready = true;
	if (news->records.GetN() > 0)
	{
		news->cur_pts = news->records[0].delta_time * 900;
		news->cur_record = 0;
	}

	close(sub->fd);

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#if 0
{
int sub_idx = 0;
if (sub->subs.GetN() > sub_idx && sub->subs[sub_idx]->is_ready)
{
	int pi = sub->subs[sub_idx]->data_idx, pp = sub->subs[sub_idx]->data_start_pos, t = 0;
	for (int z = 0; z < sub->subs[sub_idx]->records.GetN(); z++)
	{
		int l = sub->subs[sub_idx]->records[z].string_length;
		char *s;
		t += sub->subs[sub_idx]->records[z].delta_time;
		if (l > 0)
		{
			s = sub->data[pi]->ptr + pp;
			pp += l + 1;
		}
		else
			s = "";
		if (sub->data.GetN() > 0)
		if (pp >= sub->data[pi]->len)
		{
			if (pi < sub->data.GetN() - 1)
				pi++;
			pp = 0;
		}
	}
}
}
#endif
	return TRUE;
}

BOOL show_subtitles(LONGLONG pts)
{
	if (sub == NULL || sub->cur_sub < 0 || pts < 0)
		return FALSE;

	Subtitle *cur = sub->subs[sub->cur_sub];

	const char *s = NULL;
	if (pts > cur->cur_pts)
	{
		for (int z = cur->cur_record; z < cur->records.GetN(); z++)
		{
			SubtitleRecord &r = cur->records[z];
			LONGLONG d_pts =  (LONGLONG)((int)r.delta_time * 900);

			if (z != cur->cur_record)
			{
				cur->cur_pts += d_pts;
				cur->cur_record = z;
				if (pts < cur->cur_pts)
					break;
			}

			cur->cur_last_pts = cur->cur_pts;
			cur->cur_last_record = z;
			int l = cur->records[cur->cur_record].string_length;
			if (l > 0 && sub->data.GetN() > 0)
			{
				s = sub->data[cur->cur_data_idx]->ptr + cur->cur_data_pos;
				cur->cur_data_pos += l + 1;
				if (cur->cur_data_pos >= sub->data[cur->cur_data_idx]->len)
				{
					if (cur->cur_data_idx < sub->data.GetN() - 1)
						cur->cur_data_idx++;
					cur->cur_data_pos = 0;
				}
			}
			else
				s = "";
		}
	} 
	else if (pts < cur->cur_last_pts && cur->cur_last_record > 0)
	{
		int z;
		for (z = cur->cur_last_record; z >= 0; z--)
		{
			SubtitleRecord &r = cur->records[z];
			LONGLONG d_pts = (LONGLONG)((int)r.delta_time * 900);

			cur->cur_pts = cur->cur_last_pts;
			cur->cur_record = cur->cur_last_record;
			cur->cur_last_record--;
			cur->cur_last_pts -= d_pts;
			
			int l = cur->records[z].string_length;
			if (l > 0 && sub->data.GetN() > 0)
			{
				cur->cur_data_pos -= l + 1;
				if (cur->cur_data_pos < 0)
				{
					if (cur->cur_data_idx > 0)
					{
						cur->cur_data_idx--;
						cur->cur_data_pos = sub->data[cur->cur_data_idx]->len - (l + 1);
					} else
						cur->cur_data_pos = 0;
				}
			}

			if (pts > cur->cur_last_pts)
				break;
		}
		if (z < 0 || cur->cur_last_pts == 0)
		{
			cur->cur_last_pts = 0;
			cur->cur_last_record = -1;
			if (cur->records.GetN() > 0)
			{
				cur->cur_pts = cur->records[0].delta_time * 900;
				cur->cur_record = 0;
			}
		}
		// don't show subtitles in reverse mode
		s = "";
	}

	if (s != NULL)
	{
		int t = (int)(pts / 900);
		msg("[%02d:%02d:%02d.%02d] %s\n\n", t/360000, (t % 360000) / 6000, (t % 6000) / 100, (t % 100), s);
		script_player_subtitle_callback(s);
		return TRUE;
	}
	return FALSE;
}

BOOL subtitle_next()
{
	if (sub == NULL || sub->subs.GetN() < 1 || params == NULL)
		return FALSE;
	if (++sub->cur_sub >= sub->subs.GetN())
		sub->cur_sub = -1;
	params->info.spu_stream = sub->cur_sub + 1;
	if (sub->cur_sub >= 0)
	{
		params->info.spu_lang = sub->subs[sub->cur_sub]->name;
		msg("Subtitle: * Set to '%s'.\n", *params->info.spu_lang);
	}
	else
	{
		params->info.spu_lang = SPString();
		msg("Subtitle: * Disabled.\n");
	}

	script_player_subtitle_callback("");
	return TRUE;
}
