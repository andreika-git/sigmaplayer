//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer support lib memory debugging routines
 *	\file		sp_memory.cpp
 *	\author		*****
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#include <libsp/sp_misc.h>

//#define MEMDUMP
//#define TEST_UCLIBC

// undefine our macros
#undef	new
#undef	delete


static bool special_case = false;

//////////////////////////////////////////////////////////////////////////
// this is needed to ensure that mm starts first and ends last
int _crt_deinit()
{
#ifdef SP_MEMDEBUG
	special_case = true;
	delete SPMemoryManager::mm;
	special_case = false;
#endif
	return 0;
}
int _crt_init()
{
	onexit(_crt_deinit);

#ifdef SP_MEMDEBUG
	special_case = true;
	SPMemoryManager::mm = new SPMemoryManager();
	special_case = false;
#endif

	return 0;
}

// this is VC-specific hack for calling before CRT
typedef int cb(void);
#pragma data_seg(".CRT$XIU")
static cb *autostart[] = { _crt_init };
#pragma data_seg(".CRT$XPU")
static cb *autoexit[] = { _crt_deinit };
#pragma data_seg()    // reset data-segment

#ifdef SP_MEMDEBUG

#ifdef _DEBUG
#define SP_CRT_DEBUG
#endif


// global manager definition
SPMemoryManager *SPMemoryManager::mm = NULL;

#ifdef SP_CRT_DEBUG
#define _NORMAL_BLOCK    1
#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */
_CRTIMP void * __cdecl _malloc_dbg(size_t,int,const char *,int);
_CRTIMP void * __cdecl _realloc_dbg(void *,size_t,int,const char *,int);
_CRTIMP void __cdecl _free_dbg(void *,int);
#ifdef  __cplusplus
}
#endif
#endif

// Global new/new[] & delete/delete[]
//////////////////////////////////////////////////////////////////////////

void *operator new(size_t reqSize)
{
	if (special_case)	// used by mm itself
		return calloc(reqSize, 1);

#ifdef TEST_UCLIBC
		return SPcalloc(reqSize);
#else

	SPMemoryManager &memManager = SPMemoryManager::GetHandle();

	// zero allocation is valid (but strange...)
	if(reqSize == 0)
	{
		reqSize = 1;	
		memManager.Log("Warning: Zero allocation in %s (%d)", memManager.
			GetSourceName(memManager.srcFile), memManager.srcLine);
	}
	void *ptr = memManager.Alloc(memManager.srcFile,
		memManager.srcLine, reqSize, SP_ALLOC_NEW);
	
	return ptr;
#endif
}

// -----------------------------------------------------------------------

void *operator new[](size_t reqSize)
{
#ifdef TEST_UCLIBC
		return SPcalloc(reqSize);
#else
	SPMemoryManager &memManager = SPMemoryManager::GetHandle();

	// zero allocation is valid (but strange...)
	if(reqSize == 0)
	{
		reqSize = 1;	
		memManager.Log("Warning: Zero allocation in %s (%d)", memManager.
			GetSourceName(memManager.srcFile), memManager.srcLine);
	}
	void *ptr = memManager.Alloc(memManager.srcFile,
		memManager.srcLine, reqSize, SP_ALLOC_NEW_ARRAY);

	return ptr;
#endif
}

// -----------------------------------------------------------------------

void operator delete(void *reqAddress)
{
	if (special_case)	// used by mm itself
	{
		free(reqAddress);
		return;
	}

#ifdef TEST_UCLIBC
	SPfree(reqAddress);
#else

	SPMemoryManager &memManager = SPMemoryManager::GetHandle();

	// zero deallocation is valid (but strange...)
	if (!reqAddress)
	{
		memManager.Log("Warning: Zero deallocation in %s (%d)",
			memManager.GetSourceName(memManager.srcFile),
			memManager.srcLine);
		return;
	}

	memManager.Dealloc(memManager.srcFile, memManager.srcLine,
		SP_ALLOC_DELETE, reqAddress);
#endif
}

// -----------------------------------------------------------------------

