//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - CDROM driver's functions source file.
 * 								For Technosonic-compatible players ('MP')
 *	\file		sp_cdrom.cpp
 *	\author		bombur
 *	\version	0.1
 *	\date		4.05.2004
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



//#define USE_UCLINUX_26

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/time.h>

#ifndef __USE_LARGEFILE64
#define __USE_LARGEFILE64
#endif

#include <sys/stat.h>

#include <linux/cdrom.h>
#include <linux/hdreg.h>

#include "sp_misc.h"
#include "sp_msg.h"
#include "sp_cdrom.h"

/// CD-ROM fs mount flag
static BOOL cdrom_cd_mounted = FALSE, cdrom_hdd_mounted = FALSE, cdrom_cd_inserted = FALSE;
static BOOL cdrom_cd = FALSE, cdrom_hdd = FALSE, cdrom_hdd_root = FALSE;
static int  cdrom_mount_tries = 0;
/// CD-ROM fs mount language
static const char *def_cdrom_language = "iso8859-1";
static char *cdrom_language = NULL;
static char cdrom_last_letter = 'c';
static CDROM_STATUS cdrom_hdd_status = CDROM_STATUS_HAS_ISO;

/// Internal function used by cdrom_getstatus()
static int cdrom_getmediumtype();

#if 0
static void cdrom_count_tracks();
#endif

/// OS-specific path to CD-ROM. Used by cdrom_getdevicepath()
static const char *curdvdpath = "/dev/cdroms/cdrom0";

/// CD-ROM device handle
int cdrom_handle = -1;
int cdrom_hdd_handle[2] = { -1, -1 };

/// Used by cdrom_getmediumtype().
/// Taken from linux/cdrom.h (kernel mode)

#ifndef USE_UCLINUX_26
struct mode_page_header 
{
	WORD mode_data_length;
	BYTE medium_type;
	BYTE reserved1;
	BYTE reserved2;
	BYTE reserved3;
	WORD desc_length;
};
#endif


/// Used by cdrom_getmediumtype().
/// Taken from drivers/ide/ide-cd.h (uClinux source)
struct atapi_capabilities_page 
{
	struct mode_page_header header;
	BYTE caps[20];
};


BOOL cdrom_init()
{
	cdrom_deinit();

	cdrom_language = SPstrdup(def_cdrom_language);
	cdrom_hdd_mounted = FALSE;
	cdrom_cd_mounted = FALSE;
	cdrom_mount_tries = 0;
	cdrom_cd_inserted = FALSE;

	// try CD-ROM
	cdrom_handle = open("/dev/cdroms/cdrom0", O_NONBLOCK);
	if (cdrom_handle >= 0)
	{
		cdrom_cd = TRUE;
		printf("cdrom: CD-ROM Detected!\n");
	}
	// try HDD
	for (int i = 0; i < 2; i++)
	{
		char hdd_name[40];
		sprintf(hdd_name, "/dev/discs/disc%d/disc", i);
		cdrom_hdd_handle[i] = open(hdd_name, O_NONBLOCK);
		if (cdrom_hdd_handle[i] >= 0)
		{
			cdrom_hdd = TRUE;
			printf("cdrom: HDD-%d Detected!\n", i+1);
		}
	}
	
	return (cdrom_handle != -1 || cdrom_hdd_handle[0] != -1 || cdrom_hdd_handle[1] != -1);
}

