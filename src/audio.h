//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - Audio player (WAV/MP3) header file
 *  \file       audio.h
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

#ifndef SP_AUDIO_H
#define SP_AUDIO_H

// see audio_get_audio_format_string()
typedef enum 
{
	RIFF_AUDIO_UNKNOWN = -1,
	RIFF_AUDIO_NONE = 0,
	RIFF_AUDIO_LPCM,
	RIFF_AUDIO_PCM,
	RIFF_AUDIO_MP1,
	RIFF_AUDIO_MP2,
	RIFF_AUDIO_MP3,
	RIFF_AUDIO_AC3,
	RIFF_AUDIO_DTS,
	RIFF_AUDIO_VORBIS,
} RIFF_AUDIO_FORMAT;	

typedef enum
{
	AUDIO_XING_HAS_NUMFRAMES = 0x0001,
	AUDIO_XING_HAS_FILESIZE  = 0x0002,
	AUDIO_XING_HAS_TOC       = 0x0004,
	AUDIO_XING_HAS_VBR_SCALE = 0x0008,
} AUDIO_XING_FLAGS;

#ifdef WIN32
#pragma pack(1)
#endif

typedef struct ATTRIBUTE_PACKED
{
  WORD  w_format_tag;
  WORD  n_channels;
  DWORD n_samples_per_sec;
  DWORD n_avg_bytes_per_sec;
  WORD  n_block_align;
  WORD  w_bits_per_sample;
  WORD  cb_size;
} RIFF_WAVEFORMATEX;

#ifdef WIN32
#pragma pack()
#endif

typedef struct RiffAudioTrack
{
	RIFF_WAVEFORMATEX *wfe;		// audio format

	DWORD    flags;
    long     mp3rate;			// mp3 bitrate kbs
    bool     a_vbr;				// Variable BitRate?
    long     padrate;			// byte rate used for zero padding

    long     audio_strn;		// Audio stream number 
	long     audio_samples;		// Number of samples
    LONGLONG audio_bytes;		// Total number of bytes of audio data 
    long     audio_chunks;		// Chunks of audio data in the file 

    char     audio_tag[4];		// Tag of audio data
    long     audio_pos;			// Audio position: chunk

    LONGLONG a_codech_off;		// absolute offset of audio codec information
    LONGLONG a_codecf_off;		// absolute offset of audio codec information

} RiffAudioTrack;

#ifdef AUDIO_INTERNAL


/// Audio player class
class Audio
{
public:
	/// ctor
	Audio();

	/// dtor
	~Audio();

	int ParseMp3Packet(MpegPacket *packet, BYTE * &buf, int len);
	int ParsePCMPacket(MpegPacket *packet, BYTE * &buf, int len);
	int ParseAC3Packet(MpegPacket *packet, BYTE * &buf, int len);
	int ParseRawPacket(MpegPacket *packet, BYTE * &buf, int len);

	int ParseXingHeader(BYTE *buf, int len);
	int ParseVBRIHeader(BYTE *buf, int len);
	int ParseAC3Header(BYTE *buf, int len, bool fill_info = true);

	int Seek(LONGLONG pos, int msecs);

public:
	struct mad_stream mp3_stream;
	struct mad_frame mp3_frame;
	struct mad_synth mp3_synth;

	RIFF_AUDIO_FORMAT audio_fmt;
	SPString audio_fmt_str;

	RIFF_WAVEFORMATEX *fmtex;

	int audio_numbufs, audio_bufsize;
	int audio_bufidx;
	int audio_samplerate, audio_channels;
	int samples_per_frame;

	int audio_frame_number;
	int is_partial_packet, is_partial_next_packet;

	// for seeking
	int bitrate, avg_bitrate, avg_num;
	// output bitrate - for PTS calc.
	int out_bps;
	/// audio data full size, in bytes.
	LONGLONG filesize, cur_offset, cur_tmpoffset;
	/// audio length, in msecs.
	int length;

	// displayed values
	int cur_bitrate, cur_audio_format;
	int cur_samples, cur_bits, cur_channels;

	bool needs_resample;
	bool fast;
	int resample_packet_size;

	BYTE *mp3_tmpbuf;
	int mp3_tmppos, mp3_tmpstart, mp3_tmpread;
	bool mp3_want_more;
	bool wait;
	
	int ac3_scan_header;
	BYTE ac3_header[8];
	int ac3_framesize;
	bool ac3_gatherinfo, ac3_gather_always;

	BYTE *mux_buf;
	int mux_numbufs, mux_bufsize;

	bool playing, stopping;
	LONGLONG saved_pts;
	int fd;

	SPList<DWORD> seek_toc;
	LONGLONG seek_offset;
	/// length of TOC, in msecs
	int toc_length;

	int (*mp3_output)(BYTE *data, struct mad_pcm *pcm);

private:
	/// Get big-endian number
	DWORD GetBE(BYTE *buf, int offset, int numbytes);
	/// Set length & filesize from VBR headers data
	int SetVbrLength(int total_num_frames, int vbr_filesize);


};
#endif

#ifdef __cplusplus
//extern "C" {
#endif

/// Play audio file
int audio_play(char *filepath = NULL, bool is_folder = false, bool continue_next = true);

/// Advance playing
int audio_loop();

/// Pause playing
BOOL audio_pause();

/// Stop playing
BOOL audio_stop(BOOL continue_next = TRUE);

/// Seek to given time and play
BOOL audio_seek(int seconds, int from);

/// Continue audio play
int audio_continue_play();

int audio_forward();
int audio_rewind();

void audio_setdebug(BOOL ison);
BOOL audio_getdebug();


/// Initialize audio player
int audio_init(RIFF_AUDIO_FORMAT fmt, bool fast);
/// Deinitialize audio player
int audio_deinit();

/// Get next file name from the list
char *audio_get_next(char *filepath, bool get_next);
/// Delete file list (free memory)
void audio_delete_filelist();

/// Return TRUE if audio decoder is ready
BOOL audio_ready();
/// Reset audio and drop old frames
int audio_reset();

// internal funcs used by media
BOOL audio_open(const char *filepath);

BOOL audio_close();

int audio_read(BYTE *buf, int *len);

/// Parse audio packet
int audio_parse_packet(MpegPacket *packet, BYTE * &buf, int len);

RIFF_AUDIO_FORMAT audio_get_audio_format(int fmt);

/// Get format text string
SPString audio_get_audio_format_string(RIFF_AUDIO_FORMAT fmt);

/// little-endian!
RIFF_WAVEFORMATEX *audio_read_formatex(int fd, BYTE *buf, int len);
/// Set audio params
int audio_setaudioparams(RIFF_AUDIO_FORMAT fmt, RIFF_WAVEFORMATEX *wfe, int halfrate);

/// Get audio bitrate
int audio_get_bitrate();

/// Get audio output bytes-per-second
int audio_get_output_bps();

void audio_update_format_string();

/// Set by video player to save some CPU resources...
void audio_set_ac3_getinfo(bool always);

#ifdef __cplusplus
//}
#endif

#endif // of SP_AUDIO_H
