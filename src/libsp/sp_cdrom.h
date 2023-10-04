//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - CDROM driver's interface header.
 *	\file		sp_cdrom.h
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

#ifndef SP_CDROM_H
#define SP_CDROM_H

#include <dirent.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////
/// Enums:

typedef enum 
{
	CDROM_STATUS_CURRENT = -1,

	CDROM_STATUS_UNKNOWN = 0,
	CDROM_STATUS_NODISC,
	CDROM_STATUS_TRAYOPEN,
	
	CDROM_STATUS_HASDISC,

	CDROM_STATUS_HAS_ISO 	= CDROM_STATUS_HASDISC | (1 << 4),
	CDROM_STATUS_HAS_DVD 	= CDROM_STATUS_HASDISC | (2 << 4),
	CDROM_STATUS_HAS_AUDIO 	= CDROM_STATUS_HASDISC | (3 << 4),
	CDROM_STATUS_HAS_MIXED 	= CDROM_STATUS_HASDISC | (4 << 4),
	
} CDROM_STATUS;

////////////////////////////////////
/// Functions:

/// Init (open) CDROM driver
BOOL cdrom_init();

/// Release CDROM driver
BOOL cdrom_deinit();

/// Get device path (set src_path to NULL to get root device path)
const char *cdrom_getdevicepath(const char *src_path);

/// Eject CD-ROM
int cdrom_eject(BOOL open);

/// Mount or remount CD-ROM (if language changed)
int cdrom_mount(char *language, BOOL cd_only);
/// Unmount CD-ROM
int cdrom_umount();
/// Return TRUE if CD-ROM is mounted
BOOL cdrom_ismounted();

/// Get CDROM status
CDROM_STATUS cdrom_getstatus(BOOL *force_update);

/// Return TRUE if CDROM is ready (use additional checks)
BOOL cdrom_isready();

/// Switch CDROM on/off
BOOL cdrom_switch(BOOL on);

int cdrom_stat(const char *, struct stat64 *);

DIR *cdrom_opendir(const char *);
struct dirent *cdrom_readdir(DIR *);
int cdrom_closedir(DIR *);

int cdrom_open(const char *fname, int flags);

char *cdrom_getrealpath(const char *);

#ifdef __cplusplus
}
#endif

#endif // of SP_CDROM_H
