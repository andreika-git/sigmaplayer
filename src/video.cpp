//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - Video player source file.
 *  \file       video.cpp
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_fip.h>
#include <libsp/sp_khwl.h>
#include <libsp/sp_mpeg.h>
#include <libsp/sp_cdrom.h>

#include <script.h>
#include <player.h>
#include <media.h>
#include <settings.h>

#ifdef INTERNAL_VIDEO_PLAYER

#define RESYNC_TIMEOUT INT64(120000)

#define NEW_SYNC
#define USE_RESYNCS_CONTROL

//#define SKIP_AUDIO

#define VIDEO_INTERNAL
#include <audio.h>
#include <video.h>
#include <avi.h>
#include <mpg.h>
#include <subtitle.h>
#include <bitstream.h>

int num_packets = 0;
static int info_cnt = 0;
bool video_msg = true;
#define MSG if (video_msg) msg

static int max_info_cnt = 32;
static const int DIVX3_BUF_MARGIN = 128;


//#define VIDEO_PACKET_DEBUG
//#define VIDEO_USE_MMAP

#define DUMP_FRAME msg
//#define DUMP_FRAME gui_update();msg

static Video *video = NULL;

BYTE *VideoAlloc(int size)
{
#ifdef VIDEO_USE_MMAP
	return (BYTE *)mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
#else
	return (BYTE *)SPmalloc(size);
#endif
}

void VideoFree(BYTE *&buf, int /*size*/)
{
	if (buf != NULL)
	{
#ifdef VIDEO_USE_MMAP
		munmap(buf, size);
#else
		SPfree(buf);
#endif
		buf = NULL;
	}
	
}

////////////////////////////////////////
// some helper functions for MPEG stream parsing

static void video_bits_init(BYTE *buf, int len);
static int video_skip_bits(int num_bits);
static DWORD video_get_bits(int num_bits);
static int video_log2(DWORD v);


Video::Video()
{
	int i;
	type = VIDEO_CONTAINER_UNKNOWN;

	playing = false;
	stopping = false;
	is_eof = false;
	event = MEDIA_EVENT_OK;
	scale = 1000;
	rate = 24000;
	scr = 0;
	saved_pts = 0;
	displ_pts_base = 0;
	video_fmt = RIFF_VIDEO_UNKNOWN;
	audio_fmt = RIFF_AUDIO_UNKNOWN;
	chunktype = VIDEO_CHUNK_UNKNOWN;
	chunkleft = 0;
	look4chunk = false;
	test_for_qpel_gmc = true;
	wait_for_resync = false;
	
	for (i = 0; i < 16; i++)
	{
		mux_buf[i] = NULL;
		allocated_mux_buf[i] = NULL;
		allocated_mux_buf_size[i] = 0;
		divx_buf[i] = NULL;
		divx_base[i] = NULL;
	}
	mux_numbufs = 12;
	mux_bufsize = 32768;
	allocated_numbufs = 0;
	no_partial = false;

	fd = -1;

	width = height = 0;
	fps = 0;
	qpel_flag = false;
	gmc_flag = false;
	video_frames = 0;
	fourcc[0] = '\0';

	cur_track = 0;
	first_track = 0;
	num_tracks = 0;
	cur_audio_bits = 0;

	for (i = 0; i < VIDEO_MAX_AUDIO_TRACKS; i++)
	{
		track[i].wfe = NULL;
	}
	
	min_delta_pts = 2000;
	max_delta_pts = 8000;
	minus_delta_pts = 0;
	saved_video_pts = 0;
	video_pos_base = 0;
	abs_video_pos = 0;
	ResetAvgPts();

	total_frames = 0;
	audio_total_bytes = 0;
	audio_delta = SPTM_SCR_FLAG;
	audio_pts_offset = 0;

	video_pos = 0;
	last_key_pos = -min_delta_keyframes - 1;
	cur_key_pos = 0;
	
	cur_key_idx = 0;
	last_good_key_idx = -1;
	last_key_idx = -1;

	frame_pos = 0;
	last_frame_pos = 0;

	max_rev_incr = def_max_rev_incr;

	searching = false;
	skip_fwd = false;
	skip_rev = false;
	skip_wait = false;
	skip_waiting = false;
	last_keyframe_time = 0;
	delayed_skip = 0;
	cancel_skip = false;
	audio_halfrate = false;
	skip_cnt = 0;

	cur_offs = 0;

	video_packet_len = 0;
	old_video_packet_len = 0;
	
	good_delta_stc = 90000;

	in_divx_packet = false;
	need_to_stop = false;
}

Video::~Video()
{
	int i;
	for (i = VIDEO_MAX_AUDIO_TRACKS - 1; i >= 0; i--)
		SPSafeFree(track[i].wfe);
	for (i = 15; i >= 0; i--)
	{
		//SPSafeFree(divx_base[i]);
		VideoFree(divx_base[i], (video->mux_bufsize + DIVX3_BUF_MARGIN * 2 + 8));
	}
	for (i = allocated_numbufs - 1; i >= 0; i--)
	{
		//SPSafeFree(mux_buf[i]);
		VideoFree(allocated_mux_buf[i], allocated_mux_buf_size[i]);
	}
}

VideoKeyFrame *Video::GetIndex(int idx)
{
	int y = idx / num_key_frames_block;
	if (y >= indexes.GetN())
		return NULL;
	int x = idx % num_key_frames_block;
	if (x >= indexes[y].GetN())
		return NULL;
	return &indexes[y][x];
}

int Video::GetKeyFrame(LONGLONG time)
{
	int idx1 = 0, idx2 = last_key_idx;
	static LONGLONG stime;
	
	bool need_to_restore_pos = false;

	if (time == VIDEO_KEY_FRAME_CONTINUE)
		time = stime;
	else
		stime = time;

	switch (time)
	{
	case VIDEO_KEY_FRAME_NEXT:
		msg("Video: SEARCH NEXT!\n");
		idx1 = idx2 = cur_key_idx + 1;
		skip_cnt++;
		break;
	case VIDEO_KEY_FRAME_PREV:
		msg("Video: SEARCH PREV!\n");
		idx1 = idx2 = cur_key_idx - 1;
		skip_cnt++;
		break;
	default:
		{
			msg("Video: SEARCH TIME = %d !\n", (int)time);
			VideoKeyFrame *curidx = GetIndex(cur_key_idx);
			if (curidx != NULL)
			{
				if (time < curidx->i * scale / rate)
				{
					idx1 = 0; idx2 = cur_key_idx - 1;
				} else
				{
					idx1 = cur_key_idx; idx2 = last_key_idx;
				}
			}
		}
		break;
	}

	// now we have idx range, so use dichotomy
	int ret = 0;
	while (idx1 <= idx2)
	{
		VideoKeyFrame *lastidx = GetIndex(last_key_idx);
		if (lastidx != NULL)
		{
			LONGLONG last_time = lastidx->i * scale / rate;
			if (time > last_time)
			{
				idx1 = last_key_idx + 1;
				ret = GetNextIndexes();
				need_to_restore_pos = true;
				idx2 = MAX(last_key_idx, idx1);
				if (ret < 0)
					break;
				continue;
			}
		}
		// if we need to extend index more...
		if (idx2 > last_key_idx)
		{
			int k;
			for (k = 0; k < max_rev_incr; k++)
			{
				ret = GetNextIndexes();
				need_to_restore_pos = true;
				if (ret != 0)
					break;
			}
			max_rev_incr += def_max_rev_incr;
			if (ret < 0 && k == 0)
				break;
			ret = 0;
			continue;
		}
		if (idx1 == idx2)
			break;
		// ok, now all range is inside buf
		int newidx = (idx1 + idx2) / 2;
		VideoKeyFrame *median = GetIndex(newidx);
		if (median == NULL)
			break;
		if (time <= median->i * scale / rate)
		{
			idx2 = newidx;
		} else
		{
			idx1 = newidx + 1;
		}
	}

msg("[%d] ret=%d idx1=%d idx2=%d\n", cur_key_idx, ret, idx1, idx2);

	if (ret >= 0 && idx1 == idx2)
	{
		if (idx1 < 0)
		{
			video->cancel_skip = true;
			return 0;
		}
		VideoKeyFrame *idx = GetIndex(idx1);
		if (idx != NULL)
		{
			abs_video_pos = idx->i;
			last_chunk_len = video_packet_len = idx->len;
			cur_key_offs = last_chunk_offs = video_offs = idx->offs;
			video_lseek(video_offs, SEEK_SET);
			MSG("AVI: I-FRAME #%d @ (" PRINTF_64d ",%d)\n", abs_video_pos, idx->offs, idx->len);
			if (time != VIDEO_KEY_FRAME_NEXT && time != VIDEO_KEY_FRAME_PREV)
				cur_key_idx = 0;
			last_good_key_idx = idx1;
			scr = INT64(90000) * abs_video_pos * scale / rate;
			info_cnt = max_info_cnt + 1;
			return 1;
		}
	}

	// we failed to search in index
	if (ret < 0 && time == VIDEO_KEY_FRAME_NEXT)
	{
		if (need_to_restore_pos)
		{
			if (last_chunk_len < 1)
				return -1;
			video_lseek(last_chunk_offs + 8 + PAD_EVEN(last_chunk_len), SEEK_SET);
		}
		return GetNextKeyFrame();
	}

	return -1;
}

