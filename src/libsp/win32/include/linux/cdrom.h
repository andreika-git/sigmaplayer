
#define _WINSOCKAPI_
#include <windows.h>
#include <mmsystem.h>
#define NOTIMEVAL

#define CDROMPAUSE		0x5301 /* Pause Audio Operation */ 
#define CDROMRESUME		0x5302 /* Resume paused Audio Operation */
#define CDROMPLAYMSF		0x5303 /* Play Audio MSF (struct cdrom_msf) */
#define CDROMPLAYTRKIND		0x5304 /* Play Audio Track/index 
                                           (struct cdrom_ti) */
#define CDROMREADTOCHDR		0x5305 /* Read TOC header 
                                           (struct cdrom_tochdr) */
#define CDROMREADTOCENTRY	0x5306 /* Read TOC entry 
                                           (struct cdrom_tocentry) */
#define CDROMSTOP		0x5307 /* Stop the cdrom drive */
#define CDROMSTART		0x5308 /* Start the cdrom drive */
#define CDROMEJECT		0x5309 /* Ejects the cdrom media */
#define CDROMVOLCTRL		0x530a /* Control output volume 
                                           (struct cdrom_volctrl) */
#define CDROMSUBCHNL		0x530b /* Read subchannel data 
                                           (struct cdrom_subchnl) */
#define CDROMREADMODE2		0x530c /* Read CDROM mode 2 data (2336 Bytes) 
                                           (struct cdrom_read) */
#define CDROMREADMODE1		0x530d /* Read CDROM mode 1 data (2048 Bytes)
                                           (struct cdrom_read) */
#define CDROMREADAUDIO		0x530e /* (struct cdrom_read_audio) */
#define CDROMEJECT_SW		0x530f /* enable(1)/disable(0) auto-ejecting */
#define CDROMMULTISESSION	0x5310 /* Obtain the start-of-last-session 
                                           address of multi session disks 
                                           (struct cdrom_multisession) */
#define CDROM_GET_MCN		0x5311 /* Obtain the "Universal Product Code" 
                                           if available (struct cdrom_mcn) */
#define CDROM_GET_UPC		CDROM_GET_MCN  /* This one is depricated, 
                                          but here anyway for compatability */
#define CDROMRESET		0x5312 /* hard-reset the drive */
#define CDROMVOLREAD		0x5313 /* Get the drive's volume setting 
                                          (struct cdrom_volctrl) */
#define CDROMREADRAW		0x5314	/* read data in raw mode (2352 Bytes)




/* Address in MSF format */
struct cdrom_msf0		
{
	BYTE	minute;
	BYTE	second;
	BYTE	frame;
};

/* Address in either MSF or logical format */
union cdrom_addr		
{
	struct cdrom_msf0	msf;
	int			lba;
};

/* This struct is used by the CDROMPLAYMSF ioctl */ 
struct cdrom_msf 
{
	BYTE	cdmsf_min0;	/* start minute */
	BYTE	cdmsf_sec0;	/* start second */
	BYTE	cdmsf_frame0;	/* start frame */
	BYTE	cdmsf_min1;	/* end minute */
	BYTE	cdmsf_sec1;	/* end second */
	BYTE	cdmsf_frame1;	/* end frame */
};

/* This struct is used by the CDROMPLAYTRKIND ioctl */
struct cdrom_ti 
{
	BYTE	cdti_trk0;	/* start track */
	BYTE	cdti_ind0;	/* start index */
	BYTE	cdti_trk1;	/* end track */
	BYTE	cdti_ind1;	/* end index */
};

/* This struct is used by the CDROMREADTOCHDR ioctl */
struct cdrom_tochdr 	
{
	BYTE	cdth_trk0;	/* start track */
	BYTE	cdth_trk1;	/* end track */
};

/* This struct is used by the CDROMVOLCTRL and CDROMVOLREAD ioctls */
struct cdrom_volctrl
{
	BYTE	channel0;
	BYTE	channel1;
	BYTE	channel2;
	BYTE	channel3;
};

/* This struct is used by the CDROMSUBCHNL ioctl */
struct cdrom_subchnl 
{
	BYTE	cdsc_format;
	BYTE	cdsc_audiostatus;
	BYTE	cdsc_adr:	4;
	BYTE	cdsc_ctrl:	4;
	BYTE	cdsc_trk;
	BYTE	cdsc_ind;
	union cdrom_addr cdsc_absaddr;
	union cdrom_addr cdsc_reladdr;
};


