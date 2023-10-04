//////////////////////////////////////////////////////////////////////////
/**
 * support lib small memory bin implementation
 *	\file		libsp/containers/membin.cpp
 *	\author		bombur
 *	\version	1.0
 *	\date		xx.xx.xxxx
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

#include <libsp/sp_misc.h>
#include <libsp/containers/membin.h>

#include <malloc.h>


// bin size, in bytes (incl. all headers)
const int binsize = 65536;
// end of list
const WORD EOL = (WORD)0xffff;

#define IS_FREE(hdr) ((hdr->binidx & 0x8000) == 0)
#define MARK_FREE(hdr) { hdr->binidx &= 0x7fff; }
#define MARK_BUSY(hdr) { hdr->binidx |= 0x8000; }
#define BINIDX(hdr) (hdr->binidx & 0x7fff)

#define GET_HDR(data) ((BinHeader *)((char *)data - sizeof(BinHeader)))
#define GET_PTR(hdr) ((char *)hdr + sizeof(BinHeader))


#ifdef WIN32
#pragma pack(1)
#endif
typedef struct BinHeader
{
public:
	// bin index; high bit of binidx is 'busy' flag
	WORD binidx;
	
	// real data size, in bytes
	WORD size;
	
	// offset to prev. BinHeader, in WORDs
	WORD prev;
	
	// offset to next BinHeader, in WORDs
	WORD next;

} ATTRIBUTE_PACKED BinHeader;
#ifdef WIN32
#pragma pack()
#endif


/// Memory bin
class MemBin
{
public:
	/// ctor
	MemBin()
	{
		bin = NULL;
		last = EOL;
	}
	
	// a little hack!
	MemBin & operator = (MemBin & mb)
	{
		bin = mb.bin;
		mb.bin = NULL;
		last = mb.last;
		return *this;
	}
	
	/// Allocate 
	BOOL Alloc(WORD idx)
	{
		bin = (WORD *)SPmalloc(binsize * sizeof(char));
		if (bin != NULL)
		{
			BinHeader *hdr = (BinHeader *)bin;
			hdr->binidx = idx;
			hdr->size = (binsize - sizeof(BinHeader));
			hdr->prev = hdr->next = EOL;

			last = 0;
			return TRUE;
		} else
		{
			last = EOL;
			return FALSE;
		}
	}

	~MemBin()
	{
		SPSafeFree(bin);
	}

	// find a free chunk of 'size' bytes
	BinHeader *Find(int size)
	{
		// the real size is greater
		WORD s = (WORD)(size + sizeof(BinHeader));
		BinHeader *hdr = (BinHeader *)(bin + last);
		if (IS_FREE(hdr) && hdr->size >= s)
			return hdr;
		WORD cur = 0;
		while (cur != EOL)
		{
			BinHeader *hdr = (BinHeader *)(bin + cur);
			if (IS_FREE(hdr) && hdr->size >= s)
				return hdr;
			cur = hdr->next;
		}
		return NULL;
	}

	BOOL Occupy(BinHeader *hdr, WORD size)
	{
		if (hdr->size > size + 1 + sizeof(BinHeader))
		{
			BinHeader *nexthdr = (BinHeader *)((char *)hdr + sizeof(BinHeader) + size);
			BinHeader *oldnexthdr = hdr->next == EOL ? NULL : (BinHeader *)(bin + hdr->next);
			hdr->next = (WORD)((WORD *)nexthdr - bin);
			if (oldnexthdr != NULL)
			{
				oldnexthdr->prev = hdr->next;
				nexthdr->next = (WORD)((WORD *)oldnexthdr - bin);
			} else
			{
				nexthdr->next = EOL;
				last = hdr->next;
			}
			nexthdr->binidx = hdr->binidx;
			nexthdr->prev = (WORD)((WORD *)hdr - bin);
			nexthdr->size = (WORD)(hdr->size - size - sizeof(BinHeader));
			hdr->size = size;
		}
		MARK_BUSY(hdr);
		return TRUE;
	}

	BOOL Free(BinHeader *hdr)
	{
		BinHeader *prev = hdr->prev != EOL ? (BinHeader *)(bin + hdr->prev) : NULL;
		BinHeader *next = hdr->next != EOL ? (BinHeader *)(bin + hdr->next) : NULL;
		// combine with left chunk if it's free

		if (prev != NULL && IS_FREE(prev))
		{
			prev->size = (WORD)(prev->size + hdr->size + sizeof(BinHeader));
			prev->next = hdr->next;
			if (next != NULL)
			{
				// add right chunk if it's also free
				if (IS_FREE(next))
				{
					BinHeader *nextnext = next->next != EOL ? (BinHeader *)(bin + next->next) : NULL;
					if (nextnext != NULL)
						nextnext->prev = hdr->prev;
					else
						last = hdr->prev;
					prev->next = next->next;
					prev->size = (WORD)(prev->size + next->size + sizeof(BinHeader));
				} else
					next->prev = hdr->prev;
			}
			return TRUE;
		}
		
		MARK_FREE(hdr);
		
		// add right chunk if it's also free
		if (next != NULL && IS_FREE(next))
		{
			hdr->size = (WORD)(hdr->size + next->size + sizeof(BinHeader));
			hdr->next = next->next;
			BinHeader *nextnext = next->next != EOL ? (BinHeader *)(bin + next->next) : NULL;
			if (nextnext != NULL)
				nextnext->prev = next->prev;
			else
				last = next->prev;
		}

		return TRUE;
	}

public:
	WORD *bin;
	// offset to the last BinHeader, in WORDs
	WORD last;
};

SPClassicList<MemBin> bins;
static int curbin = -1;

void *membin_alloc(void *data, size_t size)
{
	// huge block
	if (size > binsize - sizeof(BinHeader))
	{
		BinHeader *hdr = (BinHeader *)SPmalloc(size + sizeof(BinHeader));
		if (hdr != NULL)
		{
			hdr->binidx = EOL;
			hdr->size = 0;	// special 'huge' external chunk
			hdr->prev = hdr->next = EOL;
		}
		return GET_PTR(hdr);
	}

	// size should be positive and DWORD-aligned
	WORD siz = (size > 0) ? (WORD)((size + 3) & ~3) : (WORD)4;

	BinHeader *oldhdr = NULL;
	if (data != NULL)	// first, see what we have...
	{
		oldhdr = GET_HDR(data);
		// we don't resize free blocks
		if (!IS_FREE(oldhdr) && BINIDX(oldhdr) < bins.GetN())
		{
			// no shrinking, sorry...
			if (siz <= oldhdr->size)
				return data;
		} else
			oldhdr = NULL;
	}

	// first in the current bin
	BinHeader *freehdr = NULL;
	if (curbin != -1)
	{
		freehdr = bins[curbin].Find(siz);
	}
	if (freehdr == NULL)
	{
		// we don't use reverse order because last 'curbin' was at the end already...
		for (curbin = 0; curbin < bins.GetN(); curbin++)
		{
			freehdr = bins[curbin].Find(siz);
			if (freehdr != NULL)
				break;
		}
	}
	// no free space left, so allocate a new bin
	if (freehdr == NULL)
	{
		curbin = -1;
		// already too many bins - we failed...
		if (bins.GetN() > 32767)
			return NULL;

		membin_next();
		freehdr = bins[curbin].Find(siz);
	}
	// absolute failure...
	if (freehdr == NULL || curbin < 0)
		return NULL;
	
	// now we have a new hdr, so split & mark
	bins[curbin].Occupy(freehdr, siz);

	// return to user
	void *ptr = GET_PTR(freehdr);
	
	if (oldhdr != NULL)
	{
		// copy old data after "resize"
		memcpy(ptr, data, oldhdr->size);
		// free old data
		bins[BINIDX(oldhdr)].Free(oldhdr);
	}

	return ptr;
}

void membin_free(void *data)
{
	if (data == NULL)
		return;
	
	BinHeader *hdr = GET_HDR(data);
	// if huge block
	if (hdr->binidx == EOL && hdr->size == 0 && hdr->prev == 0 && hdr->next == 0)
	{
		SPfree(hdr);
		return;
	}

	if (IS_FREE(hdr))	// it's already free
		return;
	// a valid bin
	if (BINIDX(hdr) < bins.GetN())
	{
		// mark hdr as free block
		bins[BINIDX(hdr)].Free(hdr);
	}
}

void membin_next()
{
	curbin = bins.GetN();
	if (curbin >= 10)
		return;
	bins.SetN(curbin + 1);
	bins[curbin].Alloc((WORD)curbin);
}
