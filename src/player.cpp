//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - File player source file.
 *  \file       player.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       22.10.2006
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
#include <signal.h>
#include <pty.h>
#include <termios.h>
#include <sys/poll.h>
#include <sys/wait.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_fip.h>
#include <libsp/sp_khwl.h>
#include <libsp/sp_mpeg.h>
#include <libsp/sp_cdrom.h>

#include "script.h"
#include "player.h"

#ifdef WIN32
#include "info/player_info.h"
#endif

#ifdef EXTERNAL_PLAYER
#define MSG if (player_msg) msg
static BOOL player_msg = TRUE;
#endif

#ifdef FILEPLAYER_DEBUG

//#define FILEPLAYER_DEBUG
//#define DEBUG_MSG msg
#define DEBUG_MSG printf


static int player_debug_num_packets = 0;
//extern "C"
//{
void player_printpacket(MpegPacket *packet)
{
	DEBUG_MSG("[%d] t=%d fl=%ld\n", player_debug_num_packets++, packet->type, packet->flags);
	DEBUG_MSG("SIZE=%ld\n", packet->size);
	if (packet->pData != NULL)
	{
		int s = (packet->size + 7) / 8;
		for (int i = 0; i < s; i += 8)
		{
			DEBUG_MSG("DATA:    %02x %02x %02x %02x %02x %02x %02x %02x\n",
				packet->pData[i+0], packet->pData[i+1], packet->pData[i+2], packet->pData[i+3], 
				packet->pData[i+4], packet->pData[i+5], packet->pData[i+6], packet->pData[i+7]);
		}
	}
	DEBUG_MSG("pts=%u+%u dts=%u scr=%u+%u vobu=%u+%u\n",
		(unsigned)(packet->pts >> 32), (unsigned)packet->pts,
		(unsigned)packet->dts,
		(unsigned)(packet->scr >> 32), (unsigned)packet->scr,
		(unsigned)(packet->vobu_sptm >> 32), (unsigned)packet->vobu_sptm);
	DEBUG_MSG("ei=%d nfh=%d faup=%d at=%d r=%d,%d,%d\n",
		(int)packet->encryptedinfo, (int)packet->nframeheaders, (int)packet->firstaccessunitpointer,
		(int)packet->AudioType, 
		(int)packet->reserved[0], (int)packet->reserved[1], (int)packet->reserved[2]);
	DEBUG_MSG("\n\n");
	fflush(stdout);
}

/*
void player_feed(MpegPlayStruct *feed)
{
	MpegPlayStruct *from = MPEG_PLAY_STRUCT;
	if (from->in != NULL)
		player_printpacket(from->in);
	mpeg_feed(feed);
}
//}
int player_nextfeedpacket(MpegPlayStruct *feed)
{
	MpegPlayStruct *from = MPEG_PLAY_STRUCT;
	MpegPacket *feedin = feed->in;
	if (feedin == NULL)
		return FALSE;

	player_printpacket(feedin);
	
	feed->in = feed->in->next;
	
	feed->num--;
	feed->in_cnt++;
	from->num++;
	from->out_cnt++;
	
	if (feedin->next == NULL)
		feed->out = NULL;
	if (from->out == NULL)
		from->in = feedin;
	else
		from->out->next = feedin;
	from->out = feedin;
	feedin->next = NULL;
	
	////// simulate budidx decrement
	if (feedin->bufidx)
	{
		if (*(feedin->bufidx) > 0)
			(*feedin->bufidx)--;
	}
	
	return TRUE;
}

void player_debug_packets()
{
	for (;;)
	{
		//khwl_blockirq(TRUE);
		while (MPEG_VIDEO_STRUCT->in != NULL)
		{
			player_nextfeedpacket(MPEG_VIDEO_STRUCT);
		}
		while (MPEG_AUDIO_STRUCT->in != NULL)
		{
			player_nextfeedpacket(MPEG_AUDIO_STRUCT);
		}
		while (MPEG_SPU_STRUCT->in != NULL)
		{
			player_nextfeedpacket(MPEG_SPU_STRUCT);
		}
		//khwl_blockirq(FALSE);
		usleep(1000);
	}
}
*/
#endif

