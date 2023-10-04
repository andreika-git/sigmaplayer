//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - Audio player source file.
 *  \file       audio.cpp
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

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_fip.h>
#include <libsp/sp_khwl.h>
#include <libsp/sp_mpeg.h>
#include <libsp/sp_cdrom.h>

#include "script.h"
#include "player.h"
#include "media.h"


#define AUDIO_USE_FAST_OUTPUT
//#define AUDIO_USE_FAST_OUTPUT_PTR

#ifdef INTERNAL_AUDIO_PLAYER

#include <contrib/libmad/mad.h>

#define AUDIO_INTERNAL
#include "audio.h"

static bool audio_msg = true;
#define MSG if (audio_msg) msg

static Audio *audio = NULL;
static int num_packets = 0, info_cnt = 0, max_info_cnt = 32;

const int min_avg_num = 10;
const int mp3_mintmpbuf = 16384;
const int mp3_numtmpbuf = mp3_mintmpbuf*8;

typedef struct AudioDir
{
	SPString path;
	DIR *dir;
} AudioDir;

static SPClassicList<AudioDir> *audio_dir_list = NULL;
static SPDLinkedList<SPString> *audio_prev_list = NULL;


#ifdef WIN32
#define audio_lseek _lseeki64
#else
#define audio_lseek lseek
#endif


// return big-endian 16-bit
static inline WORD mp3_scale(mad_fixed_t sample)
{
	// round
	sample += (1L << (MAD_F_FRACBITS - 16));

	// clip
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;

	// quantize
	return (WORD)(((sample >> (MAD_F_FRACBITS + 1 - 8)) & 0xff) | ((sample >> 5) & 0xff00));
}

static int normal_mp3_output(BYTE *data, struct mad_pcm *pcm)
{
	unsigned int nchannels, nsamples;
	mad_fixed_t const *left_ch, *right_ch;

	// pcm->samplerate contains the sampling frequency
	nchannels = pcm->channels;
	nsamples  = pcm->length;
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];

	WORD *d = (WORD *)data;
	if (nchannels == 2) 
	{
		while (nsamples--) 
		{
			*d++ = mp3_scale(*left_ch++);
			*d++ = mp3_scale(*right_ch++);
		}
	} else
	{
		while (nsamples--) 
		{
			// output sample(s) in 16-bit signed big-endian PCM
			*d++ = mp3_scale(*left_ch++);
		}
	}
	return (DWORD)d - (DWORD)data;
}

#ifdef AUDIO_USE_FAST_OUTPUT

static const DWORD and1_mask = 0x1FE00000, and2_mask = 0x1FE000;

static int fast_mp3_output(BYTE *data, struct mad_pcm *pcm)
{
	unsigned int nchannels, nsamples;
	mad_fixed_t const *left_ch, *right_ch;

	// pcm->samplerate contains the sampling frequency
	nchannels = pcm->channels;
	nsamples  = pcm->length;
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];

	register DWORD and1 = and1_mask, and2 = and2_mask;
	if (nchannels == 2) 
	{
		DWORD *bd = (DWORD *)data;
		register DWORD d, d1, d2;
		
		while (nsamples--) 
		{
			d1 = *left_ch++;
			d2 = *right_ch++;
			d  = (d1 & and1) >> 21;
			d |= (d2 & and1) >> 5;
			d |= (d2 & and2) << 11;
			d |= (d1 & and2) >> 5;
			*bd++ = d;
		}
		return (DWORD)bd - (DWORD)data;
	} else
	{
		WORD *bw = (WORD *)data;
		DWORD d, d1;
		while (nsamples--) 
		{
			d1 = *left_ch++;
			d  = (d1 & and1) >> 21;
			d |= (d1 & and2) >> 5;
			*bw++ = (WORD)d;
		}
		return (DWORD)bw - (DWORD)data;
	}
}

#endif

////////////////////////////////////////////////////

Audio::Audio()
{
	audio_numbufs = 226;
	audio_bufsize = 4608;
	audio_bufidx = 0;

	audio_samplerate = audio_channels = -1;
	samples_per_frame = 1152;

	audio_frame_number = 0;

	bitrate = 0;
	out_bps = 1;	// avoid div.zero

	avg_bitrate = 0; avg_num = 0;
	filesize = 0;
	cur_offset = 0;
	cur_tmpoffset = 0;
	length = 0;
	seek_offset = 0;

	needs_resample = false;
	resample_packet_size = 4096;

	mp3_tmpbuf = NULL;
	mp3_tmppos = 0; mp3_tmpstart = 0; mp3_tmpread = 0;
	mp3_want_more = false;

	ac3_scan_header = -1;
	ac3_framesize = 0;
	ac3_gatherinfo = true;
	ac3_gather_always = false;	// disabled by video player

	is_partial_packet = false;
	is_partial_next_packet = false;

	cur_bitrate = cur_audio_format = -1;
	cur_samples = cur_bits = cur_channels = -1;

	playing	= false;
	stopping = false;
	wait = false;
	fast = false;
	fd = -1;

	saved_pts = 0;

	mux_buf = NULL;
	mux_numbufs = 8;
	mux_bufsize = 32768;

	audio_fmt = RIFF_AUDIO_UNKNOWN;
	fmtex = NULL;

	max_info_cnt = 32;

	mp3_output = NULL;
}

Audio::~Audio()
{
	SPSafeFree(mp3_tmpbuf);
	SPSafeFree(mux_buf);
	SPSafeFree(fmtex);
}

DWORD Audio::GetBE(BYTE *buf, int offset, int numbytes)
{	
	DWORD res = 0;
	int nb_1 = numbytes - 1;

	buf += offset;

	for (int n = 0; n < numbytes; n++)
	{
		res |= (*buf++) << (nb_1-- << 3);
	}
	
	return res;
}

int Audio::SetVbrLength(int total_num_frames, int vbr_filesize)
{
	length = (int)(INT64(1000) * total_num_frames * samples_per_frame / audio_samplerate);
	if (vbr_filesize > 0 && (filesize == 0 || vbr_filesize < filesize))
		filesize = vbr_filesize;
	if (length > 0)
	{
		bitrate = (int)(filesize * 8000 / length);
	}
	if (fd >= 0)
	{
		script_totaltime_callback(length / 1000);
	}
	return 0;
}

