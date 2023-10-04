//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - Video player header file
 *  \file       video.h
 *  \author     bombur
 *  \version    0.1
 *  \date       07.03.2007
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

#ifndef SP_VIDEO_H
#define SP_VIDEO_H

typedef enum 
{
	VIDEO_CONTAINER_UNKNOWN = 0,
	
	VIDEO_CONTAINER_AVI = 1,
	VIDEO_CONTAINER_MPEG = 2,
	VIDEO_CONTAINER_QT = 3,

} VIDEO_CONTAINER_TYPE;

typedef enum 
{
	VIDEO_CHUNK_UNKNOWN = 0,
	VIDEO_CHUNK_VIDEO = 1,
	VIDEO_CHUNK_AUDIO = 2,
	VIDEO_CHUNK_VIDEO_PARTIAL = 3,
	VIDEO_CHUNK_AUDIO_PARTIAL = 4,
	VIDEO_CHUNK_SUBT = 5,

	VIDEO_CHUNK_FRAGMENT = 6,	// if chunk header is split
	VIDEO_CHUNK_HEADER = 7,		// MPEG header w/o audio/video data

	VIDEO_CHUNK_RECOVERY = 8,

	VIDEO_CHUNK_EOF = 100,
} VIDEO_CHUNK_TYPE;

typedef enum 
{
	RIFF_VIDEO_UNKNOWN = -1,
	RIFF_VIDEO_MPEG12 = 0,
	RIFF_VIDEO_MPEG4 = 1,
	RIFF_VIDEO_DIV3 = 2,
	
} RIFF_VIDEO_FORMAT;

typedef enum 
{
	VIDEO_FRAME_NONE = -1,
	VIDEO_FRAME_I = 0,
	VIDEO_FRAME_P,
	VIDEO_FRAME_B,
} VIDEO_FRAME_TYPE;

#ifdef VIDEO_INTERNAL

#include "divx.h"

extern bool video_msg;

typedef struct VideoKeyFrame
{
	int i;
	int len;
	LONGLONG offs;
} VideoKeyFrame;

static const int num_key_frames_block = 32768 / sizeof(VideoKeyFrame);
static const int min_delta_keyframes = 10;
static const int def_max_rev_incr = 10;
static const int saved_delta_pts_num = 25;

#define VIDEO_MAX_AUDIO_TRACKS 8

#define VIDEO_KEY_FRAME_NEXT INT64(-0x7ffffffffffffff0)
#define VIDEO_KEY_FRAME_PREV INT64(-0x7ffffffffffffff1)
#define VIDEO_KEY_FRAME_CONTINUE  INT64(-0x7ffffffffffffff2)

/// Generic base video player class
class Video
{
public:
	/// ctor
	Video();

	/// dtor
	virtual ~Video();

	virtual BOOL Parse() = 0;
	virtual VIDEO_CHUNK_TYPE GetNext(BYTE *buf, int buflen, int *pos, int *left, int *len) = 0;
	
	/// Returns 0 if found, -1 if failed, 1 for EOF.
	virtual int GetNextIndexes() = 0;
	/// Find next key-frame in raw mode
	virtual int GetNextKeyFrame() = 0;

	virtual int GetKeyFrame(LONGLONG time);
	virtual void UpdateTotalTime();

	int SetCurrentIndex(int i, int len, LONGLONG offs);
	int AddIndex(int i, int len, LONGLONG offs);

	/// Fix audio packet PTS if audio is far ahead video
	LONGLONG FixAudioPts(LONGLONG pts);

public:
	VIDEO_CONTAINER_TYPE type;

	bool playing, stopping, is_eof;
	MEDIA_EVENT event;
	int scale, rate;
	LONGLONG scr;
	LONGLONG good_delta_stc;
	LONGLONG saved_pts, displ_pts_base;
	bool wait_for_resync;

	RIFF_VIDEO_FORMAT video_fmt;
	RIFF_AUDIO_FORMAT audio_fmt;
	SPString video_fmt_str, audio_fmt_str;

	VIDEO_CHUNK_TYPE chunktype;
	int chunkleft;
	int video_packet_len, old_video_packet_len;
	bool look4chunk;
	bool test_for_qpel_gmc;
	BYTE chunkheader[8];

	BYTE *mux_buf[16], *allocated_mux_buf[16];
	int mux_numbufs, mux_bufsize;
	int allocated_numbufs;
	int allocated_mux_buf_size[16];
	bool no_partial;

	BYTE *divx_buf[16], *divx_base[16];
	int divx_cur_bufpos, divx_cur_bufleft;
	bool in_divx_packet;
	bool need_to_stop;
	// for divx3 optimizations
	bool audio_halfrate;

	//// file container reader - common vars:
	int fd;

	int width, height;
	float fps;
	int video_frames;
	bool qpel_flag, gmc_flag;
	char fourcc[5];
	
	RiffAudioTrack track[VIDEO_MAX_AUDIO_TRACKS];
	int cur_track, first_track, num_tracks;
	int cur_audio_bits;

	int total_frames;
	LONGLONG audio_delta, audio_total_bytes, audio_pts_offset;	// needed for PTS calcs.
	int num_idx;
	
	SPClassicList<SPList<VideoKeyFrame> > indexes;

	int abs_video_pos, video_pos_base, video_pos, last_key_pos, cur_key_pos;
	int audio_pos;
	int cur_key_idx, last_good_key_idx, last_key_idx;
	int frame_pos, last_frame_pos;
	int max_rev_incr;

	bool skip_fwd, skip_rev, searching, skip_wait, skip_waiting;
	int delayed_skip;	// used to set skip_fwd/rev if not ready immediately
	int last_keyframe_time;
	int skip_cnt;
	bool cancel_skip;

	LONGLONG cur_offs, video_offs, cur_key_offs;
	
	// for raw-mode seeking
	LONGLONG last_chunk_offs;
	int last_chunk_len;

	LONGLONG saved_video_pts;

private:
	/// Get given key-frame index
	VideoKeyFrame *GetIndex(int idx);

	void ResetAvgPts();

	LONGLONG saved_delta_pts[saved_delta_pts_num];
	LONGLONG avg_delta_pts, min_delta_pts, max_delta_pts, minus_delta_pts;
	int saved_delta_pts_idx;
};

LONGLONG video_lseek(LONGLONG off, int where);
int video_read(BYTE *buf, int len);


#endif

/// Play Video file
int video_play(char *filepath = NULL);

/// Advance playing
int video_loop();

/// Pause playing
BOOL video_pause();

/// Stop playing
BOOL video_stop();

/// Seek to given time and play
BOOL video_seek(int seconds);

int video_forward(BOOL fast);
int video_rewind(BOOL fast);

void video_set_audio_offset(LONGLONG offset);
LONGLONG video_get_audio_offset();

void video_setdebug(BOOL ison);
BOOL video_getdebug();

/// Set audio track (if -1 then cycle through all)
int video_set_audio_track(int track);

bool video_search_fourcc(DWORD fourcc, const DWORD *values);

////////////////////////////////
BOOL video_open(const char *filepath);
BOOL video_close();

LONGLONG video_lseek(LONGLONG off, int where);
int video_read(BYTE *buf, int len);
bool video_eof();

VIDEO_CHUNK_TYPE video_getnext(BYTE *buf, int buflen, int *pos, int *left, int *len);
void video_shiftpos(int offs);
void video_detect_formats();
void video_update_info();

int video_play_from_last_keyframe();

void video_decode_vol_header(BYTE *buf, int len);

#endif // of SP_VIDEO_H