void Video::UpdateTotalTime()
{
	// do nothing, normally total play time cannot change
}

int Video::SetCurrentIndex(int i, int len, LONGLONG offs)
{
	int cur_index_x, cur_index_y;
	if (i >= last_key_pos + min_delta_keyframes)
	{
		if (AddIndex(i, len, offs) < 0)
			return -1;
	}
	if (cur_key_idx > 0)
	{
		int cki = cur_key_idx - 1;
		cur_index_x = cki % num_key_frames_block;
		for (cur_index_y = cki / num_key_frames_block; cur_index_y >= 0; cur_index_y--, cur_index_x = indexes[cur_index_y].GetN() - 1)
		{
			for (; cur_index_x >= 0; cur_index_x--, cki--)
			{
				if (indexes[cur_index_y][cur_index_x].i < i)
				{
					goto next;
				}
				cur_key_idx = cki;
			}
		}
	}
	
next:
	// find new index positions...
	cur_index_x = cur_key_idx % num_key_frames_block;
	for (cur_index_y = cur_key_idx / num_key_frames_block; cur_index_y < indexes.GetN(); cur_index_y++, cur_index_x = 0)
	{
		for (; cur_index_x < indexes[cur_index_y].GetN(); cur_index_x++, cur_key_idx++)
		{
			if (indexes[cur_index_y][cur_index_x].i >= i)
			{
				return 0;
			}
		}
	}
	return 0;
}

int Video::AddIndex(int i, int len, LONGLONG offs)
{
	static VideoKeyFrame kf;

//msg("*** %d %d %I64d\n", i, len, offs);

	if (indexes.GetN() < 1 || indexes[indexes.GetN() - 1].GetN() >= num_key_frames_block)
	{
		indexes.SetN(indexes.GetN() + 1);
		indexes[indexes.GetN() - 1].Reserve(num_key_frames_block);
	}
	int y = indexes.GetN() - 1;
	
	kf.i = i;
	kf.len = len;
	kf.offs = offs;
	int x = indexes[y].Add(kf);
	if (x < 0)
		return -1;
	
	last_key_pos = i;
	last_key_idx = y * num_key_frames_block + x;
	return 0;
}

void Video::ResetAvgPts()
{
	for (int i = 0; i < saved_delta_pts_num; i++)
		saved_delta_pts[i] = 0;
	saved_delta_pts_idx = 0;
	avg_delta_pts = 0;

}

LONGLONG Video::FixAudioPts(LONGLONG pts)
{
	static int num_corrections = 0;
	LONGLONG delta = avg_delta_pts / saved_delta_pts_num;
	if (delta > max_delta_pts + min_delta_pts)
	{
		minus_delta_pts = delta - min_delta_pts;
		if (num_corrections++ < 10)
			msg("AVI: Audio far ahead video (%d). Correction = %d.\n", (int)delta, (int)minus_delta_pts);
		// reset avg
		ResetAvgPts();
	}

	pts -= minus_delta_pts;

	avg_delta_pts -= saved_delta_pts[saved_delta_pts_idx];
	saved_delta_pts[saved_delta_pts_idx++] = pts - saved_video_pts;
	avg_delta_pts += pts - saved_video_pts;
	saved_delta_pts_idx %= saved_delta_pts_num;

	

	return pts;
}

////////////////////////////////////////////////

