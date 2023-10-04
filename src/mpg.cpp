//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - MPEG1/MPEG2/VOB files player source file.
 *  \file       mpg.cpp
 *  \author     bombur
 *  \version    0.2
 *  \date       02.07.2010 07.05.2007
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
#include <errno.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_khwl.h>
#include <libsp/sp_mpeg.h>
#include <libsp/sp_cdrom.h>

#include "script.h"
#include "player.h"
#include "media.h"
#include "settings.h"

#ifdef INTERNAL_VIDEO_PLAYER

#define VIDEO_INTERNAL

#include "audio.h"
#include "video.h"
#include "mpg.h"

#define MSG if (video_msg) msg

static const int seek_delta_pts = 60 * 90000;	// 1 min.

VideoMpg::VideoMpg()
{
	type = VIDEO_CONTAINER_MPEG;
	new_frame_size = true;
	cur_rate = 0;
	
	scan_for_length = true;
	seek_start = true;
	start_time_pts = -1;
	end_time_pts = -1;
	totaltime_pts = 0;
	pts_base = 0;
	wait_for_new_pts = false;
	set_pts = true;

	elementary_stream = false;
	is_cdxa = false;

	packet_left = 0;
	saved_sct = START_CODE_UNKNOWN;
	saved_length = -1;
	saved_start_counter = 0;
	saved_feed = true;

	//tmp_packet_header_buf_active = false;
	//tmp_packet_header_buf_size = 0;
	packet_header_incomplete = false;

	was_pack = false;

	mpeg_format = -1;
	mpeg_aspect = -1;

	old_pts = 0;
	video_pts = 0;

	is_totaltime_from_rate = false;

}

VideoMpg::~VideoMpg()
{
}

////////////////////////////////////////////////

BOOL VideoMpg::Parse()
{
	// Read first 4 bytes and check that this is an MPEG file
	BYTE data[16];
	if (video_read(data, 12) != 12) 
	{
		msg_error("Mpg: Read error.\n");
		return FALSE;
	}
	if (strncasecmp((char *)data, "RIFF", 4) == 0 && strncasecmp((char *)data+8, "AVI ", 4) == 0) 
	{
		msg_error("Mpg: Avi file found in MPEG container.\n");
		return FALSE;
	}
	if (strncasecmp((char *)data, "RIFF", 4) == 0 && strncasecmp((char *)data+8, "CDXA", 4) == 0) 
	{
		msg("Mpg: CDXA format detected.\n");
		is_cdxa = true;
	}
	video_lseek(0, SEEK_END);
	filesize = video_lseek(0L, SEEK_CUR);
	seek_filesize = filesize;

//msg("! seek_filesize = %d\n", (int)seek_filesize);

	video_lseek(0, SEEK_SET);

	video_fmt = RIFF_VIDEO_MPEG12;
	video_fmt_str.Printf("MPEG-1/2");

	//MPEG_PACKET_LENGTH = 2048;
	MPEG_PACKET_LENGTH = 32768;
	MPEG_NUM_PACKETS = 512;
	set_media_type(MEDIA_TYPE_MPEG);
	
	return TRUE;
}

BOOL VideoMpg::GetTimeLength()
{
	LONGLONG pos = video_lseek(-32768, SEEK_END);
	int ret = mpeg_find_free_blocks(MPEG_BUFFER_1);
	// we cannot wait
	if (ret < 1)
		return FALSE;
	BYTE *base = mpeg_getcurbuf(MPEG_BUFFER_1);
	if (base == NULL)
		return FALSE;
	int len = (int)video_read(base, (int)(filesize - pos));
	if (len < 1)
		return FALSE;

	// now parse the stream
	MPEG_PACKET_LENGTH = len;
	start_time_pts = -1;
	end_time_pts = -1;
	totaltime_pts = 0;
	seek_start = false;
	scan_for_length = true;
	fps = 30;	// not correct, but it works
	ProcessChunk(base, len);
	MPEG_PACKET_LENGTH = 32768;

	video_lseek(0, SEEK_SET);
	seek_start = true;
	scan_for_length = true;

	return TRUE;
}

//////////////////////////////////////////////////////////////////

VIDEO_CHUNK_TYPE VideoMpg::GetNext(BYTE *, int , int *, int *, int *)
{
	return VIDEO_CHUNK_UNKNOWN;
}

