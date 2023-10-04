//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - MMSL file loader/saver impl.
 *  \file       mmsl/mmsl-file.cpp
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

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_memory.h>

#include <mmsl/mmsl-file.h>

static DWORD crc_table[256];
static BOOL crc_was_init = 0;

static void mmso_init_crc()
{
	DWORD c, poly;
	unsigned n;
	static const BYTE p[] = { 0,1,2,4,5,7,8,10,11,12,16,22,23,26 };

	poly = 0L;
	for (n = 0; n < sizeof(p); n++)
		poly |= 1L << (31 - p[n]);

	for (n = 0; n < 256; n++)
	{
		c = (DWORD)n;
		for(int k = 0; k < 8; k++)
			c= (c & 1) ? poly ^ (c >> 1) : c >> 1;
		crc_table[n] = c;
	}
	crc_was_init = 1;
}

static DWORD mmso_crc(BYTE *str, int size)
{
	if (!crc_was_init)
		mmso_init_crc();

	DWORD crc32 = 0xffffffffL;
	for(int i = 0; i < size;i++)
	{
		BYTE c = str[i];
		crc32 = crc_table[(crc32 & 0xff) ^ c] ^ (crc32 >> 8);
	}
	return (crc32 ^ 0xffffffffL);
}


BYTE *mmso_load(const char *fname, int *buflen)
{
	BYTE *buf = NULL;
	FILE *fp = fopen(fname, "rb");
	if (fp != NULL)
	{
		fseek(fp, 0, SEEK_END);
		*buflen = ftell(fp);
		rewind(fp);

		MmslSerFileHeader hdr;
		if (*buflen <= (int)sizeof(hdr))
		{
			msg("Mmso: File %s is too small.\n", fname);
			fclose(fp);
			return NULL;
		}
		fread(&hdr, sizeof(hdr), 1, fp);
		*buflen -= sizeof(hdr);

		if (memcmp(hdr.magic, "MMSO", 4) != 0 || hdr.ver > MMSO_VERSION)
		{
			msg("Mmso: Wrong file header or version (%d).\n", hdr.ver);
			fclose(fp);
			return NULL;
		}

		buf = (BYTE *)SPmalloc(*buflen + 1);
		if (buf != NULL)
			fread(buf, *buflen, 1, fp);

		// now check crc
		DWORD real_crc = mmso_crc(buf, *buflen);
		if (real_crc != hdr.crc)
		{
			msg("Mmso: Wrong checksum!\n");
			fclose(fp);
			SPSafeFree(buf);
			return NULL;
		}

		fclose(fp);
	}

	return buf;
}

BOOL mmso_save(const char *fname, BYTE *buf, int buflen)
{
	int prev_buflen;
	BYTE *prev_buf = mmso_load(fname, &prev_buflen);
	if (prev_buf != NULL)
	{
		BOOL is_the_same = (buflen == prev_buflen && memcmp(buf, prev_buf, buflen) == 0);
		SPSafeFree(prev_buf);
		if (is_the_same)
			return TRUE;
	}

	FILE *fp = fopen(fname, "wb");
	if (fp != NULL)
	{
		MmslSerFileHeader hdr;
		memcpy(hdr.magic, "MMSO", 4);
		hdr.ver = MMSO_VERSION;
		hdr.crc = mmso_crc(buf, buflen);
		fwrite(&hdr, sizeof(hdr), 1, fp);
		
		fwrite(buf, buflen, 1, fp);
		fclose(fp);
	}

	return TRUE;
}