int video_play(char *filepath)
{
	if (filepath == NULL)
	{
		if (video != NULL)
		{
			if (video->skip_fwd || video->skip_rev || video->searching)
				video->cancel_skip = true;
			else
			{
				fip_write_special(FIP_SPECIAL_PLAY, 1);
				fip_write_special(FIP_SPECIAL_REPEAT, 0);
				fip_write_special(FIP_SPECIAL_PAUSE, 0);
				mpeg_setspeed(MPEG_SPEED_NORMAL);
			}
		}
		return 0;
	}

	if (strncasecmp(filepath, "/cdrom/", 7) != 0 && strncasecmp(filepath, "/hdd/", 5) != 0)
		return -1;

	int ret = media_open(filepath, MEDIA_TYPE_VIDEO);
	if (ret < 0)
	{
		msg_error("Video: media open FAILED.\n");
		video_stop();
		return ret;
	}
	MSG("Video: Start...\n");

	num_packets = 0;

	video_detect_formats();

	if (video->video_fmt == RIFF_VIDEO_UNKNOWN)
	{
		msg_error("Video: Video codec not supported.\n");
		script_error_callback(SCRIPT_ERROR_BAD_CODEC);
		video_stop();
		return -1;
	}

	video->scr = 0;

	if (video->video_fmt == RIFF_VIDEO_MPEG12)
	{
		max_info_cnt = 1;

		mpeg_init(MPEG_2, FALSE, TRUE, TRUE);
		video->mux_numbufs = 16;
		video->mux_bufsize = 32768;
		mpeg_setbuffer(MPEG_BUFFER_1, BUF_BASE, video->mux_numbufs, video->mux_bufsize);

		((VideoMpg *)video)->GetTimeLength();
	} else
	{
		max_info_cnt = 16;

		//video->UpdateTotalTime();
		int totaltime = video->rate != 0 ? (int)((LONGLONG)video->video_frames * video->scale / video->rate) : 0;
		script_totaltime_callback(totaltime);
		
		video->indexes.Reserve(1024);

		mpeg_init(MPEG_4, FALSE, TRUE, FALSE);

		mpeg_set_scale_rate((DWORD *)&video->scale, (DWORD *)&video->rate);
		video->good_delta_stc = RESYNC_TIMEOUT * video->scale / video->rate;

		int tvtype = settings_get(SETTING_TVTYPE);
		KHWL_VIDEOMODE vmode = KHWL_VIDEOMODE_NORMAL;
		if (tvtype == 2 || tvtype == 3)
			vmode = KHWL_VIDEOMODE_WIDE;
		//khwl_setvideomode(KHWL_VIDEOMODE_NORMAL, TRUE);
		khwl_setvideomode(vmode, FALSE);

		mpeg_setframesize(video->width, video->height, true);

		if (video->video_fmt == RIFF_VIDEO_DIV3)
		{
			video->mux_numbufs = 2;
			video->mux_bufsize = 200960;
		}
		
		bool all_allocated = false;
		int max_allocs[4];
#if 0
		max_allocs[0] = 1;
		max_allocs[1] = 2;
		max_allocs[2] = video->mux_numbufs;
		max_allocs[3] = 0;
#endif
		max_allocs[0] = video->mux_numbufs;
		max_allocs[1] = 0;

		for (int num_tries = 0; max_allocs[num_tries] != 0; num_tries++)
		{
			all_allocated = true;
			int i = 0;
			for (int num_allocs = 0; num_allocs < max_allocs[num_tries]; num_allocs++)
			{
				int nb = video->mux_numbufs / max_allocs[num_tries];
				int bsize = video->mux_bufsize * nb + 8;
				BYTE *buf = VideoAlloc(bsize);
				if (buf == NULL)
				{
					all_allocated = false;
					break;
				}
				video->allocated_mux_buf_size[video->allocated_numbufs] = bsize;
				video->allocated_mux_buf[video->allocated_numbufs] = buf;
				video->allocated_numbufs++;
				
				for (int j = 0; j < nb; j++, i++)
				{
					video->mux_buf[i] = buf;
					buf += video->mux_bufsize;
				}
			}
			if (all_allocated)
				break;
		}
		if (!all_allocated)
		{
			msg_error("Video: Cannot allocate mux buffers.\n");
			video_stop();
			return -1;
		}
		mpeg_setbuffer_array(MPEG_BUFFER_1, video->mux_buf, video->mux_numbufs, video->mux_bufsize);
	}

	if (video->video_fmt == RIFF_VIDEO_DIV3)
	{
		khwl_display_clear();
		script_error_callback(SCRIPT_ERROR_WAIT);
		gui_update();

		if (divx_transcode_preinit() < 0)
		{
			msg_error("Video: Cannot pre-init DivX3 transcoder.\n");
			video_stop();
			return -1;
		}

		if (divx_transcode_init(video->width, video->height, video->rate) < 0)
		{
			msg_error("Video: Cannot init DivX3 transcoder.\n");
			video_stop();
			return -1;
		}

		for (int i = 0; i < video->mux_numbufs; i++)
		{
			video->divx_base[i] = VideoAlloc(video->mux_bufsize + DIVX3_BUF_MARGIN * 2 + 8);
			if (video->divx_base[i] == NULL)
			{
				msg_error("Video: Cannot allocate DivX3 transcoding buffer [%d].\n", i);
				video_stop();
				return -1;
			}
			video->divx_buf[i] = video->divx_base[i] + DIVX3_BUF_MARGIN;
		}

		mpeg_setbuffer_array(MPEG_BUFFER_3, video->divx_buf, video->mux_numbufs, video->mux_bufsize);
		video->divx_cur_bufpos = 0;
		video->divx_cur_bufleft = mpeg_getbufsize(MPEG_BUFFER_3);

		video->no_partial = true;

		//script_error_callback(SCRIPT_ERROR_NONE);
	}
	media_skip_buffer(NULL);

	// if we need a separate decompression for audio...
	if (video->video_fmt != RIFF_VIDEO_MPEG12)
	{
		if (video->audio_fmt != RIFF_AUDIO_UNKNOWN && video->audio_fmt != RIFF_AUDIO_NONE)
		{
			video->audio_halfrate = (video->video_fmt == RIFF_VIDEO_DIV3);
			if (!audio_init(video->audio_fmt, video->audio_halfrate))
			{
				msg_error("Video: Cannot init audio decompressor.\n");
				script_error_callback(SCRIPT_ERROR_BAD_AUDIO);
			}
		} else
			script_audio_info_callback("");

		if (video->first_track >= 0)
		{
			if (video_set_audio_track(video->first_track) < 0)
			{
				msg_error("Video: Cannot init audio.\n");
				script_error_callback(SCRIPT_ERROR_BAD_AUDIO);
			}
		} else
			MSG("Video: * No audio tracks...\n");

		audio_set_ac3_getinfo(false);
	} else
		script_audio_info_callback("");

	// read & parse subtitle files
	read_subtitles(filepath);

	// write dvd-specific FIP stuff...
	const char *digits = "  00000";
	fip_write_string(digits);
	fip_write_special(FIP_SPECIAL_COLON1, 1);
	fip_write_special(FIP_SPECIAL_COLON2, 1);

	fip_write_special(FIP_SPECIAL_PLAY, 1);
	fip_write_special(FIP_SPECIAL_REPEAT, 0);
	fip_write_special(FIP_SPECIAL_PAUSE, 0);

	script_time_callback(0);

	script_video_info_callback(video->video_fmt_str);
	if (video->video_fmt != RIFF_VIDEO_MPEG12)
	{
		script_framesize_callback(video->width, video->height);
		script_framerate_callback(mpeg_get_fps());
	}

	video->playing = true;
	video->searching = false;
	video->stopping = false;
	video->skip_fwd = false;
	video->skip_rev = false;
	video->skip_wait = false;
	video->skip_waiting = false;

	video->in_divx_packet = false;
	video->need_to_stop = false;

	info_cnt = 0;
	mpeg_start();

	return 0;	
}

bool video_search_fourcc(DWORD fourcc, const DWORD *values)
{
	for (int i = 0; values[i] != 0xffffffff; i++)
	{
		if (fourcc == values[i])
			return true;
	}
	return false;
}

void video_detect_formats()
{
	static const DWORD mpeg12[] = { 
					FOURCC('M','P','E','G'), FOURCC('m','p','e','g'),
					FOURCC('P','I','M','1'), FOURCC('p','i','m','1'),
					FOURCC('M','P','G','2'), FOURCC('m','p','g','2'),
					0xffffffff
	};
	static const DWORD div3[] = { 
					FOURCC('D','I','V','3'), FOURCC('d','i','v','3'), 
					FOURCC('D','I','V','4'), FOURCC('d','i','v','4'), 
					FOURCC('D','I','V','5'), FOURCC('d','i','v','5'), 
					FOURCC('D','I','V','6'), FOURCC('d','i','v','6'), 
					FOURCC('M','P','4','3'), FOURCC('m','p','4','3'), 
					FOURCC('A','P','4','1'), FOURCC('a','p','4','1'), 
					FOURCC('M','P','G','3'), FOURCC('m','p','g','3'), 
					FOURCC('C','O','L','0'), FOURCC('c','o','l','0'), 
					FOURCC('C','O','L','1'), FOURCC('c','o','l','1'), 
					FOURCC('N','A','V','I'), FOURCC('S','A','N','3'), 
					0xffffffff
	};
	static const DWORD mpeg4[] = { 
					FOURCC('D','I','V','X'), FOURCC('d','i','v','x'),
					FOURCC('D','i','v','x'), FOURCC('D','i','v','X'),
					FOURCC('D','X','5','0'), FOURCC('d','x','5','0'),
					FOURCC('M','P','4','S'), FOURCC('m','p','4','s'), 
					FOURCC('M','P','4','V'), FOURCC('m','p','4','v'),
					FOURCC('M','4','S','2'), FOURCC('m','4','s','2'),
					FOURCC('F','M','P','4'), FOURCC('f','m','p','4'),
					FOURCC('X','V','I','D'), FOURCC('x','v','i','d'),
					FOURCC('3','I','V','0'), FOURCC('3','I','V','1'),
					FOURCC('3','I','V','2'), FOURCC('3','I','V','D'),
					FOURCC('3','I','V','X'), FOURCC('3','V','I','D'),
					FOURCC('A','T','M','4'), FOURCC('B','L','Z','0'),
					FOURCC('F','V','F','W'), FOURCC('H','D','X','4'), 
					FOURCC('D','M','4','V'), FOURCC('M','P','G','4'), 
					FOURCC('M','4','C','C'), FOURCC('m','4','c','c'), 
					FOURCC('P','V','M','M'), FOURCC('R','M','P','4'), 
					FOURCC('S','E','D','G'), FOURCC('N','D','I','G'), 
					FOURCC('W','V','1','F'), FOURCC('X','V','I','X'), 
					0xffffffff
					
	};
					
	DWORD v4cc = FOURCC(video->fourcc[0], video->fourcc[1], video->fourcc[2], video->fourcc[3]);

	if (video_search_fourcc(v4cc, mpeg12))
	{
		video->video_fmt = RIFF_VIDEO_MPEG12;
		video->video_fmt_str.Printf("MPEG-1/2 ('%s')", video->fourcc);
	}
	else if (video_search_fourcc(v4cc, div3))
	{
		video->video_fmt = RIFF_VIDEO_DIV3;
		video->video_fmt_str.Printf("DivX3 ('%s')", video->fourcc);
	}
	else if (video_search_fourcc(v4cc, mpeg4))
	{
		video->video_fmt = RIFF_VIDEO_MPEG4;
		video->video_fmt_str.Printf("MPEG-4 ('%s')", video->fourcc);
	}
	MSG("Video: * Video format %s detected!\n", *video->video_fmt_str);

	if (video->video_fmt != RIFF_VIDEO_MPEG12)
	{
		if (video->cur_track < 0 || video->track[video->cur_track].wfe == NULL)
		{
			video->audio_fmt = RIFF_AUDIO_NONE;
			video->audio_fmt_str = (char *)"None";
			MSG("Video: * Audio track not found.\n");
		}
		else
		{
			video->audio_fmt = audio_get_audio_format(video->track[video->cur_track].wfe->w_format_tag);
			video->audio_fmt_str = audio_get_audio_format_string(video->audio_fmt);
			MSG("Video: * Audio format %s detected.\n", *video->audio_fmt_str);
		}
	}
}