///////////////////////////////////////////////////

class PlayerTerm
{
public:
	/// ctor
	PlayerTerm()
	{
		master_handle = -1;
		slave_handle = -1;
		old_stdin = -1;
		old_stdout = -1;
		pts_id = -1;
	}

	/// Create and open terminal
	int Open();
	/// Close terminal
	void Close();

	/// Return true if terminal is opened
	bool IsOpened();

	/// Set terminal to current stdin/stdout
	void Set();
	
	/// Restore default stdin/stdout
	void Restore();

	/// Read data from the terminal (non-blocked).
	/// Returns number of bytes read.
	int Read(char *data, int max_data_size);

	/// Write zero-terminated string to the terminal.
	void Write(const char *str);

public:
	int master_handle, slave_handle;
	int old_stdin, old_stdout;
	int pts_id;
};


int PlayerTerm::Open()
{
	if (slave_handle >= 0)
		Restore();
	if (master_handle >= 0)
		Close();

	char pts_str[10];
	pts_id = openpty (&master_handle, &slave_handle, pts_str, NULL, NULL);
	if (pts_id == -1)
	{
		msg_error("Player: Cannot open pty.\n");
		return -1;
	}
	
	struct termios tattr;
	if (tcgetattr (master_handle, &tattr) == -1)
	{
		msg_error("Player: pty getattr failed.\n");
		return -1;
	}
	tattr.c_oflag &= ~ONLCR;
	tattr.c_lflag &= ~ECHO;
	tcsetattr(master_handle, TCSANOW, &tattr);
	
	int flgs = fcntl(master_handle, F_GETFL, 0);
	fcntl(master_handle, F_SETFL, flgs | O_NONBLOCK);

	msg("Player: * pty #%d (%s) opened.\n", pts_id, pts_str);

	return 0;
}

void PlayerTerm::Close()
{
	if (master_handle >= 0)
	{
		close(master_handle);
		master_handle = -1;

		msg("Player: * pty #%d closed.\n", pts_id);
		pts_id = -1;
	}
}

void PlayerTerm::Set()
{
	old_stdin = dup(STDIN_FILENO);
	old_stdout = dup(STDOUT_FILENO);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);

	dup2(slave_handle, STDIN_FILENO);
	dup2(slave_handle, STDOUT_FILENO);
}

void PlayerTerm::Restore()
{
	dup2(old_stdin, STDIN_FILENO);
	dup2(old_stdout, STDOUT_FILENO);
	close(old_stdin);
	close(old_stdout);
	close(slave_handle);
	old_stdin = -1;
	old_stdout = -1;
	slave_handle = -1;
}

bool PlayerTerm::IsOpened()
{
	return master_handle >= 0;
}

void PlayerTerm::Write(const char *str)
{
	if (master_handle >= 0)
		write(master_handle, str, strlen(str) + 1);
}

int PlayerTerm::Read(char *data, int max_data_size)
{
	struct pollfd pfd;
	pfd.fd = master_handle;
	pfd.events = POLLIN;
	pfd.revents = 0;
	if (poll(&pfd, 1, 100) <= 0)
		return 0;
	if (pfd.revents & POLLIN)
	{
		return read(master_handle, data, max_data_size);
	}
	return 0;
}

/////////////////////////////////////////////////////

static PlayerTerm id3term;
static void player_parseid3(char *data, int data_size);
static void stopid3();

enum PLAYER_READID3
{
	PLAYER_READID3_NONE = 0,
	PLAYER_READID3_TITLE,
	PLAYER_READID3_ARTIST,
	PLAYER_READID3_WIDTH,
	PLAYER_READID3_HEIGHT,
	PLAYER_READID3_CLRS,
};

static PLAYER_READID3 player_reading_id3 = PLAYER_READID3_NONE;
const int id3_max_data_size = 256;
static char id3_data[id3_max_data_size + 1];
static int id3id = -1;

inline void to_enter(char* &data, int &data_size)
{
	char *end = strchr(data, '\n');
	if (end != NULL)
	{
		*end = '\0';
		data_size -= (end - data + 1);
		data = end + 1;
	} else
		data_size = 0;
}