void operator delete[](void *reqAddress)
{
#ifdef TEST_UCLIBC
	SPfree(reqAddress);
#else
	SPMemoryManager &memManager = SPMemoryManager::GetHandle();

	// zero deallocation is valid (but strange...)
	if (!reqAddress)
	{
		memManager.Log("Warning: Zero deallocation in %s (%d)",
			memManager.GetSourceName(memManager.srcFile),
			memManager.srcLine);
		return;
	}
	memManager.Dealloc(memManager.srcFile, memManager.srcLine,
		SP_ALLOC_DELETE_ARRAY, reqAddress);
#endif
}

// SPMemoryManager member-functions
//////////////////////////////////////////////////////////////////////////

SPMemoryManager::SPMemoryManager() : srcFile(NULL), srcLine(0),
	prefixPattern(0xAAAAAAAA), postfixPattern(0xBBBBBBBB), 
	unusedPattern(0xCCCCCCCC), releasedPattern(0xDDDDDDDD),
	currentAllocated(0), unitsBufferSize(0), borderSize(8), units(NULL), 
	unitsBuffer(NULL)
{
	memset(&memStats, 0, sizeof(memStats));
	BeginLog();
}

void* SPMemoryManager::Alloc(const char *file, const DWORD line,
			const DWORD reqSize, const DWORD allocType)
{
	currentAllocated++;
	if(!units)
	{
		// allocate 256 unit elements
		units = (SPAllocUnit *)malloc(sizeof(SPAllocUnit) * 256);
		if(units == NULL)
		{
			OutOfMemory();
			return NULL;
		}

		memset(units, 0, sizeof(SPAllocUnit) * 256);
		for(DWORD i = 0; i < 256 - 1; i++)
			units[i].next = &units[i+1];
		SPAllocUnit	**temp = (SPAllocUnit **)realloc(unitsBuffer,
			(unitsBufferSize + 1) * sizeof(SPAllocUnit *));
		if(temp)
		{
			unitsBuffer = temp;
			unitsBuffer[unitsBufferSize++] = units;
		}
		else
		{
			OutOfMemory();
			return NULL;
		}
	}

	SPAllocUnit *cur = units;
	units = cur->next;

	// fill alloc unit
	cur->reqSize = reqSize;
	cur->actualSize = reqSize + borderSize * sizeof(DWORD) * 2;
	cur->actualAddress = malloc(cur->actualSize);
	if (cur->actualAddress == NULL)
	{
		OutOfMemory();
		return NULL;
	}
	cur->allocationType = allocType;
	cur->reportedAddress = reinterpret_cast<char*>(cur->actualAddress) +
		borderSize * sizeof(DWORD);
	strncpy(cur->sourceFile, GetSourceName(const_cast<char*>(file)),
		sizeof(cur->sourceFile) - 1);
	cur->sourceLine = line;
	cur->allocationNumber = currentAllocated;

	// insert unit into hash table
	DWORD hashIndex = (reinterpret_cast<DWORD>(cur->reportedAddress) >>
		4) & SP_MM_HASHSIZEM;
	if(hashTable[hashIndex])
		hashTable[hashIndex]->prev = cur;
	cur->next = hashTable[hashIndex];
	cur->prev = NULL;
	hashTable[hashIndex] = cur;
	
	// update stats
	memStats.totalReportedMemory += cur->reqSize;
	memStats.totalActualMemory += cur->actualSize;
	memStats.totalAllocUnitCount++;
	if(memStats.totalReportedMemory > memStats.peakReportedMemory)
		memStats.peakReportedMemory = memStats.totalReportedMemory;
	if(memStats.totalActualMemory > memStats.peakActualMemory)
		memStats.peakActualMemory   = memStats.totalActualMemory;
	if(memStats.totalAllocUnitCount > memStats.peakAllocUnitCount)
		memStats.peakAllocUnitCount = memStats.totalAllocUnitCount;
	memStats.accumulatedReportedMemory += cur->reqSize;
	memStats.accumulatedActualMemory += cur->actualSize;
	memStats.accumulatedAllocUnitCount++;

	// prepare memory block
	FillWithPattern(cur, unusedPattern);

	// calloc expects zero-filled memory
	if(allocType == SP_ALLOC_CALLOC)
		memset(cur->reportedAddress, 0, cur->reqSize);

	// preserve globals
	ClearGlobals();

#ifdef MEMDUMP
	FILE *fp = fopen(SP_MMDUMP_LOGFILENAME, "at");
	fprintf(fp, "+[%d] %s (%d):\t\t%d\t\t[%d]\n", currentAllocated, file, line, reqSize,memStats.totalReportedMemory);
	fclose(fp);
#endif

	return cur->reportedAddress;
}