void video_update_info()
{
	if (video == NULL)
		return;
	static int old_secs = 0;
	KHWL_TIME_TYPE displ;
	displ.pts = 0;
	displ.timeres = 90000;
	if (video->skip_fwd || video->skip_rev || video->searching)
		displ.pts = video->scr;
	else if (mpeg_is_displayed())
	{
		khwl_getproperty(KHWL_TIME_SET, etimVideoFrameDisplayedTime, sizeof(displ), &displ);
		displ.pts += INT64(90000) * video->video_pos_base * video->scale / video->rate;
		displ.pts += video->displ_pts_base;
	}

	if ((LONGLONG)displ.pts != video->saved_pts)
	{
		if ((LONGLONG)displ.pts >= 0)
		{
			video->saved_pts = displ.pts;

			if (!video->skip_fwd && !video->skip_rev && !video->searching)
				show_subtitles(displ.pts);

			char fip_out[10];
			int secs = (int)(displ.pts / 90000);
			if (secs < 0)
				secs = 0;
			if (secs >= 10*3600)
				secs = 10*3600-1;
			if (secs != old_secs)
			{
				script_time_callback(secs);
				// check if total time changed
				video->UpdateTotalTime();
			
				fip_out[0] = ' ';
				fip_out[1] = ' ';
				fip_out[2] = (char)((secs/3600) + '0');
				int secs3600 = secs%3600;
				fip_out[3] = (char)(((secs3600/60)/10) + '0');
				fip_out[4] = (char)(((secs3600/60)%10) + '0');
				fip_out[5] = (char)(((secs3600%60)/10) + '0');
				fip_out[6] = (char)(((secs3600%60)%10) + '0');
				fip_out[7] = '\0';
				fip_write_string(fip_out);

				old_secs = secs;
			}
		}
	}
}

int video_read_input(BYTE * &buf, int &len, bool wait)
{
	int ret = 0;

	// don't read more blocks if we just need more output
	if (video->skip_fwd || video->skip_rev || video->audio_fmt == RIFF_AUDIO_UNKNOWN
		|| video->audio_fmt == RIFF_AUDIO_NONE
		// don't read more blocks if audio is not ready
		|| (video->audio_fmt != RIFF_AUDIO_UNKNOWN 
			&& video->audio_fmt != RIFF_AUDIO_NONE && audio_ready()))
	{
		do 
		{
			int skiphdr = 0;
			if (video->skip_fwd || video->skip_rev)
			{
				if (video->event != MEDIA_EVENT_VIDEO_PARTIAL && video->event != MEDIA_EVENT_NOP)
				{
					video->chunkleft = 0;
					video->old_video_packet_len = video->video_packet_len;
					video->video_packet_len = 0;

					ret = video->GetKeyFrame(video->skip_fwd ? VIDEO_KEY_FRAME_NEXT : VIDEO_KEY_FRAME_PREV);
					if (ret == 0)	// wait...
					{
						if (wait)
						{
							if (cycle() < 0)
								return -1;
							continue;
						}
						return 0;
					}
					skiphdr = 8;
					video->chunkleft = video->video_packet_len + skiphdr;
					if (ret < 1)
					{
						ret = 1;
						video->event = MEDIA_EVENT_STOP;
					}
				}
			}
			if (video->skip_fwd || video->skip_rev)
			{
				if (video->event != MEDIA_EVENT_STOP)
				{
					len = video->chunkleft;
					video->event = MEDIA_EVENT_NOP;
					ret = media_read_block(&buf, &video->event, &len);
					if (video->event == MEDIA_EVENT_OK)
					{
						video->chunkleft -= len;
						video->event = (video->chunkleft > 0) ? MEDIA_EVENT_VIDEO_PARTIAL : MEDIA_EVENT_VIDEO;
						len -= skiphdr;
						buf += skiphdr;
					}
				}
			}
			if (!video->skip_fwd && !video->skip_rev)	// normal play
			{
				video->event = MEDIA_EVENT_OK;
				len = 0;
				buf = NULL;

				ret = media_get_next_block(&buf, &video->event, &len);
			}
			if (ret > 0 && buf == NULL)
			{
				msg_error("Video: Not initialized. STOP!\n");
				return -1;
			}
			if (ret == -1)
			{
				msg_error("Video: Error getting next block!\n");
				return -1;
			}
			else if (ret == 0)		// wait...
			{
				if (wait)
				{
					if (cycle() < 0)
						return -1;
					continue;
				}
				return 0;
			}
			break;
		} while (wait);

		if (video->event == MEDIA_EVENT_STOP)
		{
			MSG("Video: STOP Event triggered!\n");
			if (!video->skip_fwd && !video->skip_rev)
				mpeg_play_normal();
			video->stopping = true;
			return 0;
		}

		if (video->video_fmt == RIFF_VIDEO_DIV3)
		{
			if (video->event == MEDIA_EVENT_VIDEO || video->event == MEDIA_EVENT_VIDEO_PARTIAL)
			{
				mpeg_setbufidx(MPEG_BUFFER_1, NULL);

//msg("[%d] DECODE len=%d   <%02x %02x %02x %02x %02x %02x %02x %02x>    (buf=%08x)\n", num_packets, (video->event == MEDIA_EVENT_VIDEO ? len + 8 : len), 
//	buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf);
//gui_update();

				bitstream_decode_buf_init(0, buf, video->event == MEDIA_EVENT_VIDEO ? len + 8 : len, true);
			}
		}
	}

	return 1;
}