int VideoMpg::ProcessChunk(BYTE *buf, int )
{
	START_CODE_TYPES sct;
	BYTE *base = buf, *lastbuf = base;

	for (; ;)
	{
		if (packet_left > 0 || (elementary_stream && saved_sct != START_CODE_UNKNOWN))
			sct = saved_sct;
		else
		{
			sct = mpeg_findpacket(buf, base, saved_start_counter);
			if (sct == START_CODE_END)
			{
				return 1;
			}
			saved_start_counter = 0;
		}
		bool datapacket = false;
		switch (sct)
		{
		case START_CODE_PACK:
			{
			int left = MPEG_PACKET_LENGTH - (buf - base);
			if (packet_left > 0)
			{
				memcpy(tmp_packet_header_buf + packet_left, buf, 8 - packet_left);
				BYTE *b = tmp_packet_header_buf;
				scr = mpeg_parse_program_stream_pack_header(b, tmp_packet_header_buf);
				buf += 8;
				packet_left = 0;
			}
			else if (left < 8)
			{
				memcpy(tmp_packet_header_buf, buf, left);
				packet_left = left;
				saved_sct = sct;
				return 1;
			}
			else
				scr = mpeg_parse_program_stream_pack_header(buf, base);
			scr += pts_base;
			if (scr < 0)
				scr = 0;
			if (scr > 0 && scan_for_length)
			{
				//scan_for_length = false;
				if (seek_start)
				{
					if (start_time_pts < 0)
					{
						start_time_pts = scr;
						UpdateTotalTime();
					}
				}
				else
				{
					if (scr > end_time_pts && (end_time_pts < 0 || scr < end_time_pts + 120000))
						end_time_pts = scr;
					// early exit
					//return 1;
				}
			}
			// this is needed for 'khwl'
			scr |= SPTM_SCR_FLAG;
			was_pack = true;
			lastbuf = buf;
			}
			break;
		case START_CODE_SYSTEM:
		case START_CODE_PCI:
		case START_CODE_PADDING:
			{
				int len;
				int left = MPEG_PACKET_LENGTH - (buf - base);
				if (left < 1)
					return 1;
				if (packet_left > 0)
				{
					// if we read only 1 byte of packet length
					if (saved_length >= 0)
					{
						// one byte was already read
						len = ((saved_length << 8) | buf[0]) + 1;
					} else
						len = packet_left;
					packet_left = 0;
					saved_length = -1;
				}
				else
				{
					if (left < 2)	// cannot get length
					{
						saved_length = buf[0];
						packet_left = 1;
						saved_sct = sct;
						return 1;
					}
					len = ((buf[0] << 8) | buf[1]) + 2;
				}
				
				if (len > left) // length is greater than packet size
				{
					packet_left = len - left;
					saved_length = -1;
					len = left;
					saved_sct = sct;
					return 1;
				}
				// it's safe now
				buf += len;
				lastbuf = buf;
				break;
			}
		case START_CODE_MPEG_VIDEO_ELEMENTARY:
			elementary_stream = true;
			saved_sct = sct;
		case START_CODE_MPEG_VIDEO:
		case START_CODE_MPEG_AUDIO1:
		case START_CODE_MPEG_AUDIO2:
		case START_CODE_PRIVATE1:
			datapacket = true;
			break;
		default:;
		}

		if (datapacket)
		{
			if (scan_for_length)
			{
				LONGLONG pts = 0;
				if (!seek_start)
				{
					if (sct == START_CODE_MPEG_VIDEO && was_pack)
					{
						MpegPacket tmppacket;
						tmppacket.flags = 0;
						tmppacket.pts = 0;
						if (mpeg_extractpacket(buf, base, &tmppacket, sct, FALSE) >= 0)
						{
							if (tmppacket.flags == 2 && tmppacket.pts > 0)
								pts = tmppacket.pts;
						}
						buf += tmppacket.size;
						lastbuf = buf;
					}
				}
#if 0
				if (sct == START_CODE_MPEG_VIDEO || sct == START_CODE_MPEG_VIDEO_ELEMENTARY)
				{
					BYTE *last = base + MIN(buflen, MPEG_PACKET_LENGTH); 
					for(BYTE *b = buf; b < last; b++)
					{
						if (b[0] == 0 && b[1] == 0 && b[2] == 1 && b[3] == 0xb8) // GOP
						{
							// xhhhhhmm mmmmxsss sssfffff fxxxxxxx
							int hours = (b[4] >> 2) & 63;
							int mins = ((b[4] & 3) << 4) | (b[5] >> 4);
							int secs = ((b[5] & 7) << 3) | (b[6] >> 5);
							int frames = ((b[6] << 1) & 63) | (b[7] >> 7);
							pts = ((LONGLONG)(hours * 3600) + (LONGLONG)(mins * 60) + (LONGLONG)secs) *
								INT64(90000) + (LONGLONG)frames * INT64(90000) / fps;
							break;
						}
					}
					// we don't need packets if we scan at the end of file
					if (pts == 0 && !seek_start)
						return 1;
				}
#endif
				if (seek_start)
				{
					if (pts > 0)
					{
						scan_for_length = false;
						start_time_pts = pts;
						UpdateTotalTime();
					}
				}
				else
				{
					if (pts > 0 && pts > end_time_pts)
					{
						end_time_pts = pts;
						scan_for_length = false;
						return 1;
					}
					if (elementary_stream)
						return 1;
					continue;
				}
			}
			
			// audio or video
			MpegPacket *packet = NULL;
			packet = mpeg_feed_getlast();
			if (packet == NULL)		// well, it won't really help
				return 0;
			memset((BYTE *)packet + 4, 0, sizeof(MpegPacket) - 4);
			packet->pts = pts_base;

			//!!!!!!!!!!!!!!!!!!!
			packet->scr = scr;

			bool feed = true;

			if (packet_header_incomplete)	// we haven't read the header last time
			{
				packet_left = 0;
				packet_header_incomplete = false;
			}

			if (packet_left > 0)
			{
				packet->type = saved_packet->type;
				packet->pData = buf;
				packet->size = packet_left;
				packet->pts = saved_packet->pts;
				packet->nframeheaders = saved_packet->nframeheaders;
				packet->firstaccessunitpointer = saved_packet->firstaccessunitpointer;
				packet->scr = 0;
				feed = saved_feed;
				packet_left = 0;
			} else
			{
				BYTE *lb = buf;
				int ret = mpeg_extractpacket(buf, base, packet, sct, wait_for_new_pts);
				if (ret < 0)		// if not enough data in the packet
				{
					LONGLONG off = -(base + MPEG_PACKET_LENGTH - lb);
					video_lseek(off, SEEK_CUR);
					saved_sct = sct;
					packet_left = 1;	// the value doesn't really matter
					packet_header_incomplete = true;
					return 1;
				}
				if (ret == 0)
					feed = false;
			}

			int left = MPEG_PACKET_LENGTH - (buf - base);
			bool need_return = false;
			if ((int)packet->size > left) // length is greater than packet size
			{
				packet_left = packet->size - left;
				packet->size = left;
				saved_sct = sct;
				saved_packet = packet;
				saved_feed = feed;
				need_return = true;
			}

			if (new_frame_size)
			{
				int w, h;
				mpeg_getframesize(&w, &h);
				if (w != 0 && h != 0)
				{
					script_framesize_callback(w, h);
					new_frame_size = false;
				}

				int aspect = mpeg_getaspect();
				if (mpeg_aspect != aspect)
				{
					mpeg_aspect = aspect;
					
					int tvtype = settings_get(SETTING_TVTYPE);
					KHWL_VIDEOMODE vmode = KHWL_VIDEOMODE_NORMAL;
					if (aspect == 2)	// 4:3
					{
						if (tvtype == 0 || tvtype == 1)
							vmode = KHWL_VIDEOMODE_NORMAL;
						else 
							vmode = (tvtype == 2) ? KHWL_VIDEOMODE_HCENTER : KHWL_VIDEOMODE_VCENTER;
					} 
					else if (aspect == 3)	// 16:9
					{
						if (tvtype == 2 || tvtype == 3)
							vmode = KHWL_VIDEOMODE_WIDE;
						else 
							vmode = (tvtype == 0) ? KHWL_VIDEOMODE_LETTERBOX : KHWL_VIDEOMODE_PANSCAN;
					}
					msg("MPEG: set vmode = %d (%d)\n", vmode, aspect);

					khwl_display_clear();
					khwl_set_window_zoom(KHWL_ZOOMMODE_DVD);
					khwl_setvideomode(vmode, TRUE);
				}

				int cur_fps = mpeg_get_fps();
				if (cur_fps > 0)
				{
					fps = cur_fps;
					script_framerate_callback(fps);
				}
				
				int cur_fmt = mpeg_get_video_format();
				if (cur_fmt != mpeg_format)
				{
					if (!elementary_stream)
					{
						video_fmt_str.Empty();
						video_fmt_str.Printf("MPEG-%d", cur_fmt == MPEG_1 ? 1 : 2);
						script_video_info_callback(video_fmt_str);
						MSG("Video: * Video format %s detected!\n", *video_fmt_str);
						mpeg_format = cur_fmt;
					}
				}
			}

			if (packet->flags == 2 && feed)
			{
				if (wait_for_new_pts)
				{
					if (packet->type == 0)
					{
						if (mpeg_needs_seq_start_header(&packet))
							wait_for_new_pts = false;
						else
							feed = false;
					}
					else
						feed = false;
				}
				if (packet->type == 0)
				{
					pts_base += mpeg_detect_and_fix_pts_wrap(packet);
					if (set_pts)
					{
						mpeg_setpts(packet->pts);
						set_pts = false;
					}
					video_pts = packet->pts;
				} else
				{
					int cur_numaudio_streams = mpeg_getaudiostreamsnum();
					if (cur_numaudio_streams != num_tracks)
					{
						num_tracks = cur_numaudio_streams;
					}
				}
				
				if (displ_pts_base == 0)
				{
					if (packet->type == 0)
					{
						displ_pts_base = -MAX(packet->pts - 45000, 0);
						/*
						pts_base = -MAX(packet->pts - 45000, 0);
						packet->pts += pts_base;
						scr = packet->scr = ((packet->scr & (~SPTM_SCR_FLAG)) + pts_base) | SPTM_SCR_FLAG;
						*/
					}
				}
				int rate = mpeg_getrate(elementary_stream);
				if (rate != cur_rate)
				{
					// calc. total time
					if (rate > 0)
					{
						UpdateTotalTime();
						cur_rate = rate;
					}
				}
				old_pts = packet->pts;
			} else
			{
				if (wait_for_new_pts)
					feed = false;
				packet->pts = old_pts;
			}
#if 0
{
static int num_packets = 0;

static LONGLONG last_pts = 0;
if (feed)
{
if (packet->type == 1)
msg("[%d] a - %d ---%d: %8d\t\t%d\t\t[%d]\t%d\n", num_packets, packet->size, packet->flags, (int)packet->pts, (int)(packet->scr & 0xfffffff), (int)(packet->pts - last_pts), pts_base);
else if (packet->type == 0)
msg("[%d] v - %d ---%d: %8d\t\t%d\t\t[%d]\t%d\n", num_packets, packet->size, packet->flags, (int)packet->pts, (int)(packet->scr & 0xfffffff), (int)(packet->pts - last_pts), pts_base);
last_pts = packet->pts;
}
/*
extern void player_printpacket(MpegPacket *packet);
player_printpacket(packet);
*/
num_packets++;
}
//!!!!!!!!!!!!!!!!!!!!!
/*
if (msg_get_output() == MSG_OUTPUT_SHOW && packet->type != 0)
feed = false;
if (msg_get_output() == MSG_OUTPUT_FREEZE && packet->type != 0 && packet->type != 1)
feed = false;
*/
//!!!!!!!!!!!!!!!!!!!!!
#endif

			//packet->vobu_sptm = (vobu_sptm + pts_base) | SPTM_SCR_FLAG;
			// increase bufidx
			if (packet->size >= 1 && feed)
			{
				mpeg_setbufidx(MPEG_BUFFER_1, packet);
				mpeg_feed((MPEG_FEED_TYPE)packet->type);
				//num_packets++;
			}
			buf += packet->size;
			lastbuf = buf;

			if (need_return)
				return 1;
		}
	}
	return 1;
}

