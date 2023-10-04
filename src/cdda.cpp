//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - CDDA (CD-Audio) player impl.
 *  \file       cdda.cpp
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
#include <time.h>
#include <fcntl.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_fip.h>
#include <libsp/sp_khwl.h>
#include <libsp/sp_mpeg.h>
#include <libsp/sp_cdrom.h>

#include <sys/ioctl.h>
#include <linux/cdrom.h>


#include "script.h"

#include "cdda.h"
#include "media.h"


extern int cdrom_handle;


static Cdda *cdda = NULL;

static bool cdda_msg = true;
#define MSG if (cdda_msg) msg


bool cdda_read_track(CddaTrack *track, int i)
{
	struct cdrom_tocentry e;
	memset(&e, 0, sizeof(struct cdrom_tocentry));
	e.cdte_track = (BYTE)i;
	e.cdte_format = CDROM_MSF;
	if (ioctl(cdrom_handle, CDROMREADTOCENTRY, &e) >= 0) 
	{
		if ((e.cdte_ctrl & CDROM_DATA_TRACK) == 0)
		{
			if (track != NULL)
			{
				track->idx = i;
				track->frame_idx = e.cdte_addr.msf.minute * CD_SECS * CD_FRAMES
					+ e.cdte_addr.msf.second * CD_FRAMES + e.cdte_addr.msf.frame;
			}
			return true;
		}
	}
	return false;
}

Cdda *cdda_open()
{
	int i;
	if (cdda != NULL)
		return cdda;

#ifdef WIN32
	// only for real device!
	if (cdrom_handle == 2)
	{
		MCI_OPEN_PARMS mciOpen;
		MCI_STATUS_PARMS mciParams;
		MCI_SET_PARMS mciSet;
		mciOpen.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
		mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID, (DWORD)&mciOpen);

		mciSet.dwTimeFormat = MCI_FORMAT_MSF;
		mciSendCommand(mciOpen.wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&mciSet); 
		
		mciParams.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
		mciSendCommand(mciOpen.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&mciParams); 
		int numtr = mciParams.dwReturn;

		cdda = new Cdda();
		cdda->tracks.Reserve(numtr);
		
		for (i = 0; i < numtr; i++)
		{
			mciParams.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
			mciParams.dwTrack = i + 1;
			mciSendCommand(mciOpen.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)&mciParams); 
			if (mciParams.dwReturn == MCI_CDA_TRACK_AUDIO)
			{
				CddaTrack track;
				mciParams.dwItem = MCI_STATUS_LENGTH;
				mciParams.dwTrack = i + 1;
				mciSendCommand(mciOpen.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)&mciParams); 

				track.length = MCI_MSF_MINUTE(mciParams.dwReturn) * CD_SECS * CD_FRAMES +
								MCI_MSF_SECOND(mciParams.dwReturn) * CD_FRAMES +
								MCI_MSF_FRAME(mciParams.dwReturn);

				mciParams.dwItem = MCI_STATUS_POSITION;
				mciParams.dwTrack = i + 1;
				mciSendCommand(mciOpen.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)&mciParams); 

				track.frame_idx = MCI_MSF_MINUTE(mciParams.dwReturn) * CD_SECS * CD_FRAMES +
								MCI_MSF_SECOND(mciParams.dwReturn) * CD_FRAMES +
								MCI_MSF_FRAME(mciParams.dwReturn);

				cdda->tracks.Add(track);
			}
		}
		mciSendCommand(mciOpen.wDeviceID, MCI_CLOSE, 0, 0);
		return cdda;
	} else
	{
		return NULL;
	}
