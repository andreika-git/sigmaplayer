//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer support lib memory debugging routines
 *	\file		sp_memory.h
 *	\author		Storm
 *	\version	0.1
 *	\date		14.07.2002 (23.08.2001)
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

#ifndef SP_MEMORY_H
#define SP_MEMORY_H

// The memory remains... (c) M.
#ifdef SP_MEMDEBUG

#ifndef __cplusplus
#error Include with SP_MEMDEBUG for C++ files only!
#endif

#include <stdio.h>
#include <malloc.h>

/// allocation types
enum
{
	SP_ALLOC_UNKNOWN = 0,
	SP_ALLOC_NEW,
	SP_ALLOC_NEW_ARRAY,
	SP_ALLOC_MALLOC,
	SP_ALLOC_CALLOC,
	SP_ALLOC_REALLOC,
	SP_ALLOC_STRDUP,
	SP_ALLOC_DELETE,
	SP_ALLOC_DELETE_ARRAY,
	SP_ALLOC_FREE
};

/// internal stats
struct SPMemoryStats
{
	DWORD	totalReportedMemory;
	DWORD	totalActualMemory;
	DWORD	peakReportedMemory;
	DWORD	peakActualMemory;
	DWORD	accumulatedReportedMemory;
	DWORD	accumulatedActualMemory;
	DWORD	accumulatedAllocUnitCount;
	DWORD	totalAllocUnitCount;
	DWORD	peakAllocUnitCount;
};

/// internal memory manager's struct
struct SPAllocUnit
{
	DWORD		actualSize;
	DWORD		reqSize;
	void		*actualAddress;
	void		*reportedAddress;
	char		sourceFile[40];
	DWORD		sourceLine;
	DWORD		allocationType;
	DWORD		allocationNumber;
	SPAllocUnit	*next;
	SPAllocUnit	*prev;
};

const DWORD SP_MM_HASHSIZE				= 4096;
const DWORD SP_MM_HASHSIZEM				= SP_MM_HASHSIZE - 1;
static const char* SP_MM_LOGFILENAME	= "sp_mem.log";
static const char* SP_MMDUMP_LOGFILENAME	= "sp_mem_dump.log";
static char *allocTypes[] = {"unknown", "new", "new[]", "malloc",
							"calloc", "realloc", "strdup", "delete",
							"delete[]",	"free"};

//////////////////////////////////////////////////////////////////////////
/// Memory allocator/tracer
class SPMemoryManager
{
public:
	/// Functions

	/// dtor. we use it to let us know when we're in static deinit.
	/// and log all leaks info
	~SPMemoryManager() {LeakReport();}

	/// singleton-related function
	static SPMemoryManager& GetHandle() {return *mm;}

	/// basic mem functions
	void *Alloc(const char *file, const DWORD line,	const DWORD reqSize,
		const DWORD allocType);
	void *Realloc(const char *file, const DWORD line, const DWORD reqSize,
		void* reqAddress, const DWORD reallocType);
	void Dealloc(const char *file, const DWORD line, 
		const DWORD deallocType, void* reqAddress);
	char *Strdup(const char *file, const DWORD line, const char *source);

	/// final leak report
	void LeakReport();
	/// dump all current allocations
	void DumpAllocs(FILE *fp);
	void DumpLastAlloc(FILE *fp);

	/// print memory usage statistics
	void LogStats();
	/// set file and line number
	void SetOwner(const char *file, const DWORD line);

//!!!!!!!!!!!!!!!!
	void TEST_DUMP();
//!!!!!!!!!!!!!!!!
	
	// friends
	friend void *operator new(size_t reqSize);
	friend void *operator new[](size_t reqSize);
	friend void operator delete(void *reqAddress);
	friend void operator delete[](void *reqAddress);

protected:
	friend int _crt_init();
	friend int _crt_deinit();

	static SPMemoryManager *mm;				// singleton

private:
	/// Variables
	char *srcFile;							/// source file of call
	DWORD srcLine;							/// line number in scrFile
	
	SPAllocUnit *units;						/// allocation units array
	SPAllocUnit **unitsBuffer;				/// list linked to hash
	SPAllocUnit	*hashTable[SP_MM_HASHSIZE];	/// hash for fast unit search
	SPMemoryStats memStats;					/// memory stats

	DWORD currentAllocated;					/// currently allocated bytes
	DWORD unitsBufferSize;					/// number of allocated units
	DWORD borderSize;						/// size of safe border

