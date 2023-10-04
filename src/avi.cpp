//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - AVI player source file.
 *  \file       avi.cpp
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

#ifdef INTERNAL_VIDEO_PLAYER

#define VIDEO_INTERNAL

#include "audio.h"
#include "video.h"
#include "avi.h"

#include "script-vars.h"

#define MSG if (video_msg) msg

VideoAvi::VideoAvi()
{
	type = VIDEO_CONTAINER_AVI;

	video_strn = 0;
	avi_flags = 0;
	bitmap_info_header = NULL;
	video_superindex.idx = NULL;

	memset(video_tag1, 0, 4);
	memset(video_tag2, 0, 4);
	movi_start = 0;
	idx_start = -1;
	idx_offset = -1;

	has_indx = false;
	num_idx = 0;
	
	buf_idx = (AviOldIndexEntry *)SPmalloc(MAX(max_buf_idx_size[0], max_buf_idx_size[1]));
	buf_idx_size = 0;

	cur_old_buf_idx = 0;
	cur_buf_video_idx = 0;

	cur_indx_buf_idx = 0;
	cur_indx_buf_pos = 0;
	cur_indx_base_offset = 0;
	cur_buf_idx_size = 1;
}

VideoAvi::~VideoAvi()
{
	SPSafeFree(bitmap_info_header);
	SPSafeFree(video_superindex.idx);
	SPSafeFree(buf_idx);
}

////////////////////////////////////////////////

