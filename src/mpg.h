//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - MPEG1/MPEG2/VOB files player header file
 *  \file       mpg.h
 *  \author     bombur
 *  \version    0.1
 *  \date       07.05.2007
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

#ifndef SP_MPG_H
#define SP_MPG_H

#include "video.h"

#ifdef VIDEO_INTERNAL

/// MPEG container player class
class VideoMpg : public Video
{
public:
	/// ctor
	VideoMpg();

	/// dtor
	virtual ~VideoMpg();

public:
	virtual BOOL Parse();
	virtual VIDEO_CHUNK_TYPE GetNext(BYTE *buf, int buflen, int *pos, int *left, int *len);
	/// Returns 0 if found, -1 if failed, 1 for EOF.
	virtual int GetNextIndexes();
	/// Find next key-frame in raw mode
	virtual int GetNextKeyFrame();

	virtual int GetKeyFrame(LONGLONG time);
	virtual void UpdateTotalTime();

	/// Called from video.cpp
	int ProcessChunk(BYTE *buf, int buflen);

	BOOL GetTimeLength();
	
private:
	int mpeg_format, mpeg_aspect;
	bool new_frame_size;
	bool elementary_stream, is_cdxa;
	bool wait_for_new_pts, set_pts;
	bool is_totaltime_from_rate;
	int cur_rate;
	LONGLONG filesize, seek_filesize;
	LONGLONG video_pts;

	bool scan_for_length, seek_start, was_pack;
	LONGLONG start_time_pts, end_time_pts;
	LONGLONG totaltime_pts, pts_base;
	int fps;

	int packet_left;
	START_CODE_TYPES saved_sct;
	int saved_length;
	int saved_start_counter;
	bool saved_feed;
	MpegPacket *saved_packet;
	
	BYTE tmp_packet_header_buf[128];
	bool packet_header_incomplete;
	
	LONGLONG old_pts;
};

#endif

///////////////////////////////////////////////////////////////

#endif // of SP_MPG_H
