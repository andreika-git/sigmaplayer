//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - input media header file
 *  \file       media.h
 *  \author     bombur
 *  \version    0.1
 *  \date       12.10.2004
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

#ifndef SP_MEDIA_H
#define SP_MEDIA_H


#ifdef __cplusplus
extern "C" {
#endif

int media_open(const char *dvdpath, MEDIA_TYPES mtype);
void set_media_type(MEDIA_TYPES mtype);

typedef enum
{
	MEDIA_EVENT_OK = 0,
	MEDIA_EVENT_NOP = 1,
	MEDIA_EVENT_STOP = 8,
	MEDIA_EVENT_PARTIAL = 0x10,

	MEDIA_EVENT_AUDIO = 0x1000,
	MEDIA_EVENT_VIDEO = 0x1001,
	MEDIA_EVENT_AUDIO_PARTIAL = MEDIA_EVENT_AUDIO | MEDIA_EVENT_PARTIAL,
	MEDIA_EVENT_VIDEO_PARTIAL = MEDIA_EVENT_VIDEO | MEDIA_EVENT_PARTIAL,
} MEDIA_EVENT;

int media_get_next_block(BYTE **buf, MEDIA_EVENT *event, int *len);

// call this if we needed the block
int media_accept_block();

int media_free_block(BYTE *data);

/// used by AVI. 
/// \todo: UGLY!
int media_read_block(BYTE **buf, MEDIA_EVENT *event, int *len);
int media_seek_curleft();

// called by cache if cache-miss happens
int media_skip_buffer(BYTE **buf);

int media_close();

int media_seek(int blockid, int from);

int media_rewind();

// used by mpeg player to seek 'bad' files
LONGLONG media_get_filepos();
void media_set_filepos(LONGLONG);

const char *media_geterror();

#ifndef DVDNAV_H_INCLUDED
typedef struct dvdnav_s dvdnav_t;
#endif

int media_update_fip();


#ifdef __cplusplus
}
#endif

#endif // of SP_MEDIA_H