// -----------------------------------------------------------------------

char* SPMemoryManager::Strdup(const char *file, const DWORD line,
			 const char *source)
{
	DWORD reqSize;
	if(!source)
		reqSize = 1;
	else
		reqSize = strlen(source) + 1;
	DWORD allocType = SP_ALLOC_STRDUP;

	currentAllocated++;
	if(!units)
	{
		// allocate 256 unit elements
		units = (SPAllocUnit *)malloc(sizeof(SPAllocUnit) * 256);
		if(units == NULL)
		{
			OutOfMemory();
			return NULL;
		}

		memset(units, 0, sizeof(SPAllocUnit) * 256);
		for(DWORD i = 0; i < 256 - 1; i++)
			units[i].next = &units[i+1];
		SPAllocUnit	**temp = (SPAllocUnit **)realloc(unitsBuffer,
			(unitsBufferSize + 1) * sizeof(SPAllocUnit *));
		if (temp)
		{
			unitsBuffer = temp;
			unitsBuffer[unitsBufferSize++] = units;
		}
		else
		{
			OutOfMemory();
			return NULL;
		}
	}

	SPAllocUnit *cur = units;
	units = cur->next;

	// fill alloc unit
	cur->reqSize = reqSize;
	cur->actualSize = reqSize + borderSize * sizeof(DWORD) * 2;
	cur->actualAddress = malloc(cur->actualSize);
	if(!cur->actualAddress)
	{
		OutOfMemory();
		return NULL;
	}
	cur->allocationType = allocType;
	cur->reportedAddress = reinterpret_cast<char*>(cur->actualAddress) +
		borderSize * sizeof(DWORD);
	strncpy(cur->sourceFile, GetSourceName(const_cast<char*>(file)),
		sizeof(cur->sourceFile) - 1);
	cur->sourceLine = line;
	cur->allocationNumber = currentAllocated;

	// insert unit into hash table
	DWORD hashIndex = (reinterpret_cast<DWORD>(cur->reportedAddress) >>
		4) & SP_MM_HASHSIZEM;
	if(hashTable[hashIndex])
		hashTable[hashIndex]->prev = cur;
	cur->next = hashTable[hashIndex];
	cur->prev = NULL;
	hashTable[hashIndex] = cur;
	
	// update stats
	memStats.totalReportedMemory += cur->reqSize;
	memStats.totalActualMemory += cur->actualSize;
	memStats.totalAllocUnitCount++;
	if(memStats.totalReportedMemory > memStats.peakReportedMemory)
		memStats.peakReportedMemory = memStats.totalReportedMemory;
	if(memStats.totalActualMemory > memStats.peakActualMemory)
		memStats.peakActualMemory   = memStats.totalActualMemory;
	if(memStats.totalAllocUnitCount > memStats.peakAllocUnitCount)
		memStats.peakAllocUnitCount = memStats.totalAllocUnitCount;
	memStats.accumulatedReportedMemory += cur->reqSize;
	memStats.accumulatedActualMemory += cur->actualSize;
	memStats.accumulatedAllocUnitCount++;

	// prepare memory block
	FillWithPattern(cur, unusedPattern);

	// copy strdup string
	if(source)
		strcpy(static_cast<char*>(cur->reportedAddress), source);

	// preserve globals
	ClearGlobals();

#ifdef MEMDUMP
	FILE *fp = fopen(SP_MMDUMP_LOGFILENAME, "at");
	fprintf(fp, "+[%d] %s (%d):\t\t%d\t\t[%d]\n", currentAllocated, file, line, reqSize,memStats.totalReportedMemory);
	fclose(fp);
#endif

	return static_cast<char*>(cur->reportedAddress);
}

