//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - win32-unix compatibility header.
 *	\file		win32-stuff.h
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


#ifndef __WIN32_STUFF__
#define __WIN32_STUFF__

#include <sys/types.h>
#include <process.h>
#include <setjmp.h>

/* Standard file descriptors.  */
#define	STDIN_FILENO	0	/* Standard input.  */
#define	STDOUT_FILENO	1	/* Standard output.  */
#define	STDERR_FILENO	2	/* Standard error output.  */

#define SIGKILL		 9
#define SIGALRM		14
#define	SIGCHLD		17	/* Child status has changed (POSIX).  */

#define	SA_NOCLDSTOP  1		 /* Don't send SIGCHLD when children stop.  */
#define SA_NOCLDWAIT  2		 /* Don't create zombie on child death.  */
#define SA_SIGINFO    4		 /* Invoke signal-catching function with three arguments instead of one.  */

#define WIFSIGNALED(status)  0

#define	WNOHANG		1	/* Don't block waiting.  */
#define	WUNTRACED	2	/* Report status of stopped children.  */

#define ONLCR	0000004
#define ECHO	0000010

#define	TCSANOW		0
#define	TCSADRAIN	1
#define	TCSAFLUSH	2

#define F_DUPFD		0	/* Duplicate file descriptor.  */
#define F_GETFD		1	/* Get file descriptor flags.  */
#define F_SETFD		2	/* Set file descriptor flags.  */
#define F_GETFL		3	/* Get file status flags.  */
#define F_SETFL		4	/* Set file status flags.  */

#define O_NONBLOCK	 04000

#define PROT_READ	0x1		/* Page can be read.  */
#define PROT_WRITE	0x2		/* Page can be written.  */
#define PROT_EXEC	0x4		/* Page can be executed.  */
#define PROT_NONE	0x0		/* Page can not be accessed.  */

#define MAP_SHARED	0x01		/* Share changes.  */
#define MAP_PRIVATE	0x02		/* Changes are private.  */
#define MAP_ANONYMOUS	0x20		/* Don't use a file.  */
#define MAP_ANON	MAP_ANONYMOUS

typedef int __pid_t;
typedef int __uid_t;

# ifndef __pid_t_defined
typedef __pid_t pid_t;
#  define __pid_t_defined
# endif
# ifndef __uid_t_defined
typedef __uid_t uid_t;
#  define __uid_t_defined
# endif


