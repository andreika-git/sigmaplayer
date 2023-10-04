//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - FLASH-ROM interface functions source file.
 *  \file       sp_flash.cpp
 *  \author     bombur
 *  \version    0.2
 *  \date       30.03.2010 4.05.2004
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
#include <stdlib.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/mtd/mtd.h>

#include "sp_misc.h"
#include "sp_fip.h"
#include "sp_cdrom.h"
#include "sp_flash.h"
#include "sp_msg.h"

static int file_handle = -1, mtd_handle = -1;
static DWORD file_read_cnt, num_read = 0, num_written;
static DWORD flen;
static void *buf = NULL;
static BYTE *curbuf = NULL;
static BYTE *cur_addr, *cur_a;

static struct region_info_user riu[16];
static int reg_cnt = 0;

static char fwver[16];

int flash_end()
{
	if (buf != NULL)
		SPSafeFree(buf);
	if (file_handle >= 0)
	{
		close(file_handle);
		file_handle = -1;
	}
	if (mtd_handle >= 0)
	{
		close(mtd_handle);
		mtd_handle = -1;
	}
	return 0;
}


int flash_file(char *fname, DWORD address)
{
	flash_end();

	int mtdpos;
	int err = 0;

	file_handle = cdrom_open(fname, O_RDONLY | O_BINARY);
	if (file_handle == -1)
		return -1;
	flen = lseek(file_handle, 0, SEEK_END);
	lseek(file_handle, 0, SEEK_SET);

#ifdef WIN32
	char *mtdpath = "mtd0";
	mtd_handle = open(mtdpath, O_RDWR | O_BINARY | O_CREAT | O_TRUNC);
	chmod(mtdpath, 0777);
	static BYTE mtdbuf[2*1024*1024];
	cur_addr = mtdbuf;
#else
	mtd_handle = open("/dev/mtd/0", O_RDWR);
	cur_addr = (BYTE *)address;
#endif

	if (mtd_handle == -1)
	{
		err = -2;
		goto err;
	}

	mtdpos = lseek(mtd_handle, address, SEEK_SET);
	if (mtdpos == -1)
	{
		err = -3;
		goto err;
	}

	buf = SPmalloc(0x10000);
	if (buf == NULL)
	{
		err = -4;
		goto err;
	}

	file_read_cnt = 0;
	num_read = 0;

	// gather info
	struct mtd_info_user miu;
	if (ioctl(mtd_handle, MEMGETINFO, &miu) == 0)
	{
		msg("Flash: MTD size=%d erase_size=%d\n", miu.size, miu.erasesize);
	}
	if (ioctl(mtd_handle, MEMGETREGIONCOUNT, &reg_cnt) == 0)
	{
		msg("Flash: %d erase regions:\n", reg_cnt);
		if (reg_cnt > 16)
			reg_cnt = 16;
		for (int i = 0; i < reg_cnt; i++)
		{
			riu[i].regionindex = i;
			if (ioctl(mtd_handle, MEMGETREGIONINFO, &riu[i]) == 0)
			{
				msg("  [%d] %08x %08x num=%d\n", i, riu[i].offset, riu[i].erasesize, riu[i].numblocks);
			}
		}
	}

	num_read = read(file_handle, buf, 32);
	if (num_read < 32 || memcmp(buf, "-rom1fs-", 8) != 0)
	{
		msg("Flash: Wrong firmware file type or file error.\n");
		err = -5;
		goto err;
	}
	
	lseek(file_handle, 0, SEEK_SET);
	num_read = 0;
	strncpy(fwver, (char *)buf + 16, 16);

	// test size
	if (flen > address + miu.size)
	{
		msg("Flash: Firmware size (%d) exceeds flash size (%d) for given offset (%d)\n", flen, miu.size, address);
		err = -6;
		goto err;
	}

	msg("Flash: Found '%s'. Starting flash...\n", fwver);

	return 0;

err:
	flash_end();

	return err;
}

int flash_cycle()
{
	if (buf == NULL || file_handle < 0 || mtd_handle < 0)
		return -1;

	if (num_read == 0)
	{
		// find corresponding erase zone
		int len = 0;
#ifdef WIN32
		len = 0x1000;
#else
		for (int i = 0; i < reg_cnt; i++)
		{
			if ((DWORD)cur_addr >= riu[i].offset && (DWORD)cur_addr < riu[i].offset + riu[i].erasesize * riu[i].numblocks)
			{
				len = riu[i].erasesize;
				break;
			}
		}
#endif
		if (len == 0)
		{
			msg("Flash: Cannot find erase region for address %08x", (DWORD)cur_addr);
			return -1;
		}
		num_read = read(file_handle, buf, len);
		curbuf = (BYTE *)buf;
		if (num_read <= 0)
		{
			flash_end();
			return 100;
		}

		// test
		if (memcmp(cur_addr, buf, num_read) != 0)
		{
			// erase FLASH memory
			struct erase_info_user eiu;
			eiu.start = (DWORD)cur_addr;
			eiu.length = len;
			ioctl(mtd_handle, MEMUNLOCK, &eiu);

			if (ioctl(mtd_handle, MEMERASE, &eiu) != 0)
			{
				msg("Flash: MEMERASE ERR(%x, %x)\n", eiu.start, eiu.length);
				goto err;
			}

			lseek(mtd_handle, (DWORD)cur_addr, SEEK_SET);
			
			cur_a = cur_addr;
			cur_addr += len;
		}
		else
		{
			cur_addr += len;
			file_read_cnt += num_read;

			num_read = 0;
		}
	}

	if (num_read > 0)
	{
		int len = Min(0x1000, (int)num_read);
		num_written = write(mtd_handle, curbuf, len);
		if (num_written != (DWORD)len)
		{
			msg("Flash: write ERR(%x, %x)\n", file_read_cnt, num_written);
			goto err;
		}
#ifndef WIN32
		if (memcmp(cur_a, curbuf, len) != 0)
		{
			msg("Flash: Verify error @ %08x...\n", cur_a);
			return -2;
		}
#endif
		cur_a += len;
		curbuf += len;
		num_read -= len;
		file_read_cnt += len;
	}

	if (file_read_cnt > flen)
		file_read_cnt = flen;

	return file_read_cnt * 100 / flen;
err:
	flash_end();
	return -1;
}

DWORD flash_get_memory_size()
{
#ifdef WIN32
	return 1*1024*1024;
#else
	int mtd_handle = open("/dev/mtd/0", O_RDWR);
	if (mtd_handle == -1)
		return 0;
	struct mtd_info_user miu;
	int size = 0;
	if (ioctl(mtd_handle, MEMGETINFO, &miu) == 0)
		size = miu.size;
	close(mtd_handle);
	return size;
#endif
}