#else

	if (cdrom_handle < 0)
	{
		msg_error("CDDA: Cannot find opened CD-ROM handle.\n");
		return NULL;
	}

	struct cdrom_tochdr toc;
    if (ioctl(cdrom_handle, CDROMREADTOCHDR, &toc) < 0) 
	{
		return NULL;
    }
	
	cdda = new Cdda();
    cdda->track1 = toc.cdth_trk0;
    cdda->track2 = toc.cdth_trk1;

	cdda->tracks.Reserve(cdda->track2 - cdda->track1 + 1);
	int track_idx = 0;
	cdda->total_length = 0;
	for (i = cdda->track1; i <= cdda->track2; i++)
	{
		CddaTrack track;
		if (cdda_read_track(&track, i))
		{
			track_idx = cdda->tracks.Add(track);
			if (track_idx > 0)
				cdda->tracks[track_idx - 1].length = track.frame_idx - cdda->total_length;
			cdda->total_length = track.frame_idx;
		}
	}
	CddaTrack track_leadout;
	if (cdda_read_track(&track_leadout, CDROM_LEADOUT))
	{
		cdda->tracks[track_idx].length = track_leadout.frame_idx - cdda->total_length;
		cdda->total_length = track_leadout.frame_idx;
	}

	return cdda;
#endif
}

BOOL cdda_close()
{
	cdda_stop();
	SPSafeDelete(cdda);
	return TRUE;
}

int cdda_play(char *path)
{
	if (cdrom_handle < 0)
		return -1;
	if (path == NULL)
	{
		fip_write_special(FIP_SPECIAL_PLAY, 1);
		fip_write_special(FIP_SPECIAL_PAUSE, 0);
		mpeg_setspeed(MPEG_SPEED_NORMAL);
		
		return 0;
	}
	
	if (strncasecmp(path, "/cdrom/", 7) != 0)
		return -1;
	
	int ret = media_open(path, MEDIA_TYPE_CDDA);
	if (ret < 0 || cdda == NULL)
	{
		msg_error("CDDA: media open FAILED.\n");
		return ret;
	}
	MSG("CDDA: start...\n");

	if (strlen(path) > 7)
	{
		cdda->cur_trackid = atoi(path + 7) - 1;
	} else
		cdda->cur_trackid = 0;
	if (cdda->cur_trackid < 0 || cdda->cur_trackid >= cdda->tracks.GetN())
		return -1;
	cdda->cur = cdda->tracks[cdda->cur_trackid].frame_idx;
	MSG("CDDA: * Play Track = %d (pos = %d).\n", cdda->cur_trackid+1, cdda->cur);

	cdda->feof = false;

	mpeg_init(MPEG_1, TRUE, FALSE, FALSE);
	mpeg_setbuffer(MPEG_BUFFER_1, BUF_BASE, 8, 30576);
	
	MSG("CDDA: Setting default audio params.\n");
	MpegAudioPacketInfo defaudioparams;
	defaudioparams.type = eAudioFormat_PCM;
	defaudioparams.samplerate = 44100;
	defaudioparams.numberofbitspersample = 16;
	defaudioparams.numberofchannels = 2;
	defaudioparams.fromstream = 0;
	mpeg_setaudioparams(&defaudioparams);

	// write dvd-specific FIP stuff...
	fip_write_special(FIP_SPECIAL_CD, 1);
	const char *digits = "  00000";
	fip_write_string(digits);
	fip_write_special(FIP_SPECIAL_COLON1, 1);
	fip_write_special(FIP_SPECIAL_COLON2, 1);

	fip_write_special(FIP_SPECIAL_PLAY, 1);
	fip_write_special(FIP_SPECIAL_PAUSE, 0);

	script_audio_info_callback("");
	script_video_info_callback("");

	cdda->playing = true;
	mpeg_start();

	return 0;
}

int cdda_get_length(char *path)
{
	if (path == NULL)
		return 0;
	if (strncasecmp(path, "/cdrom/", 7) != 0)
		return 0;
	if (cdda_open() == NULL)
		return 0;
	int trackid = atoi(path + 7) - 1;
	if (trackid < 0 || trackid >= cdda->tracks.GetN())
		return 0;
	return cdda->tracks[trackid].length / CD_FRAMES;
}