// -----------------------------------------------------------------------

void* SPMemoryManager::Realloc(const char *file, const DWORD line,
		const DWORD reqSize, void* reqAddress, const DWORD reallocType)
{
	if(!reqAddress)
			return Alloc(file, line, reqSize, reallocType);

	SPAllocUnit	*u = FindAllocUnit(reqAddress);
	if(u == NULL)
	{
		Log("Request to reallocate RAM that was never allocated in %s (%d)",
			GetSourceName(const_cast<char*>(file)), line);
		return NULL;
	}

	currentAllocated++;

	ValidateUnit(u);

	if(!(u->allocationType == SP_ALLOC_MALLOC|| u->allocationType ==
		SP_ALLOC_CALLOC || u->allocationType == SP_ALLOC_REALLOC))
		 Log("Allocation/deallocation mismatch in %s (%d)",
		 GetSourceName(const_cast<char*>(file)), line);

	DWORD originalSize = u->reqSize;

	// do the reallocation
	void *oldReqAddress = reqAddress;
	DWORD newActualSize = reqSize + borderSize * sizeof(DWORD) * 2;
	void *newActualAddress = realloc(u->actualAddress, newActualSize);	
	if(!newActualAddress)
	{
		OutOfMemory();
		return NULL;
	}
	
	memStats.totalReportedMemory -= u->reqSize;
	memStats.totalActualMemory -= u->actualSize;

	u->actualSize = newActualSize;
	u->actualAddress = newActualAddress;
	u->reqSize = reqSize;
	u->reportedAddress = reinterpret_cast<char*>(newActualAddress) +
		borderSize * sizeof(DWORD);
	u->allocationType = reallocType;
	u->sourceLine = line;
	u->allocationNumber = currentAllocated;

	strncpy(u->sourceFile, GetSourceName(const_cast<char*>(file)),
		sizeof(u->sourceFile) - 1);
	
	DWORD hashIndex;
	if(oldReqAddress != u->reportedAddress)
	{
		// remove this allocation unit from the hash table
		{
			hashIndex = (reinterpret_cast<DWORD>(oldReqAddress) >> 4) &
				SP_MM_HASHSIZEM;
			if(hashTable[hashIndex] == u)
				hashTable[hashIndex] = hashTable[hashIndex]->next;
			else
			{
				if(u->prev)	u->prev->next = u->next;
				if(u->next)	u->next->prev = u->prev;
			}
		}

		// re-insert it back into the hash table
		hashIndex = (reinterpret_cast<DWORD>(u->reportedAddress) >> 4) &
			SP_MM_HASHSIZEM;
		if(hashTable[hashIndex])
			hashTable[hashIndex]->prev = u;
		u->next = hashTable[hashIndex];
		u->prev = NULL;
		hashTable[hashIndex] = u;
	}

	memStats.totalReportedMemory += u->reqSize;
	memStats.totalActualMemory += u->actualSize;
	if(memStats.totalReportedMemory > memStats.peakReportedMemory)
		memStats.peakReportedMemory = memStats.totalReportedMemory;
	if(memStats.totalActualMemory   > memStats.peakActualMemory)
		memStats.peakActualMemory = memStats.totalActualMemory;
	DWORD	deltaReportedSize = reqSize - originalSize;
	if (deltaReportedSize > 0)
	{
		memStats.accumulatedReportedMemory += deltaReportedSize;
		memStats.accumulatedActualMemory += deltaReportedSize;
	}
	// prepare the allocation
	FillWithPattern(u, unusedPattern, originalSize);

	// preserve globals
	ClearGlobals();

#ifdef MEMDUMP
	FILE *fp = fopen(SP_MMDUMP_LOGFILENAME, "at");
	fprintf(fp, "*[%d] %s (%d):\t\t%d->%d\t\t[%d]\n", currentAllocated, file, line, originalSize, reqSize, memStats.totalReportedMemory);
	fclose(fp);
#endif

	return u->reportedAddress;
}

