//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - CDDA (CD-Audio) player header file
 *  \file       cdda.h
 *  \author     bombur
 *  \version    0.1
 *  \date       2.08.2004
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

#ifndef SP_CDDA_H
#define SP_CDDA_H

class CddaTrack
{
public:
	/// ctor
	CddaTrack()
	{
		idx = -1;
		frame_idx = -1;
		length = -1;
	}

	int idx;
	int frame_idx;
	
	int length;		// in frames
};


class Cdda
{
public:
	/// ctor
	Cdda()
	{
		cur = -1;
		track1 = -1;
		track2 = -1;
		total_length = -1;

		cur_trackid = -1;
		old_trackid = -1;
		saved_pts = -1;

		feof = false;
		playing = false;
	}
	// current frame
	int cur;

	int cur_trackid, old_trackid;
	LONGLONG saved_pts;

	// first and last
	int track1, track2;

	SPClassicList<CddaTrack> tracks;
	int total_length;	// in frames

	bool feof;
	bool playing;
};


////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

#include <libsp/sp_mpeg.h>


Cdda *cdda_open();
BOOL cdda_close();
	
/// Play CD-Audio disc or track
int cdda_play(char *path);

/// Get number of tracks
int cdda_get_numtracks();
/// Get track length, in seconds
int cdda_get_length(char *path);

/// Advance playing
int cdda_player_loop();

/// Pause playing
BOOL cdda_pause();

/// Stop playing
BOOL cdda_stop();

/// Returns TRUE if track ended.
BOOL cdda_feof();

void cdda_setdebug(BOOL ison);
BOOL cdda_getdebug();

/// Seek to given time and play
BOOL cdda_seek(int seconds, int from_frame = -1);
BOOL cdda_seek_track(int track);

void cdda_get_cur(int *track);

void cdda_forward();
void cdda_rewind();

/// Read CDDA data (used by media)
int cdda_read(BYTE *buf, int numblocks);

#ifdef __cplusplus
}
#endif

#endif // of SP_CDDA_H