BOOL VideoAvi::Parse()
{
	long i, n;
	BYTE *hdrl_data;
	BYTE data[256];
	long header_offset = 0, hdrl_len = 0;
	int j;
	VIDEO_CHUNK_TYPE lasttag = VIDEO_CHUNK_UNKNOWN;
	bool vids_strh_seen = false;
	bool vids_strf_seen = false;
	bool auds_strh_seen = false;
	int num_stream = 0;
	LONGLONG oldpos = -1, newpos = -1;

	// Read first 12 bytes and check that this is an AVI file
	if (video_read(data, 12) != 12) 
	{
		msg_error("AVI: Read error.\n");
		return FALSE;
	}
	if (strncasecmp((char *)data, "RIFF", 4) != 0 || strncasecmp((char *)data+8, "AVI ", 4) != 0) 
	{
		msg_error("AVI: File header not found. Wrong file format.\n");
		return FALSE;
	}

	// Go through the AVI file and extract the header list,
    // the start position of the 'movi' list and an optionally present idx1 tag

	hdrl_data = 0;
	movi_start = 0;
	idx_start = -1;

	// find all main AVI parts
	for (;;)
	{
		// EOF?
		if (video_read(data, 8) != 8) 
			break;
		newpos = video_lseek(0, SEEK_CUR);
		if (oldpos == newpos) 
		{
			msg_error("AVI: AVI file is broken.\n");
			return FALSE;
		}
		oldpos=newpos;

		n = GET_DWORD(data+4);
		n = PAD_EVEN(n);

		if (strncasecmp((char *)data, "LIST", 4) == 0)
		{
			if (video_read(data, 4) != 4) 
			{
				msg_error("AVI: Read error.\n");
				return FALSE;
			}
			n -= 4;
			if (n <= 0)
				break;
			if (strncasecmp((char *)data, "hdrl", 4) == 0)
			{
				hdrl_len = n;
				hdrl_data = (BYTE *) SPmalloc(n);
				if (hdrl_data == NULL) 
					return FALSE;
				
				header_offset = (long)video_lseek(0, SEEK_CUR);
				if (video_read(hdrl_data, n) != n) 
				{
					msg_error("AVI: Read error.\n");
					return FALSE;
				}
			}
			else if (strncasecmp((char *)data, "movi", 4) == 0)
			{
				movi_start = video_lseek(0, SEEK_CUR);
				if (video_lseek(n, SEEK_CUR) == (LONGLONG)-1) 
					break;
			}
			else
			{
				if (video_lseek(n, SEEK_CUR) == (LONGLONG)-1) 
					break;
			}
		}
		else if(strncasecmp((char *)data, "idx1", 4) == 0)
		{
			// n must be a multiple of 16, but the reading does not
			// break if this is not the case
			num_idx = n / sizeof(AviOldIndexEntry);
			idx_start = video_lseek(0, SEEK_CUR);
			
			if (buf_idx == NULL) 
				return FALSE;
			buf_idx_size = MIN(n, max_buf_idx_size[0]);
			// \warning! little-endian ONLY!
			if (video_read((BYTE *)buf_idx, buf_idx_size) != buf_idx_size) 
			{
				buf_idx_size = 0;
				num_idx = 0;
			}
			video_lseek(n - buf_idx_size, SEEK_CUR);
		}
		else
			video_lseek(n, SEEK_CUR);
	}

	if (hdrl_data == NULL) 
	{
		msg_error("AVI: no AVI header found.\n");
		return FALSE;
	}
	if (movi_start == 0) 
	{
		msg_error("AVI: no movie data found.\n");
		return FALSE;
	}

	// parse header
	for (i = 0; i < hdrl_len; )
	{
		if (strncasecmp((char *)hdrl_data+i,"LIST", 4) == 0) 
		{ 
			i+= 12; 
			continue; 
		}

		n = GET_DWORD(hdrl_data+i+4);
		n = PAD_EVEN(n);

		if (strncasecmp((char *)hdrl_data + i, "avih", 4) == 0)
		{
			i += 8;
			avi_flags = GET_DWORD(hdrl_data+i+12);
			if (avi_flags & AVI_FLAG_MUSTUSEINDEX)
			{
				msg_error("Avi: Reordered stream cannot be played correctly!\n");
			}
			if (!(avi_flags & AVI_FLAG_ISINTERLEAVED))
			{
				msg_error("Avi: Non-interleaved stream cannot be played correctly!\n");
			}
			/*
			if (!(avi_flags & AVI_FLAG_TRUSTCKTYPE))
			{
				msg("Avi: We cannot trust key-frames? Maybe...\n");
			}
			*/
		}
		else if (strncasecmp((char *)hdrl_data + i, "strh", 4) == 0)
		{
			i += 8;
			if (strncasecmp((char *)hdrl_data + i, "vids", 4) == 0 && !vids_strh_seen)
			{
				scale = GET_DWORD(hdrl_data+i+20);
				rate  = GET_DWORD(hdrl_data+i+24);
				if (scale != 0) 
					fps = (float)rate / (float)scale;
				video_frames = GET_DWORD(hdrl_data+i+32);
				video_strn = num_stream;
				vids_strh_seen = true;
				lasttag = VIDEO_CHUNK_VIDEO;
			}
			else if (strncasecmp ((char *)hdrl_data+i,"auds",4) == 0 && ! auds_strh_seen)
			{
				//inc audio tracks
				cur_track = num_tracks;
				++num_tracks;
	
				if (num_tracks > VIDEO_MAX_AUDIO_TRACKS) 
				{
					msg_error("Avi: Only %d audio tracks supported!\n", VIDEO_MAX_AUDIO_TRACKS);
					return FALSE;
				}
	
				track[cur_track].flags = GET_DWORD(hdrl_data+i+8);
				track[cur_track].padrate = GET_DWORD(hdrl_data+i+24);
				track[cur_track].audio_samples = GET_DWORD(hdrl_data+i+32);
				track[cur_track].a_vbr = GET_DWORD(hdrl_data+i+44) == 0; // samplesize -- 0?
				track[cur_track].audio_strn = num_stream;
				track[cur_track].a_codech_off = header_offset + i;
				lasttag = VIDEO_CHUNK_AUDIO;
			}
			else if (strncasecmp ((char *)hdrl_data+i, "iavs", 4) == 0 && ! auds_strh_seen) 
			{
				msg_error("AVI: DV AVI Type 1 not supported.\n");
				return FALSE;
			}
			else
				lasttag = VIDEO_CHUNK_UNKNOWN;
			num_stream++;
		}
		else if (strncasecmp((char *)hdrl_data+i, "dmlh", 4) == 0) 
		{
			total_frames = GET_DWORD(hdrl_data+i+8);
			i += 8;
		}
		else if (strncasecmp((char *)hdrl_data+i,"strf", 4) == 0)
		{
			i += 8;
			if (lasttag == VIDEO_CHUNK_VIDEO)
			{
				RIFF_BITMAPINFOHEADER bih;
				memcpy(&bih, hdrl_data + i, sizeof(RIFF_BITMAPINFOHEADER));
				bih.bi_size = GET_DWORD((BYTE *)&bih.bi_size);
				bitmap_info_header = (RIFF_BITMAPINFOHEADER *)SPmalloc(bih.bi_size);
				if (bitmap_info_header != NULL)
				{
					memcpy(bitmap_info_header, hdrl_data + i, bih.bi_size);
				}

				width  = GET_DWORD(hdrl_data+i+4);
				height = GET_DWORD(hdrl_data+i+8);
				memcpy(fourcc, hdrl_data+i+16, 4); fourcc[4] = 0;
				
				vids_strf_seen = true;
			}
			else if (lasttag == VIDEO_CHUNK_AUDIO)
			{
				LONGLONG lpos = video_lseek(0, SEEK_CUR);
				video_lseek(header_offset + i + sizeof(RIFF_WAVEFORMATEX), SEEK_SET);
				
				RIFF_WAVEFORMATEX *wfe = audio_read_formatex(fd, hdrl_data + i, hdrl_len - i);
				
				video_lseek(lpos, SEEK_SET);

				if (wfe != NULL)
				{
					track[cur_track].a_codecf_off = header_offset + i;
					track[cur_track].mp3rate = 8 * wfe->n_avg_bytes_per_sec / 1000;
					int sampsize = MAX(((wfe->w_bits_per_sample + 7) / 8) 
										* wfe->n_channels, 4);
					track[cur_track].audio_bytes = 
									track[cur_track].audio_samples * sampsize;
				}

				track[cur_track].wfe = wfe;
			}
		}
		else if(strncasecmp((char *)hdrl_data+i, "indx", 4) == 0) 
		{
			MSG("AVI: New v2.0 index detected!\n");
			if (!(avi_flags & AVI_FLAG_TRUSTCKTYPE))
			{
				msg_error("AVI: Index may contain invalid keyframes!\n");
			}

			if (lasttag == VIDEO_CHUNK_VIDEO)
			{
				BYTE *a = hdrl_data + i;
				memcpy (video_superindex.fcc, a, 4);             a += 4;
				video_superindex.dwSize = GET_DWORD(a);          a += 4;
				video_superindex.wLongsPerEntry = GET_WORD(a); a += 2;
				video_superindex.bIndexSubType = *a;             a += 1;
				video_superindex.bIndexType = *a;                a += 1;
				video_superindex.nEntriesInUse = GET_DWORD(a);   a += 4;
				memcpy (video_superindex.dwChunkId, a, 4);       a += 4;

				// 3 * reserved
				a += 4; a += 4; a += 4;

				if (video_superindex.bIndexSubType != 0) 
				{
					msg("AVI: Invalid ODML superindex header.\n"); 
				}
	
				int superidx_size = video_superindex.wLongsPerEntry 
								* video_superindex.nEntriesInUse * sizeof (DWORD);
				video_superindex.idx = superidx_size < 256*1024 ? 
								(AviSuperIndexEntry *)SPmalloc (superidx_size) : NULL;
				if (video_superindex.idx != NULL)
				{
					// position of ix## chunks
					for (DWORD j = 0; j < video_superindex.nEntriesInUse; ++j) 
					{
						video_superindex.idx[j].qwOffset = GET_ULONGLONG (a);  a += 8;
						video_superindex.idx[j].dwSize = GET_DWORD (a);     a += 4;
						video_superindex.idx[j].dwDuration = GET_DWORD (a); a += 4;
					}
					has_indx = true;
				}
			}
			i += 8;
		}
		else if ((strncasecmp((char *)hdrl_data+i,"JUNK", 4) == 0) ||
              (strncasecmp((char *)hdrl_data+i,"strn", 4) == 0) || 
			  (strncasecmp((char *)hdrl_data+i,"strd", 4) == 0) ||
              (strncasecmp((char *)hdrl_data+i,"vprp", 4) == 0))
		{
			// do not reset lasttag
			i += 8;
		} else
		{
			i += 8;
			lasttag = VIDEO_CHUNK_UNKNOWN;
		}
		i += n;
	}

	SPfree(hdrl_data);

	if (!vids_strh_seen || !vids_strf_seen) 
	{
		msg_error("AVI: No video found.\n");
		return FALSE;
	}

	video_tag1[0] = (char)(video_strn/10 + '0');
	video_tag1[1] = (char)(video_strn%10 + '0');
	video_tag1[2] = 'd';
	video_tag1[3] = bitmap_info_header != NULL 
		&& bitmap_info_header->bi_compression == 0 ? 'b' : 'c';
	
	video_tag2[0] = (char)(video_strn/10 + '0');
	video_tag2[1] = (char)(video_strn%10 + '0');
	video_tag2[2] = 'd';
	video_tag2[3] = video_tag1[3] == 'b' ? 'c' : 'b';

	// Audio tag is set to "99wb" if no audio present
	if (track[0].wfe == NULL || track[0].wfe->n_channels < 1) 
		track[0].audio_strn = 99;

	first_track = -1;
	int good_first_track = -1;
	for(i = 0, j = 0; j < num_tracks + 1; ++j) 
	{
		if (j == video_strn) 
			continue;
		if (first_track < 0)
			first_track = i;
		// if not disabled
		if (good_first_track < 0 && (track[i].flags & 1) == 0)
			good_first_track = i;
		track[i].audio_tag[0] = (char)(j/10 + '0');
		track[i].audio_tag[1] = (char)(j%10 + '0');
		track[i].audio_tag[2] = 'w';
		track[i].audio_tag[3] = 'b';
		++i;
	}
	
	// set first non-disabled track. 
	// we can change them later with video_set_audio_track().
	if (good_first_track >= 0)
		first_track = good_first_track;

	cur_track = first_track;

	video_lseek(movi_start, SEEK_SET);

	// if the file has an idx1, check if this is relative
	// to the start of the file or to the start of the 'movi' list
	idx_offset = -1;
	if (idx_start > 0 && !has_indx)
	{
		int num_buf_idx = buf_idx_size / sizeof(AviOldIndexEntry);
		bool found_av = false;
		for (i = 0; i < num_buf_idx; i++)
		{
			if (strncasecmp((char *)&buf_idx[i].chunkid, (char *)video_tag1, 3) == 0) 
			{
				found_av = true;
				break;
			}
		}
		// if no video chunks present in current buffer, search for audio...
		if (!found_av)
		{
			for (i = 0; !found_av && i < num_buf_idx; i++)
			{
				for (j = 0; j < num_tracks; ++j) 
				{
					if (strncasecmp((char *)&buf_idx[i].chunkid, track[j].audio_tag, 4) == 0) 
					{
						found_av = true;
						break;
					}
				}
			}
		}
		
		if (found_av)
		{
			LONGLONG pos = buf_idx[i].offset;
			LONGLONG len = buf_idx[i].size;
			if (pos >= 0 && len > 0)
			{
				video_lseek(pos, SEEK_SET);
				if (video_read(data, 8) == 8)
				{
					// index from start of file?
					if (strncasecmp((char *)data, (char *)&buf_idx[i].chunkid, 4) == 0 
						&& GET_DWORD(data + 4) == len)
					{
						idx_offset = 0;
					}
				}
				if (idx_offset < 0)
				{
					video_lseek(pos + movi_start - 4,SEEK_SET);
					if (video_read(data, 8) == 8) 
					{
						// index from start of 'movi' list?
						if (strncasecmp((char *)data, (char *)&buf_idx[i].chunkid, 4) == 0 
							&& GET_DWORD((unsigned char *)data+4) == len)
						{
							idx_offset = movi_start - 4;
						}
					}
				}
			}
		}
		// broken index...
		if (idx_offset >= 0)
		{
			AddIdx1Block();
		}
	}

	// Reposition the file
	video_lseek(movi_start, SEEK_SET);
	abs_video_pos = -1;
	video_pos_base = 0;
	video_pos = 0;
	audio_pos = 0;
	cur_key_offs = last_chunk_offs = cur_offs = video_offs = movi_start;
	frame_pos = 0;
	last_frame_pos = -1;

	return TRUE;
}