// -----------------------------------------------------------------------

void SPMemoryManager::Dealloc(const char *file, const DWORD line,
		const DWORD deallocType, void* reqAddress)
{
	SPAllocUnit	*u = FindAllocUnit(reqAddress);
	if (u == NULL)
	{
		Log("Request to deallocate RAM that was never allocated in %s"
			" (%d)", GetSourceName(const_cast<char*>(file)), line);
		return;
	}

	ValidateUnit(u);

	if(!((deallocType == SP_ALLOC_DELETE && u->allocationType == SP_ALLOC_NEW) ||
		 (deallocType == SP_ALLOC_DELETE_ARRAY && u->allocationType == SP_ALLOC_NEW_ARRAY) ||
		 (deallocType == SP_ALLOC_FREE && u->allocationType == SP_ALLOC_MALLOC) ||
		 (deallocType == SP_ALLOC_FREE && u->allocationType == SP_ALLOC_CALLOC) ||
		 (deallocType == SP_ALLOC_FREE && u->allocationType == SP_ALLOC_REALLOC) ||
		 (deallocType == SP_ALLOC_FREE && u->allocationType == SP_ALLOC_STRDUP) ||
		 (deallocType == SP_ALLOC_UNKNOWN)))
	{
		Log("Allocation/deallocation mismatch in %s (%d)",
			GetSourceName(const_cast<char*>(file)), line);
	}

	// unfortunately, code has no effect because of MS debug libs :(
	FillWithPattern(u, releasedPattern);

	free(u->actualAddress);

	DWORD hashIndex = ((DWORD) u->reportedAddress >> 4) & SP_MM_HASHSIZEM;
	if (hashTable[hashIndex] == u)
		hashTable[hashIndex] = u->next;
	else
	{
		if (u->prev) u->prev->next = u->next;
		if (u->next) u->next->prev = u->prev;
	}

	// remove this allocation from our stats
	memStats.totalReportedMemory -= u->reqSize;
	memStats.totalActualMemory -= u->actualSize;
	memStats.totalAllocUnitCount--;

	memset(u, 0, sizeof(SPAllocUnit));
	u->next = units;
	units = u;

	// preserve globals
	ClearGlobals();

#ifdef MEMDUMP
	FILE *fp = fopen(SP_MMDUMP_LOGFILENAME, "at");
	fprintf(fp, "-[%d] %s (%d):\t\t%d\t\t[%d]\n", currentAllocated, file, line, u->reqSize, memStats.totalReportedMemory);
	fclose(fp);
#endif

}

// -----------------------------------------------------------------------

void SPMemoryManager::Log(char *message, ...)
{
	va_list l;
	va_start(l, message);

	char *lasterr = (char *)malloc (1024);
	vsprintf(lasterr, message, l);
	
	FILE *fp = fopen(SP_MM_LOGFILENAME, "at");
	vfprintf(fp, message, l);
	fprintf(fp, "\n");
	fclose(fp);

	free(lasterr);
}

// -----------------------------------------------------------------------

void SPMemoryManager::BeginLog()
{
	FILE *fp = fopen(SP_MM_LOGFILENAME, "wt");
	
	fprintf(fp, "SigmaPlayer memory manager report\n");
	fprintf(fp, "--------------------------------------------------------"
		"------------------------\n\n");
	fclose(fp);

#ifdef MEMDUMP
	fp = fopen(SP_MMDUMP_LOGFILENAME, "wt");
	fclose(fp);
#endif

}

// -----------------------------------------------------------------------

