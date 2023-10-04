//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - CDROM driver's functions source file (Win32)
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

#include <windows.h>

#pragma warning (disable : 4201) 
#include <winioctl.h>
#pragma warning (default : 4201) 

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

#include "sp_misc.h"
#include "sp_msg.h"
#include "sp_cdrom.h"

#include "resource.h"

int cdrom_handle = -1;

int cdrom_last_letter = 'z';

static int dvd_letter = 0;
static BOOL cdrom_mounted = FALSE;

/// CD-ROM fs mount language
static char *def_cdrom_language = "iso8859-1";
static char *cdrom_language = NULL;

static CDROM_STATUS cdrom_type = /*CDROM_STATUS_HAS_ISO; */CDROM_STATUS_NODISC;

static int cdrom_getmediumtype();

static char dvdpath[3], dvdpath2[1024];
static const char *curdvdpath = (const char *)dvdpath;

static BOOL cdrom_cd_hdd = FALSE, cdrom_hdd = FALSE, cdrom_hdd_root = FALSE;
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
char *CDROMDIR = "cdrom";
//char *CDROMDIR = dvdpath;
extern int khwl_handle;

// taken from linux/cdrom.h (kernel mode)
struct mode_page_header 
{
	WORD mode_data_length;
	BYTE medium_type;
	BYTE reserved1;
	BYTE reserved2;
	BYTE reserved3;
	WORD desc_length;
};

struct atapi_capabilities_page 
{
	struct mode_page_header header;
	BYTE caps[20];
};


BOOL cdrom_init()
{
	cdrom_deinit();
	cdrom_language = SPstrdup(def_cdrom_language);

	char tmp[256];
	DWORD mask = GetLogicalDrives();
	for (char letter = 'A'; letter <= 'Z'; letter++)
	{
		if ((mask & (1 << (letter - 'A'))) != 0)
		{
			sprintf( tmp, "%c:\\", letter );
			if (GetDriveType(tmp) == DRIVE_CDROM)
			{
				dvd_letter = letter;
				break;
			}
		}
	}

	if (dvd_letter != 0)
	{
		dvdpath[0] = (char)dvd_letter;
		dvdpath[1] = ':';
		dvdpath[2] = '\0';
	} else
		curdvdpath = NULL;

	// fake for sure...
	cdrom_handle = 1;

	return TRUE;
}

BOOL cdrom_deinit()
{
	if (cdrom_handle == -1)
		return FALSE;
//	close (cdrom_handle);
	cdrom_handle = -1;
	SPSafeFree(cdrom_language);
	return TRUE;
}

BOOL cdrom_switch(BOOL )
{
	return TRUE;
}

const char *cdrom_getdevicepath(const char *src_path)
{
	if (src_path != NULL)
	{
		static char tmpp[2014];
		if (_strnicmp(src_path, "/cdrom", 6) == 0)	// real device!
			sprintf(tmpp, "%s%s", CDROMDIR, src_path + 6);
		else if (_strnicmp(src_path, "/hdd", 4) == 0)	// real device!
			sprintf(tmpp, "%s%s", CDROMDIR, src_path + 4);
		else
			return curdvdpath;
		return tmpp;
	}
	return curdvdpath;
}

BOOL CALLBACK eject_dlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int timer_id = 0;
	ULONG wID, code;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hwndDlg, IDC_RADIO1, BST_CHECKED);
		timer_id = SetTimer(hwndDlg, NULL, 10, NULL);
		return 0;

	case WM_CLOSE:
		KillTimer(hwndDlg, timer_id);
		EndDialog(hwndDlg, 0);
		break;
	case WM_COMMAND:
		wID = LOWORD(wParam);
		code = HIWORD(wParam);
		switch(wID)
		{
		case IDOK:
			if (IsDlgButtonChecked(hwndDlg, IDC_RADIO1) == BST_CHECKED)
				EndDialog(hwndDlg, 0);
			else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO2) == BST_CHECKED)
				EndDialog(hwndDlg, 1);
			else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO3) == BST_CHECKED)
				EndDialog(hwndDlg, 2);
			else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO4) == BST_CHECKED)
				EndDialog(hwndDlg, 3);
			break;
		case IDCANCEL:
			EndDialog(hwndDlg, -1);
			break;
		}
		break;
	// bigfix: strange XP-style behaviour...
	case WM_NCCALCSIZE:
		break;
	case WM_TIMER:
		gui_update();
		break;

	default:
		return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
	}
	return 0;
}