BOOL cdrom_switch(BOOL on)
{
	if (cdrom_hdd)
	{
		BYTE args1[4], args2[4];
		int r1, r2 = 0;

		for (int i = 0; i < 2; i++)
		{
			if (on)
			{
				static const BYTE on_args1[4] = { WIN_SETIDLE1, 0, 0, 0 };
				static const BYTE on_args2[4] = { WIN_SETIDLE2, 0, 0, 0 };

				memcpy(args1, on_args1, 4);
				memcpy(args2, on_args2, 4);
			} else
			{
				static const BYTE off_args1[4] = { WIN_STANDBYNOW1, 0, 0, 0 };
				static const BYTE off_args2[4] = { WIN_STANDBYNOW2, 0, 0, 0 };

				memcpy(args1, off_args1, 4);
				memcpy(args2, off_args2, 4);
			}

			if (cdrom_hdd_handle[i] == -1)
				continue;
			r1 = ioctl(cdrom_hdd_handle[i], HDIO_DRIVE_CMD, &args1);
			if (r1 != 0)
				r2 = ioctl(cdrom_hdd_handle[i], HDIO_DRIVE_CMD, &args2);

			printf("HDD-%d: Switch %s (result=%d,%d)\n", i+1, (on ? "On" : "Off"), r1, r2);

			// now check the results...
			BYTE cp_args1[4] = { WIN_CHECKPOWERMODE1, 0, 0, 0 };
			BYTE cp_args2[4] = { WIN_CHECKPOWERMODE2, 0, 0, 0 };
	 		const char *state;
	        if (ioctl(cdrom_hdd_handle[i], HDIO_DRIVE_CMD, &cp_args1) && ioctl(cdrom_hdd_handle[i], HDIO_DRIVE_CMD, &cp_args2)) 
	        {
	        	if (errno != EIO || cp_args1[0] != 0 || cp_args1[1] != 0) 
					state = "Unknown";
				else
					state = "Sleeping";
			} else 
			{
				if (cp_args1[2] == 255 || cp_args2[2] == 255)
					state = "Active/Idle";
				else
					state = "Standby";
			}
			msg("HDD-%d: Drive state is: %s\n", i+1, state);
		}
	}
	if (cdrom_cd)
	{
		if (on)
			ioctl(cdrom_handle, CDROMSTART);
		else
			ioctl(cdrom_handle, CDROMSTOP);

		printf("CD: Switch %s\n", (on ? "On" : "Off"));
	}
	return TRUE;
}

BOOL cdrom_deinit()
{
	if (cdrom_handle != -1)
		close (cdrom_handle);
	if (cdrom_hdd_handle[0] != -1)
		close (cdrom_hdd_handle[0]);
	if (cdrom_hdd_handle[1] != -1)
		close (cdrom_hdd_handle[1]);
	cdrom_handle = -1;
	cdrom_hdd_handle[0] = cdrom_hdd_handle[1] = -1;
	SPSafeFree(cdrom_language);
	return TRUE;
}

int cdrom_eject(BOOL open)
{
	if (cdrom_cd)
	{
		struct timeval tv;
		struct timezone tz;
		int start_time, stop_time;

		cdrom_generic_command cmd;
		struct request_sense sense;
	
		cdrom_umount();
		
		// [bombur]: I wish it was true... :-(
		//return ioctl(cdrom_handle, CDROMEJECT, 0);

		// [bombur]: this is the exact code used in original init!
		memset(&cmd, 0, sizeof(cmd));
		cmd.cmd[0] = GPCMD_PREVENT_ALLOW_MEDIUM_REMOVAL;
		cmd.cmd[4] = 0; // unlock
		cmd.buffer = NULL;
		cmd.buflen = 0;
		cmd.sense = &sense;
		cmd.data_direction = CGC_DATA_READ;
		ioctl(cdrom_handle, CDROM_SEND_PACKET, &cmd);

		gettimeofday(&tv, &tz);
		start_time = tv.tv_sec * 1000 + (tv.tv_usec / 1000);

		cmd.cmd[0] = GPCMD_START_STOP_UNIT;
		cmd.cmd[1] = 1;
		cmd.cmd[4] = open ? 2 : 3;
		if (ioctl(cdrom_handle, CDROM_SEND_PACKET, &cmd))
			return -1;
		
		do
		{
			gui_update();
			usleep(10000);
			gettimeofday(&tv, &tz);
			stop_time = tv.tv_sec * 1000 + (tv.tv_usec / 1000);
		} while (stop_time - start_time < 2000);	// less than 2 seconds passed - wait!
			
		return 0;
	} 
	else if (cdrom_hdd)
	{
		// emulate 'ejecting' HDD - to enable 'Setup' mode possibility
		cdrom_hdd_status = open ? CDROM_STATUS_TRAYOPEN : CDROM_STATUS_HAS_ISO;
		return 0;
	}

	return -1;
}

