//////////////////////////////////////////////////////////////////////////
/**
 * support lib small memory bin header
 *	\file		libsp/containers/membin.h
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

#ifndef SP_MEMORY_BIN_H
#define SP_MEMORY_BIN_H

/*
///////////////////////////////////////////////////////////////////////
   Used for small memory (<64k) chunks to decrease memory fragmentation.
   All mem.blocks are WORD-aligned.
   Warning! No bounds-check/mem.leak/debug control!
///////////////////////////////////////////////////////////////////////
*/

/// Allocate or reallocate small memory chunk
void *membin_alloc(void *, size_t);

/// Free small memory chunk
void membin_free(void *);

/// Use next memory bit for new allocs (faster?)
void membin_next();

#endif // of SP_MEMORY_BIN_H
