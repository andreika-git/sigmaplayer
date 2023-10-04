//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - MMSL file manager header file
 *  \file       mmsl/mmsl-file.h
 *  \author     bombur
 *  \version    0.1
 *  \date       10.04.2010
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

#ifndef SP_MMSL_FILE_H
#define SP_MMSL_FILE_H

#define MMSO_VERSION 100

#ifdef WIN32
#pragma pack(1)
#endif

typedef struct ATTRIBUTE_PACKED
{
	char magic[4];
	DWORD ver;
	DWORD crc;
} MmslSerFileHeader;

typedef struct ATTRIBUTE_PACKED
{
	int num_var;
	int var_buf_size;
	int num_class;
	int num_event;
	int num_assignedobj;
	int num_expr;
	int num_lut;
	int num_clut;
	int num_assignedvar;
	int num_cond;
	int num_token;
} MmslSerHeader;

typedef struct ATTRIBUTE_PACKED
{
	short ID;
	short idx_vars1, idx_vars2;
} MmslSerClass;

typedef struct ATTRIBUTE_PACKED
{
	short idx_triggertokens1, idx_triggertokens2;
	short idx_conditions1, idx_conditions2;
	short idx_execute1, idx_execute2;
} MmslSerEvent;

typedef struct ATTRIBUTE_PACKED
{
	short idx_assigned_vars1, idx_assigned_vars2;
} MmslSerAssignedObject;

typedef struct ATTRIBUTE_PACKED
{
	short idx_tokens1, idx_tokens2;
} MmslSerExpression;

#ifdef WIN32
#pragma pack()
#endif


/// Load MMSO file into the buffer (with buffer's memory allocation)
BYTE *mmso_load(const char *fname, int *buflen);

/// Save buffer into the MMSO file
BOOL mmso_save(const char *fname, BYTE *buf, int buflen);


#endif // of SP_MMSL_FILE_H