//////////////////////////////////////////////////////////////////

VIDEO_CHUNK_TYPE VideoAvi::GetNext(BYTE *buf, int buflen, int *pos, int *left, int *len)
{
	// there was a partial chunk read
	int tmp_read;
	if (chunkleft > 0)
	{
		*len = MIN(chunkleft, buflen);

		chunkleft -= buflen;
		if (chunkleft < 0)
			chunkleft = 0;
		
		if (chunktype != VIDEO_CHUNK_UNKNOWN)
		{
			if ((tmp_read = video_read(buf, PAD_EVEN(*len))) == 0)
			{
				return VIDEO_CHUNK_EOF;
			}
		} else
			video_lseek(PAD_EVEN(*len), SEEK_CUR);

		if (chunkleft == 0)
		{
			if (chunktype == VIDEO_CHUNK_AUDIO_PARTIAL)
				return VIDEO_CHUNK_AUDIO;
			else if (chunktype == VIDEO_CHUNK_VIDEO_PARTIAL)
				return VIDEO_CHUNK_VIDEO;
		}
		if (chunktype != VIDEO_CHUNK_UNKNOWN)
			return chunktype;
		
		//buf += PAD_EVEN(*len);
		//*pos += PAD_EVEN(*len);
		//*left -= PAD_EVEN(*len);
	}

	*len = 0;
	chunktype = VIDEO_CHUNK_UNKNOWN;
	//chunkleft = 0;

	int curpos = *pos;
	LONGLONG start_offs;

	// in recovery mode, use buffered read 
	if (look4chunk)
	{
		buflen = video_read(buf, buflen - curpos);
		if (buflen <= 0)
			return VIDEO_CHUNK_EOF;
		start_offs = cur_offs;
		buflen += curpos;
	}


	// we're at the start of the chunk and got chunk header
	while (curpos < buflen)
	{
		// if we got a list tag, ignore it
		BYTE hdr_data[8];
		char *hdr = (char *)hdr_data;

		if (!look4chunk)
		{
			if ((tmp_read = video_read(hdr_data, 8)) == 0)
			{
				return VIDEO_CHUNK_EOF;
			}
		} else
		{
			curpos = (int)(cur_offs - start_offs);
			hdr = (char *)(buf + curpos);
		}

		cur_offs += 8;

		int n, reallen;
		if (strncasecmp(hdr, "RIFF", 4) == 0 || strncasecmp(hdr, "LIST", 4) == 0)
		{
			ResetRecoveryMode();
			video_lseek(4, SEEK_CUR);
			
			cur_offs += 4;
			continue;
		}
		if (*left < 8)
		{
			chunktype = VIDEO_CHUNK_FRAGMENT;
			break;
		}

		reallen = n = GET_DWORD((BYTE *)hdr + 4);

		//buf += 8;
		//*pos += 8;
		//*left -= 8;
		bool partial = false;

		frame_pos++;

		if (strncasecmp(hdr, video_tag1, 3) == 0)
		{
//msg("00dc\n");
//printf("00dc: [%d] n=%d %d/%d\n", frame_pos, n, *pos, buflen);
			abs_video_pos++;
			video_pos++;
			if (n == 0)
				continue;

			ResetRecoveryMode();

			int read_n;
			if (*pos + n > buflen)
			{
				// XXXXXXXXXXXXXXX
				if (no_partial)
				{
					frame_pos--;
					abs_video_pos--;
					video_pos--;
					video_lseek(-8, SEEK_CUR);
					return VIDEO_CHUNK_UNKNOWN;
				}

				chunkleft = n - buflen + *pos;
				read_n = n = buflen - *pos;
				partial = true;
				
			} else
				read_n = PAD_EVEN(n);

			if ((tmp_read = video_read(buf, read_n)) == 0)
			{
				return VIDEO_CHUNK_EOF;
			}

			*len = n;
			video_packet_len = reallen;
			chunktype = partial ? VIDEO_CHUNK_VIDEO_PARTIAL : VIDEO_CHUNK_VIDEO;
			last_chunk_offs = video_offs = cur_offs - 8;
			cur_offs += read_n;
			last_chunk_len = reallen;
			break;
		}
		else if (cur_track >= 0 && strncasecmp(hdr, track[cur_track].audio_tag, 4) == 0)
		{
//msg("01wb\n");
			audio_pos++;
			if (n == 0)
				continue;

			ResetRecoveryMode();

			int read_n;
			if (*pos + n > buflen)
			{
				// XXXXXXXXXXXXXXX
				if (no_partial)
				{
					frame_pos--;
					audio_pos--;
					video_lseek(-8, SEEK_CUR);
					return VIDEO_CHUNK_UNKNOWN;
				}

				chunkleft = n - buflen + *pos;
				read_n = n = buflen - *pos;
				partial = true;
			} else
				read_n = PAD_EVEN(n);


			if ((tmp_read = video_read(buf, read_n)) == 0)
			{
				return VIDEO_CHUNK_EOF;
			}
#if 0
{
FILE *fp = fopen("out-avi.mp3", "ab");
fwrite(buf, n, 1, fp);
fclose(fp);
}
#endif
			*len = n;
			chunktype = partial ? VIDEO_CHUNK_AUDIO_PARTIAL : VIDEO_CHUNK_AUDIO;
			last_chunk_offs = cur_offs - 8;
			cur_offs += read_n;
			last_chunk_len = reallen;
			track[cur_track].audio_pos++;
			break;
		}
		else
		{
			if (hdr[0] == 'i' && hdr[1] == 'd' && hdr[2] == 'x' && hdr[3] == '1')
			{
				ResetRecoveryMode();

				//chunktype = VIDEO_CHUNK_EOF;
				chunktype = VIDEO_CHUNK_UNKNOWN;
				//buf += PAD_EVEN(n);
				//*pos += PAD_EVEN(n);
				//*left -= PAD_EVEN(n);

				cur_offs += PAD_EVEN(n);
				video_lseek(cur_offs, SEEK_SET);

				//return chunktype;
				continue;
			}
			else if ((hdr[2] == 'w' && hdr[3] == 'b') || 
				(hdr[0] == 'i' && hdr[1] == 'x') ||
				(hdr[0] == 'J' && hdr[1] == 'U' && hdr[2] == 'N' && hdr[3] == 'K'))
			{
				ResetRecoveryMode();

				//buf += PAD_EVEN(n);
				//*pos += PAD_EVEN(n);
				//*left -= PAD_EVEN(n);
				chunktype = VIDEO_CHUNK_UNKNOWN;

				video_lseek(PAD_EVEN(n), SEEK_CUR);

				cur_offs += PAD_EVEN(n);
			}
			// file is corrupted ?
			else
			{
				cur_offs -= 7;

				if (!look4chunk)
				{
					LONGLONG file_offs = video_lseek(0, SEEK_CUR) - 8;
					msg("AVI: File corrupted @ " PRINTF_64d ". Trying to recover...\n", file_offs);
					
					video_lseek(-7, SEEK_CUR);
					
					chunkleft = 0;
					look4chunk = true;

					script_error_callback(SCRIPT_ERROR_CORRUPTED);
					
					return VIDEO_CHUNK_RECOVERY;
				}
			}
		}
	}
	return chunktype;
}