int Audio::ParseXingHeader(BYTE *buf, int len)
{
	if (audio_samplerate <= 0)
		return -1;
	int flags = GetBE(buf, 4, 4);
	int offs = 8;
	int total_num_frames = 0;
	int vbr_filesize = 0;
	if (flags & AUDIO_XING_HAS_NUMFRAMES)
	{
		if (offs + 4 > len)
			return -1;
		total_num_frames = GetBE(buf, offs, 4);
		offs += 4;
	}
	if (flags & AUDIO_XING_HAS_FILESIZE)
	{
		if (offs + 4 > len)
			return -1;
		vbr_filesize = GetBE(buf, offs, 4);
		offs += 4;
	}
	SetVbrLength(total_num_frames, vbr_filesize);
	if (flags & AUDIO_XING_HAS_TOC)
	{
		int table_num = MIN(100, len - offs);
		if (table_num < 1)
			return -1;
		toc_length = length;
		seek_toc.SetN(table_num);
		for (int i = 0; i < 100; i++, offs++)
			seek_toc[i] = (DWORD)(filesize * GetBE(buf, offs, 1) / 256);
	}
	return 0;
}

int Audio::ParseVBRIHeader(BYTE *buf, int len)
{
	if (audio_samplerate <= 0)
		return -1;
	if (len < 26)
		return -1;
	int total_num_frames = GetBE(buf, 14, 4);
	int vbr_filesize = GetBE(buf, 10, 4);
	SetVbrLength(total_num_frames, vbr_filesize);
	
	int table_num = GetBE(buf, 18, 2) + 1;
	int scale = GetBE(buf, 20, 2);
	int numb = GetBE(buf, 22, 2);
	int num_toc_frames = GetBE(buf, 24, 2);

	table_num = MIN(table_num, (len - 26) / numb);
	if (table_num < 1)
		return 0;
	
	toc_length = (int)(INT64(1000) * table_num * num_toc_frames * samples_per_frame / audio_samplerate);
	seek_toc.SetN(table_num);
	DWORD last_pos = 0;
	for (int i = 0; i < table_num; i++)
		seek_toc[i] = last_pos = last_pos + GetBE(buf, 26 + i * numb, numb) * scale;
	return 0;
}

int Audio::ParseAC3Header(BYTE *buf, int len, bool fill_info)
{
	int sample_rate, channels, lfe;

	// fast&dirty way to get frame size
	if (!fill_info)
		return mpeg_parse_ac3_header(buf, len, NULL, NULL, NULL, NULL, &ac3_framesize);
		
	if (mpeg_parse_ac3_header(buf, len, &bitrate, &sample_rate, &channels, &lfe, &ac3_framesize) < 0)
		return -1;

	if (bitrate > 0)
	{
		out_bps = bitrate / 8;

		if (filesize > 0 && length == 0)
		{
			int new_length = (int)(filesize * 8000 / bitrate);
			if (new_length != length)
			{
				length = new_length;
				script_totaltime_callback(length / 1000);
			}
		}
	}
	
	if (fmtex == NULL)
	{
		fmtex = (RIFF_WAVEFORMATEX *)SPmalloc(sizeof(RIFF_WAVEFORMATEX));
		memset(fmtex, 0, sizeof(RIFF_WAVEFORMATEX));
	}
	if (fmtex != NULL)
	{
		WORD nch = (WORD)((channels << 8) | lfe);
		if (fmtex->n_samples_per_sec != (DWORD)sample_rate || fmtex->n_channels != nch)
		{
			fmtex->n_samples_per_sec = sample_rate;
			fmtex->n_channels = nch;
			audio_update_format_string();
		}
	}

	if (!audio->ac3_gather_always)
		audio->ac3_gatherinfo = false;

    return 0;
}