int cdrom_eject(BOOL open)
{
	if (open)
	{
		cdrom_handle = 1;
		gui_update();
		cdrom_hdd = FALSE;

		int ret = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EJECT), 
									(HWND)khwl_handle, eject_dlg);
		SetFocus((HWND)khwl_handle);
		if (ret == -1)
			cdrom_type = CDROM_STATUS_NODISC;
		else if (ret == 0)	// autodetect from real device
		{
			cdrom_type = CDROM_STATUS_NODISC;
			cdrom_handle = 2;

			::SetCursor(::LoadCursor(NULL, IDC_WAIT));
			
			if (dvd_letter != 0)
			{
				char tmp[256];
				sprintf(tmp, "\\\\.\\%c:", dvd_letter);

				HANDLE fd = CreateFile(tmp, GENERIC_READ | GENERIC_WRITE,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL, OPEN_EXISTING,
								FILE_FLAG_RANDOM_ACCESS, NULL);

				if (fd == INVALID_HANDLE_VALUE)
					fd = CreateFile(tmp, GENERIC_READ, FILE_SHARE_READ,
									NULL, OPEN_EXISTING,
									FILE_FLAG_RANDOM_ACCESS, NULL);
				if (fd != INVALID_HANDLE_VALUE)
				{
					struct MY_GET_MEDIA_TYPES
					{
						DWORD DeviceType;              // FILE_DEVICE_XXX values
						DWORD MediaInfoCount;
						DEVICE_MEDIA_INFO MediaInfo[256];
					} mt;
					int BytesReturned;
					if (DeviceIoControl(fd, IOCTL_STORAGE_GET_MEDIA_TYPES_EX, NULL, 0, 
						(LPVOID)&mt, (DWORD)sizeof(mt), (LPDWORD) &BytesReturned, NULL) != 0)
					{
						BOOL mounted = FALSE;
						for (DWORD i = 0; i < mt.MediaInfoCount; i++)
						{
							if ((mt.DeviceType == FILE_DEVICE_DVD || mt.DeviceType == FILE_DEVICE_CD_ROM) && 
								(mt.MediaInfo[i].DeviceSpecific.DiskInfo.MediaCharacteristics & MEDIA_CURRENTLY_MOUNTED)
								== MEDIA_CURRENTLY_MOUNTED)
							{
								mounted = TRUE;
								break;
							}
						}
						if (mounted)
						{
							MCI_OPEN_PARMS mciOpen;
							MCI_STATUS_PARMS mciParams;
							mciOpen.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
							mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID, (DWORD)&mciOpen);
							
							mciParams.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
							mciSendCommand(mciOpen.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&mciParams); 
							int numtr = mciParams.dwReturn;
							bool has_audio = false, has_data = false;
							for (int i = 0; i < numtr; i++)
							{
								mciParams.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
								mciParams.dwTrack = i + 1;
								mciSendCommand(mciOpen.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)&mciParams); 
								if (mciParams.dwReturn == MCI_CDA_TRACK_AUDIO)
									has_audio = true;
								else
									has_data = true;
							}
							mciSendCommand(mciOpen.wDeviceID, MCI_CLOSE, 0, 0);

							curdvdpath = dvdpath;
							CDROMDIR = dvdpath;

							DIR *dir = opendir(dvdpath);
							struct dirent *entry;
							while ((entry = readdir(dir)) != NULL)
							{
								if (_stricmp(entry->name, "VIDEO_TS") == 0)
								{
									cdrom_type = CDROM_STATUS_HAS_DVD;
									break;
								}
							}
							closedir(dir);
							// one more chance...
							if (cdrom_type != CDROM_STATUS_HAS_DVD)
							{
								dir = opendir(dvdpath);
								struct dirent *entry;
								while ((entry = readdir(dir)) != NULL)
								{
									sprintf(dvdpath2, "%s/%s", dvdpath, entry->name);
									DIR *dir2 = opendir(dvdpath2);
									struct dirent *entry2;
									while ((entry2 = readdir(dir2)) != NULL)
									{
										if (_stricmp(entry2->name, "VIDEO_TS") == 0)
										{
											cdrom_type = CDROM_STATUS_HAS_DVD;
											break;
										}
									}
									closedir(dir2);
									if (cdrom_type == CDROM_STATUS_HAS_DVD)
									{
										curdvdpath = dvdpath2;
										break;
									}
								}
								closedir(dir);
							}

							if (cdrom_type != CDROM_STATUS_HAS_DVD)
							{
								if (has_audio && !has_data)
									cdrom_type = CDROM_STATUS_HAS_AUDIO;
								else if (has_audio && has_data)
									cdrom_type = CDROM_STATUS_HAS_MIXED;
								else
									cdrom_type = CDROM_STATUS_HAS_ISO;
							}
						}
					} else
					{
						//DWORD err = GetLastError();
					}

					
					CloseHandle(fd);
				}
			}
			::SetCursor(::LoadCursor(NULL, IDC_ARROW));
		}
		else if (ret == 1)
		{
			cdrom_hdd = FALSE;
			CDROMDIR = "cdrom";
			cdrom_type = CDROM_STATUS_HAS_ISO;
		}
		else if (ret == 2)
		{
			cdrom_hdd = TRUE;
			CDROMDIR = "cdrom/hdd";
			cdrom_type = CDROM_STATUS_HAS_ISO;
		}
		else if (ret == 3)
		{
			cdrom_hdd = FALSE;
			curdvdpath = "cdrom/dvd";
			cdrom_type = CDROM_STATUS_HAS_DVD;
		}
	} else
	{
		gui_update();
	}
	
	return -1;
}


