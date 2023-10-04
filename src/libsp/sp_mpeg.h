//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - MPEG support header file
 *  \file       sp_mpeg.h
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

#ifndef SP_MPEG_H
#define SP_MPEG_H

#ifdef __cplusplus
extern "C" {
#endif

#define SPTM_SCR_FLAG INT64(0x8000000000000000)

#define FOURCC(ch0, ch1, ch2, ch3)           \
				((DWORD)(BYTE)(ch0) |        \
				((DWORD)(BYTE)(ch1) << 8)  | \
				((DWORD)(BYTE)(ch2) << 16) | \
				((DWORD)(BYTE)(ch3) << 24))

static inline ULONGLONG GET_ULONGLONG(BYTE *b)
{
   ULONGLONG r = (b[0] | (b[1]<<8) | (b[2]<<16) | (b[3]<<24));
   ULONGLONG s = (b[4] | (b[5]<<8) | (b[6]<<16) | (b[7]<<24));
   return ((s << 32) & INT64(0xffffffff00000000U)) | (r & INT64(0xffffffff));
}


static inline DWORD GET_DWORD(BYTE *b)
{
	return (b[0] | (b[1]<<8) | (b[2]<<16) | (b[3]<<24));
}

static inline DWORD GET_SYNCSAFE_DWORD(BYTE *b)
{
	return ((b[0] & 0x7f) << 21) | ((b[1] & 0x7f) << 14) 
		 | ((b[2] & 0x7f) << 7) | (b[3] & 0x7f);
}

static inline WORD GET_WORD(BYTE *b)
{
	return (WORD)(b[0] | (b[1] << 8));
}

#ifdef WIN32
#pragma pack(1)
#endif

/// MPEG Write packet (not 100% accurate info)
typedef struct MpegPacket_s MpegPacket;
struct MpegPacket_s
{
	MpegPacket *next;
	int 		type;
	DWORD 		flags;
	
    DWORD 		reserved[3];	// indexes, counters - not used (i guess)

	BYTE       *pData;
	DWORD 		size;
	LONGLONG 	pts;
	LONGLONG 	dts;		// not used ?
	DWORD 		encryptedinfo;
	WORD		nframeheaders;
	WORD 		firstaccessunitpointer;
    BYTE       *bufidx;
    DWORD 		AudioType;

    LONGLONG 	scr;
    LONGLONG 	vobu_sptm;
} ATTRIBUTE_PACKED;

#ifdef WIN32
#pragma pack()
#endif
			
typedef struct 
{
	MpegPacket *in;
	MpegPacket *out;
	int num;
	int out_cnt;
	int in_cnt;
	DWORD reserved;
} MpegPlayStruct;

typedef struct 
{
	KHWL_AUDIO_FORMAT_TYPE type;
	int dynrange;
	int samplerate;
	int numberofchannels;
	int numberofbitspersample;

	BOOL fromstream;
} MpegAudioPacketInfo;

typedef struct 
{
	void *payload;
	DWORD payload_size;
	ULONGLONG pts;
} MpegSpuPacketTnfo;

extern int MPEG_PACKET_LENGTH;		// = 2048 for standard MPEG1/2
extern int MPEG_NUM_PACKETS;		// = 512 (DVD, VCD) or 2048 (others)

/// Data pointers used by player code
extern MpegPlayStruct *MPEG_PLAY_STRUCT;
extern MpegPlayStruct *MPEG_VIDEO_STRUCT;
extern MpegPlayStruct *MPEG_AUDIO_STRUCT;
extern MpegPlayStruct *MPEG_SPU_STRUCT;

/// Packet feed types
typedef enum
{
	MPEG_FEED_VIDEO = 0,
	MPEG_FEED_AUDIO,
	MPEG_FEED_SPU,
} MPEG_FEED_TYPE;

/// Default buffers storage (use it in mpeg_setbuffer())
extern BYTE *BUF_BASE;

/// Source media types
typedef enum
{
	MEDIA_TYPE_UNKNOWN,
	MEDIA_TYPE_DVD = 1,
	MEDIA_TYPE_VCD = 2,
	MEDIA_TYPE_MPEG = 3,
	MEDIA_TYPE_AUDIO = 4,
	MEDIA_TYPE_VIDEO = 5,
	MEDIA_TYPE_CDDA = 6,
	//...
} MEDIA_TYPES;

/// Packet types
typedef enum 
{
	START_CODE_UNKNOWN = 0xff,
	START_CODE_END = 0xfe,

	START_CODE_PACK = 0,
	START_CODE_SYSTEM,
	START_CODE_PCI,
	START_CODE_PADDING,
	START_CODE_MPEG_VIDEO,
	START_CODE_MPEG_VIDEO_ELEMENTARY,
	START_CODE_MPEG_AUDIO1,
	START_CODE_MPEG_AUDIO2,
	START_CODE_PRIVATE1,
} START_CODE_TYPES;

/// Playing speed
typedef enum 
{
	MPEG_SPEED_STOP = 0,
	MPEG_SPEED_NORMAL = 1,	// play/resume
	MPEG_SPEED_PAUSE = 2,
	MPEG_SPEED_STEP = 3,

	///////////////////////////////////////////////////////
	/// masks:
	MPEG_SPEED_FWD_MASK = 0x100,
	MPEG_SPEED_REV_MASK = 0x200,
	MPEG_SPEED_FAST_FWD_MASK = MPEG_SPEED_FWD_MASK | 0x10,
	MPEG_SPEED_FAST_REV_MASK = MPEG_SPEED_REV_MASK | 0x20,
	MPEG_SPEED_SLOW_FWD_MASK = MPEG_SPEED_FWD_MASK | 0x40,
	MPEG_SPEED_SLOW_REV_MASK = MPEG_SPEED_REV_MASK | 0x80,
	///////////////////////////////////////////////////////

	MPEG_SPEED_FWD_4X  = MPEG_SPEED_FAST_FWD_MASK | 1,
	MPEG_SPEED_FWD_8X  = MPEG_SPEED_FAST_FWD_MASK | 2,
	MPEG_SPEED_FWD_16X = MPEG_SPEED_FAST_FWD_MASK | 3,
	MPEG_SPEED_FWD_32X = MPEG_SPEED_FAST_FWD_MASK | 4,
	MPEG_SPEED_FWD_48X = MPEG_SPEED_FAST_FWD_MASK | 5,
	MPEG_SPEED_FWD_MAX = MPEG_SPEED_FAST_FWD_MASK | 6,

	MPEG_SPEED_REV_4X  = MPEG_SPEED_FAST_REV_MASK | 1,
	MPEG_SPEED_REV_8X  = MPEG_SPEED_FAST_REV_MASK | 2,
	MPEG_SPEED_REV_16X = MPEG_SPEED_FAST_REV_MASK | 3,
	MPEG_SPEED_REV_32X = MPEG_SPEED_FAST_REV_MASK | 4,
	MPEG_SPEED_REV_48X = MPEG_SPEED_FAST_REV_MASK | 5,
	MPEG_SPEED_REV_MAX = MPEG_SPEED_FAST_REV_MASK | 6,

	MPEG_SPEED_SLOW_FWD_2X = MPEG_SPEED_SLOW_FWD_MASK | 1,	// 1/2
	MPEG_SPEED_SLOW_FWD_4X = MPEG_SPEED_SLOW_FWD_MASK | 2,	// 1/4
	MPEG_SPEED_SLOW_FWD_8X = MPEG_SPEED_SLOW_FWD_MASK | 3,	// 1/8

	MPEG_SPEED_SLOW_REV_2X = MPEG_SPEED_SLOW_REV_MASK | 1,	// 1/2
	MPEG_SPEED_SLOW_REV_4X = MPEG_SPEED_SLOW_REV_MASK | 2,	// 1/4
	MPEG_SPEED_SLOW_REV_8X = MPEG_SPEED_SLOW_REV_MASK | 3,	// 1/8

	MPEG_SPEED_UNKNOWN = 0xffffffff,

} MPEG_SPEED_TYPE;


typedef enum
{
	MPEG_BUFFER_1 = 0,
	MPEG_BUFFER_2 = 1,
	MPEG_BUFFER_3 = 2,
} MPEG_BUFFER;

typedef enum
{
	MPEG_UNKNOWN = 0,
	MPEG_1 = 1,
	MPEG_2 = 2,
	MPEG_4 = 4,
	
} MPEG_VIDEO_FORMAT;

//////////////////////////////////////////////////////
/// Functions

/// Initialize MPEG player.
/// If 'videobuf' is set, a separate videobuffer is created.
BOOL mpeg_init(MPEG_VIDEO_FORMAT fmt, BOOL audio_only, BOOL mpa_parsing, BOOL need_seq_start_packet);

/// Set buffers from 1 big memory chunk
BOOL mpeg_setbuffer(MPEG_BUFFER which, BYTE *base, int num_bufs, int buf_size);
/// Set buffers from array
BOOL mpeg_setbuffer_array(MPEG_BUFFER which, BYTE **base, int num_bufs, int buf_size);

/// Get main buffer base address
BYTE *mpeg_getbufbase();

/// Get buffer size
int mpeg_getbufsize(MPEG_BUFFER which);

/// Deinitialize MPEG player
BOOL mpeg_deinit();

/// Change playing speed
BOOL mpeg_setspeed(MPEG_SPEED_TYPE speed);

/// Get last frame dimensions
BOOL mpeg_getframesize(int *width, int *height);

/// Get current frames-per-second, in x1000 format (25000 = 25 fps)
int mpeg_get_fps();

/// Get current aspect ratio (1 = 1:1, 2 = 4:3, 3 = 16:9)
int mpeg_getaspect();

/// Zoom window horizontally (scale = 0, 1, 2,... for zoom-out, and negative for zoom-in)
BOOL mpeg_zoom_hor(int scale);
/// Zoom window vertically (scale = 0, 1, 2,... for zoom-out, and negative for zoom-in)
BOOL mpeg_zoom_ver(int scale);

/// Scroll window (offset = ..., -2, -1, 0, 1, 2,...)
/// <0 = window is shifted to the left
/// >0 = window is shifted to the right
BOOL mpeg_scroll(int offsetx, int offsety);

/// Reset zoom&scroll to defaults
BOOL mpeg_zoom_reset();

/// Reset MPEG queues and data before next play
BOOL mpeg_reset();

/// Start playing, but wait for 
void mpeg_start();

/// Feed given struct directly to khwl queue
BOOL mpeg_feed(MPEG_FEED_TYPE type);

/// Cached feed version - push packet into the stack (up to 16 packets in stack)
BOOL mpeg_feed_push();

/// Pop stored packet from the FIFO stack into khwl queue
BOOL mpeg_feed_pop();

/// Return TRUE if feed stack is empty
BOOL mpeg_feed_isempty();

BOOL mpeg_feed_reset();

BOOL mpeg_correct_pts();

/// Init queue
BOOL mpeg_feed_init(MpegPacket *start, int num);

/// Make sure we start playing
void mpeg_play();

/// Make sure we stopped playing
void mpeg_stop();

/// Start normal play (but don't change speed) - used for 'wait' func.
void mpeg_play_normal();

/// Return true if we are actually playing (not just buffering)
BOOL mpeg_is_playing();

/// Return true if any frame was displayed
BOOL mpeg_is_displayed();

/// Get video format.
MPEG_VIDEO_FORMAT mpeg_get_video_format();

/// Reset (clear) packets data
BOOL mpeg_init_packets();

/// Return how many packets are in processing
int mpeg_feed_getstackdepth();

/// Wait for all packets are processed by decoder.
BOOL mpeg_wait(BOOL onetime);

/// Get Last packet ready to fill in
MpegPacket *mpeg_feed_getlast();

/// Return current buffer's pointer
BYTE *mpeg_getcurbuf(MPEG_BUFFER which);

int mpeg_getpacketlength();

/// Set system PTS value
void mpeg_setpts(ULONGLONG estpts);

/// Get system PTS value
ULONGLONG mpeg_getpts();

/// Get stream rate, in bytes per sec. (or 0 if not ready)
int mpeg_getrate(BOOL now);

void mpeg_resetstreams();

/// Change audio stream
void mpeg_setaudiostream(int id);

/// Get current audio stream
int mpeg_getaudiostream();

/// Get audio streams number
int mpeg_getaudiostreamsnum();

/// Change SPU stream
void mpeg_setspustream(int id);

/// Get current SPU stream
int mpeg_getspustream();

/// Get SPU streams number
int mpeg_getspustreamsnum();

/// Set current buffer index to free chunk (or wait for it)
int mpeg_find_free_blocks(const MPEG_BUFFER which);

/// Increase and set buffer index for packet
void mpeg_setbufidx(MPEG_BUFFER which, MpegPacket *packet);

/// Decrease buffer index and release packet
void mpeg_release_packet(MPEG_BUFFER which, MpegPacket *packet);

/// Check for the special 'sequence start' header (and send one for MPEG1)
BOOL mpeg_needs_seq_start_header(MpegPacket **cur_packet);

// sorry, a packet parsing part is for CPP only
#ifdef __cplusplus

/// Retrieve the next packet and determine its type
START_CODE_TYPES mpeg_findpacket(BYTE * &buf, BYTE *base, int &startCounter);

/// Get PTS from packet header
ULONGLONG mpeg_extractTimingStampfromPESheader(BYTE * &buf);

/// Get MPEG type and bitrate, and return SCR
LONGLONG mpeg_parse_program_stream_pack_header(BYTE * &buf, BYTE *base);

/// Get Dolby AC-3 bitrate and audio params.
int mpeg_parse_ac3_header(BYTE *buf, int len, int *bitrate, int *sample_rate, int *channels, int *lfe, int *framesize);

/// Set audio params to KHWL
int mpeg_setaudioparams(MpegAudioPacketInfo *paudio);

/// Get current audio params copy
MpegAudioPacketInfo mpeg_getaudioparams();

/// Set audio sample rate and check if it's supported by decoder.
/// Return the closest available rate.
int mpeg_set_sample_rate(int samplerate);

/// Returns if resampling needed for non-standard audio sample rates.
BOOL mpeg_is_resample_needed();

/// Returns TRUE if audio format changed from the parsed MPEG stream
BOOL mpeg_audio_format_changed();

/// Fill packet data from raw packet
int mpeg_extractpacket(BYTE * &buf, BYTE *base, MpegPacket *packet, START_CODE_TYPES type, BOOL parse_video);

/// Detect & fix PTS wrap. Return fix PTS offset for new packets.
LONGLONG mpeg_detect_and_fix_pts_wrap(MpegPacket *packet);

/// Set MPEG4 VOP rate and scale
void mpeg_set_scale_rate(DWORD *scale, DWORD *rate);

/// Set video frame size
BOOL mpeg_setframesize(int width, int height, bool noaspect);

/// Converts little-endian PCM to big-endian LPCM
void mpeg_PCM_to_LPCM(BYTE *buf, int len);

/// Resample PCM audio and return bytes read.
/// Also converts little-endian PCM to big-endian.
int mpeg_PCM_resample_to_LPCM(BYTE *buf, int len, BYTE *out, int *outlen);

/// Advance data pointer with bounds check
inline int mpeg_incParsingBufIndex(BYTE * &buf, BYTE *base, DWORD inc)
{
	if (buf + inc <= base + MPEG_PACKET_LENGTH) 
	{
		buf += inc;
		return TRUE;
	}
	return FALSE;
}

inline bool mpeg_eofBuf(BYTE * buf, BYTE *base)
{
	return (buf >= base + MPEG_PACKET_LENGTH);
}

WORD mpeg_calc_crc16(BYTE *buf, DWORD bitsize);

#endif // of #ifdef __cplusplus

#ifdef __cplusplus
}
#endif

#endif // of SP_MPEG_H
