//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - Module system support source file.
 * 								For Technosonic-compatible players ('MP')
 *  \file       sp_module.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       10.12.2008
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <features.h>
#include <errno.h>
#include <linux/unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <libsp/sp_misc.h>

extern "C"
{

// portions of code from uClibc...
#ifdef COMPILE_MODULE

void (*__app_fini)(void) = NULL;

extern int  main(int argc, char **argv, char **envp);


void __attribute__ ((__noreturn__)) 
__uClibc_start_main(int argc, char **argv, char **envp,
	void (*app_init)(void), void (*app_fini)(void))
{
	/* Arrange for the application's dtors to run before we exit.  */
    __app_fini = app_fini;

    /* Run all the application's ctors now.  */
    if (app_init!=NULL) {
		app_init();
    }

    exit(main(argc, argv, envp));
}

void exit(int rv)
{
	if (__app_fini != NULL)
		(__app_fini)();

	_exit(rv);
}

void module_wait()
{
	kill(getpid(), SIGSTOP);
}

/* Copy SRC to DEST.  */
char *strcpy (char *dest, const char *src)
{
  char c;
  char *s = (char *) (src);
  const int off = (dest) - s - 1;

  do
    {
      c = *s++;
      s[off] = c;
    }
  while (c != '\0');

  return dest;
}

void abort(void)
{
	struct sigaction act;
	sigset_t sset;

	(void) fflush (NULL);

	if ((sigemptyset (&sset) == 0) && (sigaddset (&sset, SIGABRT) == 0)) {
   		(void) sigprocmask (SIG_UNBLOCK, &sset, (sigset_t *) NULL);
	}

    raise(SIGABRT);
    
    memset (&act, '\0', sizeof (struct sigaction));
	act.sa_handler = SIG_DFL;
	sigfillset (&act.sa_mask);
	act.sa_flags = 0;
	sigaction (SIGABRT, &act, NULL);

	raise (SIGABRT);

	_exit (127);
}

int atexit(void (*) (void))
{
	// empty
	return 0;
}


#undef LOAD_ARGS_1
#define LOAD_ARGS_1(a1)				\
  _a1 = (int) (a1);				
#undef ASM_ARGS_1
#define ASM_ARGS_1	, "r" (_a1)
#ifndef SYS_ify
# define SYS_ify(syscall_name)	(__NR_##syscall_name)
#endif

#undef INLINE_SYSCALL
#define INLINE_SYSCALL(name, nr, args...)			\
  ({ unsigned int _sys_result;					\
     {								\
       register int _a1 asm ("a1");				\
       LOAD_ARGS_##nr (args)					\
       asm volatile ("swi	%1	@ syscall " #name	\
		     : "=r" (_a1)				\
		     : "i" (SYS_ify(name)) ASM_ARGS_##nr	\
		     : "memory");				\
       _sys_result = _a1;					\
     }								\
     (int) _sys_result; })


void _exit(int status)
{
	INLINE_SYSCALL(exit, 1, status);
}


#else	///////////////////// server side

int module_binary_load(char *fname, char *arg)
{
	char *args[3];
	args[0] = fname;
	args[1] = arg;
	args[2] = NULL;
	
	return exec_file(args[0], (const char **)args);
}

int module_binary_unload(int pid)
{
	// kill it by force
	kill(pid, SIGKILL);
	int pstat;
	waitpid(pid, &pstat, 0);
	return 0;
}

void module_copy_func(void *from, void *to)
{
	#define JMP_OPCODE_SIZE 2
	DWORD diff = (((DWORD)from)/4 - (((DWORD)to)/4 + JMP_OPCODE_SIZE));
	*((DWORD *)to) = 0xEA000000 | (diff & 0xFFFFFF);
}

void module_clear_func(void *func)
{
	*((DWORD *)func) = 0;
}


#endif

}
