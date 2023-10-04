//////////////////////////////////////////////////////////////////////////
/**
 * support lib linear dynamic list template prototype
 *	\file		libsp/containers/list.h
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

#ifndef SP_LIST_H
#define SP_LIST_H

const int SP_LIST_BUFFER_SIZE = 2;

#define GROW(num) (9 * num / 8)

/// Dynamic linear list template ('vector')
///	\warning:	Only single-threaded version!!!
/// \warning:	Don't use SPList<Object>, where Object uses ctors for initialization.
///				Ctors will not be called! Use SPList<Object *> or SPClassicList<Object> instead!
/// \warning:	Copying lists is SLOW 'cause copy ctors are called for items.
template <typename T>
class SPList
{
public:

	/// comparison function prototype (see Sort())
	typedef int (*SPListCompareFunc)(T *e1, T *e2);

	/// Ctor. Creates empty list
	SPList ()
	{
		l = (T *)SPmalloc (sizeof(T) * SP_LIST_BUFFER_SIZE);
		//SPCHECKMSG(l != NULL, "Memory error");
		lastnum = num = 0;
	}

	/// Ctor. Creates list with n elements.
	SPList (int n)
	{
		lastnum = num = n;
		l = (T *)SPmalloc ((lastnum + SP_LIST_BUFFER_SIZE) * sizeof(T));
	}

	/// Copy ctor.
	SPList (const SPList & list)
	{
		lastnum = num = list.num;
		l = (T *)SPmalloc ((lastnum + SP_LIST_BUFFER_SIZE) * sizeof(T));
		//SPCHECKMSG(l != NULL, "Memory error");
		if (num > 0)
		{
			for (int i = 0; i < num; i++)
			{
				l[i] = list.l[i];	// call elements' copy ctors
			}
		}
	}

	SPList & operator = (const SPList & list)
	{
		lastnum = num = list.num;
		SPSafeFree(l);
		l = (T *)SPmalloc ((lastnum + SP_LIST_BUFFER_SIZE) * sizeof(T));
		//SPCHECKMSG(l != NULL, "Memory error");
		if (num > 0)
		{
			for (int i = 0; i < num; i++)
			{
				l[i] = list.l[i];	// call elements' copy ctors
			}
		}
		return (*this);
	}
	
	/// Dtor. Destroys only the list, not the elements.
	~SPList ()
	{
		SPSafeFree(l);
		lastnum = num = 0;
	}

	/// Reserve memory for N items
	BOOL Reserve(int num)
	{
		if (lastnum <= num)
		{
			lastnum = num + 1;
			l = (T *)SPrealloc ((void *)l, (lastnum + SP_LIST_BUFFER_SIZE) * sizeof(T));
			if (l == NULL)
			{
				SPERRMES("Memory error");
				return FALSE;
			}
			return TRUE;
		}
		return FALSE;
	}

	/// Add item to the end of the list
	int Add (T item)
	{
		if (num + 1 >= lastnum + SP_LIST_BUFFER_SIZE)	// need to expand buffer
		{
			lastnum = GROW(num) + 1;
			l = (T *)SPrealloc ((void *)l, (lastnum + SP_LIST_BUFFER_SIZE) * sizeof(T));
			if (l == NULL)
			{
				SPERRMES("Memory error");
				return -1;
			}
		}
		l[num] = item;
		return num++;
	}

	/// Add one list to the end of another list.
	/// Returns the last element added.
	int Add (const SPList<T> & list)
	{
		int ret = -1;
		for (int i = 0; i < list.GetN(); i++)
			ret = Add (list[i]);
		return ret;
	}

	/// Add unique item to the list, and return the existing or new index.
	int Merge (T item)
	{
		int g = Get (item);
		if (g == -1)
			return Add (item);
		return g;
	}
	
	/// Merge lists (avoid item duplicates)
	int Merge (const SPList<T> & list)
	{
		int ret = -1;
		for (int i = 0; i < list.GetN(); i++)
		{
			T item = list[i];
			if (Get (item) == -1)
				ret = Add (item);
		}
		return ret;
	}

	/// Insert the 'item' element to the 'where' position in the list
	bool Insert (T item, int where)
	{
		int n = num;
		if (where < 0)
			return FALSE;
		if (where > (int)num)
			n = where;
		
		if (n + 1 >= lastnum + SP_LIST_BUFFER_SIZE)	// need to expand buffer
		{
			lastnum = n + 1;
			l = (T *)SPrealloc ((void *)l, (lastnum + SP_LIST_BUFFER_SIZE) * sizeof(T));
			if (l == NULL)
			{
				SPERRMES("Memory error");
				return FALSE;
			}
		}
		
		register int i;
		for (i = num; i > where; i--)
			l[i] = l[i - 1];
		
		l[where] = item;
		num = n + 1;
		return TRUE;
	}

	/// Replace item with new one; expand list if needed.
	bool Put (T item, int where)
	{
		if (where < 0)
			return FALSE;
		if (where >= (int)num)
		{
			num = where + 1;
			if (num >= lastnum + SP_LIST_BUFFER_SIZE)	// need to expand buffer
			{
				lastnum = num;
				l = (T *)SPrealloc ((void *)l, (lastnum + SP_LIST_BUFFER_SIZE) * sizeof(T));
				if (l == NULL)
				{
					SPERRMES("Memory error");
					return FALSE;
				}
			}
		}
		l[where] = item;
		return TRUE;
	}

	/// Find item's index number
	int Get (const T & item, int idx1 = 0, int idx2 = -1)
	{
		if (idx1 < 0)
			return -1;
		if (idx2 < 0) idx2 = num - 1;
		for (int i = idx1; i <= idx2; i++)
			if (l[i] == item)
				return i;
		return -1;
	}

	/// Remove the item from the list
	bool Remove (int n)
	{
		if (n < 0 || n >= (int)num)
			return FALSE;
		for (int i = n; i < num - 1; i++)
			l[i] = l[i + 1];
		if (num >= 1)
		{
			if (num - 1 < lastnum)	// need to shrink buffer
			{
				l = (T *)SPrealloc ((void *)l, lastnum * sizeof(T));
				if (lastnum > SP_LIST_BUFFER_SIZE)
					lastnum -= SP_LIST_BUFFER_SIZE;
				else
					lastnum = 0;
				if (l == NULL)
				{
					SPERRMES("Memory error");
					return FALSE;
				}
			}
			num--;
			return TRUE;
		}
		return FALSE;
	}

	/// Remove item from the list (search used)
	bool Remove (const T & item)
	{
		int i;
		while ((i = Get (item)) != -1)
		{
			if (!Remove (i))
				return FALSE;
		}
		return TRUE;
	}

	/// Remove elements from 'this' list found in the 'list'.
	/// Returns the number of elements removed.
	int Remove (const SPList<T> & list)
	{
		int numdel = 0;
		for (int i = 0; i < list.GetN(); i++)
		{
			int ret = Get (list[i]);
			if (ret != -1)
			{
				Remove (ret);
				numdel++;
			}
		}
		return numdel;
	}

	/// Remove all items from the list
	bool Clear ()
	{
		if (lastnum > 1)
			l = (T *)SPrealloc ((void *)l, sizeof(T) * SP_LIST_BUFFER_SIZE);
		if (l == NULL)
			return FALSE;
		lastnum = num = 0;
		return TRUE;
	}

	/// Moves item in the list (delete + insert)
	bool Move (int from, int to)
	{
		if (from < 0 || from >= (int)num)
			return FALSE;
		T item = l[from];
		if (Remove (from) == FALSE)
			return FALSE;
		if (Insert (item, to) == FALSE)
			return FALSE;
		return TRUE;
	}

	/// Delete all objects themself and clear the list
	bool DeleteObjects ()
	{
		for (int i = 0; i < num; i++)
			SPSafeDelete(l[i]);
		return Clear ();
	}

	/// The main item access operator
	FORCEINLINE T & operator [] (int n) const
	{
		return l[n];
	}

	/// List concatenation and assign
	SPList<T> & operator += (const SPList<T> & list)
	{
		Add (list);
		return *this;
	}

	/// Boolean 'and' operation - returns elements present in both of the lists
	SPList<T> & operator &= (const SPList<T> & list)
	{
		for (int i = 0; i < num; i++)
		{
			if (list.Get(l[i]) == -1)
				this->Delete (i);
		}
		return *this;
	}

	/// Boolean 'or' operation - returns unique elements present in any of the lists
	SPList<T> & operator |= (const SPList<T> & list)
	{
		Merge(list);
		return *this;
	}

	/// Add one list to another and return the sum.
	SPList<T> operator + (const SPList<T> & list)
	{
		SPList<T> r(*this);
		r.Add(list);
		return r;	
	}

	/// Return the elements of the first list not present in the second.
	SPList<T> operator - (const SPList<T> & list)
	{
		SPList<T> r;
		for (int i = 0; i < num; i++)
		{
			if (list.Get(l[i]) == -1)
				r.Add (l[i]);
		}
		return r;
	}

	/// Sort list using user-defined comparison function
	void Sort (SPListCompareFunc compare)
	{
		qsort((void *)l, num, sizeof(T), 
			(int (/*__cdecl*/ *)(const void *, const void *))compare);
	}

	/// Sort list range using user-defined comparison function
	void Sort (SPListCompareFunc compare, int idx1, int idx2)
	{
		qsort((void *)(l + idx1), idx2 - idx1 + 1, sizeof(T), 
			(int (/*__cdecl*/ *)(const void *, const void *))compare);
	}

	/// Get items number (count)
	FORCEINLINE const int & GetN () const
	{
		return num;
	}

	/// Set items count (reallocate the memory but don't change the data)
	BOOL SetN(int n)
	{
	    num = n;
	    if ((num >= lastnum + SP_LIST_BUFFER_SIZE) || // need to expand buffer
        	(num <= lastnum - SP_LIST_BUFFER_SIZE))	  // need to shrink buffer
	    {
	    	lastnum = num;
			l = (T *)SPrealloc ((void *)l, (lastnum + SP_LIST_BUFFER_SIZE) * sizeof(T));
			if (l == NULL)
			{
				SPERRMES("Memory error");
				return FALSE;
			}
		}
		return TRUE;
	}

	// Set items count and clear them
	BOOL SetClearN(int n)
	{
		if (!SetN(n))
			return FALSE;
		memset(l, 0, n * sizeof(T));
		return TRUE;
	}

	/// This is 'hack' method used by fast-loading operations
	T *GetData() const
	{
		return l;
	}

	/// List concatenation operator
	template <typename U> friend SPList<U> operator + (const SPList<U> &, const SPList<U> &);
	/// Boolean 'or' operation - returns unique elements present in any of the lists
	template <typename U> friend SPList<U> operator | (const SPList<U> &, const SPList<U> &);
	/// Boolean 'and' operation - returns elements present in both of the lists
	template <typename U> friend SPList<U> operator & (const SPList<U> &, const SPList<U> &);

protected:
	/// Dynamic array, contains all data
	T *l;
	/// Number of stored items. Use GetN() for retrieving
	int num;
	int lastnum;	/// used for bufferized memory reallocs
};

/// List concatenation operator
template <typename T> inline SPList<T> operator + (const SPList<T> &a, const SPList<T> &b)
{
	return (SPList <T>(a) += (b));
}

/// Boolean 'or' operation - returns unique elements present in any of the lists
template <typename T> inline SPList<T> operator | (const SPList<T> &a, const SPList<T> &b)
{
	return (SPList<T>(a) |= b);
}

/// Boolean 'and' operation - returns elements present in both of the lists
template <typename T> inline SPList<T> operator & (const SPList<T> &a, const SPList<T> &b)
{
	return (SPList<T>(a) &= b);
}

/// string list
typedef SPList<char *> SP_LIST_STR;
/// dword values list
typedef SPList<DWORD> SP_LIST_DWORD;

#endif // of SP_LIST_H
