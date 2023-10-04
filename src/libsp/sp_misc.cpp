//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - misc. functions source file
 *  \file       sp_misc.cpp
 *  \author     bombur
 *  \version    0.2
 *  \date       10.10.2006 4.05.2004
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
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "sp_misc.h"

BOOL module_apply(const char *modname)
{
    static const char *envps[] = { "HOME=/", "TERM=linux", NULL };
    static const char *args1[] = { "busybox", "insmod", NULL, NULL };
    static const char *args2[] = { "minimod", NULL, NULL };

    args1[2] = modname;
    args2[1] = modname;

    int pid = vfork();
    if (pid == 0)
    {
        if (errno = 0, !(execve ("/bin/busybox", (char * const *)args1, (char * const *)envps)+1))
        	if (errno = 0, !(execve ("/bin/minimod.bin", (char * const *)args2, (char * const *)envps)+1))
	            _exit(1); 
        _exit(0); // for safety
    }
	
	int pstatus;
	waitpid(pid, &pstatus, 0);
	return WIFSIGNALED(pstatus) == 0;
}

int exec_file(const char *binname, const char **pargs)
{
    static char *envps[] = { NULL };
    static char *args[] = { NULL, NULL };
    static char **ar = NULL;
    if (pargs == NULL)
    	ar = args;
	else
		ar = (char **)pargs;
    if (binname[0] != '/')
    	ar[0] = (char *)binname;
    else
    	ar[0] = (char *)binname + 1;

	//msg("EXECVE %s\n", binname);
	//for (int i = 0; ar[i]; i++)
	//	msg("  ARG%d = %s\n", i, ar[i]);

   	int pid = vfork();
    if (pid == 0)
    {
		if (errno = 0, !(execve (binname, ar, envps)+1))
       		_exit(1); 
	}
	return pid;
}

// patch for old UCLIBC
#ifdef __UCLIBC_HAS_THREADS__
extern "C"
{
pid_t __libc_fork(void)
{
	errno = ENOSYS;
	return -1;
}
}
#endif