int VideoAvi::GetNextIndexes()
{
	if ((idx_offset >= 0 && idx_start > 0) || has_indx)
	{
		media_update_fip();

		LONGLONG buf_idx_offs;
		if (has_indx)
		{
			cur_buf_idx_size = video_superindex.idx[cur_indx_buf_idx].dwSize;
			if (cur_indx_buf_pos >= cur_buf_idx_size)
			{
				cur_indx_buf_idx++;
				cur_indx_buf_pos = 0;
			}
			if ((DWORD)cur_indx_buf_idx >= video_superindex.nEntriesInUse)
				return -1;
			
			cur_buf_idx_size -= cur_indx_buf_pos;
			buf_idx_offs = video_superindex.idx[cur_indx_buf_idx].qwOffset + cur_indx_buf_pos;
		} else
		{
			cur_buf_idx_size = (num_idx - cur_old_buf_idx) * sizeof(AviOldIndexEntry);
			buf_idx_offs = idx_start + cur_old_buf_idx * sizeof(AviOldIndexEntry);
		}

		buf_idx_size = MIN(max_buf_idx_size[has_indx ? 1 : 0], cur_buf_idx_size);

		if (buf_idx_size < 1)
			return -1;
		if (video_lseek(buf_idx_offs, SEEK_SET) < 0)
			return -1;
		buf_idx_size = video_read((BYTE *)buf_idx, buf_idx_size);
		//msg("^^^^^^^ read = %d\n", buf_idx_size);
		return has_indx ? AddIdx2Block() : AddIdx1Block();
	}
	return -1;
}

