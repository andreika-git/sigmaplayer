//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - AVI player header file
 *  \file       avi.h
 *  \author     bombur
 *  \version    0.1
 *  \date       07.03.2007
 *
 * Based on AVILIB (C) 1999 Rainer Johanni <Rainer@Johanni.de>
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

#ifndef SP_AVI_H
#define SP_AVI_H

#include "video.h"

#ifdef VIDEO_INTERNAL

#ifdef WIN32
#pragma pack(1)
#endif

typedef struct AviOldIndexEntry
{
	DWORD    chunkid;
	DWORD    flags;
    DWORD    offset;
    DWORD    size;
} AviOldIndexEntry;

typedef struct AviNewIndexEntry
{
	DWORD    offset;
	DWORD    size;
} AviNewIndexEntry;

typedef struct AviSuperIndexEntry
{
	LONGLONG qwOffset;			// absolute file offset
    DWORD    dwSize;			// size of index chunk at this offset
    DWORD    dwDuration;		// time span in stream ticks
} AviSuperIndexEntry;

typedef struct AviSuperIndex
{
    char  fcc[4];
    DWORD dwSize;				// size of this chunk
    WORD  wLongsPerEntry;		// size of each entry in aIndex array (must be 8 for us)
    BYTE  bIndexSubType;		// future use. must be 0
    BYTE  bIndexType;			// one of AVI_INDEX_* codes
    DWORD nEntriesInUse;		// index of first unused member in aIndex array
    char  dwChunkId[4];			// fcc of what is indexed
    DWORD dwReserved[3];		// meaning differs for each index type/subtype.
    AviSuperIndexEntry *idx;	// where are the ix## chunks
} AviSuperIndex;

typedef struct AviNewIndex
{
	char     fcc[4];          // ’ix##’
	DWORD    cb;
	WORD     wLongsPerEntry;  // must be sizeof(aIndex[0])/sizeof(DWORD)
	BYTE     bIndexSubType;   // must be 0
	BYTE     bIndexType;      // must be AVI_INDEX_OF_CHUNKS
	DWORD    nEntriesInUse;   //
	char     dwChunkId[4];    // ’##dc’ or ’##db’ or ’##wb' etc..
	LONGLONG qwBaseOffset;    // all dwOffsets in aIndex are relative to this
	DWORD    dwReserved3;     // must be 0
} AviNewIndex;

typedef struct RIFF_BITMAPINFOHEADER
{
  DWORD bi_size;
  DWORD bi_width;
  DWORD bi_height;
  WORD  bi_planes;
  WORD  bi_bit_count;
  DWORD bi_compression;
  DWORD bi_size_image;
  DWORD bi_x_pels_per_meter;
  DWORD bi_y_pels_per_meter;
  DWORD bi_clr_used;
  DWORD bi_clr_important;
} RIFF_BITMAPINFOHEADER;

#ifdef WIN32
#pragma pack()
#endif

enum AVI_FLAGS
{
	AVI_FLAG_HASINDEX       = 0x00000010,	// Index at end of file
	AVI_FLAG_MUSTUSEINDEX   = 0x00000020,
	AVI_FLAG_ISINTERLEAVED	= 0x00000100,
	AVI_FLAG_TRUSTCKTYPE	= 0x00000800,	// Use CKType to find key frames
	AVI_FLAG_WASCAPTUREFILE	= 0x00010000,
	AVI_FLAG_COPYRIGHTED	= 0x00020000,
};

enum AVI_IDX1_FLAGS
{
	AVI_IDX1_FLAG_KEYFRAME = 0x00000010,

	AVI_IDX2_FLAG_KEYFRAME = 0x80000000,
};


const int max_buf_idx_size[2] = { 32768 /* idx1 */, 32768/2 /* indx2.0 */ };
const int index_block_size = 1024;

/// AVI container player class
class VideoAvi : public Video
{
public:
	/// ctor
	VideoAvi();

	/// dtor
	virtual ~VideoAvi();

public:
	virtual BOOL Parse();
	virtual VIDEO_CHUNK_TYPE GetNext(BYTE *buf, int buflen, int *pos, int *left, int *len);
	/// Returns 0 if found, -1 if failed, 1 for EOF.
	virtual int GetNextIndexes();
	/// Find next key-frame in raw mode
	virtual int GetNextKeyFrame();

	RIFF_BITMAPINFOHEADER *bitmap_info_header;

	char video_tag1[4], video_tag2[4];
	int video_strn, avi_flags;
	LONGLONG movi_start;
	LONGLONG idx_start;
	LONGLONG idx_offset;	// base offset - must be added to idx offsets
	int num_idx;
	
	union
	{
		AviOldIndexEntry *buf_idx;
		AviNewIndexEntry *buf_newidx;
	};
	int buf_idx_size;

	bool has_indx;
	AviSuperIndex video_superindex;
	int cur_indx_buf_idx, cur_indx_buf_pos, cur_buf_idx_size;
	LONGLONG cur_indx_base_offset;

	int cur_old_buf_idx, cur_buf_video_idx;

protected:
	int AddIndexBlock();
	int AddIdx1Block();
	int AddIdx2Block();

	void ResetRecoveryMode();
};

#endif

///////////////////////////////////////////////////////////////

#endif // of SP_AVI_H