// called when child player process ends
static void player_sig_handler(int sig, siginfo_t *info, void *)
{
	if (sig != SIGCHLD)
		return;
	msg("Player: * SIG handler (%d)!\n", info->si_pid);
	int pstat;
	waitpid(info->si_pid, &pstat, 0);
#ifdef EXTERNAL_PLAYER
	if (info->si_pid == playerid)
		stopfile();
#endif
	if (info->si_pid == id3id)
		stopid3();
}

void set_sig(void (*siga) (int, siginfo_t *, void *))
{
	struct sigaction sig;
	memset(&sig, 0, sizeof(sig));
	if (siga != NULL)
		sig.sa_flags = SA_SIGINFO;
	sig.sa_sigaction = siga;
	sigaction(SIGCHLD, &sig, NULL);
}

/////////////////////////////////////////////////////////////////////

#ifdef EXTERNAL_PLAYER

static PlayerTerm pterm;
const int player_max_data_size = 1024;
static char player_data[player_max_data_size + 1];
static char player_type[10];
static char vmodebuf[10];
static bool player_paused = false;

enum PLAYER_READINFO
{
	PLAYER_READINFO_NONE = 0,
	PLAYER_READINFO_TIME = 1,
	PLAYER_READINFO_TITLE,
	PLAYER_READINFO_ARTIST,
	PLAYER_READINFO_VIDEOINFO,
	PLAYER_READINFO_AUDIOINFO,
};
static PLAYER_READINFO player_reading_info = PLAYER_READINFO_NONE;

static int playerid = -1;
static BOOL playing = FALSE, video = FALSE, mpegplayer = FALSE;
static int mpegplayer_info_cnt = 0;

static int playfile(const char *filename, const char *type, int vmode);
static BOOL player_command(const char *command);
static void stopfile();
static void stopid3();
static void player_parseinfo(char *data, int data_size);
static void set_sig(void (*siga) (int, siginfo_t *, void *));

int playfile(const char *filename, const char *type, int vmode)
{
	char *args1[] = { "/bin/fileplayer.bin", (char *)type, (char *)filename, (char *)vmodebuf, NULL };
	char *args2[] = { "/bin/mpegplayer.bin", (char *)filename, (char *)vmodebuf, NULL };
	char **args = (char **)args1;
	video = FALSE;
	mpegplayer = FALSE;
	mpegplayer_info_cnt = 0;
	sprintf(vmodebuf, "%d", vmode);
	if (type != NULL)
	{
		if ((type[0] == 'M' && type[1] == 'P' && type[2] == 'G') ||
			(type[0] == 'M' && type[1] == 'P' && type[2] == 'E'  && type[3] == 'G') ||
			(type[0] == 'V' && type[1] == 'O' && type[2] == 'B') ||
			(type[0] == 'D' && type[1] == 'A' && type[2] == 'T')
			)
		{
				args = (char **)args2;
				video = TRUE;
				mpegplayer = TRUE;
		}
		else
		{
			if ((type[0] == 'A' && type[1] == 'V' && type[2] == 'I'))
				video = TRUE;
		}
	}

	if (playerid != -1)
		player_stop();

	pterm.Open();
	set_sig(player_sig_handler);
	pterm.Set();
	playerid = exec_file(args[0], (const char **)args);
	pterm.Restore();

	MSG("Player: * Started (%d)...\n", playerid);
    playing = TRUE;

	return 0;
}

void stopfile()
{
	if (playerid < 0)
		return;

	fip_clear();
	
	// we'll have to wait for the next player_loop()
	playerid = -1;
	playing = FALSE;
	video = FALSE;
	mpegplayer = FALSE;

	MSG("Player: * Stopped.\n");
}

BOOL player_command(const char *command)
{
	if (playerid < 0 || command == NULL)
		return FALSE;
	MSG("Player: *COMMAND = %s\n", command);
	pterm.Write(command);
	return TRUE;
}

///////////////////////////////////////////////////////////