int VideoAvi::GetNextKeyFrame()
{
	int n = 0;
	LONGLONG curpos = 0;
	const int extra_bytes = 5;
	BYTE buf[8 + extra_bytes];

	//*len = 0;
	if (buf_idx == NULL)
		return -1;

	// we're at the start of the chunk and got chunk header
	for (int i = 0; ; i++)
	{
		// split our task...
		if (i > 30)
		{
			media_update_fip();
			//if (info_cnt++ > 1)
			{
			//	info_cnt = 0;
				video_update_info();
			}
			last_chunk_offs = curpos;
			last_chunk_len = n;
			return 0;
		}

		curpos = video_lseek(0, SEEK_CUR);
		int extra = read(fd, buf, 8 + extra_bytes) - 8;

		if (extra < 0)
			break;
		if (strncasecmp((char *)buf, "LIST", 4) == 0)
		{
			video_lseek(4 - extra, SEEK_CUR);
			continue;
		}
		n = GET_DWORD(buf + 4);
		frame_pos++;
		if (strncasecmp((char *)buf, video_tag1, 3) == 0)
		{
			abs_video_pos++;
			video_pos++;
			scr = INT64(90000) * abs_video_pos * scale / rate;
			if (video_fmt == RIFF_VIDEO_MPEG4)
			{
				// VOP start code
				if (n > 4 && buf[8] == 0 && buf[9] == 0 && buf[10] == 1 && buf[11] == 0xb6)
				{
					if ((buf[12] & 0xc0) == 0)	// I-frame
					{
						goto found;
					}
				} 
				// or we have to search for it...
				else
				{
					int nleft = n - extra;
					while (nleft > 0)
					{
						int nb = MIN(nleft, max_buf_idx_size[0]);
						nb = video_read((BYTE *)buf_idx, nb);
						if (nb == 0)
							break;
						BYTE *b = (BYTE *)buf_idx;
						// find VOP start
						int vlen = nb;
						for (; vlen >= 0 && (b[0] != 0 || b[1] != 0 
											|| b[2] != 1 || b[3] != 0xb6); vlen--)
						{
							b++;
						}
						if (vlen > 0 && (b[4] & 0xc0) == 0)	// I-frame
						{
							goto found;
						}
						extra += nb;
						nleft -= nb;
					}
				}
			}
			else if (video_fmt == RIFF_VIDEO_DIV3)
			{
				// VOP start code
				if (n > 2 && (buf[8] & 0xc0) == 0)	// I-frame
				{
					goto found;
				}
			}
		}
		/*
		else if (cur_track >= 0 && strncasecmp((char *)buf, track[cur_track].audio_tag, 4) == 0)
		{
			audio_pos++;
		}
		*/
		video_lseek(PAD_EVEN(n) - extra, SEEK_CUR);
	}

	MSG("AVI: I-FRAME seek EOF!\n");
	return -1;

found:
	last_chunk_offs = curpos;
	last_chunk_len = n;

	cur_key_offs = video_offs = curpos;
	video_lseek(video_offs, SEEK_SET);
	MSG("AVI: I-FRAME at %d/%d (%d)\n", abs_video_pos, video_frames, curpos);
	video_packet_len = n;
	AddIndex(abs_video_pos, video_packet_len, video_offs);
	return 1;
}