int video_more_output(bool wait)
{
	if (video->divx_cur_bufleft < 24/*16*/)
	{
		do
		{
			if (video->need_to_stop)
				return -1;
			if (mpeg_find_free_blocks(MPEG_BUFFER_3) == 0)
			{
				if (wait)
				{
					if (cycle() < 0)
						return -1;
					continue;
				}
				return 0;		// wait...
			}
			break;
		} while (wait);
		if (video == NULL)
			return -1;
		video->divx_cur_bufpos = 0;
		video->divx_cur_bufleft = mpeg_getbufsize(MPEG_BUFFER_3);
	}
	BYTE *tmpbuf = mpeg_getcurbuf(MPEG_BUFFER_3);
	BYTE *buf = tmpbuf + video->divx_cur_bufpos;
	int len = video->divx_cur_bufleft - 8;
	
//msg("[%d] ENCODE len=%d       (buf=%08x)\n", num_packets, len, buf);
//gui_update();

	bitstream_encode_buf_init(buf, len);

	return 1;
}

int video_send_video_packet(MPEG_BUFFER which, BYTE *buf, int len, bool start_of_packet)
{
	while (len > 0)
	{
		MpegPacket *packet = NULL;
		packet = mpeg_feed_getlast();
		if (packet == NULL)		// well, it won't really help
			return 0;
		memset((BYTE *)packet + 4, 0, sizeof(MpegPacket) - 4);

		BYTE *base = buf;
		int curlen = len;

		// find next VOP start
		if (video->video_fmt == RIFF_VIDEO_MPEG4)
		{
			for (; len >= 0 && (base == buf || *buf != 0 || buf[1] != 0 || buf[2] != 1 || buf[3] != 0xb6); len--)
			{
				if (video->test_for_qpel_gmc)
				{
					if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1 && (buf[3] >= 0x20 && buf[3] <= 0x2F))
					{
						video_decode_vol_header(buf + 4, len - 4);
						if (video->gmc_flag)
						{
							msg("Video: Error! Cannot play MPEG-4 with GMC.\n");
							script_error_callback(SCRIPT_ERROR_GMC);
							return -1;
						}
						else if (video->qpel_flag)
						{
							msg("Video: Error! Cannot play MPEG-4 with QPEL.\n");
							script_error_callback(SCRIPT_ERROR_QPEL);
							return -1;
						}
						video->test_for_qpel_gmc = false;
					}
				}
				buf++;
			}

			if (len > 0)
				curlen -= len;
		} else
			len = -1;

		VIDEO_FRAME_TYPE frm_type = VIDEO_FRAME_NONE;
		packet->type = 0;
		packet->pData = base;
		packet->size = curlen;
		if (base[0] == 0 && base[1] == 0 && base[2] == 1 && base[3] == 0xb6)
		{
			frm_type = (VIDEO_FRAME_TYPE)(base[4] >> 6);
			// add I-frames to the index (but ignore too frequent I-frames)
			if (frm_type == VIDEO_FRAME_I)
			{
				video->SetCurrentIndex(video->abs_video_pos, 
					video->video_packet_len, video->video_offs);
				video->cur_key_offs = video->video_offs;
				video->cur_key_pos = video->abs_video_pos;
				video->last_frame_pos = video->frame_pos;
			} else
			{
				// wrong index - skip it
				if (video->skip_fwd || video->skip_rev)
				{
					video->cur_key_idx = video->last_good_key_idx;
					break;
				}
			}
			if (frm_type == VIDEO_FRAME_I || frm_type == VIDEO_FRAME_P)
			{
				if (video->skip_fwd || video->skip_rev)
				{
					if (video->skip_rev)
					{
						// \TODO: it'd be better to patch 'time_incr' header bits...
						mpeg_setpts(0);
					}
					packet->pts = 0;
					packet->flags = 0;
					packet->scr = 0 | SPTM_SCR_FLAG;
				} else
				{
					packet->pts = (LONGLONG)video->video_pos * video->scale;
					packet->scr = packet->pts | SPTM_SCR_FLAG;
					packet->pts += video->scale * 10;
					packet->flags = 0x80;
					
					video->saved_video_pts = packet->pts;

#if 0
msg("v: %d\n", packet->pts);
#endif

#ifdef USE_RESYNCS_CONTROL
					if (mpeg_is_playing())
					{
						LONGLONG stc = mpeg_getpts();
						LONGLONG good_scr = INT64(90000) * packet->pts / video->rate;

						//msg("%d %d\n", (int)stc, (int)good_scr);
						if (stc > good_scr + video->good_delta_stc)
						{
							khwl_pause();
							mpeg_start();
							msg("Video: Resync...\n");
							video->wait_for_resync = false;
						}
					}
#endif
				}
			}

		}

#if 0
msg("PACKET [%d] len=%d       (buf=%08x)\n", num_packets, packet->size, packet->pData);
if (packet->size > 0)
{
BYTE *buf = packet->pData;
int len = packet->size;
DUMP_FRAME("%02x %02x %02x %02x %02x %02x %02x %02x ... %02x %02x %02x %02x %02x %02x %02x %02x\n",
		   buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], 
		   buf[len-8], buf[len-7], buf[len-6], buf[len-5], buf[len-4], buf[len-3], buf[len-2], buf[len-1]);
}
#endif

#if 0
{
FILE *fp;
fp = fopen("out.m4v", "ab");
//fwrite("00dc\0\0\0\0", 8, 1, fp);
fwrite(packet->pData, packet->size, 1, fp);
fclose(fp);
}
#endif
#if 0
msg("[%d]\t\tsize=%d\t\tpts=%d\n", packet->type, packet->size, (int)packet->pts);
#endif

		// increase bufidx
		mpeg_setbufidx(which, packet);
		num_packets++;

		// frame order analysis
#ifdef NEW_SYNC
  		if (frm_type == VIDEO_FRAME_I)
		{
			mpeg_correct_pts();
			while (mpeg_feed_pop())
				;
			mpeg_feed(MPEG_FEED_VIDEO);
		}
		else if (frm_type == VIDEO_FRAME_P)
		{
			if (!mpeg_feed_isempty())
			{
				mpeg_correct_pts();
				while (mpeg_feed_pop())
					;
			}
			mpeg_feed_push();
		}
		else
		{
			if (!mpeg_feed_isempty())
				mpeg_feed_push();
			else
				mpeg_feed(MPEG_FEED_VIDEO);
		}
#else
		mpeg_feed(MPEG_FEED_VIDEO);
#endif		
	}
	return 1;
}

void video_decode_vol_header(BYTE *buf, int len)
{
	if (video == NULL)
		return;
	video->qpel_flag = false;
	video->gmc_flag = false;

	video_bits_init(buf, len);
	video_skip_bits(1);
	video_skip_bits(8);
	int vo_ver_id;
	if (video_get_bits(1)) 
	{
        vo_ver_id = video_get_bits(4);
        video_skip_bits(3);
    } else
        vo_ver_id = 1;
    
	int aspect_ratio_info = video_get_bits(4);
    if (aspect_ratio_info == 15)	// extended
        video_skip_bits(16);

    if (video_get_bits(1))	// vol control parameters
	{
        video_skip_bits(3);
        if (video_get_bits(1))	// vbv parameters
		{
			video_skip_bits(16);
            video_skip_bits(16);
            video_skip_bits(16);
            video_skip_bits(15);
            video_skip_bits(16);
        }
    }

    int vol_shape = video_get_bits(2);
    if(vol_shape == 3 && vo_ver_id != 1)	// gray shape
        video_skip_bits(4);

    // marker
	video_skip_bits(1);

    int time_base_den = video_get_bits(16);

    // marker
	video_skip_bits(1);

    if (video_get_bits(1))   // fixed_vop_rate
	{
		int time_increment_bits = video_log2(time_base_den - 1) + 1;
		if (time_increment_bits < 1)
			time_increment_bits = 1;

        video_skip_bits(time_increment_bits);
	}

	if (vol_shape != 2)	// not bin-only shape
	{
        if (vol_shape == 0)	// rect shape
		{
            video_skip_bits(1);
            /*int width = */video_get_bits(13);
            video_skip_bits(1);
            /*int height = */video_get_bits(13);
            video_skip_bits(1);
        }

        /*int progressive_frame = 1 - */video_get_bits(1);
        video_skip_bits(1);
		int vol_sprite_usage = video_get_bits(vo_ver_id == 1 ? 1 : 2);
		if (vol_sprite_usage == 2)
			video->gmc_flag = true;
        if (vol_sprite_usage == 1 || vol_sprite_usage == 2)	// static or GMC
		{
            if (vol_sprite_usage == 1)
			{
                video_skip_bits(14);
                video_skip_bits(14);
                video_skip_bits(14);
                video_skip_bits(14);
            }
            video_skip_bits(9);
            if (vol_sprite_usage == 1)
                video_skip_bits(1);
        }
        if (video_get_bits(1) == 1) 
            video_skip_bits(8);

        if (video_get_bits(1)) // vol_quant_type
		{ 
            int i;
            if (video_get_bits(1))
			{
                for (i = 0; i < 64; i++)
				{
                    if (video_get_bits(8) == 0)
						break;
                }
            }

            if (video_get_bits(1))
			{
                for (i = 0; i < 64; i++)
				{
                    if (video_get_bits(8) == 0)
						break;
                }
            }
        }

        if (vo_ver_id != 1)
			video->qpel_flag = video_get_bits(1) != 0;
	}
}