#ifdef	__cplusplus
extern "C"
{
#endif

typedef long int __clock_t;

// from inttypes.h
typedef __int64 ssize_t;

extern int fcntl (int __fd, int __cmd, ...);
extern void usleep(unsigned int);
extern unsigned int sleep (unsigned int __seconds);
extern unsigned int alarm (unsigned int __seconds);
extern int kill (__pid_t __pid, int __sig);
extern __pid_t waitpid (__pid_t __pid, int *__stat_loc, int __options);
extern __pid_t vfork (void);

extern void *mmap (void *__addr, int __len, int __prot, int __flags, int __fd, int __offset);
extern int munmap (void *__addr, int __len);

#ifndef lseek64
#define lseek64 _lseeki64
#endif

void *alloca (unsigned  __size);

extern int fsync(int);
extern int fchdir (int __fd);

enum __itimer_which
  {
    /* Timers run in real time.  */
    ITIMER_REAL = 0,
#define ITIMER_REAL ITIMER_REAL
    /* Timers run only when the process is executing.  */
    ITIMER_VIRTUAL = 1,
#define ITIMER_VIRTUAL ITIMER_VIRTUAL
    /* Timers run when the process is executing and when
       the system is executing on behalf of the process.  */
    ITIMER_PROF = 2
#define ITIMER_PROF ITIMER_PROF
  };

typedef long suseconds_t;

/////////////////////////////////////////////////////

#define POLLIN		0x001		/* There is data to read.  */
#define POLLPRI		0x002		/* There is urgent data to read.  */
#define POLLOUT		0x004		/* Writing now will not block.  */

struct pollfd
{
    int fd;			/* File descriptor to poll.  */
    short int events;		/* Types of events poller cares about.  */
    short int revents;		/* Types of events that actually occurred.  */
};

typedef unsigned long int nfds_t;

extern int poll (struct pollfd *__fds, nfds_t __nfds, int __timeout);

#ifndef NOTIMEVAL

struct timeval {
	time_t		tv_sec;		/* seconds */
	suseconds_t	tv_usec;	/* microseconds */
};

struct itimerval
  {
    /* Value to put into `it_value' when the timer expires.  */
    struct timeval it_interval;
    /* Time to the next timer expiration.  */
    struct timeval it_value;
  };

struct timezone
  {
    int tz_minuteswest;		/* Minutes west of GMT.  */
    int tz_dsttime;		/* Nonzero if DST is ever in effect.  */
  };

#endif

/* Set the timer WHICH to *NEW.  If OLD is not NULL,
   set *OLD to the old value of timer WHICH.
   Returns 0 on success, -1 on errors.  */
extern int setitimer (enum __itimer_which __which,
			   const struct itimerval *__new,
			   struct itimerval *__old);


extern int gettimeofday (struct timeval *__tv,
			      struct timezone *__tz);


///////////////////////////////////////////////////////////////

typedef unsigned char	cc_t;
typedef unsigned int	speed_t;
typedef unsigned int	tcflag_t;

#define NCCS 32
struct termios
  {
    tcflag_t c_iflag;		/* input mode flags */
    tcflag_t c_oflag;		/* output mode flags */
    tcflag_t c_cflag;		/* control mode flags */
    tcflag_t c_lflag;		/* local mode flags */
    cc_t c_line;			/* line discipline */
    cc_t c_cc[NCCS];		/* control characters */
    speed_t c_ispeed;		/* input speed */
    speed_t c_ospeed;		/* output speed */
  };

struct winsize
  {
    unsigned short int ws_row;
    unsigned short int ws_col;
    unsigned short int ws_xpixel;
    unsigned short int ws_ypixel;
  };

extern int openpty (int *__amaster, int *__aslave, char *__name,
		    struct termios *__termp, struct winsize *__winp);

extern int tcgetattr (int __fd, struct termios *__termios_p);

extern int tcsetattr (int __fd, int __optional_actions,
		      struct termios *__termios_p);

///////////////////////////////////////////////////////////////
typedef void (*__sighandler_t) (int);
# define _SIGSET_NWORDS	(1024 / (8 * sizeof (unsigned long int)))
typedef struct
  {
    unsigned long int __val[_SIGSET_NWORDS];
  } __sigset_t;

typedef union sigval
  {
    int sival_int;
    void *sival_ptr;
  } sigval_t;

#define __WORDSIZE	32
# define __SI_MAX_SIZE     128
# if __WORDSIZE == 64
#  define __SI_PAD_SIZE     ((__SI_MAX_SIZE / sizeof (int)) - 4)
# else
#  define __SI_PAD_SIZE     ((__SI_MAX_SIZE / sizeof (int)) - 3)
# endif

typedef struct siginfo
  {
    int si_signo;		/* Signal number.  */
    int si_errno;		/* If non-zero, an errno value associated with
				   this signal, as defined in <errno.h>.  */
    int si_code;		/* Signal code.  */

    union
      {
	int _pad[__SI_PAD_SIZE];

	 /* kill().  */
	struct
	  {
	    __pid_t si_pid;	/* Sending process ID.  */
	    __uid_t si_uid;	/* Real user ID of sending process.  */
	  } _kill;

	/* POSIX.1b timers.  */
	struct
	  {
	    unsigned int _timer1;
	    unsigned int _timer2;
	  } _timer;

	/* POSIX.1b signals.  */
	struct
	  {
	    __pid_t si_pid;	/* Sending process ID.  */
	    __uid_t si_uid;	/* Real user ID of sending process.  */
	    sigval_t si_sigval;	/* Signal value.  */
	  } _rt;

	/* SIGCHLD.  */
	struct
	  {
	    __pid_t si_pid;	/* Which child.  */
	    __uid_t si_uid;	/* Real user ID of sending process.  */
	    int si_status;	/* Exit value or signal.  */
	    __clock_t si_utime;
	    __clock_t si_stime;
	  } _sigchld;

	/* SIGILL, SIGFPE, SIGSEGV, SIGBUS.  */
	struct
	  {
	    void *si_addr;	/* Faulting insn/memory ref.  */
	  } _sigfault;

	/* SIGPOLL.  */
	struct
	  {
	    long int si_band;	/* Band event for SIGPOLL.  */
	    int si_fd;
	  } _sigpoll;
      } _sifields;
  } siginfo_t;

struct sigaction
  {
    /* Signal handler.  */
	union
	{
    __sighandler_t sa_handler;
	void (*sa_sigaction) (int, siginfo_t *, void *);
	} __sigaction_handler;
	# define sa_handler	__sigaction_handler.sa_handler
	# define sa_sigaction	__sigaction_handler.sa_sigaction

    /* Additional set of signals to be blocked.  */
    __sigset_t sa_mask;

    /* Special flags.  */
    int sa_flags;

    /* Restore handler.  */
    void (*sa_restorer) (void);
  };

# define si_pid		_sifields._kill.si_pid

typedef jmp_buf sigjmp_buf;

extern int sigaction (int __sig, struct sigaction *__act, struct sigaction *__oact);

///////////////////////////////////////////////////////////////

struct sysinfo {
	long uptime;			/* Seconds since boot */
	unsigned long loads[3];		/* 1, 5, and 15 minute load averages */
	unsigned long totalram;		/* Total usable main memory size */
	unsigned long freeram;		/* Available memory size */
	unsigned long sharedram;	/* Amount of shared memory */
	unsigned long bufferram;	/* Memory used by buffers */
	unsigned long totalswap;	/* Total swap space size */
	unsigned long freeswap;		/* swap space still available */
	unsigned short procs;		/* Number of current processes */
	unsigned short pad;			/* Padding needed for m68k */
	unsigned long totalhigh;	/* Total high memory size */
	unsigned long freehigh;		/* Available high memory size */
	unsigned int mem_unit;		/* Memory unit size in bytes */
	char _f[20-2*sizeof(long)-sizeof(int)];	/* Padding: libc5 uses this.. */
};

extern int sysinfo (struct sysinfo *__info);

///////////////////////////////////////////////////////////////

#ifndef _WINSOCKAPI_

#undef __NFDBITS
#define __NFDBITS	(8 * sizeof(unsigned long))

#undef __FD_SETSIZE
#define __FD_SETSIZE	1024

#undef __FDSET_LONGS
#define __FDSET_LONGS	(__FD_SETSIZE/__NFDBITS)

typedef struct {
	unsigned long fds_bits [__FDSET_LONGS];
} fd_set;

#undef	__FD_SET
#define __FD_SET(fd, fdsetp) \
		(((fd_set *)fdsetp)->fds_bits[fd >> 5] |= (1<<(fd & 31)))

#undef	__FD_CLR
#define __FD_CLR(fd, fdsetp) \
		(((fd_set *)fdsetp)->fds_bits[fd >> 5] &= ~(1<<(fd & 31)))

#undef	__FD_ISSET
#define __FD_ISSET(fd, fdsetp) \
		((((fd_set *)fdsetp)->fds_bits[fd >> 5] & (1<<(fd & 31))) != 0)

#undef	__FD_ZERO
#define __FD_ZERO(fdsetp) \
		(memset (fdsetp, 0, sizeof (*(fd_set *)fdsetp)))


#define FD_SET(fd,fdsetp)	__FD_SET(fd,fdsetp)
#define FD_CLR(fd,fdsetp)	__FD_CLR(fd,fdsetp)
#define FD_ISSET(fd,fdsetp)	__FD_ISSET(fd,fdsetp)
#define FD_ZERO(fdsetp)		__FD_ZERO(fdsetp)


int select (int __nfds, fd_set *__readfds,
			fd_set *__writefds, fd_set *__exceptfds,
			struct timeval *__timeout);

extern int ioctl (int __fd, unsigned long int __request, ...);
extern int ftruncate(int fd, off_t length);

#endif // of _WINSOCKAPI_

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif

#define snprintf _snprintf
#define vsnprintf _vsnprintf

#define bswap_16(x) __bswap_16 (x)

/* Return a value with all bytes in the 32 bit argument swapped.  */
#define bswap_32(x) __bswap_32 (x)

/* Return a value with all bytes in the 64 bit argument swapped.  */
# define bswap_64(x) __bswap_64 (x)

extern __int64 strtoll (const char *__nptr, char ** __endptr, int __base);
extern unsigned __int64 strtoull (const char * __nptr, char **__endptr, int __base);

#ifdef	__cplusplus
}
#endif

#endif // of __WIN32_STUFF__