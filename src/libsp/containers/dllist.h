//////////////////////////////////////////////////////////////////////////
/**
 * support lib double-linked dynamic list template prototype
 *	\file		libsp/containers/dllist.h
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

#ifndef SP_DLLIST_H
#define SP_DLLIST_H

/// Double-linked list abstract container. Assumes that 'T' has 'next' and 'prev' fields.
/// For ordinary item classes, use SPDLinkedList instead.
///	\warning	Only single-threaded version!!!
///	\warning	Check the params! Must be (elem1 < elem2) and (from < to) !
template <typename T, typename BaseT>
class SPDLinkedListAbstract
{
protected:
	/// the first and last elements
	T *first, *last;
    /// size for the stacks. 
    /// >0 means first deleting (FIFO), <0 means last one (FILO).
	int size;
	int num;	/// real number of elements (for optimization only)

public:
	/// ctor
	SPDLinkedListAbstract()
	{
		first = last = NULL;
		num = size = 0;
	}
	
	/// copy ctor
	SPDLinkedListAbstract(const SPDLinkedListAbstract & dllist)
	{
		first = dllist.first;
		last = dllist.last;
		size = dllist.size;
		num = dllist.num;
	}

    /// copy ctor
	SPDLinkedListAbstract(T *elem1, T *elem2 = NULL)
	{
		first = elem1;
		if (elem2 != NULL)
			last = elem2;
		else
			last = elem1;
		size = 0;
		num = GetNum();
	}

	/// dtor
	~SPDLinkedListAbstract()
	{
		///Delete();
	}

	/// Get the first element of the list
	T *GetFirst()
	{
		return first;
	}

    /// Get the last element of the list
	T *GetLast()
	{
		return last;
	}

	/// Get(Find) the element by it's pointer
	T *Get(/*const */BaseT *t)
	{
		T *cur = first;
	   	for (int i = 0; cur != NULL; i++)
		{
			if (*cur == *t)
				return cur;
			cur = cur->next;
		}
		return NULL;
	}
	
	/// Get the element with the given offset from the current
	T *Get(T *from, int offset)
    {
    	T *cur = from;
		if (offset > 0)
    	{
	    	for (int i = 0; i < offset && cur != NULL; i++)
			{
				cur = (T *)cur->next;
			}
	    } else
	    {
	    	for (int i = 0; i > offset && cur != NULL; i--)
			{
				cur = (T *)cur->prev;
			}
    	}
		return cur;
   	}

    /// Get the next element of the list
    T *GetNext(T *from, int offset = 1)
	{
        if (from != NULL)
        {
			if (offset < 0)
				offset = -offset;
			if (offset == 1)
				return (T *)from->next;
			return Get(from, offset);
	    }
		return NULL;
	}

    /// Get the previous element of the list
    T *GetPrev(T *from, int offset = 1)
	{
		if (from != NULL)
		{
			if (offset > 0)
				offset = -offset;
			if (offset == -1)
       			return from->prev;
			return Get(from, offset);
        }
		return NULL;
	}

    /// The numbered item access operator
	T *operator [] (int n)
	{
		if (first == NULL || last == NULL)
			return NULL;
        T *cur = first;
		for (int i = 0; i < n; i++)
		{
			if (cur->next == NULL)
				break;
			cur = cur->next;
		}
		return cur;
	}

    /// Get the number of given elements (distance). NULLs = all list
    /// ! If the order is reversed, the number is negative !
    int GetNum(T *from = NULL, T *to = NULL)
	{
		if (from == NULL)
			from = first;
		if (to == NULL)
			to = last;
		int n = 0;
		BOOL found = FALSE;
		if (from == first && to == last)
			return num;
		if (to != first && from != last)
		{
	        for (T *cur = from; cur != NULL; n++)
			{
				if (cur == to)
				{
					n++;
					found = TRUE;
					break;
			    }
				cur = (T *)cur->next;
			}
		} else
		{
			if (from == first || to == last)
				return 1;
			if (from == last && to == first)
				return -num;
			return 0;
		}
	    
		if (!found)
		{
			n = 0;
			for (T *cur = to; cur != NULL; n--)
			{
				if (cur == from)
				{
					n--;
					found = TRUE;
					break;
			    }
				cur = (T *)cur->prev;
			}
	    }
	    //SPASSERT(found);
		return n;
	}

	/// Set the maximum list size
	BOOL SetSize(int s)
	{
		size = s;
		if (s == 0)
			return TRUE;
        int n = (int)num;
        if (n > abs(size))
        {
        	if (size > 0)
				Delete(first, n - size);
		    else
		    	Delete(last, -n - size);
        }
        return TRUE;
	}

	/// Insert the element range to the list after the 'from' element
	BOOL InsertAfter(T *from, T *elem1, T *elem2 = NULL)
	{
		int addn = 1;
		if (elem2 == NULL)
			elem2 = elem1;
		else
			addn = (int)GetNum(elem1, elem2);
		if (first == NULL || last == NULL)
		{
			first = elem1;
			last = elem2;
			num = addn;
			return TRUE;
		}
		if (from == NULL || elem1 == NULL)
			return FALSE;

		if (size != 0)
		{
			int n = num;
			if (n + addn > Abs(size))
			{
				if (size > 0)
					Delete(first, n + addn - size - 1);
			    else
			    	Delete(last, -n - addn - size - 1);
			}
	    }
    	num += addn;

		if (from->next != NULL)
		{
			from->next->prev = elem1;
			elem2->next = from->next;
		} else
		{
			elem2->next = NULL;
			last = elem2;
		}
		from->next = elem1;
		elem1->prev = from;
		return TRUE;
	}

    /// Insert the elements in the list after the 'from' element
	BOOL InsertAfter(T *from, const SPDLinkedListAbstract & ll)
	{
		return InsertAfter(from, ll.first, ll.last);
	}

    /// Insert the element range to the list before the 'from' element
	BOOL InsertBefore(T *from, T *elem1, T *elem2 = NULL)
	{
        int addn = 1;
        if (elem2 == NULL)
			elem2 = elem1;
		else
			addn = GetNum(elem1, elem2);
		if (first == NULL || last == NULL)
		{
			first = elem1;
			last = elem2;
			num = addn;
			return TRUE;
		}
		if (from == NULL || elem1 == NULL)
			return FALSE;

		if (size != 0)
		{
			int n = num;
			if (n + addn > Abs(size))
			{
				if (size > 0)
					Delete(first, n + addn - Abs(size));
			    else
			    	Delete(last, Abs(size) - n - addn);
		    }
	    } else
	    	num += addn;

		if (from->prev != NULL)
		{
			from->prev->next = elem1;
			elem1->prev = from->prev;
		} else
		{
			elem1->prev = NULL;
			first = elem1;
		}
		from->prev = elem2;
		elem2->next = from;
		return TRUE;
	}

    /// Insert the elements in the list before the 'from' element
	BOOL InsertBefore(T *from, const SPDLinkedListAbstract & ll)
	{
		return InsertBefore(from, ll.first, ll.last);
	}

	/// Add the elements to the list
	BOOL Add(T *elem1, T *elem2 = NULL)
	{
		return InsertAfter(last, elem1, elem2);
	}

	/// Allocate and add element to the list (stack push)
	BOOL Push(const BaseT & elem)
	{
		T *e = new T(elem);
		return Add(e);
	}

	/// Retreive the last element and remove it from the list (stack pop)
	T & Pop()
	{
		T *e = GetLast();
		BaseT & t = e->item;
		e->item = NULL;
		Delete(e);
		return t;
	}


	/// Remove the elements range from the list
	BOOL Remove(T *from, T *to = NULL)
	{
		int remn = 1;
		if (from == NULL)
		{
			if (to != NULL)
				from = to;
			else 
				return FALSE;
		}
		if (to == NULL)
			to = from;
		else
			remn = GetNum(from, to);

		if (from == first)
			first = (T *)to->next;
		if (to == last)
			last = (T *)from->prev;
		if (from->prev != NULL)
			from->prev->next = to->next;
		if (to->next != NULL)
			to->next->prev = from->prev;
		from->prev = NULL;
		to->next = NULL;
		num -= remn;
		return TRUE;
	}

	/// Remove and delete the elements in the range
	BOOL Delete(T *from = NULL, T *to = NULL)
	{
		if (from == NULL)
		{
			if(to != NULL)
			{
				from = to;
				to = NULL;
			} else	// delete entire list
			{
				from = first;
				to = last;
			}
		}
		if (first == NULL || last == NULL)
			return TRUE;

		if (Remove(from, to) == FALSE)
			return FALSE;

		T *cur = from;
		while (cur != NULL)
		{
            T *next = (T *)cur->next;
            ///////////
			if (cur != NULL)
				delete cur;
            ///////////
            if (cur == to)
            	break;
			cur = next;
		}

		return TRUE;
	}

    /// Remove and delete 'n' elements starting from the current
    BOOL Delete(T *from, int n)
    {
    	T *to = Get(from, n);
    	return Delete(from, to);
   	}

    /// Detach the elements range from the list and put them to the new one.
	SPDLinkedListAbstract *Detach(T *from, T *to = NULL)
	{
   		if (Remove(from, to) == FALSE)
			return NULL;
		SPDLinkedListAbstract *ll = new SPDLinkedListAbstract;
		ll->size = size;
		ll->start = from;
		if (to != NULL)
			ll->end = to;
		else
			ll->end = from;
		return ll;
	}

	/// Move elements in the list to the right or left (sign of 'offset')
	BOOL Move(int offset, T *elem1, T *elem2 = NULL)
	{
		if (offset == 0)
			return TRUE;
		if (elem2 == NULL)
			elem2 = elem1;
		if (offset > 0)
		{
			T *to = Get(elem2, offset);
            Remove(elem1, elem2);
			if (to == NULL)
				return InsertAfter(last, elem1, elem2);
            return InsertAfter(to, elem1, elem2);
		}
		T *to = Get(elem1, offset);
        Remove(elem1, elem2);
		if (to == NULL)
			return InsertBefore(first, elem1, elem2);
		return InsertBefore(to, elem1, elem2);
	}

	/// Move elements in the list to the specified place (instead of 'to')
	/// \todo: optimise and check it!!!
	BOOL Move(T *to, T *elem1, T *elem2 = NULL)
	{
		Remove(elem1, elem2);
		if (to == NULL)
			return InsertBefore(first, elem1, elem2);
		if (to == last)
        	return InsertAfter(to, elem1, elem2);
        if (GetNum(elem1, to) > 0)
        	return InsertAfter(to, elem1, elem2);
        return InsertBefore(to, elem1, elem2);
	}

	/// Swaps two elements, the order isn't important (elem1<>elem2)
	BOOL Swap(T *elem1, T *elem2)
	{
		if (elem1 == NULL || elem2 == NULL)
			return FALSE;

		BOOL closeto1 = (elem1->next == elem2);
		BOOL closeto2 = (elem2->next == elem1);
		
		if (elem1->prev != NULL && !closeto2)
			elem1->prev->next = elem2;
        if (elem1->next != NULL && !closeto1)
			elem1->next->prev = elem2;
        if (elem2->prev != NULL && !closeto1)
			elem2->prev->next = elem1;
        if (elem2->next != NULL && !closeto2)
			elem2->next->prev = elem1;
	    
	    if (elem1 == first)
	    	first = elem2;
        else if (elem2 == first)
	    	first = elem1;
	    if (elem1 == last)
	    	last = elem2;
	    else if (elem2 == last)
	    	last = elem1;
        
        T *tmp = elem1->prev;
        if (closeto1)
	        elem1->prev = elem2;
	    else
	        elem1->prev = elem2->prev;
        if (closeto2)
        	elem2->prev = elem1;
        else
        	elem2->prev = tmp;
        
        tmp = elem2->next;
        if (closeto1)
        	elem2->next = elem1;
        else
	        elem2->next = elem1->next;
	    if (closeto2)
	    	elem1->next = elem2;
	    else
        	elem1->next = tmp;
	    return TRUE;
	}

	/// Check if everything is ok with the list (integrity check)
	BOOL Check()
	{
		if (first == NULL && last == NULL)	// empty list
			return TRUE;
		T *cur = first;
		int cnum = 1;
		while (true)
		{
			if (cur->next != NULL)
			{
				cnum++;
				if (cur->next == cur)		// no cyclic chains
					return FALSE;
				if (cur->next->prev != cur)	// breaked chain
					return FALSE;
			} else
				break;
			cur = cur->next;
		}
		if (cur != last)					// invalid 'last' chain marker
			return FALSE;
		if (cnum != num)					// invalid number of elements
			return FALSE;
		if (size != 0 && cnum > size)		// number of elements exceeded
			return FALSE;
		return TRUE;
	}

	/// Check if the element belongs to the list
	BOOL IsInList(T *elem)
	{
		if (elem == NULL)
			return FALSE;
		T *cur = first;
		while (cur != NULL)
		{
			if (cur == elem)
				return TRUE;
			cur = cur->next;
		}
		return FALSE;
	}

    /*/// Check if the element belongs to the list
	BOOL IsInList(const T & elem)
	{
		if (elem == NULL)
			return FALSE;
		T *cur = first;
		while (cur != NULL)
		{
			if (cur == elem)
				return TRUE;
			cur = cur->next;
		}
		return FALSE;
	}*/

	/// Check if the list is empty
	BOOL IsEmpty()
	{
		if (first == NULL && last == NULL)
		{
			//if (num != 0)
			//	SPASSERT("DLList damaged!");
			return TRUE;
		}
		return FALSE;
	}
};

