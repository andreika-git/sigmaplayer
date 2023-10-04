//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - win32 compatibility stuff...
 *	\file		win32-stuff.c
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

#include <stdio.h>
#undef _WINSOCKAPI_
#include "win32-stuff.h"
#define _WINSOCKAPI_ // no timeval
#pragma warning (disable : 4115) 
#include <windows.h>
#include <mmsystem.h>
#pragma warning (default : 4115) 
#include <time.h>

typedef void (*sigaction_type) (int, siginfo_t *, void *);

static HANDLE sigathr = NULL;
static DWORD sigathr_id = 0;
static sigaction_type sa_siga = NULL;


DWORD WINAPI SigThreadProc(LPVOID lpParameter)
{
	// wait 10 sec and call sigaction
	Sleep(10000);
	if (sa_siga != NULL)
	{
		siginfo_t sigi;
		memset(&sigi, 0, sizeof(sigi));
		sigi.si_pid = (int)lpParameter;
		sa_siga(SIGCHLD, &sigi, 0);
	}
	return 0;
}


#undef _WINSOCKAPI_

#pragma comment(lib, "winmm.lib")

#pragma warning (disable : 4100) 
#pragma warning (disable : 4054) 

extern int fcntl (int __fd, int __cmd, ...)
{
	return 0;
}

void usleep(unsigned int usecs)
{
	Sleep(usecs / 1000);
}

unsigned int sleep (unsigned int __seconds)
{
	Sleep(__seconds);
	return 0;
}

unsigned int alarm (unsigned int seconds)
{
       struct itimerval old, new;
       new.it_interval.tv_usec = 0;
       new.it_interval.tv_sec = 0;
       new.it_value.tv_usec = 0;
       new.it_value.tv_sec = (long int) seconds;
       if (setitimer (ITIMER_REAL, &new, &old) < 0)
         return 0;
       else
         return (unsigned int)old.it_value.tv_sec;
}

int kill (__pid_t __pid, int __sig)
{
	//if (__pid == 1)
	{
		TerminateThread(sigathr, 0);
		sigathr = NULL;
	}
	return 0;
}
__pid_t vfork (void)
{
	int pid = 1;
	// we currently won't allow more than 1 childs...
	if (sigathr != NULL)
	{
		TerminateThread(sigathr, 0);
		sigathr = NULL;
	}
	
	sigathr = CreateThread(NULL, 0, SigThreadProc, (void *)pid,
			0, &sigathr_id);
	return pid;
}

__pid_t waitpid (__pid_t __pid, int *__stat_loc, int __options)
{
	return 1;
}

int poll (struct pollfd *__fds, nfds_t __nfds, int __timeout)
{
	usleep(10000);
	return 0;
}

int openpty (int *__amaster, int *__aslave, char *__name,
		    struct termios *__termp, struct winsize *__winp)
{
	*__amaster = 0;
	*__aslave = 0;
	if (__name != NULL)
		*__name = '\0';
	return 0;
}

int tcgetattr (int __fd, struct termios *__termios_p)
{
	return 0;
}

int tcsetattr (int __fd, int __optional_actions,
		      struct termios *__termios_p)
{
	return -1;
}

int setitimer (enum __itimer_which __which, const struct itimerval *__new, struct itimerval *__old)
{
	return 0;
}

int sigaction (int __sig, struct sigaction *__act, struct sigaction *__oact)
{
	if (__act != NULL)
	{
		sa_siga = __act->sa_sigaction;
	}
	return 0;
}

int gettimeofday (struct timeval *__tv, struct timezone *__tz)
{
	DWORD ms = timeGetTime();
	__tv->tv_sec = ms / 1000;
	__tv->tv_usec = (ms % 1000) * 1000;
	return 0;
}


void *alloca (unsigned  __size)
{
	return _alloca(__size);
}

int fsync(int fd)
{
	return 0;
}

int fchdir (int __fd)
{
	return 0;
}

#ifndef _WINSOCKAPI_

int select (int __nfds, fd_set *__readfds,
			fd_set *__writefds, fd_set *__exceptfds,
			struct timeval *__timeout)
{
	return 0;
}

#endif

#if 0
void *mmap (void *__addr, int __len, int __prot,
		   int __flags, int __fd, int __offset)
{
	return malloc(__len);
}

int munmap (void *__addr, int __len)
{
	free(__addr);
	return 0;
}
#endif

int sysinfo (struct sysinfo *__info)
{
	if (__info != NULL)
	{
		MEMORYSTATUS mstat;
		memset(__info, 0, sizeof(struct sysinfo));
		
  		GlobalMemoryStatus (&mstat);

		__info->uptime = clock();
		__info->totalram = mstat.dwTotalPhys;
		__info->freeram = mstat.dwAvailPhys;
		__info->procs = 1;
	}
	return 0;
}

int ioctl (int __fd, unsigned long int __request, ...)
{
	return 0;
}

int ftruncate(int fd, off_t length)
{
	return 0;
}

__int64 strtoll (const char *__nptr, char ** __endptr, int __base)
{
	return (__int64)strtol(__nptr, __endptr, __base);
}

unsigned __int64 strtoull (const char * __nptr, char **__endptr, int __base)
{
	return (unsigned __int64)strtoul(__nptr, __endptr, __base);
}