int cdda_get_numtracks()
{
	if (cdda_open() == NULL)
		return 0;
	return cdda->tracks.GetN();
}

BOOL cdda_feof()
{
	if (cdda == NULL || cdda->feof)
		return TRUE;
	return FALSE;
}

int cdda_read(BYTE *buf, int numblocks)
{
	if (cdrom_handle < 0 || cdda == NULL)
	{
		return 0;
	}
	if (cdda->feof)
	{
		return 0;
	}
	if (cdda->cur < 0 || cdda->cur_trackid < 0)
	{
		return 0;
	}
	
	int maxbl = cdda->tracks[cdda->cur_trackid].frame_idx + cdda->tracks[cdda->cur_trackid].length - 1;

	//MSG("CDDA: %5d / %5d [%d]\n", cdda->cur, maxbl, cdda->feof);

	if (cdda->cur + numblocks > maxbl)
	{
		numblocks = maxbl - cdda->cur;
		cdda->feof = true;
	}
	if (numblocks < 1)
		return 0;
	cdrom_read_audio rda;
	rda.addr.msf.minute = (BYTE)(cdda->cur / CD_FRAMES / CD_SECS);
	rda.addr.msf.second = (BYTE)((cdda->cur / CD_FRAMES) % CD_SECS);
	rda.addr.msf.frame = (BYTE)(cdda->cur % CD_FRAMES);
	rda.addr_format = CDROM_MSF;
	rda.nframes = numblocks;
	rda.buf = buf;
	
	if (ioctl(cdrom_handle, CDROMREADAUDIO, &rda) < 0)
		return 0;

	// LPCM requires big-endian byte order
	mpeg_PCM_to_LPCM(buf, numblocks * MPEG_PACKET_LENGTH);
	cdda->cur += numblocks;
	return numblocks;
}

