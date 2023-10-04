//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - abstract resource class impl.
 *  \file       res.cpp
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

#define _BSD_SOURCE	// needed for strdup()
#include <string.h>

#include <libsp/sp_misc.h>
#include <gui/res.h>

void Resource::SetName(char *n)
{
	SPSafeFree(name);
	if (n == NULL)
	{
		name = NULL;
		namehash = 0;
		return;
	} 
	name = SPstrdup(n);
	constname = false;
	namehash = SPStringHashFunc(name, resource_num_hash);
}

void Resource::SetConstName(const char *n)
{
	SPSafeFree(name);
	constname = true;
	if (n == NULL)
	{
		name = NULL;
		namehash = 0;
		return;
	} 
	name = (char *)n;
	namehash = SPStringHashFunc(name, resource_num_hash);
}