int divx3_callback(BITSTREAM_MODE mode, BYTE *outbuf, int outlen)
{
	if (mode != BITSTREAM_MODE_OUTPUT)
	{
		mpeg_release_packet(MPEG_BUFFER_1, NULL);
	}

	if (outlen != 0)
	{
		int ll = (outlen + 7) & (~7);
		video->divx_cur_bufleft -= ll;
		video->divx_cur_bufpos  += ll;
		if (video_send_video_packet(MPEG_BUFFER_3, outbuf, outlen, false) < 0)
			return -1;
	}

	if (mode == BITSTREAM_MODE_INPUT)
	{
		BYTE *inbuf;
		int inlen;
		if (video_read_input(inbuf, inlen, true) < 0)
			return -1;
	}

	if (mode == BITSTREAM_MODE_OUTPUT)
	{
		if (video_more_output(true) < 0)
			return -1;
	}

	return 0;
}

extern int frame_number;

/// Advance playing
int video_loop()
{
	static BYTE *buf = NULL;
	static int len = 0;
	int ret;

	// for video transcoding

#if 0
again:
#endif

	if (video == NULL)
		return 1;

	if (video->need_to_stop)
		video_stop();

	if (video == NULL)
		return 1;

	if (video->stopping)
	{
		if (video->skip_fwd || video->skip_rev || video->searching)
		{
			video_stop();
			return 1;
		}
		if (mpeg_wait(TRUE) == 1)
		{
			video_stop();
			return 1;
		}
	}

	if (video->delayed_skip != 0 && !video->in_divx_packet)
	{
		if (!video->skip_fwd && !video->skip_rev)
			media_seek_curleft();
		if (video->delayed_skip == 1)
			video->skip_fwd = true;
		else if (video->delayed_skip == 2)
			video->skip_rev = true;
		video->delayed_skip = 0;
	}

	if (video->searching && !video->cancel_skip)
	{
		int ret = video->GetKeyFrame(VIDEO_KEY_FRAME_CONTINUE);
		if (ret > 0)
			video->cancel_skip = true;
	}
	
	if (video->cancel_skip && (video->skip_fwd || video->skip_rev || video->searching) && !video->in_divx_packet)
	{
		video_lseek(video->video_offs, SEEK_SET);
		video->video_pos_base = video->abs_video_pos;// = video->last_key_pos - 1;
		video->video_pos = 0;
		video->audio_total_bytes = -1;
		video->audio_pos = 0;
		video->frame_pos = video->last_frame_pos - 1;

		fip_write_special(FIP_SPECIAL_PLAY, 1);
		fip_write_special(FIP_SPECIAL_REPEAT, 0);
		fip_write_special(FIP_SPECIAL_PAUSE, 0);

		if (video->skip_waiting)
			mpeg_release_packet(MPEG_BUFFER_1, NULL);
			
		mpeg_setspeed(MPEG_SPEED_NORMAL);
		mpeg_set_scale_rate((DWORD *)&video->scale, (DWORD *)&video->rate);
		// we'll correct display pts later...
		mpeg_setpts(0);

		script_speed_callback(MPEG_SPEED_NORMAL);
			
		video->skip_fwd = false;
		video->skip_rev = false;
		video->skip_wait = false;
		video->skip_waiting = false;
		video->searching = false;
		video->cancel_skip = false;
		video->chunkleft = 0;
		video->max_rev_incr = def_max_rev_incr;

		media_skip_buffer(NULL);
		
		if (video->audio_fmt != RIFF_AUDIO_UNKNOWN && video->audio_fmt != RIFF_AUDIO_NONE)
			audio_reset();
		
		info_cnt = 0;
	}

	if (!video->stopping && !video->searching && !video->in_divx_packet)
	{
		if (!video->skip_waiting)
		{
			ret = video_read_input(buf, len, false);
			if (ret == 0)
				return 0;
			if (ret < 0)
			{
				return 1;
			}
		}
		
		if (video->skip_wait)
		{
			int cur_time = script_get_time(NULL);
			int delta = 300;
			if (cur_time < video->last_keyframe_time + delta)
			{
				video->skip_waiting = true;
				return 0;
			}
			video->last_keyframe_time = cur_time;
		}

		if (video->video_fmt == RIFF_VIDEO_MPEG12)
		{
			if (((VideoMpg *)video)->ProcessChunk(buf, len) == 0)
				return 0;
		}

		else if (video->event == MEDIA_EVENT_VIDEO || video->event == MEDIA_EVENT_VIDEO_PARTIAL)
		{
			if (video->video_fmt == RIFF_VIDEO_DIV3)
			{
				video->in_divx_packet = true;
				ret = video_more_output(false);
				if (ret == 0)
				{
					video->in_divx_packet = false;
					return 0;
				}
				if (ret < 0)
				{
					video->event = MEDIA_EVENT_NOP;
				}
#if 0
FILE *fp;
fp = fopen("out.m4v", "ab");
fwrite("00dc", 4, 1, fp);
fwrite("\0\0\0\0", 4, 1, fp);
int fffpos = ftell(fp);
fclose(fp);
#endif

				// ok, now transcode
				if (divx_transcode(divx3_callback /*, len */) < 0)
				// skip this frame
				{
					if (video == NULL)
					{
						return 1;
					}
					video->event = MEDIA_EVENT_NOP;
					mpeg_release_packet(MPEG_BUFFER_1, NULL);
				}

//msg("-- %d done\n", frame_number);
//gui_update();

#if 0
fp = fopen("out.m4v", "rb+");
fseek(fp, 0, SEEK_END);
int fffpos2 = ftell(fp);
fseek(fp, fffpos - 4, SEEK_SET);
int fff = fffpos2 - fffpos;
fwrite(&fff, 4, 1, fp);
fseek(fp, fffpos2, SEEK_SET);
if ((fffpos2 - fffpos) & 1)
{
	fputc(0, fp);
}
fclose(fp);
#endif
				video->in_divx_packet = false;
				video->skip_waiting = false;
			}
			else
			{
				if (video_send_video_packet(MPEG_BUFFER_1, buf, len, video->event == MEDIA_EVENT_VIDEO) < 0)
					return 1;
				video->skip_waiting = false;
			}
			
		}
		else if (video->event == MEDIA_EVENT_AUDIO || video->event == MEDIA_EVENT_AUDIO_PARTIAL)
		{
#ifndef SKIP_AUDIO
			if (video->audio_fmt != RIFF_AUDIO_UNKNOWN && video->audio_fmt != RIFF_AUDIO_NONE)
			{
				// send multiple frames (packets) in one chunk
				BYTE *b = buf;
				for (;;)
				{
					MpegPacket *packet = NULL;
					packet = mpeg_feed_getlast();
					if (packet == NULL)		// well, it won't really help
						return 0;

					int numread = audio_parse_packet(packet, b, len);
					if (numread == 0)
						break;
					len -= numread;
					b += numread;

					// audio syncronisation...
					int bps = audio_get_output_bps();
					if (bps > 0)
					{
						if (video->audio_total_bytes < 0)
						{
							video->audio_total_bytes = (video->saved_video_pts + video->audio_delta - video->scale * 10 - video->audio_pts_offset) * bps / video->rate;
						}

						packet->pts = ((LONGLONG)video->audio_total_bytes * video->rate / bps) + video->audio_pts_offset;
						packet->scr = packet->pts | SPTM_SCR_FLAG;
						packet->pts += video->scale * 10;

						packet->flags = 0x80;

#if 0
						msg("a:   %d\t[%d]\n", (int)packet->pts, (int)(packet->pts - video->saved_video_pts));
#endif

						if (video->audio_delta == (LONGLONG)SPTM_SCR_FLAG && video->saved_video_pts > 0)
							video->audio_delta = packet->pts - video->saved_video_pts;

					} else
					{
						packet->pts = packet->scr = 0;
						packet->flags = 0;
#if 0
						msg("a:   %d\n", (int)packet->pts);
#endif
					}

					if (video->audio_total_bytes >= 0)
						video->audio_total_bytes += packet->size;

					mpeg_feed(MPEG_FEED_AUDIO);
					num_packets++;
				}
			} 
#endif
		}
	}

	if (info_cnt++ > max_info_cnt)
	{
		info_cnt = 0;
		video_update_info();
	}

#if 0
	if (!video->in_divx_packet)
		goto again;
#endif

	return 0;
}