int VideoAvi::AddIdx1Block()
{
	if (buf_idx == NULL || idx_offset < 0)
		return -1;

	AviOldIndexEntry *b = buf_idx;
	DWORD chunkid1 = GET_DWORD((BYTE *)video_tag1);
	DWORD chunkid2 = GET_DWORD((BYTE *)video_tag2);
	for (int i = buf_idx_size / sizeof(AviOldIndexEntry); i > 0; i--, b++)
	{
		if (b->chunkid == chunkid1 || b->chunkid == chunkid2)
		{
			if (b->flags & AVI_IDX1_FLAG_KEYFRAME)
			{
				if (cur_buf_video_idx > last_key_pos + min_delta_keyframes)
				{
					if (AddIndex(cur_buf_video_idx, b->size, b->offset + idx_offset) < 0)
						return -1;
				}
			} 
			cur_buf_video_idx++;
		}
	}
	cur_old_buf_idx += buf_idx_size / sizeof(AviOldIndexEntry);
	return 0;
}

int VideoAvi::AddIdx2Block()
{
	if (buf_idx == NULL)
		return -1;

	AviNewIndexEntry *b;
	int bis = buf_idx_size;
	if (cur_indx_buf_pos == 0)	// read header
	{
		AviNewIndex *ni = (AviNewIndex *)buf_newidx;
		cur_indx_base_offset = ni->qwBaseOffset - 8;

		b = (AviNewIndexEntry *)(ni + 1);
		bis -= sizeof(AviNewIndex);
	}
	else
		b = buf_newidx;
	for (int i = bis / sizeof(AviNewIndexEntry); i > 0; i--, b++)
	{
		if (!(b->size & AVI_IDX2_FLAG_KEYFRAME))
		{
			if (cur_buf_video_idx > last_key_pos + min_delta_keyframes)
			{
				if (AddIndex(cur_buf_video_idx, b->size, cur_indx_base_offset + b->offset) < 0)
					return -1;
			}
		} 
		cur_buf_video_idx++;
	}

	cur_indx_buf_pos += buf_idx_size;
	return 0;
}

void VideoAvi::ResetRecoveryMode()
{
	if (look4chunk)
	{
		video_lseek(cur_offs, SEEK_SET);
		look4chunk = false;
		script_update_variable(SCRIPT_VAR_PLAYER_SPEED);
	}
}

#endif
