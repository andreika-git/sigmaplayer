//////////////////////////////////////////////////////////////////////////
/**
 * support lib hash list (search) template prototype
 *	\file		libsp/containers/hashlist.h
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

#ifndef SP_HASHLIST_H
#define SP_HASHLIST_H

#include <string.h>

/// Hash-table list (search-optimised) template.
/// Use SetN() first, then Add() or Merge() to add elements;
/// Get() searches for elements, and GetN() returns the actual number stored.
///	\warning	only single-threaded version!!!
template <typename T, class BaseT>
class SPHashListAbstract : public SPClassicList<SPDLinkedListAbstract<T, BaseT> >
{
public:
	///////////////////////////////////////////////////////////////
	/// Hash function - declare it in derived class.
	/// The return value should be in the range of [0..num-1]
	/// DO NOT USE THIS IMPL. ON POINTERS!!!
	virtual DWORD HashFunc(const BaseT & item)
	{
		return (DWORD)item % ((this->num > 0) ? this->num : 1);
	}

    /// Comparison function - declare it in derived class
	/// DO NOT USE THIS IMPL. ON POINTERS!!!
    virtual BOOL Compare(const BaseT & item1, const BaseT & item2)
    {
    	return ((T &)item1 == (T &)item2);
    }
	///////////////////////////////////////////////////////////////

	/// Ctor. Creates  list with N hash-size
	SPHashListAbstract (int N = 1) : SPClassicList<SPDLinkedListAbstract<T, BaseT> >()
	{
		this->SetN(N);
	}

	/// Copy ctor.
	SPHashListAbstract (const SPHashListAbstract & list) 
				: SPClassicList<SPDLinkedListAbstract<T, BaseT> >(list)
	{
	}

	/// Dtor. Destroys only the list, not the elements.
	virtual ~SPHashListAbstract ()
	{
		DeleteObjects();
	}

	/// Add item to the list
	/// Do not pass temporary variables!!!
	BOOL Add (BaseT *item)
	{
		if (this->num == 0)
			return FALSE;
		DWORD f = HashFunc(*item);
		return this->l[f].Add((T *)item);
	}

    /// Add item to the list ONLY IF UNIQUE, and return the existing or new one.
	/// Do not pass temporary variables!!!

	BaseT *Merge (const BaseT & item)
	{
		DWORD f = HashFunc((BaseT)item);
		T *cur = this->l[f].GetFirst();
		while (cur != NULL)
		{
			if (Compare((BaseT)cur->GetItem(), (BaseT)item))
				return (BaseT *)&cur->GetItem();
			cur = cur->next;
		}
		if (this->l[f].Add((T *)item) == FALSE)
			return NULL;
		return &(BaseT)item;
	}

	/// Find item (return pointer to existing item or NULL)
	BaseT *Get (const BaseT & item)
	{
		DWORD f = HashFunc(item);
        T *cur = this->l[f].GetFirst();
		while (cur != NULL)
		{
			if (Compare(*((BaseT *)cur->GetItem()), item))
				return (BaseT *)cur->GetItem();
			cur = (T *)cur->next;
		}
		return NULL;
	}

	/// Find the first item of the hash-list
	/// \WARNING: Works only with pointer items!
	BaseT *GetFirst()
	{
    	if (this->num == 0)
    		return NULL;
    	for (int f = 0; f < this->num; f++)
    	{
    		if (this->l[f].GetFirst() != NULL)
    			return (BaseT *)(this->l[f].GetFirst()->GetItem());
    	}
    	return NULL;
   	}

	/// Find the next item of the specified one (or the first, if called with 'NULL')
	/// \WARNING: Works only with pointer items!
	BaseT *GetNext(const BaseT & item)
	{
		int f = HashFunc(item);
        T *cur = this->l[f].GetFirst();
		while (cur != NULL)
		{
			if (Compare(*((BaseT *)cur->GetItem()), item))
			{
				if (cur->next == NULL) // get from the next row
				{
					while (++f < this->num)
					{
						if (this->l[f].GetFirst() != NULL)
							return (BaseT *)(this->l[f].GetFirst()->GetItem());
					}
					return NULL;
				} else
					return (BaseT *)(cur->next->GetItem());
			}
			cur = (T *)cur->next;
		}
		return NULL; // not in this list
	}

	/// Get all items as a linear list
	SPList<BaseT> GetList()
	{
		SPList<BaseT> list;
		for (int f = 0; f < this->num; f++)
    	{
    		T *cur = this->l[f].GetFirst();
			while (cur != NULL)
			{
				list.Add(cur->GetItem());
				cur = cur->next;
    		}
		}
		return list;
	}

	/// Remove item from the list (but don't delete the item itself!)
	BOOL Remove (const BaseT & item)
	{
		DWORD f = HashFunc(item);
        T *cur = this->l[f].GetFirst();
		while (cur != NULL)
		{
			if (Compare(*((BaseT *)cur->GetItem()), item))
				return this->l[f].Remove(cur);
			cur = cur->next;
		}
		return FALSE;
	}

	/// Delete item from the list (but don't delete the item itself!)
	BOOL Delete (const BaseT & item)
	{
		DWORD f = HashFunc(item);
        T *cur = this->l[f].GetFirst();
		while (cur != NULL)
		{
			if (Compare(*((BaseT *)cur->GetItem()), item))
				return this->l[f].Delete(cur);
			cur = cur->next;
		}
		return FALSE;
	}

	/// Remove all items from the list
	BOOL Clear ()
	{
		for (int i = 0; i < this->num; i++)
		{
			this->l[i].Remove(this->l[i].GetFirst(), this->l[i].GetLast());
		}
        return TRUE;
	}

	/// Delete all objects ourself and clear the list
	BOOL DeleteObjects ()
	{
		for (int i = 0; i < this->num; i++)
		{
			if (this->l[i].Delete() == FALSE)
				return FALSE;
		}
		return TRUE;
	}

	/// Get items number (count)
	inline int GetN ()
	{
		int n = 0;
		for (int i = 0; i < this->num; i++)
			n += this->l[i].GetN();
		return n;
	}

};


/// Hash list for 'normal' elements, with built-in dllist-element class
template <class T>
class SPHashList : public SPHashListAbstract<DLElement<T>, T>
{
public:
	/// ctor
	SPHashList (int N = 1) : SPHashListAbstract<DLElement<T>, T> (N) {}
	/// Add item to the list
	BOOL Add (const T & item)
	{
		if (this->num == 0)
			return FALSE;
		DWORD f = HashFunc(item);
		DLElement<T> *elem = new DLElement<T>(item);
		return this->l[f].Add(elem);
	}

	/// Add item to the list ONLY IF UNIQUE, and return the existing or new one.
	T *Merge (const T & item)
	{
		DWORD f = HashFunc(item);
		DLElement<T> *cur = this->l[f].GetFirst();
		while (cur != NULL)
		{
			if (Compare(*cur, item))
				return &cur->item;
			cur = cur->next;
		}
        DLElement<T> *elem = new DLElement<T>(item);
		if (this->l[f].Add(elem) == FALSE)
			return NULL;
		return &(elem->item);
	}
};

/// Integer hash table implementation.
/// \warning For testing purposes only.
class SPIntHashList : public SPHashList<int>
{
public:
	/// ctor
	SPIntHashList()
	{
	}
	
	/// dtor
	virtual ~SPIntHashList()
	{
	}

};

// calculate djb2 (Bernstein) string hash
inline DWORD SPStringHashFunc(char * const & item, int /*num*/)
{
	char *s = item;
    DWORD hash = 5381;
    int c;
	if (s != NULL && s[0] != '\0')
	{
		while ((c = *s++) != '\0')
			hash = ((hash << 5) + hash) + c; // = hash * 33 + c;
	}
    return hash;
/*
	DWORD hash = 0;
	char *it = (char *)item;
	for (int i = 0; *it; i++)
		hash += ((DWORD)*it++) ^ (i % num);
	return hash;
*/
}

/// String hash table implementation.
/// \warning For testing purposes only.
class SPStringHashList : public SPHashList<char *>
{
public:
	/// ctor
	SPStringHashList()
	{
	}
	
	/// dtor
	virtual ~SPStringHashList()
	{
	}

    /// Hash function - declare it in derived class.
	/// The return value should be in the range of [0..num-1]
	virtual DWORD HashFunc(char * const & item)	// \todo	Get better function
	{
		if (item == NULL)
			return 0;
		return SPStringHashFunc(item, num) % num;
	}

    /// Comparison function - declare it in derived class
    virtual BOOL Compare(char * const & item1, char* const & item2)
    {
    	return strcmp(item1, item2) == 0;
    }
};

#endif // of SP_HASHLIST_H
