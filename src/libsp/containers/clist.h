//////////////////////////////////////////////////////////////////////////
/**
 * support lib linear classic (new-based) dynamic list template prototype
 *	\file		libsp/containers/clist.h
 *	\author		bombur
 *	\version	x.xx
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

#ifndef SP_CLIST_H
#define SP_CLIST_H

const int SP_CLIST_BUFFER_SIZE = 2;
#define GROW(num) (9 * num / 8)

/// Dynamic classic linear list template ('vector')
/// Does all needed classes handling correctly (but a little bit slower)!
///	\warning:	Only single-threaded version!!!
///	\warning:	List resizing is slower than SPList version!
/// \warning:	Copying lists is SLOW 'cause copy ctors are called for items.
template <typename T>
class SPClassicList
{
public:

	/// comparison function prototype (see Sort())
	typedef int (*SPClassicListCompareFunc)(T *e1, T *e2);

	/// Ctor. Creates empty list
	SPClassicList ()
	{
		l = new T [SP_CLIST_BUFFER_SIZE];
		//SPCHECKMSG(l != NULL, "Memory error");
		lastnum = num = 0;
	}

	/// Ctor. Creates list with n elements.
	SPClassicList (int n)
	{
		lastnum = num = n;
		l = new T [lastnum + SP_CLIST_BUFFER_SIZE];
	}

	/// Copy ctor.
	SPClassicList (const SPClassicList & list)
	{
		lastnum = num = list.num;
		l = new T [lastnum + SP_CLIST_BUFFER_SIZE];
		//SPCHECKMSG(l != NULL, "Memory error");
		if (num > 0)
		{
			for (int i = 0; i < num; i++)
			{
				l[i] = list.l[i];	// call elements' copy ctors
			}
		}
	}

	SPClassicList & operator = (const SPClassicList & list)
	{
		lastnum = num = list.num;
		delete [] l;
		l = new T [lastnum + SP_CLIST_BUFFER_SIZE];
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
	~SPClassicList ()
	{
		SPSafeDeleteArray(l);
		lastnum = num = 0;
	}

	/// Reserve memory for N items
	BOOL Reserve(int n)
	{
		lastnum = n + 1;

		T *newl = new T [lastnum + SP_CLIST_BUFFER_SIZE];
		for (int i = 0; i < num; i++)
		{
			newl[i] = l[i];	// call elements' copy ctors
		}
		delete [] l;
		l = newl;

		if (l == NULL)
		{
			SPERRMES("Memory error");
			return FALSE;
		}
		return TRUE;
	}

	/// Add item to the end of the list
	int Add (const T & item)
	{
		if (num + 1 >= lastnum + SP_CLIST_BUFFER_SIZE)	// need to expand buffer
		{
			lastnum = GROW(num) + 1;
			
			T *newl = new T [lastnum + SP_CLIST_BUFFER_SIZE];
			for (int i = 0; i < num; i++)
			{
				newl[i] = l[i];	// call elements' copy ctors
			}
			delete [] l;
			l = newl;

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
	int Add (const SPClassicList<T> & list)
	{
		int ret = -1;
		for (int i = 0; i < list.GetN(); i++)
			ret = Add (list[i]);
		return ret;
	}

	/// Add unique item to the list, and return the existing or new index.
	int Merge (const T & item)
	{
		int g = Get (item);
		if (g == -1)
			return Add (item);
		return g;
	}
	
	/// Merge lists (avoid item duplicates)
	int Merge (const SPClassicList<T> & list)
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
		
		if (n + 1 >= lastnum + SP_CLIST_BUFFER_SIZE)	// need to expand buffer
		{
			lastnum = n + 1;

			T *newl = new T [lastnum + SP_CLIST_BUFFER_SIZE];
			for (int i = 0; i < num; i++)
			{
				newl[i] = l[i];	// call elements' copy ctors
			}
			delete [] l;
			l = newl;

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
			if (num >= lastnum + SP_CLIST_BUFFER_SIZE)	// need to expand buffer
			{
				lastnum = num;
				
				T *newl = new T [lastnum + SP_CLIST_BUFFER_SIZE];
				for (int i = 0; i < num; i++)
				{
					newl[i] = l[i];	// call elements' copy ctors
				}
				delete [] l;
				l = newl;

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
				T *newl = new T [lastnum];
				for (int i = 0; i < num-1; i++)
				{
					newl[i] = l[i];	// call elements' copy ctors
				}
				delete [] l;
				l = newl;

				if (lastnum > SP_CLIST_BUFFER_SIZE)
					lastnum -= SP_CLIST_BUFFER_SIZE;
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
	int Remove (const SPClassicList<T> & list)
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
		delete [] l;
		l = new T [SP_CLIST_BUFFER_SIZE];
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
	SPClassicList<T> & operator += (const SPClassicList<T> & list)
	{
		Add (list);
		return *this;
	}

	/// Boolean 'and' operation - returns elements present in both of the lists
	SPClassicList<T> & operator &= (const SPClassicList<T> & list)
	{
		for (int i = 0; i < num; i++)
		{
			if (list.Get(l[i]) == -1)
				this->Delete (i);
		}
		return *this;
	}

	/// Boolean 'or' operation - returns unique elements present in any of the lists
	SPClassicList<T> & operator |= (const SPClassicList<T> & list)
	{
		Merge(list);
		return *this;
	}

	/// Add one list to another and return the sum.
	SPClassicList<T> operator + (const SPClassicList<T> & list)
	{
		SPClassicList<T> r(*this);
		r.Add(list);
		return r;	
	}

	/// Return the elements of the first list not present in the second.
	SPClassicList<T> operator - (const SPClassicList<T> & list)
	{
		SPClassicList<T> r;
		for (int i = 0; i < num; i++)
		{
			if (list.Get(l[i]) == -1)
				r.Add (l[i]);
		}
		return r;
	}

	/// Sort list using user-defined comparison function
	void Sort (SPClassicListCompareFunc compare)
	{
		qsort((void *)l, num, sizeof(T), 
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
	    int oldnum = num;
	    num = (int)n;
	    if ((num >= lastnum + SP_CLIST_BUFFER_SIZE) || // need to expand buffer
        	(num <= lastnum - SP_CLIST_BUFFER_SIZE))	  // need to shrink buffer
	    {
	    	lastnum = num;
			T *newl = new T [lastnum + SP_CLIST_BUFFER_SIZE];
			for (int i = 0; i < (n < oldnum ? n : oldnum); i++)
			{
				newl[i] = l[i];	// call elements' copy ctors
			}
			delete [] l;
			l = newl;

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
	template <typename U> friend SPClassicList<U> operator + (const SPClassicList<U> &, const SPClassicList<U> &);
	/// Boolean 'or' operation - returns unique elements present in any of the lists
	template <typename U> friend SPClassicList<U> operator | (const SPClassicList<U> &, const SPClassicList<U> &);
	/// Boolean 'and' operation - returns elements present in both of the lists
	template <typename U> friend SPClassicList<U> operator & (const SPClassicList<U> &, const SPClassicList<U> &);

protected:
	/// Dynamic array, contains all data
	T *l;
	/// Number of stored items. Use GetN() for retrieving
	int num;
	int lastnum;	/// used for buffered memory reallocs
};

/// List concatenation operator
template <typename T> inline SPClassicList<T> operator + (const SPClassicList<T> &a, const SPClassicList<T> &b)
{
	return (SPClassicList <T>(a) += (b));
}

/// Boolean 'or' operation - returns unique elements present in any of the lists
template <typename T> inline SPClassicList<T> operator | (const SPClassicList<T> &a, const SPClassicList<T> &b)
{
	return (SPClassicList<T>(a) |= b);
}

/// Boolean 'and' operation - returns elements present in both of the lists
template <typename T> inline SPClassicList<T> operator & (const SPClassicList<T> &a, const SPClassicList<T> &b)
{
	return (SPClassicList<T>(a) &= b);
}

/// string list
typedef SPClassicList<char *> SP_CLIST_STR;
/// dword values list
typedef SPClassicList<DWORD> SP_CLIST_DWORD;

#endif // of SP_CLIST_H