void SPMemoryManager::LeakReport()
{
	// log final stats
	LogStats();

	FILE	*fp = fopen(SP_MM_LOGFILENAME, "at");
	if (!fp) return;

	static char timeString[25];
	memset(timeString, 0, sizeof(timeString));
	time_t  t = time(NULL);
	struct  tm *tme = localtime(&t);
	
	fprintf(fp, "\n------------------------------------------------------"
		"--------------------------\n");
	fprintf(fp, "Memory leak report for:  %02d/%02d/%04d %02d:%02d:%02d\n"
		, tme->tm_mon + 1, tme->tm_mday, tme->tm_year + 1900,
		tme->tm_hour, tme->tm_min, tme->tm_sec);
	fprintf(fp, "--------------------------------------------------------"
		"------------------------\n\n");
		
	if (memStats.totalAllocUnitCount)
		fprintf(fp, "%d memory leak%s found:\n", memStats.
		totalAllocUnitCount, memStats.totalAllocUnitCount == 1 ? "":"s");
	else
	{
		fprintf(fp, "No memory leaks found.\n");
		
		// free our memory
		if (unitsBuffer)
		{
			for (DWORD i = 0; i < unitsBufferSize; i++)
				free(unitsBuffer[i]);
			free(unitsBuffer);
			unitsBuffer = 0;
			unitsBufferSize = 0;
			units = NULL;
		}
	}
	fprintf(fp, "\n");

	if(memStats.totalAllocUnitCount)
		DumpAllocs(fp);
	fclose(fp);
}

// -----------------------------------------------------------------------

void SPMemoryManager::LogStats()
{
	FILE	*fp = fopen(SP_MM_LOGFILENAME, "at");
	if (!fp) return;

	static  char    timeString[25];
	memset(timeString, 0, sizeof(timeString));
	time_t  t = time(NULL);
	struct  tm *tme = localtime(&t);

	fprintf(fp, "\nMemory stats for: %02d/%02d/%04d %02d:%02d:%02d\n",
		tme->tm_mon + 1, tme->tm_mday, tme->tm_year + 1900, tme->tm_hour,
		tme->tm_min, tme->tm_sec);
	fprintf(fp, "--------------------------------------------------------"
		"------------------------\n\n");

	fprintf(fp, "Total allocated memory : 0x%08X (%10u)\n",
		memStats.totalActualMemory, memStats.totalActualMemory);
	fprintf(fp, "Total reported memory  : 0x%08X (%10u)\n",
		memStats.totalReportedMemory, memStats.totalReportedMemory);
	fprintf(fp, "Total allocation units : 0x%08X (%10u)\n\n",
		memStats.totalAllocUnitCount, memStats.totalAllocUnitCount);

	fprintf(fp, "Peak allocated memory  : 0x%08X (%10u)\n",
		memStats.peakActualMemory, memStats.peakActualMemory);
	fprintf(fp, "Peak reported memory   : 0x%08X (%10u)\n",
		memStats.peakReportedMemory, memStats.peakReportedMemory);
	fprintf(fp, "Peak allocation units  : 0x%08X (%10u)\n\n",
		memStats.peakAllocUnitCount, memStats.peakAllocUnitCount);

	fprintf(fp, "Accum allocated memory : 0x%08X (%10u)\n",
		memStats.accumulatedActualMemory,
		memStats.accumulatedActualMemory);
	fprintf(fp, "Accum reported memory  : 0x%08X (%10u)\n",
		memStats.accumulatedReportedMemory,
		memStats.accumulatedReportedMemory);
	fprintf(fp, "Accum allocation units : 0x%08X (%10u)\n",
		memStats.accumulatedAllocUnitCount,
		memStats.accumulatedAllocUnitCount);

	fclose(fp);
}

// -----------------------------------------------------------------------

void SPMemoryManager::DumpAllocs(FILE *fp)
{
	fprintf(fp, "Alloc.   Addr       Size       Addr       Size          "
		"                            \n");
	fprintf(fp, "Number Reported   Required    Actual     Actual     Unus"
		"ed     Method   Allocated by \n");
	fprintf(fp, "------ ---------- ---------- ---------- ---------- -----"
		"----- -------- -----------------\n");

	for (DWORD i = 0; i < 4096; i++)
	{
		SPAllocUnit *ptr = hashTable[i];
		while(ptr)
		{
			fprintf(fp, "%06d 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X %-8s  %s"
				" (%d)\n",
				ptr->allocationNumber,
				reinterpret_cast<DWORD>(ptr->reportedAddress),
				ptr->reqSize,
				reinterpret_cast<DWORD>(ptr->actualAddress),
				ptr->actualSize,
				CalculateUnused(ptr), 
				allocTypes[ptr->allocationType],
				GetSourceName(ptr->sourceFile),
				ptr->sourceLine);

			ptr = ptr->next;
		}
	}
}

