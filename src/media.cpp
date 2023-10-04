//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - media access source file. Uses libdvdnav.
 *  \file       media.cpp
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_khwl.h>
#include <libsp/sp_fip.h>
#include <libsp/sp_mpeg.h>
#include <libsp/sp_cdrom.h>

#include "player.h"
#include "dvd.h"
#include "audio.h"
#include "video.h"
#include "cdda.h"
#include "media.h"


static int cur_bufpos = 0, cur_bufleft = -1;
static int cur_chunkleft = -1;
static BYTE *cur_copybuf = NULL;
static MEDIA_TYPES media_type = MEDIA_TYPE_UNKNOWN;
// fip 'disc' index
static int fipdisc_i = 0;
static int update_counter = 0;
static int fd = -1;
static int numread = 0;
static LONGLONG file_pos = 0;

#ifdef INTERNAL_VIDEO_PLAYER
static VIDEO_CHUNK_TYPE chtype = VIDEO_CHUNK_UNKNOWN;
#endif

int media_open(const char *path, MEDIA_TYPES mtype)
{
	cur_bufleft = -1;
	media_type = mtype;
	numread = 0;
	file_pos = 0;
	if (mtype == MEDIA_TYPE_DVD)
	{
		MPEG_PACKET_LENGTH = 2048;
		MPEG_NUM_PACKETS = 512;

		if (dvd_open(path) < 0)
			return -1;

	} 
	else if (mtype == MEDIA_TYPE_CDDA)
	{
		MPEG_PACKET_LENGTH = 2352;
		MPEG_NUM_PACKETS = 512;

		if (!cdda_open())
 		{
			msg_error("Media: Couldn't open CDDA: %s\n", path);
			return -1;
		}
		
	}
#ifdef INTERNAL_VIDEO_PLAYER
	else if (mtype == MEDIA_TYPE_VIDEO)
	{
		MPEG_PACKET_LENGTH = 0;	// use variable packet size
		MPEG_NUM_PACKETS = 2048;

		if (!video_open(path))
 		{
			video_close();
			msg_error("Media: Couldn't open video: %s\n", path);
			return -1;
		}
	}
#endif
#ifdef INTERNAL_AUDIO_PLAYER
	else if (mtype == MEDIA_TYPE_AUDIO)
	{
		MPEG_PACKET_LENGTH = 4096;
		MPEG_NUM_PACKETS = 512;

		if (!audio_open(path))
 		{
			audio_close();
			msg_error("Media: Couldn't open audio: %s\n", path);
			return -1;
		}
	}
#endif

	cur_bufpos = 0; 
	cur_bufleft = mpeg_getbufsize(MPEG_BUFFER_1);
	cur_chunkleft = cur_bufleft;
	cur_copybuf = NULL;

	fipdisc_i = 0;
	fip_write_special(FIP_SPECIAL_CIRCLE_1 + fipdisc_i, 1);

	return 1;
}

void set_media_type(MEDIA_TYPES mtype)
{
	media_type = mtype;
}