void VideoMpg::UpdateTotalTime()
{
	LONGLONG new_totaltime = 0;
	if (start_time_pts >= 0 && end_time_pts >= 0 && end_time_pts > start_time_pts)
	{
		new_totaltime = (end_time_pts - start_time_pts);
		seek_filesize = filesize;
		is_totaltime_from_rate = false;
//msg("! new_tt = %d (s=%d e=%d)\n", (int)new_totaltime, (int)start_time_pts, (int)end_time_pts);
	}
	else
	{
		LONGLONG rate = (LONGLONG)mpeg_getrate(elementary_stream);
		if (rate > 0)
		{
			new_totaltime = INT64(90000) * filesize / rate;
			seek_filesize = filesize;
			is_totaltime_from_rate = true;
//msg("! new_tt = %d (r=%d fs=%d)\n", (int)new_totaltime, (int)rate, (int)filesize);
		}
	}
	// if we're unable to deremine total time
	if (video_pts + displ_pts_base > new_totaltime)
	{
		new_totaltime = video_pts + displ_pts_base;
		seek_filesize = media_get_filepos();
		is_totaltime_from_rate = false;
//msg("! video_pts (%d) > new_totaltime (%d)\n", (int)video_pts, (int)new_totaltime);
//msg("! seek_filesize = %d\n", (int)seek_filesize);
	}
	if (new_totaltime > 0)
	{
		if (new_totaltime > totaltime_pts || (is_totaltime_from_rate && Abs(new_totaltime - totaltime_pts) > 90000))
		{
			script_totaltime_callback((int)(new_totaltime / 90000));
			totaltime_pts = new_totaltime;
		}
	}
}