int Audio::ParseMp3Packet(MpegPacket *packet, BYTE * &buf, int len)
{
	mp3_stream.error = MAD_ERROR_NONE;
	
	if (!wait && mp3_want_more)
	{
		int bl = len;
		if (mp3_tmpread > 0)
		{
			mp3_tmpstart += mp3_tmpread;
			if (mp3_tmpstart + mp3_mintmpbuf > mp3_numtmpbuf ||
				mp3_tmppos + bl > mp3_numtmpbuf)
			{
				mp3_tmppos -= mp3_tmpstart;
				memcpy(mp3_tmpbuf, mp3_tmpbuf + mp3_tmpstart, mp3_tmppos);
				cur_tmpoffset += mp3_tmpstart;
				mp3_tmpstart = 0;
			}
		}
		if ((bl > 0 && bl < mp3_numtmpbuf) || mp3_tmpread > 0)
		{
			if (mp3_tmppos + bl > mp3_numtmpbuf)
				bl = mp3_numtmpbuf - mp3_tmppos;
			if (bl <= 0)	
				bl = 0;
			else
			{
				// TODO: optimize - remove this memcpy!
				memcpy(mp3_tmpbuf + mp3_tmppos, buf, bl);
			}
			mp3_tmppos += bl;
			mad_stream_buffer(&mp3_stream, mp3_tmpbuf + mp3_tmpstart, mp3_tmppos - mp3_tmpstart);

			mp3_want_more = false;
		}
		mp3_tmpread = 0;
	}
	do
	{
		mp3_stream.error = MAD_ERROR_NONE;
		if (wait || mad_frame_decode(&mp3_frame, &mp3_stream) == 0)
		{
			if (!wait)
			{
				mp3_tmpstart = mp3_stream.this_frame - mp3_tmpbuf;
				mp3_tmpread = mp3_stream.next_frame - mp3_stream.this_frame;
				mp3_want_more = false;
			}

			if (mpeg_find_free_blocks(MPEG_BUFFER_2) == 0)
			{
				// we'll wait...
				wait = true;
				return 0;
			}

			wait = false;

			BYTE *data = mpeg_getcurbuf(MPEG_BUFFER_2);
			mad_synth_frame(&mp3_synth, &mp3_frame);

			cur_offset = cur_tmpoffset + mp3_tmpstart;

			if (audio->avg_num == 0)
			{
				seek_offset = cur_offset;
				if (filesize > 0)
					filesize -= seek_offset;
				int hdrlen = mp3_stream.bufend - mp3_stream.ptr.byte;
				if (hdrlen > 4)
				{
					if (mp3_frame.header.layer == 0)
						audio->samples_per_frame = 384;
					else if (mp3_frame.header.layer == 2 && mp3_frame.header.flags & MAD_FLAG_LSF_EXT)
						audio->samples_per_frame = 576;
					else
						audio->samples_per_frame = 1152;
					audio_samplerate = mp3_synth.pcm.samplerate;

					if (strncasecmp((char *)mp3_stream.ptr.byte, "Xing", 4) == 0 ||
						strncasecmp((char *)mp3_stream.ptr.byte, "Info", 4) == 0)
					{
						audio->ParseXingHeader((BYTE *)mp3_stream.ptr.byte, hdrlen);
					}
					else if (strncasecmp((char *)mp3_stream.ptr.byte, "VBRI", 4) == 0)
					{
						audio->ParseVBRIHeader((BYTE *)mp3_stream.ptr.byte, hdrlen);
					}
				}
				audio_update_format_string();
			}

			audio->avg_bitrate = audio->avg_bitrate * audio->avg_num + mp3_frame.header.bitrate;
			audio->avg_bitrate /= ++audio->avg_num;

			if (length == 0 && audio->avg_bitrate > 0 && audio->avg_num >= min_avg_num)
			{
				if (filesize > 0)
				{
					length = (int)(filesize * 8000 / audio->avg_bitrate);
					script_totaltime_callback(length / 1000);
				}
				
				if ((audio->avg_num - min_avg_num) % 1000 == 0)
					audio_update_format_string();
			}

			memset((BYTE *)packet + 4, 0, sizeof(MpegPacket) - 4);

			packet->pts = 0;
			packet->nframeheaders = 0xffff;
			packet->firstaccessunitpointer = 0;
			packet->type = 1;

			packet->pData = data;

#ifdef AUDIO_USE_FAST_OUTPUT_PTR
			packet->size = mp3_output(data, &mp3_synth.pcm);
#else
#ifdef AUDIO_USE_FAST_OUTPUT
			if (fast)
				packet->size = fast_mp3_output(data, &mp3_synth.pcm);
			else
#endif
				packet->size = normal_mp3_output(data, &mp3_synth.pcm);
#endif

#if 0
{
FILE *fp;
//fp = fopen("out.raw", "ab");
//fwrite(packet->pData, packet->size, 1, fp);
fp = fopen("out.mp3", "ab");
fwrite(mp3_stream.this_frame, mp3_tmpread, 1, fp);
fclose(fp);
}
#endif

			if (packet->size > 0)
			{
				if ((int)mp3_synth.pcm.samplerate != audio_samplerate || 
					mp3_synth.pcm.channels != audio_channels)
				{
					MpegAudioPacketInfo defaudioparams;
					defaudioparams.type = eAudioFormat_PCM;
					defaudioparams.samplerate = mp3_synth.pcm.samplerate;
					defaudioparams.numberofbitspersample = 16;
					defaudioparams.numberofchannels = mp3_synth.pcm.channels;
					defaudioparams.fromstream = TRUE;
					mpeg_setaudioparams(&defaudioparams);

					out_bps = defaudioparams.samplerate * defaudioparams.numberofbitspersample 
						* defaudioparams.numberofchannels / 8;
					
					audio_samplerate = mp3_synth.pcm.samplerate;
					audio_channels = mp3_synth.pcm.channels;
				}

				mpeg_setbufidx(MPEG_BUFFER_2, packet);
				if (len == 0)
					return 1;
				return len;
			}
		} 
		else if (mp3_stream.error == MAD_ERROR_BUFLEN)
			mp3_want_more = true;
	} while (MAD_RECOVERABLE(mp3_stream.error));
	
	return 0;
}

int Audio::ParsePCMPacket(MpegPacket *packet, BYTE * &buf, int len)
{
	if (len <= 0)
		return 0;
	BYTE *out = buf;
	int outlen = len, inlen = len;

	if (needs_resample)
	{
		if (mpeg_find_free_blocks(MPEG_BUFFER_2) == 0)
		{
			wait = true;
			return 0;
		}

		wait = false;
		out = mpeg_getcurbuf(MPEG_BUFFER_2);
		outlen = resample_packet_size;
		int numread = mpeg_PCM_resample_to_LPCM(buf, inlen, out, &outlen);
		if (numread <= 0)
			return 0;
		else
		{
			len = numread;
			buf += numread;
		}
	} else
	{
		wait = false;
		inlen = 0;
		mpeg_PCM_to_LPCM(out, outlen);
	}

	memset((BYTE *)packet + 4, 0, sizeof(MpegPacket) - 4);
	packet->type = 1;
	packet->pData = out;
	packet->size = outlen;
	packet->nframeheaders = 0xffff;
	packet->firstaccessunitpointer = 0;

	// increase bufidx
	mpeg_setbufidx(needs_resample ? MPEG_BUFFER_2 : MPEG_BUFFER_1, packet);

	return len;
}