int cdrom_mount(char *language, BOOL cd_only)
{
	int ret = 0;
	char iocharset[1024];
	if (language == NULL)
		language = cdrom_language;

	BOOL lang_changed = strcasecmp(cdrom_language, language) != 0;
	if (cdrom_cd && cdrom_cd_inserted)
	{
		if ((!cdrom_cd_mounted && cdrom_mount_tries < 1) || lang_changed)
		{
			int rmnt = ((cdrom_cd_mounted && lang_changed) ? MS_REMOUNT : 0);
			sprintf(iocharset, "iocharset=%s", language);
			ret = mount("/dev/cdroms/cdrom0", "/cdrom", "iso9660", MS_RDONLY | MS_NOSUID | MS_NODEV | rmnt, iocharset);
			msg("CD: mount ISO with %s(%d) = %d\n", iocharset, rmnt, ret < 0 ? -errno : ret);
			if (ret < 0)
			{
				ret = mount("/dev/cdroms/cdrom0", "/cdrom", "udf", MS_RDONLY | MS_NOSUID | MS_NODEV | rmnt, iocharset);
				msg("CD: mount UDF with %s(%d) = %d\n", iocharset, rmnt, ret < 0 ? -errno : ret);
				cdrom_mount_tries++;
			}
			if (ret >= 0)
			{
				cdrom_cd_mounted = TRUE;
				cdrom_mount_tries = 0;
			}
		}
//		cdrom_count_tracks();
	}

	if (cdrom_hdd && !cd_only)
	{
		if (!cdrom_hdd_mounted || lang_changed)
		{
			char tmpdir[256], tmpmnt[256];
			char dirname[40];
			const char *mntname = "/hdd/";
			char mnt_char = 'c';
			struct dirent *entry;
			//int rmnt = ((cdrom_hdd_mounted && lang_changed) ? MS_REMOUNT : 0);
			int rmnt = 0;
			for (int i = 0; i < 2; i++)
			{
				if (cdrom_hdd_handle[i] < 0)
					continue;
				sprintf(dirname, "/dev/discs/disc%d", i);
				DIR *dir = opendir(dirname);
				printf("Mounting HDD-%d...\n", i+1);
				while ((entry = readdir(dir)) != NULL)
				{
					if (memcmp(entry->d_name, "part", 4) == 0)
					{
						sprintf(tmpdir, "%s/%s", dirname, entry->d_name);
						sprintf(tmpmnt, "%s%c", mntname, mnt_char);
					
						if (lang_changed)
							umount(tmpmnt);

						printf("* Mounting %s AS FAT: ", tmpdir);fflush(stdout);
						sprintf(iocharset, "iocharset=%s", language);
						ret = mount(tmpdir, tmpmnt, "vfat", MS_RDONLY | MS_NOSUID | MS_NODEV | rmnt, iocharset);
						if (ret < 0)
						{
							printf("NO. Mounting AS NTFS: ");fflush(stdout);
							sprintf(iocharset, "nls=%s", language);
							ret = mount(tmpdir, tmpmnt, "ntfs", MS_RDONLY | MS_NOSUID | MS_NODEV | rmnt, iocharset);
						}
						printf(ret < 0 ? "FAILED\n" : "OK\n");fflush(stdout);
						if (ret >= 0)
						{
							mnt_char++;
							cdrom_hdd_mounted = TRUE;
						}
					}
				}
				cdrom_last_letter = mnt_char - 1;
				closedir(dir);

				msg("HDD-%d: mount with %s(%d) = %d\n", i+1, iocharset, rmnt, ret < 0 ? -errno : ret);
			}
		}
	}
	
	if (lang_changed)
	{
		SPSafeFree(cdrom_language);
		cdrom_language = SPstrdup(language);
	}
	
	return ret;
}

int cdrom_umount()
{
	msg("CD-ROM: umount.\n");
	if (cdrom_cd)
	{
		cdrom_cd_mounted = FALSE;
		cdrom_mount_tries = 0;
		return umount("/cdrom");
	}
	return FALSE;
}

BOOL cdrom_ismounted()
{
	return cdrom_cd ? cdrom_cd_mounted : cdrom_hdd_mounted;
}