int player_play(char *filepath)
{
#ifdef FILEPLAYER_DEBUG
	player_debug_num_packets = 0;
	/*
	DWORD *ptr = (DWORD *)0x167FFE0;
	*(ptr) = (DWORD)&player_printpacket;
	*/
#endif

	if (filepath == NULL)	// resume playing
	{
		player_command("P\n");
		return 0;
	}
	
	// start player
	char *ext = strrchr(filepath, '.');
	if (ext != NULL)
	{
		ext = ext + 1;
		strncpy(player_type, ext, 10);
		strupr(player_type);
		
		int vmode = 0;
		// TODO: set real vmode
		player_paused = false;
		return playfile(filepath, player_type, vmode);
	}
	// unknown file type...
	return -1;
}

int player_info_loop()
{
#ifdef FILEPLAYER_DEBUG
//	player_debug_packets();
#endif

	if (pterm.IsOpened())
	{
		int player_data_size = pterm.Read(player_data, player_max_data_size);
		//strcpy(player_data, "Info\n00:00:24\nN/A\nN/A\nN/A\nN/A\n");
		//int player_data_size = strlen(player_data);
			
		// parse data from player:
		if (player_data_size > 0)
		{
			player_data[player_data_size] = '\0';
			//MSG("data[%d]: %s\n", player_data_size, player_data);
			if (!mpegplayer || mpegplayer_info_cnt++ < 10)
				player_parseinfo(player_data, player_data_size);
		}
	}
	// playing stopped, so clean-up
	if (playerid < 0)
	{
		if (pterm.IsOpened())
		{
			pterm.Close();
			player_reading_info = PLAYER_READINFO_NONE;
		}
		return 1;
	}
	return 0;
}

void player_parseinfo(char *data, int data_size)
{
	while (data_size > 0)
	{
		if (player_reading_info == PLAYER_READINFO_TIME)
		{
			if (data_size > 7)
			{
				int timelen = 0;
				if (isdigit(data[0]))
				{
					timelen = ((data[0] - '0') * 10 + (data[1] - '0')) * 3600;
					timelen += ((data[3] - '0') * 10 + (data[4] - '0')) * 60;
					timelen += ((data[6] - '0') * 10 + (data[7] - '0'));
				}
				script_totaltime_callback(timelen);
				MSG("* TOTAL TIME: %d secs\n", timelen);
				data += 8;
				data_size -= 8;
				player_reading_info = PLAYER_READINFO_TITLE;
				to_enter(data, data_size);
				continue;
			}
			data++;
			data_size--;
		}
		else if (player_reading_info == PLAYER_READINFO_TITLE)
		{
			char *titl = data;
			to_enter(data, data_size);
			MSG("* TITLE: %s.\n", titl);
			player_reading_info = PLAYER_READINFO_ARTIST;
			continue;
		}
		else if (player_reading_info == PLAYER_READINFO_ARTIST)
		{
			char *artist = data;
			to_enter(data, data_size);
			MSG("* ARTIST: %s.\n", artist);
			player_reading_info = PLAYER_READINFO_AUDIOINFO;
			continue;
		}
		else if (player_reading_info == PLAYER_READINFO_AUDIOINFO)
		{
			char *ai = data;
			to_enter(data, data_size);
			script_audio_info_callback(ai);
			MSG("* AUDIO: %s.\n", ai);
			player_reading_info = PLAYER_READINFO_VIDEOINFO;
			continue;
		}
		else if (player_reading_info == PLAYER_READINFO_VIDEOINFO)
		{
			char *vi = data;
			to_enter(data, data_size);
			script_video_info_callback(vi);
			MSG("* VIDEO: %s.\n", vi);
			player_reading_info = PLAYER_READINFO_NONE;
			continue;
		}
		else 
		{
			DWORD cmd = SafeGetDword(data);
			if (cmd == 0x656d6954 && data_size >= 7)		// "Time"
			{
				if (data[4] <= 99 && data[5] <= 59 && data[6] <= 59)
				{
					int tim = data[4] * 3600 + data[5] * 60 + data[6];
					script_time_callback(tim);
					MSG("PLAYER TIME = %d\n", tim);
				}
				data += 7;
				data_size -= 7;
				player_reading_info = PLAYER_READINFO_NONE;
			}
			else if (cmd == 0x6f666e49 && data_size > 4 
						&& data[4] == '\n')				// "Info"
			{
				player_reading_info = PLAYER_READINFO_TIME;
				data += 5;
				data_size -= 5;
			}
			else if (cmd == 0x79616c50 && data_size >= 4)	// "Play"
			{
				data += 4;
				data_size -= 4;
				player_reading_info = PLAYER_READINFO_NONE;
				player_paused = false;
			}
			else if (cmd == 0x74696157 && data_size >= 4)	// "Wait"
			{
				data += 4;
				data_size -= 4;
				script_error_callback(SCRIPT_ERROR_WAIT);
				player_reading_info = PLAYER_READINFO_NONE;
			}
			else if (cmd == 0x41646142 && data_size >= 4)	// "BadA"
			{
				data += 4;
				data_size -= 4;
				script_error_callback(SCRIPT_ERROR_BAD_AUDIO);
				player_reading_info = PLAYER_READINFO_NONE;
			}
			else if (cmd == 0x43646142 && data_size >= 4)	// "BadC"
			{
				data += 4;
				data_size -= 4;
				script_error_callback(SCRIPT_ERROR_BAD_CODEC);
				player_reading_info = PLAYER_READINFO_NONE;
			}
			else
			{
				data++;
				data_size--;
				player_reading_info = PLAYER_READINFO_NONE;
			}
		}
	}
}