int VideoMpg::GetNextIndexes()
{
	return -1;
}

int VideoMpg::GetNextKeyFrame()
{
	return -1;
}

int VideoMpg::GetKeyFrame(LONGLONG time)
{
	// undo some Video actions for MPEGs:
	skip_fwd = false;
	skip_rev = false;
	skip_cnt = 0;
	searching = false;
	packet_left = 0;
	
	packet_header_incomplete = false;

	old_pts = 0;
	wait_for_new_pts = true;
	set_pts = true;
	was_pack = false;

	saved_sct = START_CODE_UNKNOWN;
	saved_length = -1;
	saved_start_counter = 0;
	saved_feed = true;

	mpeg_stop();
	mpeg_setspeed(MPEG_SPEED_NORMAL);
	media_skip_buffer(NULL);
	mpeg_setpts(0);
	mpeg_start();

	if (totaltime_pts < 1 || filesize < 1)
	{
		msg("MPEG: Cannot seek! Total time or size unknown.\n");
		return -1;
	}

	LONGLONG curtime = saved_pts;
	LONGLONG vpts = video_pts + displ_pts_base;
	if (curtime < 0)
		curtime = 0;
	if (curtime > vpts)
		curtime = vpts;
	if (curtime < vpts - 10*90000)
		curtime = vpts;
	LONGLONG pts = 0;

	if (time == VIDEO_KEY_FRAME_NEXT)
	{
		pts = curtime + seek_delta_pts;
	} 
	else if (time == VIDEO_KEY_FRAME_PREV)
	{
		pts = curtime - seek_delta_pts;
	} 
	else
	{
		pts = time * 90000;
	}

	LONGLONG filepos = (seek_filesize * pts / totaltime_pts) & (~(/*MPEG_PACKET_LENGTH*/512 - 1));
	if (filepos < 0)
		filepos = 0;
	// a special correction fix for VBR MPEGs
	if (time == VIDEO_KEY_FRAME_NEXT || time == VIDEO_KEY_FRAME_PREV)
	{
		LONGLONG rate = (LONGLONG)mpeg_getrate(elementary_stream);
		if (rate > 0)
		{
			LONGLONG avg_rate = INT64(90000) * seek_filesize / totaltime_pts;
			LONGLONG thr = 100 * Abs(rate - avg_rate) / avg_rate;
			if (thr < 10)
			{
				LONGLONG delta_pos = filepos - media_get_filepos();
				LONGLONG delta_time = INT64(90000) * delta_pos / rate;
				LONGLONG d = Abs(delta_time) - seek_delta_pts;
				d = seek_delta_pts * rate / INT64(90000);
				if (time == VIDEO_KEY_FRAME_PREV)
					d = -d;
				filepos = media_get_filepos() + d;
				msg("MPEG: * Bitrate-correcting (" PRINTF_64d ", r=" PRINTF_64d ")!\n", d, rate);
			}
		}
	}
	msg("(saved=%d fsize=%d)\n", (int)saved_pts, (int)seek_filesize);
	msg("MPEG: Seek to " PRINTF_64d " (filepos=" PRINTF_64d "/" PRINTF_64d ")\n", pts, filepos, filesize);
	LONGLONG ret = video_lseek(filepos, SEEK_SET);
	if (ret != filepos)
		msg("MPEG: Seek ERROR! Pos = " PRINTF_64d "\n", ret);
	if (ret < 0)
	{
		msg("MPEG: Cannot seek! File error %d.\n", -errno);
		return -1;
	}
	return 1;
}

#endif