static CDROM_STATUS cdrom_detect_dvd()
{
	char dvdpath2[1024];

	cdrom_cd_inserted = TRUE;

	cdrom_mount(NULL, TRUE);

	DIR *dir = opendir("/cdrom");
	struct dirent *entry;
	struct stat statbuf;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcasecmp(entry->d_name, "VIDEO_TS") == 0)
		{
			sprintf(dvdpath2, "/cdrom/%s", entry->d_name);
			if (stat(dvdpath2, &statbuf) >= 0) 
			{
				if (S_ISDIR(statbuf.st_mode))
				{
					closedir(dir);
					cdrom_umount();
					return CDROM_STATUS_HAS_DVD;
				}
			}
		}
	}
	closedir(dir);

	cdrom_umount();
	return CDROM_STATUS_HAS_ISO;
}

CDROM_STATUS cdrom_getstatus(BOOL *force_update)
{
	static int old_medium = -1;
	static CDROM_STATUS old_status = CDROM_STATUS_UNKNOWN;

	if (!cdrom_cd && cdrom_hdd)
		return cdrom_hdd_status;

	if (cdrom_handle != -1)
	{
		int medium = cdrom_getmediumtype(), auxtype;

		if (medium != old_medium)
		{
			*force_update = TRUE;
			msg("Drive: medium type = %d.\n", medium);
		}

		switch (medium)
		{
		case 112:
			old_status = cdrom_hdd ? cdrom_hdd_status : CDROM_STATUS_NODISC;
			cdrom_cd_inserted = FALSE;
			cdrom_mount_tries = 0;
			old_medium = medium;
			return old_status;
		case 113:
			old_status = CDROM_STATUS_TRAYOPEN;
			cdrom_cd_inserted = FALSE;
			cdrom_mount_tries = 0;
			old_medium = medium;
			return old_status;
		case 1:
		case 4:
		case 5:
		case 8:
		case 0x11:
		case 0x14:
		case 0x15:
		case 0x18:
		case 0x21:
		case 0x24:
		case 0x25:
		case 0x28:
			// [bombur]: mixed discs are not always recognized by mediumtype
			auxtype = ioctl(cdrom_handle, CDROM_DISC_STATUS, CDSL_CURRENT);
			old_status = (auxtype == CDS_MIXED) ? CDROM_STATUS_HAS_MIXED : CDROM_STATUS_HAS_ISO;
			old_medium = medium;
			cdrom_cd_inserted = TRUE;
			return old_status;
		case 2:
		case 6:
		case 0x12:
		case 0x16:
		case 0x22:
		case 0x26:
			old_medium = medium;
			old_status = cdrom_hdd ? CDROM_STATUS_HAS_MIXED : CDROM_STATUS_HAS_AUDIO;
			cdrom_cd_inserted = TRUE;
			return old_status;
		case 3:
		case 7:
		case 0x13:
		case 0x17:
		case 0x23:
		case 0x27:
			old_medium = medium;
			old_status = CDROM_STATUS_HAS_MIXED;
			cdrom_cd_inserted = TRUE;
			return old_status;
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47:
		case 0x48:
			if (old_medium < 0x40 || old_medium > 0x48 || old_status == CDROM_STATUS_UNKNOWN)
				old_status = cdrom_detect_dvd();
			old_medium = medium;
			return old_status;
		case 0:
			old_medium = medium;
			break;
		default:
			if (medium != old_medium)
				msg("Drive: Unknown medium type = %d.\n", medium);
			old_medium = medium;
			break;
		}
	}
	return CDROM_STATUS_UNKNOWN;
}   

BOOL cdrom_isready()
{
	// ready or not, we failed
	if (cdrom_handle == -1)
		return TRUE;
	if (cdrom_hdd && !cdrom_cd)
		return TRUE;

	int drive = ioctl(cdrom_handle, CDROM_DRIVE_STATUS, CDSL_CURRENT);
	int disc = ioctl(cdrom_handle, CDROM_DISC_STATUS, CDSL_CURRENT);
	
//	msg("CD-ROM: drive.status = %d disc.status = %d.\n", drive, disc);
//	gui_update();

	if (/*drive == CDS_TRAY_OPEN || */drive == CDS_DRIVE_NOT_READY ||
		/*disc == CDS_TRAY_OPEN || */disc == CDS_DRIVE_NOT_READY)
		return FALSE;
	return TRUE;
}