	DWORD prefixPattern;					/// beginning pattern
	DWORD postfixPattern;					/// ending pattern
	DWORD unusedPattern;					/// clean memory pattern
	DWORD releasedPattern;					/// deleted memory pattern
	
	/// Functions

	/// ctor. cleans logfile
	SPMemoryManager();

	/// create logfile
	void BeginLog();
	/// preserve globals
	void ClearGlobals();
	/// handle out-of-memory case
	void OutOfMemory();

	/// misc. functions

	/// fill allocated memory with specified pattern
	void FillWithPattern(SPAllocUnit *u, DWORD pattern, 
		DWORD originalSize = 0);
	/// internal: search unit in hash
	SPAllocUnit *FindAllocUnit(void *reqAddress);
	/// returnsname of src file
	char *GetSourceName(char *sourceFile);
	/// scan memory chunk for unused bytes (based on patterns)
	DWORD CalculateUnused(SPAllocUnit *u);
	/// validate integrity of memory allocation unit
	BOOL ValidateUnit(SPAllocUnit *u);
	/// helper function that logs into SP_MM_LOGFILENAME
	void Log(char *message, ...);
	/// dump unit
	void DumpUnit(SPAllocUnit *u);
};

// Operators' redefinition
void	*operator new(size_t reqSize);
void	*operator new[](size_t reqSize);
void	operator delete(void *reqAddress);
void	operator delete[](void *reqAddress);

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void *__cdecl operator new(size_t, void *_P)
	{return (_P); }
#if     _MSC_VER >= 1200
inline void __cdecl operator delete(void *, void *)
	{return; }
#endif
#endif

// Macros
#define	new					(SPMemoryManager::GetHandle().SetOwner  (__FILE__, __LINE__), false) ? NULL : new
#define	delete				(SPMemoryManager::GetHandle().SetOwner  (__FILE__, __LINE__), false) ? NULL : delete
#define	SPmalloc(sz)		SPMemoryManager::GetHandle().Alloc(__FILE__, __LINE__, sz, SP_ALLOC_MALLOC)
#define	SPcalloc(sz)		SPMemoryManager::GetHandle().Alloc(__FILE__, __LINE__, sz, SP_ALLOC_CALLOC)
#define	SPrealloc(ptr,sz)	SPMemoryManager::GetHandle().Realloc(__FILE__, __LINE__, sz, ptr, SP_ALLOC_REALLOC)
#define	SPfree(ptr)			SPMemoryManager::GetHandle().Dealloc(__FILE__, __LINE__, SP_ALLOC_FREE, ptr)
#define	SPstrdup(str)		SPMemoryManager::GetHandle().Strdup(__FILE__, __LINE__, str)
#define SPLogMemStats()		SPMemoryManager::GetHandle().LogStats();

#ifdef TEST_UCLIBC
#ifdef __cplusplus
extern "C"
{
#endif
void *SPmalloc (size_t __size);
void *SPcalloc (size_t __nmemb);
void *SPrealloc (void *__ptr, size_t __size);
void SPfree (void *__ptr);
char *SPstrdup (const char *__s);
#ifdef __cplusplus
}
#endif
#endif

#else // SP_MEMDEBUG

#include <stdlib.h>
 
// just use RTL-fuctions
#define SPmalloc(s)		malloc(s)
#define SPcalloc(s)		calloc(s, 1)
#define SPrealloc(d, s) realloc(d, s)
#define SPfree(d)		free(d)
#define SPstrdup(p)		strdup(p)
#define SPLogMemStats()

#ifdef __cplusplus
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void * operator new(size_t, void *_P)
	{return (_P); }
#ifdef _MSC_VER
#if     _MSC_VER >= 1200
inline void operator delete(void *, void *)
	{return; }
#endif
#endif
#endif
#endif


#endif // SP_MEMDEBUG

// Safe deallocation macros
#define SPSafeFree(ptr)			{if (ptr) SPfree(ptr); (ptr) = NULL;}
#define SPSafeDelete(ptr)		{if (ptr) delete ptr; (ptr) = NULL;}
#define SPSafeDeleteArray(ptr)	{if (ptr) delete [] ptr; (ptr) = NULL;}
// Stack allocation macro
#define SPalloca(ptr)			alloca(ptr)

#endif // SP_MEMORY_H
