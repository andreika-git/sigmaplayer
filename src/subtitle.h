//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - Subtitles for video player header file
 *  \file       subtitle.h
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

#ifndef SP_SUBTITLE_H
#define SP_SUBTITLE_H

/// 1 Subtitle record
typedef struct SubtitleRecord
{
	short delta_time;		// in 1/100 secs
	WORD string_length;		// string length (not including '\0')
} SubtitleRecord;

typedef struct SubtitleFormat
{
	const char *name;
	const char *start[32];
	const char *end;
} SubtitleFormat;

typedef struct SubtitleFields
{
	int hour;
	int min;
	int sec;
	int msec;
	int sec256;
	int msec256;
	int delta_sec;
	int delta_msec;
	int frames;
	int line_number;
	bool is_set;
} SubtitleFields;

#define SUB_TOKEN_HOUR1				0x1
#define SUB_TOKEN_HOUR2				0x101
#define SUB_TOKEN_HOUR1_1			0x2
#define SUB_TOKEN_HOUR2_1			0x102

#define SUB_TOKEN_MIN1				0x3
#define SUB_TOKEN_MIN2				0x103

#define SUB_TOKEN_SEC1				0x4
#define SUB_TOKEN_SEC2				0x104

#define SUB_TOKEN_DSEC1				0x5
#define SUB_TOKEN_DSEC2				0x105

#define SUB_TOKEN_MSEC1				0x6
#define SUB_TOKEN_MSEC2				0x106

#define SUB_TOKEN_SEC1_256			0x7
#define SUB_TOKEN_DSEC1_256			0x8

#define SUB_TOKEN_DELTA_SECS1		0x9
#define SUB_TOKEN_DELTA_SECS2		0x109

#define SUB_TOKEN_DELTA_MSECS1		0xa
#define SUB_TOKEN_DELTA_MSECS2		0x10a

#define SUB_TOKEN_FRAMES1			0xb
#define SUB_TOKEN_FRAMES2			0x10b

#define SUB_TOKEN_LINE_NUMBER		0xc

// special
#define SUB_TOKEN_SKIP_DIGITS		0x20
#define SUB_TOKEN_SKIP_DIGITS_4		0x24
#define SUB_TOKEN_SKIP_TO_NEXT		0x30
#define SUB_TOKEN_OPTIONAL_NEXT		0x40
#define SUB_TOKEN_OPTIONAL_NEXT_2	0x41

#define SUB(s) (const char *)s

enum SUB_CUR_FORMAT
{
	SUB_CUR_FORMAT_STRING = 0,
	SUB_CUR_FORMAT_DIGITS,
	SUB_CUR_FORMAT_DIGITS_SIGN,
};

enum SUB_CUR_MODE
{
	SUB_CUR_MODE_NORMAL = 0,
	SUB_CUR_MODE_SEARCH_NEXT,
	SUB_CUR_MODE_OPTIONAL,
};

enum SUB_CUR_FIELD
{
	SUB_CUR_FIELD_NONE = 0,
	SUB_CUR_FIELD_HOUR,
	SUB_CUR_FIELD_MIN,
	SUB_CUR_FIELD_SEC,
	SUB_CUR_FIELD_SEC256,
	SUB_CUR_FIELD_MSEC,
	SUB_CUR_FIELD_MSEC256,
	SUB_CUR_FIELD_DELTA_SEC,
	SUB_CUR_FIELD_DELTA_MSEC,
	SUB_CUR_FIELD_FRAMES,
	SUB_CUR_FIELD_LINE_NUMBER,
};

enum SUBTITLE_CHARSET
{
	SUBTITLE_CHARSET_DEFAULT,
	SUBTITLE_CHARSET_ISO8859_1,
	SUBTITLE_CHARSET_ISO8859_2,
	SUBTITLE_CHARSET_CP1251,
	SUBTITLE_CHARSET_KOI8R,
};

const int max_string_length = 8192;
const int read_buf_length = 4096;

///////////////////////////////////////////////////////////

class Subtitle
{
public:
	/// ctor
	Subtitle()
	{
		is_ready = false;
		data_idx = 0;
		data_start_pos = 0;
		fps = 0;
		cur_time = 0;
		last_256 = 0;
		last_256_add = 0;

		cur_record = 0;
		cur_last_record = -1;
		cur_pts = cur_last_pts = 0;
		cur_data_idx = 0;
		cur_data_pos = 0;
	}

	void Add(BYTE *str, int str_length, SubtitleFields result[2]);

	/// Get time, in 1/100s of second.
	short GetTime(const SubtitleFields & f);

public:
	SPList<SubtitleRecord> records;
	bool is_ready;

	int data_idx, data_start_pos;
	LONGLONG fps;
	SPString fname, name;

	int cur_time;
	int last_256, last_256_add;

	// the _next_ record we're waiting for
	int cur_record, cur_data_idx, cur_data_pos;
	int cur_last_record;
	LONGLONG cur_pts, cur_last_pts;
};


class SubtitleData
{
public:
	/// ctor
	SubtitleData()
	{
		ptr = NULL;
		cur = NULL;
		len = 0;
		left = max_string_length;
	}
	/// dtor
	~SubtitleData()
	{
		SPSafeFree(ptr);
	}

	BOOL Alloc(int add_size);

public:
	char *ptr, *cur;
	int len, left;
};

class Subtitles
{
public:
	/// ctor
	Subtitles();
	/// dtor
	~Subtitles();

	/// Reset subtitle parser.
	void Reset();

	void InitTranslationTable(SUBTITLE_CHARSET charset);

	/// Read more data to the buffer
	BYTE *ReadMore(int *left, bool force_read = false);

	/// Add string and return offset
	int AddString(BYTE *, int &length, bool attach);

	int Scan(BYTE *&buf, int &left, int line_num, int method, SubtitleFields fields[2]);

	void SavePos(int saveidx, BYTE *buf, int left);
	BOOL LoadPos(int saveidx, BYTE *&buf, int &left);

public:
	SPList<Subtitle *> subs;
	SPList<SubtitleData *> data;

	/// Current video file being played.
	SPString vid_file;

	int max_line_letters_cnt;

	BYTE buf[2][read_buf_length];
	int buf_idx;
	int buf_cnt[2];	// used to identify buffers (check for changes and avoid double reading)
	int buf_read_left[2];
	int buf_read_cnt;
	int fd;

	int cur_method;

	int cur_sub;

	int saved_bufidx[2], saved_left[2];
	int saved_bufcnt[2];
	BYTE *saved_buf[2];

	BYTE charset_translation_table[256];
	BYTE utf16_to_ansi_table[2048];

	bool is_start_of_file;
	bool is_utf8, is_utf16, is_utf16be;

protected:

	int ConvertUTF8(BYTE *buf, int *left);	// returns number of bytes read
	void ConvertUTF16(BYTE *buf, int *left);
	void ConvertUTF16BE(BYTE *buf, int *left);
	void FillUTFTable(SUBTITLE_CHARSET charset);
};

BOOL read_subtitles(char *video_fname);

BOOL delete_subtitles();

BOOL add_subtitle_file(char *fname);

BOOL show_subtitles(LONGLONG pts);

BOOL subtitle_next();

#endif // of SP_SUBTITLE_H