void cdda_update_info()
{
	if (cdda == NULL)
		return;
	static int old_secs = 0;
	KHWL_TIME_TYPE displ;
	displ.pts = 0;
	displ.timeres = 90000;
	if (mpeg_is_displayed())
		khwl_getproperty(KHWL_TIME_SET, etimSystemTimeClock, sizeof(displ), &displ);
//	displ.pts -= pts_base;

	if (cdda->old_trackid != cdda->cur_trackid || (LONGLONG)displ.pts != cdda->saved_pts)
	{
		if (cdda->cur_trackid >= 0 && (LONGLONG)displ.pts >= 0)
		{
			if (cdda->cur_trackid != cdda->old_trackid)
			{
				script_totaltime_callback(cdda->tracks[cdda->cur_trackid].length / CD_FRAMES);

				script_cdda_track_callback(cdda->cur_trackid + 1);
				
				old_secs = -1;
			}

			cdda->old_trackid = cdda->cur_trackid;
			cdda->saved_pts = displ.pts;

			char fip_out[10];
			int secs = (int)(displ.pts / 90000);
			int ccid = cdda->cur_trackid + 1;
			if (ccid < 0)
				ccid = 0;
			if (ccid > 99)
				ccid = 99;
			if (secs < 0)
				secs = 0;
			if (secs >= 10*3600)
				secs = 10*3600-1;
			if (secs != old_secs)
			{
				script_time_callback(secs);
			
				fip_out[0] = (char)((ccid / 10) + '0');
				fip_out[1] = (char)((ccid % 10) + '0');
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


int cdda_player_loop()
{
	static int info_cnt = 0;
	if (cdrom_handle < 0 || cdda == NULL || cdda->cur < 0)
		return 1;
	
	BYTE *buf = NULL;
	
	MEDIA_EVENT event = MEDIA_EVENT_OK;
	int len = 0;

	int ret = media_get_next_block(&buf, &event, &len);
	if (ret > 0 && buf == NULL)
	{
		msg_error("CDDA: Not initialized. STOP!\n");
		return 1;
	}
	if (ret == -1)
	{
		msg_error("CDDA: Error getting next block!\n");
		return 1;
	}
	else if (ret == 0)		// wait...
	{
		return 0;
	}
	
	if (event == MEDIA_EVENT_STOP)
	{
		MSG("CDDA: STOP Event triggered!\n");
		return 1;
	}
	BYTE *base = buf;

	MpegPacket *packet = NULL;
	packet = mpeg_feed_getlast();
	if (packet == NULL)		// well, it won't really help
		return 0;
	memset((BYTE *)packet + 4, 0, sizeof(MpegPacket) - 4);
	
	packet->pts = 0;//pts_base;
	packet->nframeheaders = 0xffff;
	packet->firstaccessunitpointer = 0;
	packet->type = 1;
	packet->pData = base;
	packet->size = MPEG_PACKET_LENGTH;

	// increase bufidx
	mpeg_setbufidx(MPEG_BUFFER_1, packet);

	mpeg_feed(MPEG_FEED_AUDIO);

	if (info_cnt++ > 32)
	{
		info_cnt = 0;
		cdda_update_info();
	}

	return 0;
}

BOOL cdda_pause()
{
	fip_write_special(FIP_SPECIAL_PLAY, 0);
	fip_write_special(FIP_SPECIAL_PAUSE, 1);

	mpeg_setspeed(MPEG_SPEED_PAUSE);

	return TRUE;
}

BOOL cdda_stop()
{
	if (cdda != NULL)
	{
		if (cdda->playing)
		{
			mpeg_deinit();
			cdda->playing = false;
		}
	}
	return TRUE;
}

void cdda_setdebug(BOOL ison)
{
	cdda_msg = ison == TRUE;
}

BOOL cdda_getdebug()
{
	return cdda_msg;
}

BOOL cdda_seek(int seconds, int from_frame)
{
	if (cdda != NULL)
	{
		if (cdda->cur_trackid >= 0 && cdda->cur_trackid < cdda->tracks.GetN())
		{
			// start of the current track
			if (from_frame < 0)
				from_frame = cdda->tracks[cdda->cur_trackid].frame_idx;

			khwl_stop();

			cdda->cur = from_frame;
			cdda->cur += seconds * CD_FRAMES;
			int offs = cdda->cur - cdda->tracks[cdda->cur_trackid].frame_idx;
			if (offs < 0)
			{
				cdda->cur = cdda->tracks[cdda->cur_trackid].frame_idx;
				offs = 0;
			}
			if (offs >= cdda->tracks[cdda->cur_trackid].length)
				cdda->cur = cdda->tracks[cdda->cur_trackid].frame_idx + cdda->tracks[cdda->cur_trackid].length - 1;
			
			mpeg_start();
			mpeg_setpts(offs * 90000 / CD_FRAMES);

			return TRUE;
		}
	}
	script_error_callback(SCRIPT_ERROR_INVALID);
	return FALSE;
}

BOOL cdda_seek_track(int track)
{
	int trck = track - 1;
	if (cdda == NULL || trck < 0 || trck >= cdda->tracks.GetN())
	{
		script_error_callback(SCRIPT_ERROR_INVALID);
		return FALSE;
	}
	khwl_stop();
	cdda->cur_trackid = trck;
	cdda->cur = cdda->tracks[trck].frame_idx;
	MSG("CDDA: * Playpos = %d.\n", cdda->cur);
	
	mpeg_start();
	mpeg_setpts(0);
	return TRUE;
}

void cdda_get_cur(int *track)
{
	if (track != NULL && cdda != NULL)
	{
		*track = cdda->cur_trackid + 1;
	}

}

void cdda_forward()
{
	if (cdda != NULL)
		cdda_seek(20, cdda->cur);
}

void cdda_rewind()
{
	if (cdda != NULL)
		cdda_seek(-20, cdda->cur);
}