int Audio::ParseAC3Packet(MpegPacket *packet, BYTE * &buf, int len)
{
	if (len <= 0)
	{
		return 0;
	}

	if (ac3_scan_header > 0 && len > 7 - ac3_scan_header)
	{
		memcpy(ac3_header + ac3_scan_header, buf, 7 - ac3_scan_header);
		if (ParseAC3Header(ac3_header, 7, ac3_gatherinfo) < 0)
		{
			// frankly to say, the rest of saved header may contain the next sync.word,
			// but we skip it to keep the code simple.
			ac3_scan_header = -1;
		} else
		{
			ac3_framesize -= ac3_scan_header;
			ac3_scan_header = 0;
		}
	}
	if (ac3_scan_header != 0)
	{
		BYTE *b = buf;
		int l = len;
		for (; l > 0; l--, b++)
		{
			if (*b == 0x0b)
			{
				if (l < 7)
				{
					ac3_scan_header = l;
					memcpy(ac3_header, b, ac3_scan_header);
					break;
				}
				if (ParseAC3Header(b, l, ac3_gatherinfo) == 0)
				{
					ac3_scan_header = 0;
					ac3_framesize = (b - buf);
					break;
				}
			}
		}
	}

	is_partial_packet = is_partial_next_packet;

	is_partial_next_packet = false;
	if (ac3_scan_header == 0)
	{
		if (ac3_framesize >= 0)
		{
			int left = len;			
			BYTE *b = buf;

			if (ac3_framesize < left)
			{
				// limit packet to current framesize, if it is set
				len = ac3_framesize;

				left -= ac3_framesize;
				b += ac3_framesize;
				ac3_framesize = 0;
				// find *next* ac3 header
				if (ParseAC3Header(b, left, ac3_gatherinfo) < 0)
				{
					if (left < 7)
					{
						// the header was just too short
						ac3_scan_header = left;
						memcpy(ac3_header, b, ac3_scan_header);
						// send incomplete next header to the driver
						len += left;
					} else
					{
						// did not found, we'll try raw byte search...
						msg("AC3: Warning! Frame sync lost!\n");
						ac3_scan_header = -1;
						return 0;
					}
				}
				if (len == 0)
				{
					len = Min(ac3_framesize, left);
					if (ac3_framesize >= left)
						ac3_framesize -= left;
				}
				audio_frame_number++;
			}
			else
			{
				ac3_framesize -= left;
				is_partial_next_packet = true;
			}
		}
	}
	
	memset((BYTE *)packet + 4, 0, sizeof(MpegPacket) - 4);
	packet->type = 1;
	packet->pData = buf;
	packet->size = len;

#if 0
{
FILE *fp;
fp = fopen("out.ac3", "ab");
fwrite(packet->pData, packet->size, 1, fp);
fclose(fp);
}
#endif
#if 0
{ BYTE *pd = packet->pData;
static int num_audio_p = 0;
msg("- [%d, %d] ---------------------------------------------", num_audio_p++, packet->size);
	for (int kk = 0; kk < packet->size; kk++)
	{
		if ((kk % 16) == 0) msg("\n");
	    msg("%02x ", *pd++);
	  
	}
msg("\n---\n");
fflush(stdout);
}
#endif

	packet->nframeheaders = 0xffff;
	packet->firstaccessunitpointer = 0;

#if 0
	if (packet->pts != 0)
	msg("[%d,%d]\t\tsize=%d\t\tpts=%d\n", num_packets, packet->type, packet->size, (int)packet->pts);
#endif

	// increase bufidx
	mpeg_setbufidx(MPEG_BUFFER_1, packet);

	return len;
}
			
int Audio::ParseRawPacket(MpegPacket *packet, BYTE * &buf, int len)
{
	if (len == 0)
		return 0;
	
	memset((BYTE *)packet + 4, 0, sizeof(MpegPacket) - 4);
	packet->type = 1;
	packet->pData = buf;
	packet->size = len;

	packet->nframeheaders = 0xffff;
	packet->firstaccessunitpointer = 0;

	// increase bufidx
	mpeg_setbufidx(MPEG_BUFFER_1, packet);
	return len;
}

int Audio::Seek(LONGLONG pos, int msecs)
{
	if (pos < 0)
		return -1;
	khwl_stop();

	audio_lseek(fd, pos + seek_offset, SEEK_SET);
	cur_tmpoffset = pos;
	
	audio_reset();
	media_skip_buffer(NULL);

	mpeg_setpts(INT64(90) * msecs);
	return audio_continue_play();
}


///////////////////////////////////////////////////

RIFF_AUDIO_FORMAT audio_get_audio_format(int fmt)
{
	switch (fmt)
	{
	case 0x50:
		return RIFF_AUDIO_MP2;
	case 0x55:
		return RIFF_AUDIO_MP3;
	case 0x0001:
		return RIFF_AUDIO_PCM;
	case 0x2000:
		return RIFF_AUDIO_AC3;
	case 0x2001:
		return RIFF_AUDIO_DTS;
	case 0x674f:
	case 0x676f:
	case 0x6750:
	case 0x6770:
	case 0x6751:
	case 0x6771:
		return RIFF_AUDIO_VORBIS;
	}
	return RIFF_AUDIO_UNKNOWN;
}

SPString audio_get_audio_format_string(RIFF_AUDIO_FORMAT fmt)
{
	if (fmt < 7)
	{
		const char *afmt[] = { "Unknown", "None", "LPCM", "PCM", "MPEGL1", "MPEGL2", "MP3", "AC3", "DTS", "Vorbis" };
		return afmt[fmt + 1];
	}
	return SPString();
}

int audio_init(RIFF_AUDIO_FORMAT fmt, bool fast)
{
	if (audio != NULL)
		audio_deinit();

	audio = new Audio();
	audio->audio_fmt = fmt;

	bool need_2nd_buf = false;

	if (audio->audio_fmt == RIFF_AUDIO_MP3 || audio->audio_fmt == RIFF_AUDIO_MP2)
	{
		mad_stream_init(&audio->mp3_stream);
		mad_frame_init(&audio->mp3_frame);
		mad_synth_init(&audio->mp3_synth);

		audio->fast = fast;
		if (fast)
		{
			audio->mp3_stream.options |= MAD_OPTION_HALFSAMPLERATE;
			audio->mp3_output = &fast_mp3_output;
		} else
		{
			audio->mp3_output = &normal_mp3_output;
		}

		// allocate MP3 frame buffer
		audio->mp3_tmpbuf = (BYTE *)SPmalloc(mp3_numtmpbuf);
		if (audio->mp3_tmpbuf == NULL)
		{
			msg_error("Audio: Cannot allocate stream buffer.\n");
			return 0;
		}

		fip_write_special(FIP_SPECIAL_MP3, 1);

		audio->audio_numbufs = 226;
		audio->audio_bufsize = 4608;
		need_2nd_buf = true;
	}
	else if (audio->audio_fmt == RIFF_AUDIO_PCM)
	{
		audio->needs_resample = mpeg_is_resample_needed() == TRUE;

		if (audio->needs_resample)
		{
			audio->audio_numbufs = 250;
			audio->audio_bufsize = audio->resample_packet_size;
			need_2nd_buf = true;
		}
	}
	else if (audio->audio_fmt == RIFF_AUDIO_AC3)
	{
		fip_write_special(FIP_SPECIAL_DOLBY, 1);
	}
	else if (audio->audio_fmt == RIFF_AUDIO_DTS)
	{
		fip_write_special(FIP_SPECIAL_DTS, 1);
	}
	if (need_2nd_buf)
		mpeg_setbuffer(MPEG_BUFFER_2, BUF_BASE, audio->audio_numbufs, audio->audio_bufsize);

	audio->audio_bufidx = 0;

	audio_reset();

	audio_update_format_string();

	return 1;
}