/// Pause playing
BOOL video_pause()
{
	fip_write_special(FIP_SPECIAL_PLAY, 0);
	fip_write_special(FIP_SPECIAL_REPEAT, 0);
	fip_write_special(FIP_SPECIAL_PAUSE, 1);

	mpeg_setspeed(MPEG_SPEED_PAUSE);

	return TRUE;
}

/// Stop playing
BOOL video_stop()
{
	if (video != NULL)
	{
		if (video->in_divx_packet)
		{
			video->need_to_stop = true;
			return FALSE;
		}

		delete_subtitles();

		mpeg_deinit();
		khwl_setvideomode(KHWL_VIDEOMODE_NONE, TRUE);
		if (video->playing)
		{
			video->playing = false;
		}
		video->stopping = false;

		media_close();
	}
	return TRUE;
}

void video_setdebug(BOOL ison)
{
	video_msg = ison == TRUE;
}
BOOL video_getdebug()
{
	return video_msg;
}

///////////////////////////////////////////

BOOL video_open(const char *filepath)
{
	if (video != NULL)
		video_close();
	int fd = cdrom_open(filepath, O_RDONLY);
	if (fd < 0)
	{
		msg_error("Video: Cannot open file %s.\n", filepath);
		return FALSE;
	}

	// detect container type
	SPString fname = filepath;
	video = NULL;
	if (fname.FindNoCase(".avi") >= 0 || fname.FindNoCase(".divx") >= 0)
	{
		video = new VideoAvi();
	} 
	else if (fname.FindNoCase(".mpg") >= 0 || fname.FindNoCase(".mpeg") >= 0 || fname.FindNoCase(".vob") >= 0
		 || fname.FindNoCase(".dat") >= 0)
	{
		video = new VideoMpg();
	}
	else if (fname.FindNoCase(".mp4") >= 0 || fname.FindNoCase(".3gp") >= 0 || fname.FindNoCase(".mov") >= 0)
	{
		//video = new VideoQt();
		msg_error("Video: MP4 container currently not supported.\n");
		close(fd);
		return FALSE;
	}
	
	for (int i = 0; i < 2; i++)
	{
		if (video == NULL)
		{
			// try to auto-detect video type
			BYTE hdr[16];
			lseek64(fd, 0, SEEK_SET);
			if (read(fd, hdr, 12) != 12)
			{
				msg_error("Video: Cannot read file header.\n");
				return FALSE;
			}
			lseek64(fd, 0, SEEK_SET);
			if (strncmp((char *)hdr, "RIFF", 4) == 0 && strncmp((char *)hdr+8, "AVI ", 4) == 0)
			{
				video = new VideoAvi();
			}
			else if ((hdr[0] == 0 && hdr[1] == 0 && hdr[2] == 1 && (hdr[3] == 0xba || hdr[3] == 0xb3))
				|| (strncmp((char *)hdr, "RIFF", 4) == 0 && strncmp((char *)hdr+8, "CDXA", 4) == 0))
			{
				video = new VideoMpg();
			}
			else
			{
				msg_error("Video: Unknown media file type.\n");
				close(fd);
				return FALSE;
			}
			i++;
		}
		// try to parse...
		if (video != NULL)
		{
			video->fd = fd;

			if (video->Parse())
			{
				video->look4chunk = false;
				return TRUE;
			}
			SPSafeDelete(video);
		}
	}
	close(fd);
	return FALSE;
}

BOOL video_close()
{
	if (video != NULL)
	{
		if (video->video_fmt == RIFF_VIDEO_DIV3)
		{
			divx_transcode_deinit();
			divx_transcode_predeinit();
		}

		if (video->audio_fmt != RIFF_AUDIO_UNKNOWN && video->audio_fmt != RIFF_AUDIO_NONE)
			audio_deinit();
		
		close(video->fd);
		SPSafeDelete(video);
		return TRUE;
	}
	return FALSE;
}

VIDEO_CHUNK_TYPE video_getnext(BYTE *buf, int buflen, int *pos, int *left, int *len)
{
	if (video != NULL)
		return video->GetNext(buf, buflen, pos, left, len);
	return VIDEO_CHUNK_UNKNOWN;
}

int video_read(BYTE *buf, int len)
{
	if (video == NULL || buf == NULL || len < 1)
		return 0;
	int n = 0, newlen = 0;
	video->cur_offs = lseek64(video->fd, 0, SEEK_CUR);
	while (newlen < len)
	{
		n = read (video->fd, buf + newlen, len - newlen);
		if (n == 0)
		{
			video->is_eof = true;
			break;
		}
		if (n < 0) 
		{
			if (errno == EINTR)
				continue;
			else
				break;
		}
		newlen += n;
	}

	return newlen;
}

bool video_eof()
{
	return video == NULL || video->is_eof;
}

void video_shiftpos(int offs)
{
	video->cur_offs -= offs;
}

LONGLONG video_lseek(LONGLONG off, int where)
{
	LONGLONG fp = lseek64(video->fd, off, where);
	media_set_filepos(fp);
	return fp;
}