BOOL player_pause()
{
	player_paused = !player_paused;
	player_command("P\n");
	return TRUE;
}

BOOL player_stop()
{
	int pstat;
	if (playerid < 0)
		return FALSE;

	MSG("Player: * Stopping...\n");
	
	// first, we ask player to stop...
	player_command("X\n");

	// sleep and wait for signals...
	sleep(1);
	usleep(200000);
	
	// if it's still alive...
	if (waitpid(playerid, &pstat, WNOHANG) == 0)
	{
		MSG("Player: * Stopping by force (%d)!\n", playerid);
khwl_stop();
		// kill it by force
		kill(playerid, SIGKILL);
		int pstat;
		waitpid(playerid, &pstat, 0);
khwl_stop();
		// clear video garbage
		if (video)
			khwl_restoreparams();
	}
	
	stopfile();

	return TRUE;
}

BOOL player_seek(int seconds)
{
	if (player_paused)
	{
		player_command("P\n");
		player_paused = false;
	}

	char seekcmd[10];
	sprintf(seekcmd, "s%06d\n", seconds);
	player_command(seekcmd);
	
	return TRUE;
}

BOOL player_zoom_hor(int /*scale*/)
{
	//player_command("zw15\n");
	//zl15
	//zr15
	//zu15
	//zd15
	//zw15
	//zw-15
	//zh15
	//zh-15
	//zF
	//zR reset zoom
	//zC center scroll
	return FALSE;
}

BOOL player_zoom_ver(int /*scale*/)
{
	return FALSE;
}

BOOL player_scroll(int /*offx*/, int /*offy*/)
{
	return FALSE;
}

int player_forward()
{
	player_command("t1\n");
	if (video && !mpegplayer)
		return 1;
	return 0;
}

int player_rewind()
{
	player_command("t2\n");
	if (video && !mpegplayer)
		return 1;
	return 0;
}

#endif

//////////////////////////////////////////////////////////////////////
// INFO

void stopid3()
{
	if (id3id < 0)
		return;
	
	id3id = -1;
	msg("Player: * ID3 Stopped.\n");
}


int player_id3_loop()
{
	if (id3term.IsOpened())
	{
		int id3_data_size = id3term.Read(id3_data, id3_max_data_size);
		//strcpy(id3_data, "Titl\nTitle\nAuth\nArtist\n");
		//strcpy(id3_data, "Dims\n1000\n500\nClrs\n3\n");
		//int id3_data_size = strlen(id3_data);
		
		// parse data from fileinfo:
		if (id3_data_size > 0)
		{
			id3_data[id3_data_size] = '\0';
			//MSG("ID3DATA (%d): [%s]\n", id3_data_size, id3_data);
			player_parseid3(id3_data, id3_data_size);
			
		}
	}
	
	// id3 info process stopped, so clean-up
	if (id3id < 0)
	{
		if (id3term.IsOpened())
		{
			id3term.Close();
			player_reading_id3 = PLAYER_READID3_NONE;
		}
		return 1;
	}
	
	return 0;
}