int audio_reset()
{
	if (audio != NULL)
	{
		audio->mp3_want_more = true;
		audio->wait = false;
		audio->mp3_tmppos = 0; 
		audio->mp3_tmpstart = 0; 
		audio->mp3_tmpread = 0;
	}
	return 0;
}

int audio_deinit()
{
	if (audio != NULL)
	{
		if (audio->audio_fmt == RIFF_AUDIO_MP3 || audio->audio_fmt == RIFF_AUDIO_MP2)
		{
			mad_synth_finish(&audio->mp3_synth);
			mad_frame_finish(&audio->mp3_frame);
			mad_stream_finish(&audio->mp3_stream);
		}
	}
	SPSafeDelete(audio);
	return 1;
}

int audio_parse_packet(MpegPacket *packet, BYTE * &buf, int len)
{
	if (audio != NULL)
	{
		switch (audio->audio_fmt)
		{
		case RIFF_AUDIO_MP3:
		case RIFF_AUDIO_MP2:
			return audio->ParseMp3Packet(packet, buf, len);
		case RIFF_AUDIO_PCM:
			return audio->ParsePCMPacket(packet, buf, len);
		case RIFF_AUDIO_AC3:
			return audio->ParseAC3Packet(packet, buf, len);
		default:
			return audio->ParseRawPacket(packet, buf, len);
		}
	}
	return 0;
}

////////////////////////////////////////////////////////

int audio_play(char *filepath, bool is_folder, bool continue_next)
{
	if (is_folder)
	{
		if (filepath != NULL)
			audio_delete_filelist();
		filepath = audio_get_next(filepath, continue_next);
		if (filepath == NULL)
			return -1;
	}

	if (filepath == NULL)
	{
		fip_write_special(FIP_SPECIAL_PLAY, 1);
		fip_write_special(FIP_SPECIAL_PAUSE, 0);
		mpeg_setspeed(MPEG_SPEED_NORMAL);
		return 0;
	}

	if (strncasecmp(filepath, "/cdrom/", 7) != 0 && strncasecmp(filepath, "/hdd/", 5) != 0)
		return -1;

	MPEG_NUM_PACKETS = 512;
	mpeg_init(MPEG_1, TRUE, TRUE, FALSE);

	int ret = media_open(filepath, MEDIA_TYPE_AUDIO);
	if (ret < 0)
	{
		msg_error("Audio: media open FAILED.\n");
		return ret;
	}
	MSG("Audio: start...\n");

	num_packets = 0;
	info_cnt = 0;

	audio->mux_buf = (BYTE *)SPmalloc(audio->mux_numbufs * audio->mux_bufsize);
	if (audio->mux_buf == NULL)
	{
		msg_error("Audio: Cannot allocate input buffer.\n");
		return -1;
	}
	mpeg_setbuffer(MPEG_BUFFER_1, audio->mux_buf, audio->mux_numbufs, audio->mux_bufsize);

	// write dvd-specific FIP stuff...
	const char *digits = "  00000";
	fip_write_string(digits);
	fip_write_special(FIP_SPECIAL_COLON1, 1);
	fip_write_special(FIP_SPECIAL_COLON2, 1);

	fip_write_special(FIP_SPECIAL_PLAY, 1);
	fip_write_special(FIP_SPECIAL_PAUSE, 0);

	script_video_info_callback("");

	audio->playing = true;
	audio->stopping = false;
	mpeg_start();

	if (is_folder)
	{
		player_update_source(filepath);
	}

	return 0;	
}

int audio_continue_play()
{
	fip_write_special(FIP_SPECIAL_PLAY, 1);
	fip_write_special(FIP_SPECIAL_PAUSE, 0);
	mpeg_play();
	return 0;
}

