//////////////////////////////////////////////////////////////////////////
/**
 * support lib self-allocated double-linked list template prototype
 *	\file		libsp/containers/salist.h
 *	\author		bombur
 *	\version	x.x
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

#ifndef SP_SALIST_H
#define SP_SALIST_H

enum SALIST_ITEM_TYPE
{
	SALIST_FREE = 0,
	SALIST_USED = 1,
};

/// Self-allocated dynamic double-linked list template 
/// with fast insertion/deletion of items.
/// STORES ITEMS OF DIFFERENT TYPES IN THE SAME POINTER LIST!
/// Used to store constantly created/deleted objects of several different types/sizes
/// WITHOUT frequent memory allocs/frees & memory fragmentation.
/// \Parameters: 'T' type must be a base class of 'U'!
///              'num_slots' is the estimated number of stored items of the same type/size.
///	\warning:	Only single-threaded version!!!
template <typename T, int num_slots>
class SPSAList : public SPList<T *>
{
protected:
	/// internal item linkage class
	class ListPrevNext
    {
    public:
    	ListPrevNext()
		{
			prev[SALIST_FREE] = next[SALIST_FREE] = -1;
			prev[SALIST_USED] = next[SALIST_USED] = -1;
		}

    	int prev[2], next[2];
    };

	/// Corresponds to the group of items of the same type/size.
    struct ListAllocType
    {
		// all these are relative indexes (0..num_slots-1)
    	SPClassicList<ListPrevNext> dllist;
    	int first[2], last[2];
		int size;	// sizeof(element)
		int num[2];	// number of elements
		// these are abs. indexes
		int begin, end;
    };

	/// Does double-linked item insertion/deletion
	/// 'idx' is relative to the start of alloc: [0..num_slots]!
	inline bool MoveFreeUsed(int alloc_idx, int idx, int from, int to)
	{
		int &from_prev = allocs[alloc_idx].dllist[idx].prev[from];
		int &from_next = allocs[alloc_idx].dllist[idx].next[from];
		// 1. remove
		if (allocs[alloc_idx].first[from] == idx)
			allocs[alloc_idx].first[from] = from_next;
		if (allocs[alloc_idx].last[from] == idx)
			allocs[alloc_idx].last[from] = from_prev;
		if (from_prev >= 0)
			allocs[alloc_idx].dllist[from_prev].next[from] = from_next;
		if (from_next >= 0)
			allocs[alloc_idx].dllist[from_next].prev[from] = from_prev;
		// detach item
		from_prev = from_next = -1;
		
		// 2. insert
		int &to_prev = allocs[alloc_idx].dllist[idx].prev[to];
		int &to_next = allocs[alloc_idx].dllist[idx].next[to];
		// if the list is empty...
		if (allocs[alloc_idx].first[to] < 0 || allocs[alloc_idx].last[to] < 0)
		{
			allocs[alloc_idx].first[to] = allocs[alloc_idx].last[to] = idx;
			to_prev = -1;
			to_next = -1;
			allocs[alloc_idx].num[from]--;
			allocs[alloc_idx].num[to]++;	// = 1
			return true;
		}
		// additional check if item was already freed/used
		if (to_prev < 0 && to_next < 0)
		{
			// add 'idx' after the last one
			allocs[alloc_idx].dllist[allocs[alloc_idx].last[to]].next[to] = idx;
			to_prev = allocs[alloc_idx].last[to];
			to_next = -1;
			allocs[alloc_idx].last[to] = idx;

			allocs[alloc_idx].num[from]--;
			allocs[alloc_idx].num[to]++;
			return true;
		}
		return false;
	}


public:
	/// Ctor. Creates empty list
	SPSAList() : SPList<T *>()
	{
	}

	~SPSAList()
	{
		for (int i = 0; i < allocs.GetN(); i++)
		{
			//delete [] l[allocs[i].begin];
			SPfree(this->l[allocs[i].begin]);
		}
	}

	/// Allocates and returns a new item via parameter. 
	/// Item index (abs.) is returned to free the item afterwards.
	template <typename U>
	int Allocate(U **newobj)
    {
		int i, alloc_idx = -1;
		ListAllocType at;
		// \todo: Optimize more! Use LUTs or hash...
		for (i = 0; i < allocs.GetN(); i++)
		{
			if (sizeof(U) == allocs[i].size)
			{
				if (allocs[i].first[SALIST_FREE] >= 0)	// at least 1 free item left
				{
					alloc_idx = i;
					break;
				}
			}
		}
		if (alloc_idx == -1)	// allocate new...
		{
			//U *arr = new U [num_slots];
			U *arr = (U *)SPmalloc(sizeof(U) * num_slots);
			at.size = sizeof(U);
			at.begin = SPList<T *>::GetN();
			at.end = at.begin + num_slots - 1;
			SetN(at.begin + num_slots);

			int abs_cur = at.begin;
			int cur_first = 0;
			at.num[SALIST_USED] = 1;
			at.num[SALIST_FREE] = num_slots - 1;
			at.first[SALIST_USED] = at.last[SALIST_USED] = cur_first;
			at.first[SALIST_FREE] = ++cur_first;
			at.dllist.SetN(num_slots);
			this->l[abs_cur++] = (T *)(arr);
			for (i = 1; i < num_slots - 1; i++)
			{
				this->l[abs_cur++] = (T *)(arr + i);
   				at.dllist[i + 1].prev[SALIST_FREE] = cur_first;
				at.dllist[i].next[SALIST_FREE] = ++cur_first;
			}
			this->l[abs_cur] = (T *)(arr + i);
			at.last[SALIST_FREE] = cur_first;
			allocs.Add(at);
			if (newobj != NULL)
			{
				*newobj = arr;
				new (*newobj) U();
			}
			return at.begin + at.first[SALIST_USED];
		}
		// find free item...
    	int idx = allocs[alloc_idx].first[SALIST_FREE];
		// mark one of free items as used and return it
    	MoveFreeUsed(alloc_idx, idx, SALIST_FREE, SALIST_USED);
		idx += allocs[alloc_idx].begin;
		if (newobj != NULL)
		{
			*newobj = (U *)this->l[idx];
			new (*newobj) U();
		}
		return idx;
    }

	/// Free used item. 
	bool Free(int idx)
	{
		if (idx < 0 || idx >= SPList<T *>::GetN())
			return false;
		this->l[idx]->~T();
		int alloc_idx = idx / num_slots;
		if (alloc_idx >= allocs.GetN())
			return false;
		return MoveFreeUsed(alloc_idx, idx % num_slots, SALIST_USED, SALIST_FREE);
	}

	/// Get the FIRST allocated element stored in the list.
	/// Initialize 'cur_idx' to the value returned from Allocate().
	T *GetFirst(int *idx) const
	{
		*idx = -1;
		return GetNext(idx);
	}

	/// Get the NEXT allocated element stored in the list.
	/// Set 'idx' to -1 for the first element or just pass the current value to advance.
	T *GetNext(int *idx) const
	{
		int alloc_idx;
		if (*idx >= 0)
		{
			alloc_idx = *idx / num_slots;
			if (alloc_idx >= allocs.GetN())
				return NULL;
			*idx = allocs[alloc_idx].dllist[*idx % num_slots].next[SALIST_USED];
		} else
			alloc_idx = -1;
		// search for the first item of the next group
		while (*idx < 0)
		{
			alloc_idx++;
			if (alloc_idx >= allocs.GetN())
				return NULL;
			*idx = allocs[alloc_idx].first[SALIST_USED];
		}
		*idx += allocs[alloc_idx].begin;
		return this->l[*idx];
	}

	/// Get total number of free or used items. Used for stats.
	int GetN(SALIST_ITEM_TYPE type)
	{
		int num = 0;
		for (int i = 0; i < allocs.GetN(); i++)
			num += allocs[i].num[type];
		return num;
	}
    	
protected:
	
	SPClassicList<ListAllocType> allocs;
};


#endif // of SP_SALIST_H
