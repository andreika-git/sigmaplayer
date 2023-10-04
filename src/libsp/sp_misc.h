//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - misc. functions header file
 *  \file       sp_misc.h
 *  \author     bombur
 *  \version    0.1
 *  \date       4.05.2004
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

#ifndef SP_MISC_H
#define SP_MISC_H

// some typedefs
#ifndef BOOL
typedef int			   BOOL;
#endif

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;

#ifdef WIN32
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;
#define INT64(d) d##i64
#define PRINTF_64d "%I64d"
#define SP_INLINE __inline
#define FORCEINLINE __forceinline
#pragma warning(disable: 4710)
#pragma warning(disable: 4996)
#else

typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
#define INT64(d) d##LL
#define PRINTF_64d "%lld"
#ifndef __GNUC__
#define SP_INLINE __inline
#elif __GNUC__ < 3
#define SP_INLINE inline
#define FORCEINLINE inline
#else
//#define FORCEINLINE inline __attribute__ ((always_inline))
#define SP_INLINE inline
#define FORCEINLINE inline
#endif
#endif

#if defined(__GNUC__)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define ATTRIBUTE_PACKED __attribute__ ((packed))
#endif
#endif
#ifdef WIN32
#define ATTRIBUTE_PACKED
#endif

#ifndef O_BINARY
#ifdef _O_BINARY
#define O_BINARY _O_BINARY
#else
#define O_BINARY 0
#endif
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif


#ifndef MIN
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))
#endif

#ifndef SP_MEMDEBUG_OFF
#ifdef __cplusplus
#ifdef WIN32
#define SP_MEMDEBUG 1
#endif
#endif
#endif

#define SPERRMES(err)

#include <libsp/sp_memory.h>

#include <libsp/sp_bswap.h>

#ifndef __USE_LARGEFILE64
#define __USE_LARGEFILE64
#define __LARGE64_FILES
#endif
#include <sys/stat.h>

#define PAD_EVEN(x) (((x)+1) & ~1)

#ifdef __cplusplus

#ifdef _MSC_VER
#pragma warning (disable : 4505) 
#pragma warning (disable : 4514) 
#endif

template <class T> inline T Max(T a, T b)
{
	return (a >= b) ? a : b;
}
template <class T> inline T Min(T a, T b)
{
	return (a <= b) ? a : b;
}
template <class T> inline T Abs(T a)
{
	return (a >= (T)0) ? a : (T)-a;
}
template <class T> inline void Swap(T& a, T& b)
{
	const T Temp = a;
	a = b;
	b = Temp;
}

// 'F' -> 15
inline int hex2char(int h)
{
	if (h >= '0' && h <= '9')
		return h - '0';
	if (h >= 'A' && h <= 'F')
		return h - 'A' + 10;
	if (h >= 'a' && h <= 'f')
		return h - 'a' + 10;
	return 0;
}

#define SafeGetWord(ptr) ((WORD)(((WORD)ptr[1] << 8) | ((WORD)ptr[0])))
#define SafeGetDword(ptr) ((DWORD)(((DWORD)ptr[3] << 24) | ((DWORD)ptr[2] << 16) | ((DWORD)ptr[1] << 8) | ((DWORD)ptr[0])))

#include <libsp/containers/list.h>
#include <libsp/containers/clist.h>
#include <libsp/containers/dllist.h>
#include <libsp/containers/hashlist.h>
#include <libsp/containers/salist.h>
#include <libsp/containers/string.h>

#else
#define Abs abs
#endif


#ifdef __cplusplus
extern "C" {
#endif

/// Inserts or deletes module to the linux kenrel.
/// Used for external firmware drivers.
BOOL module_apply(const char *modname);

/// Calls binary file in a separate child process.
/// Used for external programs and players.
/// Returns child pid.
int exec_file(const char *binname, const char **args);

/// Call GUI update
void gui_update(void);

#ifdef __cplusplus
}
#endif


#endif // of SP_MISC_H
