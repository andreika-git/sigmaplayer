//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - DivX3->MPEG-4 transcoder header file
 *  \file       divx.h
 *  \author     bombur
 *  \version    0.1
 *  \date       1.04.2007
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

#ifndef SP_DIVX_H
#define SP_DIVX_H

#include <bitstream.h>

#ifdef DIVX_INTERNAL

#define DIVX_I_TYPE 1
#define DIVX_P_TYPE 2

// run length table
#define DIVX_MAX_RUN    64
#define DIVX_MAX_LEVEL  64

typedef struct DIVX_VLC 
{
    int bits;
    signed short (*table)[2]; ///< code, bits
    int table_size, table_allocated;
} DIVX_VLC;

typedef struct DIVX_RL_VLC_ELEM 
{
    signed short level;
    signed char len;
    BYTE run;
} DIVX_RL_VLC_ELEM;

typedef struct DIVX_RL_TABLE
{
    DWORD n;                         ///< number of entries of table_vlc minus 1 
    int last;                      ///< number of values for last = 0 
    const WORD (*table_vlc)[2];
    const signed char *table_run;
    const signed char *table_level;
    BYTE *index_run[2];         ///< encoding only 
    signed char *max_level[2];          ///< encoding & decoding 
    signed char  *max_run[2];            ///< encoding & decoding 
    DIVX_VLC vlc;                       ///< decoding only
    DIVX_RL_VLC_ELEM *rl_vlc[32];       ///< decoding only 
} DIVX_RL_TABLE;

typedef struct DIVX_SCAN_TABLE
{
    const BYTE *scantable;
    int permutated[64];
	int inv_permutated[64];
    BYTE raster_end[64];
} DIVX_SCAN_TABLE;

typedef struct DIVX_MV_TABLE 
{
    int n;
    const WORD *table_mv_code;
    const BYTE *table_mv_bits;
    const BYTE *table_mvx;
    const BYTE *table_mvy;
    WORD *table_mv_index; /* encoding: convert mv to index in table_mv */
    DIVX_VLC vlc;                /* decoding: vlc */
} DIVX_MV_TABLE;

////////////////////////////////////////////////

#ifdef WIN32
#pragma pack(1)
#endif

typedef struct ATTRIBUTE_PACKED
{
	BYTE len;
	WORD bits;
} DIVX_BITS_LEN;

#ifdef WIN32
#pragma pack()
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

int divx_transcode_preinit(void);
int divx_transcode_predeinit(void);

int divx_transcode_init(int width, int height, int time_incr_res);
int divx_transcode_deinit(void);

int divx_transcode(bitstream_callback);

BOOL divx_is_key_frame(void);

#ifdef __cplusplus
}
#endif

#endif // of SP_DIVX_H
