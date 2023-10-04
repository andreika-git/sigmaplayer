//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - libdvdnav cache replacement header file
 *  \file       dvd/dvd_misc.h
 *  \author     bombur
 *  \version    0.1
 *  \date       1.02.2006
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


#ifndef SP_DVD_MISC_H
#define SP_DVD_MISC_H

#ifdef __cplusplus
extern "C" {
#endif

// all basic funcs are already in <contrib/libdvdnav/src/read_cache.h>

/// Set memory chunk enough to hold MPEG_NUM_BUFS cache buffers
int dvdnav_set_cache_memory(dvdnav_t* dvd_self, uint8_t *block_mem);
	
// used to determine if dvdnav_read_cache_block() was called
// (we want to pass a new block only if the real data was returned)
void dvdnav_reset_cache_flag(dvdnav_t* dvd_self);
int dvdnav_get_cache_flag(dvdnav_t* dvd_self);


// defined in dvd_misc.c because of dvdnav_internal.h

// like dvdnav function, but uses 'mode'
int8_t dvd_get_spu_logical_stream(dvdnav_t *dvd, uint8_t subp_num, int mode);
// like dvdnav function
int8_t dvd_get_audio_logical_stream(dvdnav_t *dvd, uint8_t audio_num);

/// set spu stream to vm register (tricky!)
int dvd_set_spu_stream(dvdnav_t *dvd, int stream, int mode);

/// set audio stream vm register (tricky!)
int dvd_set_audio_stream(dvdnav_t *dvd, int stream);

/// time in PTS, title/chapter - 0-based.
int dvd_get_time(dvdnav_t *dvd, int title, int chapter, uint64_t *time);
/// time in PTS, title/chapter - 0-based.
int dvd_get_chapter(dvdnav_t *dvd, int title, uint64_t time, int *chapter);

int64_t dvdnav_convert_time(dvd_time_t *time);

// Disc ID (CRC32) stuff:
void dvd_make_crc_table();
uint32_t dvd_get_disc_ID(const char *use_name);

void dvd_reset_vm(dvdnav_t *dvd);

int dvd_skip_sector(dvdnav_t *dvd);

#ifdef __cplusplus
}
#endif

#endif // of SP_DVD_MISC_H
