//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - KHWL driver's properties.
 *  \file       sp_khwl_prop.h
 *  \author     bombur
 *  \version    0.1
 *  \date       4.07.2004
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

#ifndef SP_KHWL_PROP_H
#define SP_KHWL_PROP_H

#ifdef __cplusplus
extern "C" {
#endif

// property sets
typedef enum 
{
	KHWL_COMMON_SET = 1,
    KHWL_EEPROM_SET = 5,			// eeprom
    KHWL_BOARDINFO_SET = 6,			// general properties
    KHWL_VIDEO_SET = 7,				// video properties
    KHWL_AUDIO_SET = 8,				// audio properties
    KHWL_TIME_SET = 9,				// time properties
    KHWL_SUBPICTURE_SET = 10,		// subpicture properties
    KHWL_DVI_TRANSMITTER_SET = 13,	// DVI transmitter
    KHWL_DECODER_SET = 14,			// Mpeg decoder
    KHWL_OSD_SET = 17, 				// OSD properties

} KHWL_PROPERTY_SET;

// struct used to set property
typedef struct _KHWL_PROPERTY
{
	KHWL_PROPERTY_SET pset;
	int id;
	int size;
	void *v;

} KHWL_PROPERTY;

////////////////////////////////////////////////////

// ----------------- KHWL_COMMON_SET (1) common
typedef enum 
{
	eDoAudioLater = 15,

} KHWL_COMMON;

// ----------------- KHWL_EEPROM_SET (5) eeprom
typedef enum 
{
	eEepromAccess = 0,

} KHWL_EEPROM_SET_PROP;

// ----------------- KHWL_BOARDINFO_SET (6) general properties
typedef enum
{
	ebiBoardVersion = 2,
	ebiAPMState = 3,
	ebiPIOAccess = 4,
	ebiCommand = 7,

} KHWL_BOARDINFO_SET_PROP;

// ----------------- KHWL_VIDEO_SET (7) video properties
typedef enum
{
	evOutputDevice = 1,
	evTvOutputFormat = 2,
	evTvStandard = 3,
	evBrightness = 4,
	evContrast = 5,
	evSaturation = 6,
	evInAspectRatio = 7,
	evInStandard = 8,
	evOutDisplayOption = 9,
	evSourceWindow = 10,
	evZoomedWindow = 11,
	evMaxDisplayWindow = 12,
	evDestinationWindow = 13,
	evValidWindow = 14,
	evCustomHdtvParams = 15,
	evSpeed = 16,
	evScartAspectRatio = 23,
	evYUVWriteParams = 25,
	evDigOvOnlyParams = 28,
	evVOBUReverseSpeed = 31,
	evForcedProgressiveAlways = 34,
	evMacrovisionFlags = 37,
	evForcePanScanDefaultSize = 56,

} KHWL_VIDEO_SET_PROP;

// ----------------- KHWL_AUDIO_SET (8) audio properties
typedef enum
{
	eaVolumeRight = 5,
	eaVolumeLeft = 6,
	eAudioFormat = 7,
	eAudioSampleRate = 8,
	eAudioNumberOfChannels = 9,
	eAudioNumberOfBitsPerSample = 10,
	eAudioDigitalOutput = 13,

} KHWL_AUDIO_SET_PROP;

// ----------------- KHWL_TIME_SET (9) time properties
typedef enum
{
	etimSystemTimeClock = 2,
	etimVOPTimeIncrRes = 5,
	etimVideoCTSTimeScale = 6,
	etimAudioCTSTimeScale = 7,
	etimVideoFrameDisplayedTime = 8,

} KHWL_TIME_SET_PROP;

// ----------------- KHWL_SUBPICTURE_SET (10) subpicture properties
typedef enum
{
	eSubpictureCmd = 0,
	eSubpictureUpdatePalette = 1,
	eSubpictureUpdateButton = 2,
} KHWL_SUBPICTURE_SET_PROP;

// ----------------- KHWL_DVI_TRANSMITTER_SET (13) DVI transmitter
typedef enum
{
	edtAccessRegister = 0,
	edtOutputEnable = 1,

} KHWL_DVI_TRANSMITTER_SET_PROP;

// ----------------- KHWL_DECODER_SET (14) Mpeg decoder
typedef enum
{
	edecVideoStd = 2,
	edecOsdFlicker = 3,
	edecForceFixedVOPRate = 5,
	edecCSSChlg = 7,		// get challenge
	edecCSSKey1 = 8,		// set key
	edecCSSChlg2 = 9,		// set challenge
	edecCSSKey2 = 10,		// get key
	edecCSSDiscKey = 11,	// set
	edecCSSTitleKey = 12,	// set

} KHWL_DECODER_SET_PROP;

// ----------------- KHWL_OSD_SET (17) OSD properties
typedef enum
{
	eOsdCommand = 0,
	eOsdDestinationWindow = 2,

} KHWL_OSD_SET_PROP;


////////////////////////////////////////////////////////

/// Aux. structures

/// Time clock type
typedef struct 
{
    DWORD 		timeres; // 90000
    ULONGLONG 	pts;

} KHWL_TIME_TYPE;

/// Board command
typedef enum 
{
    ebiCommand_HardwareReset = 1,
    ebiCommand_VideoHwBlackFrame = 2,

} KHWL_BOARD_COMMAND_TYPE;

/// Audio format type
typedef enum 
{
    eAudioFormat_MPEG1        = 1,  // mpeg1 layer 1
    eAudioFormat_MPEG2        = 2,  // mpeg1 layer 2 ?
    eAudioFormat_AC3          = 3,  // ac3
    eAudioFormat_PCM          = 4,  // lpcm
    eAudioFormat_DTS          = 5,  // dts
    eAudioFormat_DVD_AUDIO    = 6,  // dvd audio
    eAudioFormat_REVERSE_PCM  = 7,  // rpcm
    eAudioFormat_AAC          = 8,  // aac
    eAudioFormat_MPEG1_LAYER3 = 9,  // mpeg1 layer 3
    eAudioFormat_MPEG2_LAYER1 = 10, // mpeg2 layer 1
    eAudioFormat_MPEG2_LAYER2 = 11, // mpeg2 layer 2
    eAudioFormat_MPEG2_LAYER3 = 12, // mpeg2 layer 3
    
    eAudioFormat_UNKNOWN = 0, 

} KHWL_AUDIO_FORMAT_TYPE;

/// Digital audio output type
typedef enum 
{
    eAudioDigitalOutput_Pcm        = 0,
    eAudioDigitalOutput_Compressed = 1,

} KHWL_AUDIO_DIGITAL_OUTPUT_TYPE;

/// Output video device
typedef enum 
{
    evOutputDevice_VGA  = 0,
    evOutputDevice_TV   = 1,
    evOutputDevice_HDTV = 0x20,
    evOutputDevice_DigOvOnly = 0x21,
    evOutputDevice_HdtvSubd = 0x400,

} KHWL_OUTPUT_DEVICE_TYPE;

/// TV standard used
typedef enum 
{
    evTvStandard_NTSC  = 0,
    evTvStandard_PAL   = 2,
    evTvStandard_PAL60 = 8,
    evTvStandard_PALM  = 10,

	// HD/Progressive formats

	evTvStandard_480P = 11,
	evTvStandard_576P = 12,
	evTvStandard_720P = 13,
	evTvStandard_1080I = 14,

	evTvStandard_720P50 = 255,

} KHWL_TV_STANDARD_TYPE;

/// TV Output (cable) format used
typedef enum 
{
	evTvOutputFormat_NONE                = -1,
    evTvOutputFormat_COMPOSITE           = 0,
    evTvOutputFormat_OUTPUT_OFF          = 0x40,
    evTvOutputFormat_COMPONENT_YUV       = 0x80,
    evTvOutputFormat_COMPONENT_RGB       = 0xc0,
    evTvOutputFormat_COMPONENT_RGB_SCART = 0x200,

	evTvOutputFormat_DVI				 = 0x1000,

} KHWL_TV_OUTPUT_FORMAT_TYPE;

/// TV Aspect ratio used
typedef enum
{
	evInAspectRatio_none = 0,
    evInAspectRatio_4x3  = 2,
    evInAspectRatio_16x9 = 3,

} KHWL_IN_ASPECT_RATIO_TYPE;

/// Out display ratio used
typedef enum
{
    evOutDisplayOption_Normal               = 0,
    evOutDisplayOption_16x9to4x3_PanScan    = 1,
    evOutDisplayOption_16x9to4x3_LetterBox  = 2,
    evOutDisplayOption_4x3to16x9_HorzCenter = 3,
    evOutDisplayOption_4x3to16x9_VertCenter = 4,

} KHWL_OUT_DISPLAY_OPTION_TYPE;

/// OSD switch
typedef enum 
{
    eOsdOn = 0,
    eOsdOff = 1,
    eOsdFlush = 2,

} KHWL_OSD_COMMAND_TYPE;


/// YUV buffer format
typedef enum
{
    KHWL_YUV_420_UNPACKED = 0,
    KHWL_YUV_422_UNPACKED,

} KHWL_YUV_DATA_FORMAT_TYPE;

/// YUV setup
typedef struct
{
    WORD wWidth;   // Picture width in pixels
    WORD wHeight;  // Picture height in lines
    KHWL_YUV_DATA_FORMAT_TYPE YUVFormat;

} KHWL_YUV_WRITE_PARAMS_TYPE;

// DigOV params
typedef struct 
{
	DWORD HFreq;
	DWORD VFreq;
	DWORD VideoWidth;
	DWORD VideoHeight;
	
	DWORD HSyncTotal;
	DWORD PreHSync;
	DWORD HSyncActive;
	DWORD PostHSync;
	
	DWORD VSyncTotal;
	DWORD PreVSync;
	DWORD VSyncActive;
	DWORD PostVSync;
	
	DWORD PixelFreq;
	DWORD Interlaced;
	
	BYTE HSyncPolarity;
	BYTE VSyncPolarity;
	
	BYTE BitsPerClock;
	KHWL_TV_STANDARD_TYPE TvHdtvStandard;
	BYTE Ccir;
	BYTE InvertField;
	BYTE SyncEnable;
	BYTE Vip20;
	DWORD SyncGen;
} KHWL_DIG_OV_PARAMS;


/// SPU commands (bitfields)
typedef enum 
{
    KHWL_SPU_ENABLE  = 0x2,
    KHWL_SPU_BUTTONS_ENABLE = 0x100,

} KHWL_SPU_COMMAND_TYPE;

/// SPU palette entry type
typedef struct 
{
    BYTE yuvX;
    BYTE yuvY;
    BYTE yuvCr;
    BYTE yuvCb;

} KHWL_SPU_PALETTE_ENTRY;

/// SPU button type
typedef struct 
{
    int left;
    int top;
    int right;
    int bottom;
    int color;
    int contrast;

} KHWL_SPU_BUTTON_TYPE;

/// Fixed VOP rate type
typedef struct 
{
	DWORD force;    // command for set, status for get
	DWORD time_incr;
	DWORD incr_res;

} KHWL_FIXED_VOP_RATE_TYPE;

#ifdef __cplusplus
}
#endif

#endif // of SP_KHWL_PROP_H