int cdrom_mount(char *language, BOOL /*cd_only*/)
{
	int ret = 0;
	char iocharset[1024];
	if (language == NULL)
		language = cdrom_language;

	BOOL lang_changed = strcasecmp(cdrom_language, language) != 0;
	if (!cdrom_mounted || lang_changed)
	{
		sprintf(iocharset, "iocharset=%s", language);
		//int rmnt = ((cdrom_mounted && lang_changed) ? MS_REMOUNT : 0);
		//ret = mount("/dev/cdroms/cdrom0", "/cdrom", "iso9660", MS_RDONLY | MS_NOSUID | MS_NODEV | rmnt, iocharset);

		msg("CD-ROM: mount with %s.\n", iocharset);
	}
	cdrom_mounted = TRUE;
	
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
	cdrom_mounted = FALSE;
	//return umount("/cdrom");
	return 0;
}

BOOL cdrom_ismounted()
{
	return cdrom_mounted;
}


CDROM_STATUS cdrom_getstatus(BOOL *)
{
	return cdrom_type;
}   

BOOL cdrom_isready()
{
	return TRUE;
}

// [bombur]: this was die hard!
int cdrom_getmediumtype()
{
/*
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
*/
	return 0;
}

int cdrom_stat(const char *path, struct stat64 *s)
{
	if (_strnicmp(path, "/cdrom", 6) == 0)	// real device!
	{
		char tmpp[2014];
		sprintf(tmpp, "%s%s", CDROMDIR, path + 6);
		return _stati64(tmpp, s);
	}
	if (_strnicmp(path, "/hdd", 4) == 0)	// real device!
	{
		char tmpp[2014];
		sprintf(tmpp, "%s%s", CDROMDIR, path + 4);
		return _stati64(tmpp, s);
	}
	if (path[0] == '/')
	{
		path++;
	}
	return _stati64(path, s);
}

DIR *cdrom_opendir(const char *path)
{
	cdrom_hdd_root = FALSE;
	if (_strnicmp(path, "/cdrom", 6) == 0)	// real device!
	{
		char tmpp[2014];
		sprintf(tmpp, "%s%s", CDROMDIR, path + 6);
		return opendir(tmpp);
	}
	if (_strnicmp(path, "/hdd", 4) == 0)	// real device!
	{
		char tmpp[2014];
		sprintf(tmpp, "%s%s", CDROMDIR, path + 4);
		if (path[4] == '/' && path[5] == '\0')
			cdrom_hdd_root = TRUE;
		return opendir(tmpp);
	}
	return opendir(path);
}

int cdrom_open(const char *fname, int flags)
{
	if (_strnicmp(fname, "/cdrom", 6) == 0)	// real device!
	{
		char tmpp[2014];
		sprintf(tmpp, "%s%s", CDROMDIR, fname + 6);
		return open(tmpp, flags | O_BINARY);
	}
	if (_strnicmp(fname, "/hdd", 4) == 0)	// real device!
	{
		char tmpp[2014];
		sprintf(tmpp, "%s%s", CDROMDIR, fname + 4);
		return open(tmpp, flags | O_BINARY);
	}
	return -1;
}

char *cdrom_getrealpath(const char *path)
{
	static char tmpp[4096];
	if (memcmp(path, "/", 2) == 0)
	{
		if (cdrom_cd_hdd)
			return (char *)path;
		if (cdrom_hdd)
			strcpy(tmpp, "/hdd");
		else
			strcpy(tmpp, "/cdrom");
		strcat(tmpp, path);
		return tmpp;
	}
	return (char *)path;
}

struct dirent *cdrom_readdir(DIR *dir)
{
	struct dirent *d = readdir(dir);
	if (d == NULL)
		return NULL;
	// fast mount-check (exclude unmounted folders)
	if (cdrom_hdd_root && d->name[0] > cdrom_last_letter && d->name[1] == '\0')
		return NULL;
	return d;
}

int cdrom_closedir(DIR *dir)
{
	return closedir(dir);
}