void audio_update_info()
{
	if (audio == NULL)
		return;
	static int old_secs = 0;
	KHWL_TIME_TYPE displ;
	displ.pts = 0;
	displ.timeres = 90000;
	khwl_getproperty(KHWL_TIME_SET, etimSystemTimeClock, sizeof(displ), &displ);

	if ((LONGLONG)displ.pts != audio->saved_pts)
	{
		if ((LONGLONG)displ.pts >= 0)
		{
			audio->saved_pts = displ.pts;

			char fip_out[10];
			int secs = (int)(displ.pts / 90000);
			if (secs < 0)
				secs = 0;
			if (secs >= 10*3600)
				secs = 10*3600-1;
			if (secs != old_secs)
			{
				script_time_callback(secs);
				
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

/// Advance playing
int audio_loop()
{
	if (audio == NULL)
		return 1;

	if (audio->stopping)
	{
		if (mpeg_wait(TRUE) == 1)
		{
			return audio_stop() ? 1 : 0;
		}
	}
	
	static BYTE *buf = NULL;
	static MEDIA_EVENT event = MEDIA_EVENT_OK;
	static int len = 0;

	if (!audio->stopping)
	{
		if (audio_ready())
		{
			buf = NULL;
			event = MEDIA_EVENT_OK;
			len = 0;
			int ret = media_get_next_block(&buf, &event, &len);
			if (ret > 0 && buf == NULL)
			{
				msg_error("Audio: Not initialized. STOP!\n");
				return 1;
			}
			if (ret == -1)
			{
				msg_error("Audio: Error getting next block!\n");
				return 1;
			}
			else if (ret == 0)		// wait...
			{
				return 0;
			}
		
			if (event == MEDIA_EVENT_STOP)
			{
				MSG("Audio: STOP Event triggered!\n");
				mpeg_play_normal();
				audio->stopping = true;
				return 0;
			}
		}

		// send multiple frames (packets) in one chunk
		for (;;)
		{
			MpegPacket *packet = mpeg_feed_getlast();
			if (packet == NULL)		// well, it won't really help
				return 0;

			int numread = audio_parse_packet(packet, buf, len);
			if (numread == 0)
				break;
			len -= numread;
			buf += numread;
			// increase bufidx
			mpeg_feed(MPEG_FEED_AUDIO);
			num_packets++;
		}
	}

	if (info_cnt++ > max_info_cnt)
	{
		info_cnt = 0;
		audio_update_info();
	}

	return 0;
}

BOOL audio_ready()
{
	return audio == NULL || !audio->wait;
}

/// Pause playing
BOOL audio_pause()
{
	fip_write_special(FIP_SPECIAL_PLAY, 0);
	fip_write_special(FIP_SPECIAL_PAUSE, 1);

	mpeg_setspeed(MPEG_SPEED_PAUSE);

	return TRUE;
}

/// Stop playing
BOOL audio_stop(BOOL continue_next)
{
	if (audio != NULL)
	{
		if (audio->playing)
		{
			mpeg_deinit();
			audio->playing = false;
		}
		audio->stopping = false;
		media_close();
		audio_deinit();

		// check for file list
		if (audio_dir_list != NULL)
		{
			if (audio_play(NULL, true, continue_next == TRUE) >= 0)
				return FALSE;
		}
	}
	return TRUE;
}

int audio_get_bitrate()
{
	if (audio == NULL)
		return 0;
	// use known/VBR rate
	if (audio->bitrate > 0)
		return audio->bitrate;
	// use average/CBR rate
	if (audio->avg_bitrate > 0 && audio->avg_num >= min_avg_num)
		return audio->avg_bitrate;
	return 0;
}

int audio_get_output_bps()
{
	if (audio == NULL)
		return 0;
	if (!audio->is_partial_packet)
		return audio->out_bps;
	return 0;
}

/// Seek to given time and play
BOOL audio_seek(int seconds, int from)
{
	if (audio == NULL)
		return FALSE;

	// use TOC to search
	int msecs = seconds * 1000;

	if (from == SEEK_CUR)
	{
		// get current pts
		int cur_msecs = (int)(mpeg_getpts() / 90);
#if 0		
		// get stream pts
		int br = audio_get_bitrate();
		if (br > 0)
			cur_msecs = (int)(audio->cur_offset * 8000 / br);
		else
		{
			script_error_callback(SCRIPT_ERROR_INVALID);
			return FALSE;
		}
#endif
		msecs += cur_msecs;
	}

	if (msecs < 0)
		msecs = 0;

	LONGLONG pos = -1;
	// use TOC
	if (from == SEEK_SET && audio->seek_toc.GetN() > 0 && audio->toc_length > 0 && msecs < audio->toc_length)
	{
		int numtoc = audio->seek_toc.GetN();
		int idx = numtoc * msecs / audio->toc_length;
		LONGLONG p1 = audio->seek_toc[idx];
		LONGLONG p2 = idx < numtoc - 1 ? audio->seek_toc[idx + 1] : audio->filesize;
		int delta_toc = audio->toc_length / numtoc;
		pos = p1 + (p2 - p1) * (msecs - idx * delta_toc) / delta_toc;
		if (audio->bitrate > 0)
			msecs = (int)(pos * 8000 / audio->bitrate);
	}
	// use known/VBR rate
	else 
	{
		int br = audio_get_bitrate();
		if (br > 0)
		{
			pos = (LONGLONG)msecs * br / 8000;
			msecs = (int)(pos * 8000 / br);
		} else
			pos = -1;
	}

	if (pos < 0)
	{
		script_error_callback(SCRIPT_ERROR_INVALID);
		return FALSE;
	}
	return audio->Seek(pos, msecs) == 0;
}

int audio_forward()
{
	audio_seek(10, SEEK_CUR);
	return 0;
}
int audio_rewind()
{
	audio_seek(-10, SEEK_CUR);
	return 0;
}

void audio_setdebug(BOOL ison)
{
	audio_msg = ison == TRUE;
}
BOOL audio_getdebug()
{
	return audio_msg;
}

// internal funcs used by media
BOOL audio_open(const char *filepath)
{
	if (audio != NULL && audio->fd != -1)
		audio_close();

	script_time_callback(0);
	script_audio_info_callback("");
	script_video_info_callback("");

	if (filepath == NULL)
		return FALSE;
	int fd = cdrom_open(filepath, O_RDONLY);
	if (fd < 0)
	{
		msg_error("Audio: Cannot open file %s.\n", filepath);
		return FALSE;
	}

	struct stat64 statbuf;
	LONGLONG filesize = 0, cur_offs = 0;
	if (cdrom_stat(filepath, &statbuf) >= 0)
		filesize = statbuf.st_size;

	// Read first 12 bytes and check that this is an AVI file
	RIFF_AUDIO_FORMAT afmt = RIFF_AUDIO_UNKNOWN;
	BYTE data[20];
	if (read(fd, data, 16) != 16) 
	{
		msg_error("Audio: Cannot read header.\n");
		return FALSE;
	}
	RIFF_WAVEFORMATEX *fmtex = NULL;
	if (strncasecmp((char *)data, "RIFF", 4) == 0 && strncasecmp((char *)data+8, "WAVE", 4) == 0) 
	{
		MSG("Audio: * WAVE format detected!\n");
		// read WAVE header
		if (strncasecmp((char *)data+12, "fmt ", 4) == 0)
		{
			fmtex = audio_read_formatex(fd, NULL, 0);
			if (fmtex != NULL)
				afmt = audio_get_audio_format(fmtex->w_format_tag);
		}
		for (;;)
		{
			read(fd, data, 8);
			if (strncasecmp((char *)data, "data", 4) == 0)
				break;
			else if (strncasecmp((char *)data, "fact", 4) == 0)
			{
				int size = GET_DWORD(data + 4);
				audio_lseek(fd, size, SEEK_CUR);
			}
			else
			{
				audio_lseek(fd, -8, SEEK_CUR);
				break;
			}
		}
		cur_offs = audio_lseek(fd, 0, SEEK_CUR);
		if (filesize > 0)
			filesize -= cur_offs;
	} else
	{
		cur_offs = 0;
		SPString fp = filepath;
		if (fp.FindNoCase(".mp3") >= 0)
		{
			MSG("Audio: * MP3 format detected!\n");
			afmt = RIFF_AUDIO_MP3;

			// skip ID3v2
			if (strncasecmp((char *)data, "ID3", 3) == 0 && data[3] < 0xff && data[4] < 0xff)
			{
				int id3_size = GET_SYNCSAFE_DWORD(data + 6);
				if (data[5] & 0x10)	// footer present
					id3_size += 10;
				cur_offs = id3_size + 10;
			}
		}
		else if (fp.FindNoCase(".mp2") >= 0)
		{
			MSG("Audio: * MPEGL2 format detected!\n");
			afmt = RIFF_AUDIO_MP2;
		}
		else if (fp.FindNoCase(".mp1") >= 0 || fp.FindNoCase(".mpa") >= 0)
		{
			MSG("Audio: * MPEGL1 format detected!\n");
			afmt = RIFF_AUDIO_MP1;
		}
		else if (fp.FindNoCase(".ac3") >= 0)
		{
			MSG("Audio: * AC-3 format detected!\n");
			afmt = RIFF_AUDIO_AC3;
		}
		else if (fp.FindNoCase(".ogg") >= 0)
		{
			MSG("Audio: * OGG Vorbis format detected!\n");
			afmt = RIFF_AUDIO_VORBIS;
		}

		audio_lseek(fd, cur_offs, SEEK_SET);
	}

	if (afmt == RIFF_AUDIO_UNKNOWN)
	{
		msg_error("Audio: Unknown audio format!\n");
		return FALSE;
	}

	MSG("Audio: Setting audio params.\n");
	audio_setaudioparams(afmt, fmtex, 1);

	if (audio_init(afmt, false) == 0)
		return -1;
	
	audio->fd = fd;
	audio->fmtex = fmtex;
	audio->filesize = filesize;
	audio->cur_offset = 0;

	if (afmt == RIFF_AUDIO_LPCM || afmt == RIFF_AUDIO_PCM)
	{
		if (audio->fmtex != NULL)
		{
			audio->bitrate = (audio->fmtex->n_samples_per_sec
							* audio->fmtex->w_bits_per_sample
							* audio->fmtex->n_channels);
			audio_update_format_string();
			if (audio->bitrate > 0)
			{
				audio->length = (int)(filesize * 8000 / audio->bitrate);
				script_totaltime_callback(audio->length / 1000);
			}
		}
	}

	return TRUE;
}

BOOL audio_close()
{
	if (audio != NULL && audio->fd >= 0)
	{
		close(audio->fd);
		audio->fd = -1;
		return TRUE;
	}
	return FALSE;
}

int audio_read(BYTE *buf, int *len)
{
	if (audio == NULL || buf == NULL || len == NULL || *len < 1)
		return 0;
	int n = 0, newlen = 0;

	while (newlen < *len)
	{
		n = read (audio->fd, buf + newlen, *len - newlen);
		if (n == 0)
			break;
		if (n < 0) 
		{
			if (errno == EINTR)
				continue;
			else
				break;
		}
		newlen += n;
	}

	if (newlen < 1) return 0;
	*len = newlen;
	return 1;
}

RIFF_WAVEFORMATEX *audio_read_formatex(int fd, BYTE *buf, int len)
{
	RIFF_WAVEFORMATEX *wfe;
	if (len <= 0)
	{
		if (read(fd, &len, 4) != 4)
			return NULL;
	}
	if (len < 0 || len >= (long)sizeof(RIFF_WAVEFORMATEX))
		len = sizeof(RIFF_WAVEFORMATEX);
	wfe = (RIFF_WAVEFORMATEX *)SPmalloc(sizeof(RIFF_WAVEFORMATEX));
	if (wfe != NULL) 
	{
		memset(wfe, 0, sizeof(RIFF_WAVEFORMATEX));
		if (buf != NULL)
			memcpy(wfe, buf, len);
		else
		{
			if (read(fd, wfe, len) != len)
			{
				SPfree(wfe);
				return NULL;
			}
		}
		if (wfe->cb_size != 0) 
		{
			wfe = (RIFF_WAVEFORMATEX *)SPrealloc(wfe, sizeof(RIFF_WAVEFORMATEX) + wfe->cb_size);
			if (wfe != NULL) 
			{
				if (read(fd, (BYTE *)wfe + sizeof(RIFF_WAVEFORMATEX), wfe->cb_size) != wfe->cb_size)
				{
					SPfree(wfe);
					return NULL;
				}
			}
		}
	}
	return wfe;
}

void audio_update_format_string()
{
	if (audio != NULL && audio->audio_fmt != RIFF_AUDIO_UNKNOWN)
	{
		int br = audio_get_bitrate();
		if (br != audio->cur_bitrate || audio->audio_fmt != audio->cur_audio_format || (audio->fmtex != NULL 
			&& ((int)audio->fmtex->n_samples_per_sec != audio->cur_samples ||
				(int)audio->fmtex->w_bits_per_sample != audio->cur_bits ||
				(int)audio->fmtex->n_channels != audio->cur_channels
			)))
		{
			audio->cur_audio_format = audio->audio_fmt;
			audio->cur_bitrate = br;
			if (audio->fmtex != NULL)
			{
				audio->cur_samples = audio->fmtex->n_samples_per_sec;
				audio->cur_bits = audio->fmtex->w_bits_per_sample;
				audio->cur_channels = audio->fmtex->n_channels;
			}
			
			SPString fmt = audio_get_audio_format_string(audio->audio_fmt);
			audio->audio_fmt_str = fmt;
			if ((audio->audio_fmt == RIFF_AUDIO_PCM || audio->audio_fmt == RIFF_AUDIO_LPCM)
				&& audio->fmtex != NULL)
			{
				audio->audio_fmt_str.Printf(" @ %d Hz, %d bits %s", 
					audio->fmtex->n_samples_per_sec,
					audio->fmtex->w_bits_per_sample,
					audio->fmtex->n_channels > 1 ? "stereo" : "mono");
			} 
			else
			{
				if (br > 0)
					audio->audio_fmt_str.Printf(" @ %d kbps", (br + 500) / 1000);
			}
			if (audio->audio_fmt == RIFF_AUDIO_AC3 && audio->fmtex != NULL)
			{
				audio->audio_fmt_str.Printf(", %d Hz, ", audio->fmtex->n_samples_per_sec);
				if (audio->fmtex->n_channels == 0x0100)
					audio->audio_fmt_str.Printf("mono");
				else if (audio->fmtex->n_channels == 0x0200)
					audio->audio_fmt_str.Printf("stereo");
				else
					audio->audio_fmt_str.Printf("%d.%d", (audio->fmtex->n_channels >> 8) & 0xff, 
														audio->fmtex->n_channels & 0xff);
			}
			MSG("Audio: * Audio format detected: %s.\n", *audio->audio_fmt_str);
			script_audio_info_callback(audio->audio_fmt_str);
		}
	}
}

int audio_setaudioparams(RIFF_AUDIO_FORMAT fmt, RIFF_WAVEFORMATEX *wfe, int halfrate)
{
	MpegAudioPacketInfo defaudioparams;

	// set audio format
	switch (fmt)
	{
	case RIFF_AUDIO_MP2:
	case RIFF_AUDIO_MP3:
		defaudioparams.type = eAudioFormat_PCM;
		if (wfe != NULL)
			wfe->w_bits_per_sample = 16;
		max_info_cnt = 20;
		break;
	case RIFF_AUDIO_AC3:
		defaudioparams.type = eAudioFormat_AC3;
		if (wfe != NULL)
			wfe->w_bits_per_sample = 24;
		max_info_cnt = 10;
		break;
/*	case RIFF_AUDIO_DTS:
		defaudioparams.type = eAudioFormat_DTS;
		break;
*/
	case RIFF_AUDIO_PCM:
	case RIFF_AUDIO_LPCM:
		defaudioparams.type = eAudioFormat_PCM;
		max_info_cnt = 32;
		break;
	default:
		msg_error("AVI: Audio format %d not supported!\n", fmt);
		return -1;
	}

	if (wfe != NULL)
	{
		if (wfe->n_samples_per_sec > 0 && wfe->w_bits_per_sample >= 8 && wfe->n_channels > 0)
		{
			defaudioparams.samplerate = wfe->n_samples_per_sec / halfrate;
			defaudioparams.numberofbitspersample = wfe->w_bits_per_sample;
			defaudioparams.numberofchannels = wfe->n_channels;
			defaudioparams.fromstream = TRUE;
		} else
			return -1;
	} else
	{
		defaudioparams.samplerate = 48000;
		defaudioparams.numberofbitspersample = 24;
		defaudioparams.numberofchannels = 2;
		defaudioparams.fromstream = TRUE;
	}

	if (mpeg_setaudioparams(&defaudioparams) < 0)
	{
		return -1;
	}

	if (audio != NULL)
	{
		if (defaudioparams.type == eAudioFormat_AC3)
		{
			audio->out_bps = 0;	// don't guess bitrate
			audio->ac3_gatherinfo = true;
		}
		else
			audio->out_bps = defaudioparams.samplerate * defaudioparams.numberofbitspersample 
						* defaudioparams.numberofchannels / 8;

		if (audio->fmtex == NULL && wfe != NULL)
		{
			int wfe_size = sizeof(RIFF_WAVEFORMATEX) + wfe->cb_size;
			audio->fmtex = (RIFF_WAVEFORMATEX *)SPmalloc(wfe_size);
			if (audio->fmtex != NULL)
			{
				memcpy(audio->fmtex, wfe, wfe_size);
				audio_update_format_string();
			}
		}
	}

	return 0;
}

void audio_set_ac3_getinfo(bool always)
{
	if (audio != NULL)
		audio->ac3_gather_always = always;
}

////////////////////////////////////////////////////////

char *audio_get_next(char *filepath, bool get_next)
{
	static const char *audio_file_ext[] = { ".mp3", ".mp2", ".mp1", ".mpa", ".ac3", ".wav", ".ogg", NULL };
	static char tmp_path[4096];

	if (audio_dir_list == NULL)
	{
		if (filepath == NULL)
			return NULL;
		audio_dir_list = new SPClassicList<AudioDir>();
		audio_prev_list = new SPDLinkedList<SPString>();
		if (audio_dir_list == NULL || audio_prev_list == NULL)
			return NULL;
		audio_dir_list->Reserve(20);
		audio_prev_list->SetSize(10);
		tmp_path[0] = '\0';
		AudioDir ad;
		ad.dir = NULL;
		ad.path = filepath;
		audio_dir_list->Add(ad);
	}

	if (audio_dir_list->GetN() > 0)
	{
		if (!get_next)
		{
			if (audio_prev_list == NULL)
				return NULL;
			DLElement<SPString> *el = audio_prev_list->GetLast();
			if (el == NULL)
				return NULL;
			strcpy(tmp_path, el->GetItem());
			audio_prev_list->Delete(el);
			return tmp_path;
		}

		for (;;)
		{
			AudioDir &ad = (*audio_dir_list)[audio_dir_list->GetN() - 1];
			if (ad.dir == NULL)
			{
				// scan for files
				ad.dir = cdrom_opendir(ad.path);
			}
			if (ad.dir != NULL)
			{
				struct dirent *d = cdrom_readdir(ad.dir);
				if (d == NULL)
				{
					cdrom_closedir(ad.dir);
					audio_dir_list->Remove(audio_dir_list->GetN() - 1);
					if (audio_dir_list->GetN() < 1)
						break;
					continue;
				}
				if (d->d_name[0] == '.' && d->d_name[1] == '\0')
					continue;

				SPString path = ad.path + SPString("/") + SPString(d->d_name);

				struct stat64 statbuf;
				if (cdrom_stat(path, &statbuf) < 0) 
				{
					msg("Cannot stat %s\n", *path);
					continue;
				}
				if (S_ISDIR(statbuf.st_mode))
				{
					if (d->d_name[0] == '.')	// hidden
						continue;
					AudioDir ad;
					ad.dir = NULL;
					ad.path = path;
					audio_dir_list->Add(ad);
					continue;
				}
				// check for file type
				for (int k = 0; audio_file_ext[k] != NULL; k++)
				{
					if (strstr(d->d_name, audio_file_ext[k]) != NULL)
					{
						if (tmp_path[0] != '\0')
						{
							DLElement<SPString> *el = new DLElement<SPString>(tmp_path);
							if (el == NULL)
								return NULL;
							audio_prev_list->Add(el);
						}

						strcpy(tmp_path, path);
						msg("Audio: Playing %s...\n", tmp_path);
						return tmp_path;
					}
				}
			}
			else
				break;
		}
	}
	audio_delete_filelist();
	return NULL;
}

void audio_delete_filelist()
{
	if (audio_dir_list != NULL)
	{
		for (int i = 0; i < audio_dir_list->GetN(); i++)
		{
			AudioDir &ad = (*audio_dir_list)[i];
			cdrom_closedir(ad.dir);
		}
		SPSafeDelete(audio_dir_list);
	}
	
	if (audio_prev_list != NULL)
	{
		audio_prev_list->Delete();
		SPSafeDelete(audio_prev_list);
	}
	
}


#endif