int video_set_audio_track(int track)
{
	if (track == -1)
	{
		track = video->cur_track + 1;
		if (track >= video->num_tracks)
			track = video->first_track;
	}
	if (track < video->first_track || track >= video->num_tracks)
	{
		script_audio_stream_callback(-1);
		return -1;
	}

	if (video->video_fmt == RIFF_VIDEO_MPEG12)
	{
		msg("MPEG: Set audio stream #%d.\n", track);
		mpeg_setaudiostream(track);
		video->cur_track = track;
		script_audio_stream_callback(video->cur_track + 1);
		return 0;
	}

	if (video->audio_fmt == RIFF_AUDIO_UNKNOWN || video->audio_fmt == RIFF_AUDIO_NONE)
		return -1;
	
	if (video->track[track].wfe == NULL)
	{
		msg("Video: Cannot change audio tracks of unknown format.\n");
		script_error_callback(SCRIPT_ERROR_INVALID);
		return -1;
	}

	RIFF_AUDIO_FORMAT newfmt = audio_get_audio_format(video->track[track].wfe->w_format_tag);
	if (newfmt == RIFF_AUDIO_UNKNOWN || newfmt == RIFF_AUDIO_NONE)
	{
		msg_error("Video: Audio format not supported.\n");
		script_error_callback(SCRIPT_ERROR_BAD_AUDIO);
		return -1;
	}
	if (newfmt != video->audio_fmt)
	{
		audio_deinit();
		khwl_stop();
		video->audio_fmt = newfmt;
		video->audio_halfrate = (video->video_fmt == RIFF_VIDEO_DIV3);
		if (!audio_init(video->audio_fmt, video->audio_halfrate))
		{
			msg_error("Video: Cannot init audio decompressor.\n");
			script_error_callback(SCRIPT_ERROR_BAD_AUDIO);
		}
		
		video_play_from_last_keyframe();
	}

	MSG("Video: Setting audio params.\n");
	if (audio_setaudioparams(video->audio_fmt, video->track[track].wfe,
							video->audio_halfrate ? 2 : 1) < 0)
	{
		msg_error("Video: Audio format %d not supported!\n", video->audio_fmt);
		video->cur_track = -1;
		return -1;
	}

	audio_reset();
	
	if (track != video->cur_track)
	{
		video->cur_track = track;
		video->audio_total_bytes = -1;
	}
	video->cur_audio_bits = video->track[video->cur_track].wfe->w_bits_per_sample;

	script_audio_stream_callback(video->cur_track + 1);
	
	return 0;
}

/////////////////////////////////////////////////////////////

BOOL video_seek(int seconds)
{
	if (video == NULL)
		return FALSE;

	if (seconds > 10 * 60)
	{
		script_error_callback(SCRIPT_ERROR_WAIT);
		script_player_subtitle_callback("");
		gui_update();
	}

	int ret = video->GetKeyFrame(seconds);
	if (ret < 0)
	{
		script_error_callback(SCRIPT_ERROR_INVALID);
		return FALSE;
	}

	if (video->video_fmt != RIFF_VIDEO_MPEG12)
	{
		mpeg_stop();
		if (ret > 0)
		{
			video->cancel_skip = true;
		}

		video->event = MEDIA_EVENT_OK;
		video->searching = true;
	}
	
	info_cnt = 0;

	return TRUE;
}

int video_forward(BOOL fast)
{
	if (video->video_fmt == RIFF_VIDEO_MPEG12)
	{
		if (video->GetKeyFrame(VIDEO_KEY_FRAME_NEXT) < 0)
			return -1;
		info_cnt = 0;
		return 0;
	}

	fip_write_special(FIP_SPECIAL_PLAY, 0);
	fip_write_special(FIP_SPECIAL_REPEAT, 1);
	mpeg_setspeed(MPEG_SPEED_FAST_FWD_MASK);
	mpeg_setpts(0);
	script_player_subtitle_callback("");

	// skip the rest of current chunk
	if (!video->in_divx_packet)
	{
		if (!video->skip_fwd && !video->skip_rev)
			media_seek_curleft();
	}
	if (video->skip_waiting)
		mpeg_release_packet(MPEG_BUFFER_1, NULL);
	
	video->event = MEDIA_EVENT_OK;
	video->searching = false;
	video->skip_rev = false;
	video->skip_fwd = true;
	video->skip_waiting = false;
	video->skip_wait = !fast;
	video->max_rev_incr = def_max_rev_incr;
	video->skip_cnt = 0;
	video->last_keyframe_time = script_get_time(NULL);
	info_cnt = 0;

	if (video->in_divx_packet)
	{
		video->delayed_skip = 1;
		video->skip_fwd = false;
	}

	return 1;
}
int video_rewind(BOOL fast)
{
	if (video->video_fmt == RIFF_VIDEO_MPEG12)
	{
		if (video->GetKeyFrame(VIDEO_KEY_FRAME_PREV) < 0)
			return -1;
		info_cnt = 0;
		return 0;
	}

	fip_write_special(FIP_SPECIAL_PLAY, 0);
	fip_write_special(FIP_SPECIAL_REPEAT, 1);
	mpeg_setspeed(MPEG_SPEED_FAST_REV_MASK);
	mpeg_setpts(0);
	script_player_subtitle_callback("");

	// skip the rest of current chunk
	if (!video->in_divx_packet)
	{
		if (!video->skip_fwd && !video->skip_rev)
			media_seek_curleft();
	}
	if (video->skip_waiting)
		mpeg_release_packet(MPEG_BUFFER_1, NULL);

	video->event = MEDIA_EVENT_OK;
	video->searching = false;
	video->skip_fwd = false;
	video->skip_rev = true;
	video->skip_waiting = false;
	video->skip_wait = !fast;
	video->skip_cnt = 0;
	video->last_keyframe_time = script_get_time(NULL);
	info_cnt = 0;

	if (video->in_divx_packet)
	{
		video->delayed_skip = 2;
		video->skip_rev = false;
	}

	return 1;
}

void video_set_audio_offset(LONGLONG offset)
{
	if (video != NULL)
	{
		if (video->video_fmt != RIFF_VIDEO_MPEG12)
			video->audio_pts_offset = offset * (LONGLONG)video->rate / INT64(90000);
	}
}

LONGLONG video_get_audio_offset()
{
	if (video != NULL)
	{
		if (video->video_fmt != RIFF_VIDEO_MPEG12)
			return INT64(90000) * video->audio_pts_offset / video->rate;
	}
	return 0;
}

int video_play_from_last_keyframe()
{
	// skip the rest of current chunk
	if (!video->in_divx_packet)
	{
		if (!video->skip_fwd && !video->skip_rev)
			media_seek_curleft();
	}

	video_lseek(video->cur_key_offs, SEEK_SET);
	video->abs_video_pos = video->cur_key_pos;
	video->video_pos_base = video->abs_video_pos;// = video->last_key_pos - 1;
	video->video_pos = 0;
	video->audio_total_bytes = video->audio_delta;
	video->audio_pts_offset = 0;
	video->audio_pos = 0;
	video->frame_pos = video->last_frame_pos - 1;

	video->chunkleft = 0;
	video->max_rev_incr = def_max_rev_incr;

	media_skip_buffer(NULL);
		
	info_cnt = 0;

	mpeg_setspeed(MPEG_SPEED_NORMAL);
	mpeg_set_scale_rate((DWORD *)&video->scale, (DWORD *)&video->rate);
	// we'll correct display pts later...
	mpeg_setpts(0);

	return 0;
}

////////////////////////////////////////////////////////////////

static DWORD v_dec_v = 0;
static int v_bitidx = 0, v_left = 0;

static void video_bits_init(BYTE *buf, int len)
{
	bitstream_decode_buf_init(1, buf, len, true);
	bitstream_decode_start(1, v_dec_v, v_bitidx, v_left);
}

static DWORD video_get_bits(int num_bits)
{
	DWORD ret;
	bitstream_get_bits(ret, 1, v_dec_v, v_bitidx, v_left, num_bits);
	return ret;
}

static int video_skip_bits(int num_bits)
{
	bitstream_skip_bits(1, v_dec_v, v_bitidx, v_left, num_bits);
	return 0;
}

static const BYTE video_log2_tab[256] = 
{
	0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
};

int video_log2(DWORD v)
{
    int n = 0;
    if (v & 0xffff0000) 
	{
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) 
	{
        v >>= 8;
        n += 8;
    }
    
	n += video_log2_tab[v];
	
    return n;
}


#endif
