//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - abstract resource class header file
 *  \file       res.h
 *  \author     bombur
 *  \version    0.1
 *  \date       14.10.2005
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

#ifndef SP_RES_H
#define SP_RES_H


const int resource_num_hash = 347;

/// Loadable resource
class Resource
{
public:
	/// ctor
	Resource()
	{
		prev = NULL;
		next = NULL;
		
		name = NULL;
		namehash = 0;

		index = -1;
		temporary = true;
		constname = true;
	}

	/// dtor
	virtual ~Resource()
	{
		if (!constname)
			SPSafeFree(name);
	}

	/// compare
	template <typename T>
	bool operator == (const T & r)
	{
		if (name == NULL || r.name == NULL)
			return false;
		return strcmp(name, r.name) == 0;
	}

	inline operator DWORD () const
	{
		return namehash;
	}

	const Resource * const GetItem() const
	{
		return this;
	}

	/// Set variable name & calc. hash
	void SetName(char *n);	
	/// Set constant name & calc. hash
	/// Use this for const.strings to avoid strdup().
	void SetConstName(const char *n);	

public:
	bool temporary, constname;
	char *name;
	DWORD namehash;
	
	int index;

	// linked list support
	Resource *prev, *next;
};


#endif // of SP_RES_H