void SPMemoryManager::DumpLastAlloc(FILE *fp)
{
	for (DWORD i = 0; i < 4096; i++)
	{
		SPAllocUnit *ptr = hashTable[i];
		while(ptr)
		{
			if (ptr->allocationNumber == currentAllocated)
			{
			fprintf(fp, "%06d 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X %-8s  %s"
				" (%d)\n",
				ptr->allocationNumber,
				reinterpret_cast<DWORD>(ptr->reportedAddress),
				ptr->reqSize,
				reinterpret_cast<DWORD>(ptr->actualAddress),
				ptr->actualSize,
				CalculateUnused(ptr), 
				allocTypes[ptr->allocationType],
				GetSourceName(ptr->sourceFile),
				ptr->sourceLine);
			return;
			}
			ptr = ptr->next;
		}
	}
}

void SPMemoryManager::TEST_DUMP()
{
	static int j = -1;
	static SPAllocUnit au[10000];

	FILE	*fp = fopen(SP_MM_LOGFILENAME, "at");
	if (!fp) return;
	fprintf(fp, "\n\n ------- MEM.DIFFERENCE --------\n");

	if (j < 0)
	{
		fprintf(fp, "STAGE 0\n\n");
		j = 0;
		for (DWORD i = 0; i < 4096; i++)
		{
			SPAllocUnit *ptr = hashTable[i];
			while(ptr)
			{
				memcpy(&au[j++], ptr, sizeof(SPAllocUnit));
				ptr = ptr->next;
				if (j > 10000)
				{
					exit(0);
				}
			}
		}
	} else
	{
		fprintf(fp, "STAGE 1\n\n");
		for (DWORD i = 0; i < 4096; i++)
		{
			SPAllocUnit *ptr = hashTable[i];
			while(ptr)
			{
				bool was = false;
				for (int k = 0; k < j; k++)
				{
					if (au[k].allocationNumber == ptr->allocationNumber)
					{
						was = true;
						au[k].next = (SPAllocUnit *)1;
						break;
					}
				}
				if (!was)
				{
					fprintf(fp, "%06d 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X %-8s  %s"
						" (%d)\n",
						ptr->allocationNumber,
						reinterpret_cast<DWORD>(ptr->reportedAddress),
						ptr->reqSize,
						reinterpret_cast<DWORD>(ptr->actualAddress),
						ptr->actualSize,
						CalculateUnused(ptr), 
						allocTypes[ptr->allocationType],
						GetSourceName(ptr->sourceFile),
						ptr->sourceLine);
				}
				ptr = ptr->next;
			}
		}

		fprintf(fp, "\n-------------------------------------------\n\n");
		for (int k = 0; k < j; k++)
		{
			if ((DWORD)au[k].next != 1)
			{
				SPAllocUnit *ptr = &au[k];
				fprintf(fp, "%06d 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X %-8s  %s"
					" (%d)\n",
					ptr->allocationNumber,
					reinterpret_cast<DWORD>(ptr->reportedAddress),
					ptr->reqSize,
					reinterpret_cast<DWORD>(ptr->actualAddress),
					ptr->actualSize,
					CalculateUnused(ptr), 
					allocTypes[ptr->allocationType],
					GetSourceName(ptr->sourceFile),
						ptr->sourceLine);
			}
		}

		j = -1;
	}
	fclose(fp);
}

// -----------------------------------------------------------------------

void SPMemoryManager::SetOwner(const char *file, const DWORD line)
{
	srcFile = const_cast<char*>(file);
	srcLine = line;
}

// -----------------------------------------------------------------------

void SPMemoryManager::ClearGlobals()
{
	srcFile = "";
	srcLine = NULL;
}

// -----------------------------------------------------------------------

char* SPMemoryManager::GetSourceName(char *sourceFile)
{
	char *ptr = strrchr(sourceFile, '\\');
	if(ptr) return ptr + 1;
	return sourceFile;
}

// -----------------------------------------------------------------------