/// Internal function used by cdrom_getstatus()
/// [bombur]: this was die hard!
int cdrom_getmediumtype()
{
	cdrom_generic_command cmd;
	struct request_sense sense;
	struct atapi_capabilities_page caps;

	if (cdrom_handle != -1)
	{
		memset(&cmd, 0, sizeof(cmd));
		cmd.buffer = (BYTE *)&caps;
		cmd.buflen = sizeof(caps);
		cmd.cmd[0] = GPCMD_MODE_SENSE_10;
		cmd.cmd[2] = GPMODE_CAPABILITIES_PAGE;
		cmd.cmd[8] = cmd.buflen & 0xff;
		cmd.sense = &sense;
		cmd.data_direction = CGC_DATA_READ;
		ioctl(cdrom_handle, CDROM_SEND_PACKET, &cmd);

		return caps.header.medium_type; 
	}
	return 0;
}

const char *cdrom_getdevicepath(const char *src_path)
{
	if (src_path != NULL)
		return src_path;
	return curdvdpath;
}

int cdrom_stat(const char *path, struct stat64 *s)
{
	return stat64(path, s);
}

DIR *cdrom_opendir(const char *path)
{
	cdrom_hdd_root = FALSE;
	if (cdrom_hdd)
	{
		if (strcasecmp(path, "/hdd/") == 0)
			cdrom_hdd_root = TRUE;
	}
	return opendir(path);
}

struct dirent *cdrom_readdir(DIR *dir)
{
	struct dirent *d = readdir(dir);
	// fast mount-check (exclude unmounted folders)
	if (cdrom_hdd_root && d->d_name[0] > cdrom_last_letter && d->d_name[1] == '\0')
		return NULL;
	return d;
}

int cdrom_closedir(DIR *dir)
{
	return closedir(dir);
}

int cdrom_open(const char *fname, int flags)
{
	return open(fname, flags | O_LARGEFILE);
}

char *cdrom_getrealpath(const char *path)
{
	static char tmpp[4096];
msg("CD-ROM: GET_REAL_PATH (%s) = %d %d %d\n", path, cdrom_hdd, cdrom_cd, cdrom_cd_inserted);
	if (memcmp(path, "/", 2) == 0)
	{
		if (cdrom_hdd && cdrom_cd && cdrom_cd_inserted)
			return (char *)path;
		else if (cdrom_hdd)
			strcpy(tmpp, "/hdd");
		else
			strcpy(tmpp, "/cdrom");
		strcat(tmpp, path);
		return tmpp;
	}
	return (char *)path;
}


#if 0
void cdrom_count_tracks()
{
	struct cdrom_tochdr header;
	struct cdrom_tocentry entry;
	int ret, i;
/*
	tracks->data=0;
	tracks->audio=0;
	tracks->cdi=0;
	tracks->xa=0;
	tracks->error=0;
*/
	/* Grab the TOC header so we can see how many tracks there are */
	if ((ret = ioctl(cdrom_handle, CDROMREADTOCHDR, &header))) 
	{
		if (ret == -ENOMEDIUM)
			msg("cdrom_count_tracks: CDS_NO_DISC!");
		else
			msg("cdrom_count_tracks: CDS_NO_INFO!");
		return;
	}	
	/* check what type of tracks are on this disc */
	entry.cdte_format = CDROM_MSF;
	msg("Starting %d-%d:\n", header.cdth_trk0, header.cdth_trk1);
	for (i = header.cdth_trk0; i <= header.cdth_trk1; i++) 
	{
		entry.cdte_track  = i;
		if (ioctl(cdrom_handle, CDROMREADTOCENTRY, &entry)) 
		{
			msg("track %2d: CDS_NO_INFO\n");
			continue;
		}	
		/*
		if (entry.cdte_ctrl & CDROM_DATA_TRACK) 
		{
		    if (entry.cdte_format == 0x10)
			tracks->cdi++;
		    else if (entry.cdte_format == 0x20) 
			tracks->xa++;
		    else
			tracks->data++;
		} else
		    tracks->audio++;
		*/
		msg("track %2d: fmt=0x%x, ctrl=0x%x\n", i, entry.cdte_format, entry.cdte_ctrl);
	}	
	/*
	cdinfo(CD_COUNT_TRACKS, "disc has %d tracks: %d=audio %d=data %d=Cd-I %d=XA\n", 
		header.cdth_trk1, tracks->audio, tracks->data, 
		tracks->cdi, tracks->xa);
	*/
	usleep(1000000);
}	
#endif