////////////////////////////////////////////////////////////////////////////

/// Double-Linked list element. Used as wrapper for ordinary data types.
///	\warning	only single-threaded version!!!
template <class T>
class DLElement
{
public:

	T item;
	DLElement *next, *prev;

	/// ctor
	DLElement()
	{
		next = prev = NULL;
	}
	/// ctor
	DLElement(const T & t)
	{
		item = t;
		next = prev = NULL;
	}
	
	/// copy ctor
	DLElement(const DLElement & e)
	{
		item = e.item;
		next = e.next;
		prev = e.prev;
	}

	/// dtor
	virtual ~DLElement()
	{
//		delete item;
	}

    /// ref-counted copy from another DLElement
    const DLElement & operator=(const DLElement & e)
    {
    	item = e.item;
		next = e.next;
		prev = e.prev;
		return *this;
   	}

    /// ref-counted copy from 'T' object
    const DLElement & operator=(const T & src)
    {
    	item = src;
		next = NULL;
		prev = NULL;
		return *this;
   	}

	/// compare
	bool operator == (const T & t)
	{
		return item == t;
	}

    /// 'T' conversion operator
    operator const T & ( ) const
    {
    	return item;
   	}

	const T & GetItem() const
	{
		return item;
	}

	void SetItem(const T & t)
	{
		item = t;
	}

};


/// Double-linked list manager. Use it for any item classes/types.
///	\warning	Only single-threaded version!!!
///	\warning	Check the params! Must be (elem1 < elem2) and (from < to) !
template <class T>
class SPDLinkedList : public SPDLinkedListAbstract<DLElement<T>, T >
{
};

#endif // of SP_DLLIST_H