inline int media_get_next_free(const MPEG_BUFFER which, MEDIA_EVENT *event, bool &update_fip)
{
	int ret = mpeg_find_free_blocks(which);
	if (ret == 0)
	{
		// the buffer is not ready yet - wait...
		*event = MEDIA_EVENT_NOP;
		return 0;
	}
	cur_bufpos = 0;
	cur_bufleft = mpeg_getbufsize(which);
	cur_chunkleft = cur_bufleft;
		
	if (update_counter++ >= 8)
	{
		update_fip = true;
		update_counter = 0;
	}
	return ret;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//extern int num_packets;
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

int media_get_next_block(BYTE **buf, MEDIA_EVENT *event, int *len)
{
	if (cur_bufleft < 0)
	{
		cur_bufleft = mpeg_getbufsize(MPEG_BUFFER_1);
	}
	bool update_fip = false;

	*event = MEDIA_EVENT_OK;
	
	if (media_type == MEDIA_TYPE_DVD)
	{
		*len = MPEG_PACKET_LENGTH;
		if (cur_bufleft < MPEG_PACKET_LENGTH)
		{
			if (media_get_next_free(MPEG_BUFFER_1, event, update_fip) == 0)
				return 0;
		}
		BYTE *tmpbuf = mpeg_getcurbuf(MPEG_BUFFER_1);

		BYTE *bb = tmpbuf + cur_bufpos;
		int ret = dvd_get_next_block(&bb, (int *)event, len);
		if (ret < 0)
			return -1;
		else if (ret)
			media_accept_block();
		*buf = bb;
	} 
#ifdef INTERNAL_VIDEO_MPEG_PLAYER
	else if (media_type == MEDIA_TYPE_MPEG)
	{
		*len = MPEG_PACKET_LENGTH;
		if (cur_bufleft < MPEG_PACKET_LENGTH)
		{
			if (media_get_next_free(MPEG_BUFFER_1, event, update_fip) == 0)
				return 0;
		}
		BYTE *tmpbuf = mpeg_getcurbuf(MPEG_BUFFER_1);

		if (cur_bufpos == 0)
		{
			//numread  = read(fd, tmpbuf, mpeg_getbufsize(MPEG_BUFFER_1));
			numread = video_read(tmpbuf, mpeg_getbufsize(MPEG_BUFFER_1));
			file_pos += numread;
			if (update_counter++ >= 8)
			{
				update_fip = true;
				update_counter = 0;
			}
			// sorry, we won't show the last few bytes...
			numread &= ~(MPEG_PACKET_LENGTH - 1);
			MPEG_PACKET_LENGTH = numread;
		}
		if (cur_bufpos + MPEG_PACKET_LENGTH > numread || numread < 1)
		{
			if (numread < 1 || video_eof())
			{
				*buf = tmpbuf + cur_bufpos;
				*event = MEDIA_EVENT_STOP;
				return 1;
			}
			else
			{
				*event = MEDIA_EVENT_NOP;	// retry?
				return 0;
			}
		}
		*buf = tmpbuf + cur_bufpos;
		media_accept_block();
	}
#endif
	else if (media_type == MEDIA_TYPE_CDDA)
	{
		*len = MPEG_PACKET_LENGTH;
		if (cur_bufleft < MPEG_PACKET_LENGTH)
		{
			if (media_get_next_free(MPEG_BUFFER_1, event, update_fip) == 0)
				return 0;
		}
		BYTE *tmpbuf = mpeg_getcurbuf(MPEG_BUFFER_1);

		if (cur_bufpos == 0)
		{
			numread  = cdda_read(tmpbuf, mpeg_getbufsize(MPEG_BUFFER_1) / MPEG_PACKET_LENGTH);
			numread *= MPEG_PACKET_LENGTH;
			if (update_counter++ >= 6)
			{
				update_fip = true;
				update_counter = 0;
			}
		}
		else if (cur_bufpos + MPEG_PACKET_LENGTH > numread)
		{
			if (cdda_feof())
			{
				*event = MEDIA_EVENT_STOP;
			}
			else
			{
				*event = MEDIA_EVENT_NOP;	// retry?
				return 0;
			}
		}
		*buf = tmpbuf + cur_bufpos;
		media_accept_block();
	}
#ifdef INTERNAL_VIDEO_PLAYER
	else if (media_type == MEDIA_TYPE_VIDEO)
	{
		if (cur_bufleft < 1)
		{
			if (media_get_next_free(MPEG_BUFFER_1, event, update_fip) == 0)
				return 0;
			cur_copybuf = NULL;
		}

		BYTE *tmpbuf = mpeg_getcurbuf(MPEG_BUFFER_1);
fragmented:
		if (cur_bufleft < 16)
		{
			// read one more buffer
			/*
			numread = mpeg_getbufsize(MPEG_BUFFER_1) - cur_bufleft;
			if (cur_copybuf == NULL)
				cur_copybuf = tmpbuf + cur_bufpos;
			int len = cur_bufleft;
			if (media_get_next_free(MPEG_BUFFER_1, event, update_fip) == 0)
				return 0;
			tmpbuf = mpeg_getcurbuf(MPEG_BUFFER_1);
			*buf = tmpbuf + cur_bufpos;
			memcpy(tmpbuf, cur_copybuf, len);
			cur_copybuf = NULL;
			if ((numread = video_read(tmpbuf + len, numread)) == 0)
			{
				*event = MEDIA_EVENT_STOP;
				return 1;
			}
			numread += len;
			video_shiftpos(len);
			*/

			if (media_skip_buffer(&tmpbuf) == 0)
				return 0;
			numread = mpeg_getbufsize(MPEG_BUFFER_1);

		} else
		{
			*buf = tmpbuf + cur_bufpos;
			if (cur_bufpos == 0)
			{
				numread = mpeg_getbufsize(MPEG_BUFFER_1);
			}
		}

		for (;;)
		{
			chtype = video_getnext(tmpbuf + cur_bufpos, numread, &cur_bufpos, &cur_bufleft, &MPEG_PACKET_LENGTH);
			if (chtype == VIDEO_CHUNK_FRAGMENT)
				goto fragmented;
			if (chtype != VIDEO_CHUNK_UNKNOWN)
				break;
			if (media_skip_buffer(&tmpbuf) == 0)
				return 0;
			numread = mpeg_getbufsize(MPEG_BUFFER_1);
		}
		*buf = tmpbuf + cur_bufpos;

		if (chtype == VIDEO_CHUNK_EOF)
		{
			*event = MEDIA_EVENT_STOP;
			return 1;
		}
		if (chtype == VIDEO_CHUNK_RECOVERY)
		{
			if (cur_bufpos > 0)
				media_skip_buffer(&tmpbuf);
			return 0;
		}
		
		else if (chtype == VIDEO_CHUNK_AUDIO)
			*event = MEDIA_EVENT_AUDIO;
		else if (chtype == VIDEO_CHUNK_VIDEO)
			*event = MEDIA_EVENT_VIDEO;
		else if (chtype == VIDEO_CHUNK_AUDIO_PARTIAL)
			*event = MEDIA_EVENT_AUDIO_PARTIAL;
		else if (chtype == VIDEO_CHUNK_VIDEO_PARTIAL)
			*event = MEDIA_EVENT_VIDEO_PARTIAL;
		if (update_counter++ >= 6)
		{
			update_fip = true;
			update_counter = 0;
		}
		*len = MPEG_PACKET_LENGTH;
		MPEG_PACKET_LENGTH = PAD_EVEN(MPEG_PACKET_LENGTH);
		media_accept_block();
		
		if ((*event & MEDIA_EVENT_PARTIAL) == 0)
		{
			cur_chunkleft = cur_bufleft;
		}
	}
#endif
#ifdef INTERNAL_AUDIO_PLAYER
	else if (media_type == MEDIA_TYPE_AUDIO)
	{
		if (cur_bufleft < 1)
		{
			if (media_get_next_free(MPEG_BUFFER_1, event, update_fip) == 0)
				return 0;
		}

		BYTE *tmpbuf = mpeg_getcurbuf(MPEG_BUFFER_1);
		*buf = tmpbuf + cur_bufpos;
		if (cur_bufpos == 0)
		{
			numread = mpeg_getbufsize(MPEG_BUFFER_1);
			if (audio_read(tmpbuf, &numread) == 0)
			{
				*event = MEDIA_EVENT_STOP;
				return 1;
			}
		}
		if (cur_bufpos + MPEG_PACKET_LENGTH > numread)
			*len = MAX(numread - cur_bufpos, 0);
		else
			*len = MPEG_PACKET_LENGTH;

		*buf = tmpbuf + cur_bufpos;

		if (update_counter++ >= 6)
		{
			update_fip = true;
			update_counter = 0;
		}
		media_accept_block();
		
	}
#endif

	// update FIP
	if (update_fip)
	{
		media_update_fip();
	}

	return 1;
}

int media_update_fip()
{
	fip_write_special(FIP_SPECIAL_CIRCLE_1 + fipdisc_i, 0);
	fipdisc_i++;
	if (fipdisc_i >= 12)
		fipdisc_i = 0;
	fip_write_special(FIP_SPECIAL_CIRCLE_1 + fipdisc_i, 1);
	return 0;
}

int media_skip_buffer(BYTE **buf)
{
	int ret = mpeg_find_free_blocks(MPEG_BUFFER_1);
	if (ret == 0)	// sorry, out of buffers...
		return 0;
	cur_bufpos = 0;
	cur_bufleft = mpeg_getbufsize(MPEG_BUFFER_1);
	cur_chunkleft = cur_bufleft;
	if (buf != NULL)
		*buf = mpeg_getcurbuf(MPEG_BUFFER_1);
	return 1;
}

int media_seek_curleft()
{
#ifdef INTERNAL_VIDEO_PLAYER
	video_lseek(-cur_chunkleft, SEEK_CUR);
	media_skip_buffer(NULL);
#endif
	return 0;
}

LONGLONG media_get_filepos()
{
	return file_pos;
}

void media_set_filepos(LONGLONG fp)
{
	file_pos = fp;
}

int media_read_block(BYTE **buf, MEDIA_EVENT *event, int *len)
{
#ifdef INTERNAL_VIDEO_PLAYER
	*event = MEDIA_EVENT_NOP;
	if (media_type == MEDIA_TYPE_VIDEO)
	{
		if (len == NULL || *len == 0)
		{
			*buf = mpeg_getcurbuf(MPEG_BUFFER_1);
			*event = MEDIA_EVENT_STOP;
			return 1;
		}

		bool update_fip = false;
		MPEG_PACKET_LENGTH = PAD_EVEN(*len);
		if (cur_bufleft < 16 || (MPEG_PACKET_LENGTH > cur_bufleft))
		{
			if (media_get_next_free(MPEG_BUFFER_1, event, update_fip) == 0)
				return 0;
		}
		BYTE *tmpbuf = mpeg_getcurbuf(MPEG_BUFFER_1);
		*buf = tmpbuf + cur_bufpos;	
		int numread = MIN(cur_bufleft, MPEG_PACKET_LENGTH);
		if ((numread = video_read(*buf, numread)) == 0)
		{
			*event = MEDIA_EVENT_STOP;
			return 1;
		}
		if (numread < *len)
		{
			*len = numread;
			MPEG_PACKET_LENGTH = numread;
		}
		*event = MEDIA_EVENT_OK;
		media_accept_block();
		media_update_fip();
		return 1;
	}
#endif
	return -1;
}

int media_accept_block()
{
	cur_bufleft -= MPEG_PACKET_LENGTH;
	cur_bufpos  += MPEG_PACKET_LENGTH;
	return 1;
}

int media_free_block(BYTE *data)
{
	if (media_type == MEDIA_TYPE_DVD)
	{
		if (dvd_free_block(data) < 0) 
			return -1;
	} 
	else if (media_type == MEDIA_TYPE_MPEG)
	{
	}
	else if (media_type == MEDIA_TYPE_CDDA)
	{
	}
	else if (media_type == MEDIA_TYPE_VIDEO)
	{
	}
	else if (media_type == MEDIA_TYPE_AUDIO)
	{
	}
	return 1;
}

int media_close()
{
	if (media_type == MEDIA_TYPE_DVD)
	{
		if (dvd_close() < 0)
			return -1;
	} 
#ifdef INTERNAL_VIDEO_MPEG_PLAYER
	else if (media_type == MEDIA_TYPE_MPEG)
	{
		video_close();
	}
#endif
	else if (media_type == MEDIA_TYPE_CDDA)
	{
		// we'll close when disc changes
		//cdda_close();
	}
#ifdef INTERNAL_VIDEO_PLAYER
	else if (media_type == MEDIA_TYPE_VIDEO)
	{
		video_close();
	}
#endif
#ifdef INTERNAL_AUDIO_PLAYER
	else if (media_type == MEDIA_TYPE_AUDIO)
	{
		audio_close();
	}
#endif
	return 1;
}

int media_seek(int blockid, int from)
{
	if (media_type == MEDIA_TYPE_DVD)
	{
		// not used
		return -1;
	} 
	else if (media_type == MEDIA_TYPE_MPEG)
	{
		if (lseek(fd, blockid * mpeg_getbufsize(MPEG_BUFFER_1), from) != 0)
			return -1;
	}
	else if (media_type == MEDIA_TYPE_CDDA)
	{
		// TODO:
		return -1;
	}
	else if (media_type == MEDIA_TYPE_VIDEO)
	{
		// TODO:
		return -1;
	}
	else if (media_type == MEDIA_TYPE_AUDIO)
	{
		// TODO:
		return -1;
	}
	return 1;
}

int media_rewind()
{
	cur_bufpos  = 0;
	cur_bufleft = mpeg_getbufsize(MPEG_BUFFER_1);
	cur_chunkleft = cur_bufleft;

	if (media_type == MEDIA_TYPE_DVD)
	{
		if (dvd_reset() < 0)
			return -1;
	} 
	else if (media_type == MEDIA_TYPE_MPEG)
	{
		lseek(fd, 0, SEEK_SET);
	}
	else if (media_type == MEDIA_TYPE_CDDA)
	{
		// TODO:
		return -1;
	}
	else if (media_type == MEDIA_TYPE_VIDEO)
	{
		// TODO:
		return -1;
	}
	else if (media_type == MEDIA_TYPE_AUDIO)
	{
		// TODO:
		return -1;
	}
	return 1;
}

const char *media_geterror()
{
	if (media_type == MEDIA_TYPE_DVD)
	{
		return dvd_error_string();
	} 
	else if (media_type == MEDIA_TYPE_MPEG)
	{
		static char tmp[15];
		sprintf(tmp, "%d", 0);
		return tmp;
	}
	else if (media_type == MEDIA_TYPE_CDDA)
	{
		// TODO:
		return "";
	}
	else if (media_type == MEDIA_TYPE_VIDEO)
	{
		// TODO:
		return "";
	}
	else if (media_type == MEDIA_TYPE_AUDIO)
	{
		// TODO:
		return "";
	}
	return "";
	
}