/* This struct is used by the CDROMREADTOCENTRY ioctl */
struct cdrom_tocentry 
{
	BYTE	cdte_track;
	BYTE	cdte_adr	:4;
	BYTE	cdte_ctrl	:4;
	BYTE	cdte_format;
	union cdrom_addr cdte_addr;
	BYTE	cdte_datamode;
};

/* This struct is used by the CDROMREADMODE1, and CDROMREADMODE2 ioctls */
struct cdrom_read      
{
	int	cdread_lba;
	char 	*cdread_bufaddr;
	int	cdread_buflen;
};

/* This struct is used by the CDROMREADAUDIO ioctl */
struct cdrom_read_audio
{
	union cdrom_addr addr; /* frame address */
	BYTE addr_format;    /* CDROM_LBA or CDROM_MSF */
	int nframes;           /* number of 2352-byte-frames to read at once */
	BYTE *buf;           /* frame buffer (size: nframes*2352 bytes) */
};


#define CD_MINS              74 /* max. minutes per CD, not really a limit */
#define CD_SECS              60 /* seconds per minute */
#define CD_FRAMES            75 /* frames per second */
#define CD_SYNC_SIZE         12 /* 12 sync bytes per raw data frame */
#define CD_MSF_OFFSET       150 /* MSF numbering offset of first frame */
#define CD_CHUNK_SIZE        24 /* lowest-level "data bytes piece" */
#define CD_NUM_OF_CHUNKS     98 /* chunks per frame */
#define CD_FRAMESIZE_SUB     96 /* subchannel data "frame" size */
#define CD_HEAD_SIZE          4 /* header (address) bytes per raw data frame */
#define CD_SUBHEAD_SIZE       8 /* subheader bytes per raw XA data frame */
#define CD_EDC_SIZE           4 /* bytes EDC per most raw data frame types */
#define CD_ZERO_SIZE          8 /* bytes zero per yellow book mode 1 frame */
#define CD_ECC_SIZE         276 /* bytes ECC per most raw data frame types */
#define CD_FRAMESIZE       2048 /* bytes per frame, "cooked" mode */
#define CD_FRAMESIZE_RAW   2352 /* bytes per frame, "raw" mode */
#define CD_FRAMESIZE_RAWER 2646 /* The maximum possible returned bytes */ 
/* most drives don't deliver everything: */
#define CD_FRAMESIZE_RAW1 (CD_FRAMESIZE_RAW-CD_SYNC_SIZE) /*2340*/
#define CD_FRAMESIZE_RAW0 (CD_FRAMESIZE_RAW-CD_SYNC_SIZE-CD_HEAD_SIZE) /*2336*/

#define CD_XA_HEAD        (CD_HEAD_SIZE+CD_SUBHEAD_SIZE) /* "before data" part of raw XA frame */
#define CD_XA_TAIL        (CD_EDC_SIZE+CD_ECC_SIZE) /* "after data" part of raw XA frame */
#define CD_XA_SYNC_HEAD   (CD_SYNC_SIZE+CD_XA_HEAD) /* sync bytes + header of XA frame */

/* CD-ROM address types (cdrom_tocentry.cdte_format) */
#define	CDROM_LBA 0x01 /* "logical block": first frame is #0 */
#define	CDROM_MSF 0x02 /* "minute-second-frame": binary, not bcd here! */

/* bit to tell whether track is data or audio (cdrom_tocentry.cdte_ctrl) */
#define	CDROM_DATA_TRACK	0x04

/* The leadout track is always 0xAA, regardless of # of tracks on disc */
#define	CDROM_LEADOUT		0xAA

/* audio states (from SCSI-2, but seen with other drives, too) */
#define	CDROM_AUDIO_INVALID	0x00	/* audio status not supported */
#define	CDROM_AUDIO_PLAY	0x11	/* audio play operation in progress */
#define	CDROM_AUDIO_PAUSED	0x12	/* audio play operation paused */
#define	CDROM_AUDIO_COMPLETED	0x13	/* audio play successfully completed */
#define	CDROM_AUDIO_ERROR	0x14	/* audio play stopped due to error */
#define	CDROM_AUDIO_NO_STATUS	0x15	/* no current audio status to return */