SPAllocUnit* SPMemoryManager::FindAllocUnit(void *reqAddress)
{
	if(!reqAddress)
		return NULL;

	DWORD hashIndex = ((DWORD)reqAddress >> 4) & SP_MM_HASHSIZEM;
	SPAllocUnit *ptr = hashTable[hashIndex];
	while(ptr)
	{
		if(ptr->reportedAddress == reqAddress)
			return ptr;
		ptr = ptr->next;
	}
	return NULL;
}

// -----------------------------------------------------------------------

void SPMemoryManager::FillWithPattern(SPAllocUnit *u, DWORD pattern,
									  DWORD originalSize)
{
	DWORD i;
	DWORD *lptr = reinterpret_cast<DWORD*>(reinterpret_cast<char*>
		(u->reportedAddress) + originalSize);
	int length = u->reqSize - originalSize;
	if(length > 0)
	{
		for(i = 0; i<(static_cast<DWORD>(length) >> 2); i++, lptr++)
		{
			*lptr = pattern;
		}
		// fill the remainder
		DWORD shiftCount = 0;
		char *cptr = reinterpret_cast<char*>(lptr);
		for(i = 0; i<(static_cast<DWORD>(length) & 0x3); i++, cptr++,
			shiftCount += 8)
		{
			*cptr = (char)((pattern & (0xff << shiftCount)) >> shiftCount);
		}
	}

	// write in the prefix/postfix bytes
	DWORD *pre = reinterpret_cast<DWORD*>(u->actualAddress);
	DWORD *post = reinterpret_cast<DWORD*>(reinterpret_cast<char*>
		(u->actualAddress) + u->actualSize - borderSize * sizeof(DWORD));
	for(i = 0; i < borderSize; i++, pre++, post++)
	{
		*pre = prefixPattern;
		*post = postfixPattern;
	}
}

// -----------------------------------------------------------------------

DWORD SPMemoryManager::CalculateUnused(SPAllocUnit *u)
{
	const DWORD *ptr = (const DWORD *)u->reportedAddress;
	DWORD count = 0;
	for(DWORD i = 0; i < u->reqSize; i += sizeof(DWORD), ptr++)
		if(*ptr == unusedPattern) count += sizeof(DWORD);
	return count;
}

// -----------------------------------------------------------------------

BOOL SPMemoryManager::ValidateUnit(SPAllocUnit *u)
{
	// make sure the borders are untouched
	DWORD *pre = reinterpret_cast<DWORD*>(u->actualAddress);
	DWORD *post = reinterpret_cast<DWORD*>(reinterpret_cast<char*>
		(u->actualAddress) + u->actualSize - borderSize * sizeof(DWORD));
	BOOL errorFlag = false;

	for(DWORD i = 0; i < borderSize; i++, pre++, post++)
	{
		if (*pre != prefixPattern)
		{
			Log("A memory allocation unit was corrupt because of an underrun:");
			DumpUnit(u);
			errorFlag = true;
		}
		
		if (*post != postfixPattern)
		{
			Log("A memory allocation unit was corrupt because of an overrun:");
			DumpUnit(u);
			errorFlag = true;
		}
	}
	return !errorFlag;
}

// -----------------------------------------------------------------------

void SPMemoryManager::DumpUnit(SPAllocUnit *u)
{
	Log("Address (reported): %010p", u->reportedAddress);
	Log("Address (actual)  : %010p", u->actualAddress);
	Log("Size (required)   : 0x%08X", u->reqSize);
	Log("Size (actual)     : 0x%08X", u->actualSize);
	Log("Owner             : %s(%d)", u->sourceFile, u->sourceLine);
	Log("Allocation type   : %s", allocTypes[u->allocationType]);
	Log("Allocation number : %d", u->allocationNumber);
}

// -----------------------------------------------------------------------

void SPMemoryManager::OutOfMemory()
{
	Log("Ran out of memory in %s (%d)!",
		GetSourceName(srcFile), srcLine);
	SPMemoryManager::GetHandle().LeakReport();
	abort();
}

// -----------------------------------------------------------------------

#endif // SP_MEMDEBUG