void player_parseid3(char *data, int data_size)
{
	static int width = 0;
	while (data_size > 0)
	{
		if (player_reading_id3 == PLAYER_READID3_TITLE)
		{
			char *titl = data;
			to_enter(data, data_size);
			script_name_callback(titl);
			player_reading_id3 = PLAYER_READID3_NONE;
			continue;
		}
		else if (player_reading_id3 == PLAYER_READID3_ARTIST)
		{
			char *auth = data;
			to_enter(data, data_size);
			script_artist_callback(auth);
			player_reading_id3 = PLAYER_READID3_NONE;
			continue;
		}
		else if (player_reading_id3 == PLAYER_READID3_WIDTH)
		{
			char *w = data;
			to_enter(data, data_size);
			width = atoi(w);
			player_reading_id3 = PLAYER_READID3_HEIGHT;
			continue;
		}
		else if (player_reading_id3 == PLAYER_READID3_HEIGHT)
		{
			char *height = data;
			to_enter(data, data_size);
			script_framesize_callback(width, atoi(height));
			player_reading_id3 = PLAYER_READID3_NONE;
			continue;
		}
		else if (player_reading_id3 == PLAYER_READID3_CLRS)
		{
			char *clrs = data;
			to_enter(data, data_size);
			script_colorspace_callback(atoi(clrs));
			player_reading_id3 = PLAYER_READID3_NONE;
			continue;
		}
		else
		{
			DWORD cmd = SafeGetDword(data);
			if (cmd == 0x6c746954 && data_size >= 5)		// "Titl"
			{
				data += 5;
				data_size -= 5;
				player_reading_id3 = PLAYER_READID3_TITLE;
				continue;
			}
			else if (cmd == 0x74737441 && data_size >= 5)	// "Atst"
			{
				data += 5;
				data_size -= 5;
				player_reading_id3 = PLAYER_READID3_ARTIST;
				continue;
			}
			else if (cmd == 0x736d6944 && data_size >= 5)	// "Dims"
			{
				data += 5;
				data_size -= 5;
				width = 0;
				player_reading_id3 = PLAYER_READID3_WIDTH;
				continue;
			}
			else if (cmd == 0x73726c43 && data_size >= 5)	// "Clrs"
			{
				data += 5;
				data_size -= 5;
				player_reading_id3 = PLAYER_READID3_CLRS;
				continue;
			}
			else if (cmd == 0x72727245 && data_size >= 5)	// "Errr"
			{
				data += 5;
				data_size -= 5;
				script_name_callback("");
				script_artist_callback("");
				//script_error_callback();
				player_reading_id3 = PLAYER_READID3_NONE;
				continue;
			}
			else if (cmd == 0x656e6f4e && data_size >= 5)	// "None"
			{
				data += 5;
				data_size -= 5;
				script_name_callback("");
				script_artist_callback("");
				player_reading_id3 = PLAYER_READID3_NONE;
				continue;
			}
			else
				data_size = 0;
		}
	}
}



void player_startinfo(const char *fname, const char *charset)
{
#ifdef PLAYER_INFO_EMBED
	player_getinfo(fname, charset);
#else
	const char *args[] = { "/bin/fileinfo.bin", fname, charset, NULL };

	if (id3id >= 0)
	{
		kill(id3id, SIGKILL);
		int pstat;
		waitpid(id3id, &pstat, 0);
		id3id = -1;
	}

	id3term.Open();
	set_sig(player_sig_handler);
	id3term.Set();
	id3id = exec_file(args[0], (const char **)args);
	id3term.Restore();
#endif
}

#ifdef EXTERNAL_PLAYER
void player_setdebug(BOOL ison)
{
	player_msg = ison == TRUE;
}

BOOL player_getdebug()
{
	return player_msg;
}
#endif
