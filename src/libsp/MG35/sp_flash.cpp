//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - FLASH-ROM interface functions source file.
 * 								For Technosonic-compatible players ('MP')
 *  \file       sp_flash.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       4.05.2004
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/mtd/mtd.h>

#include "sp_misc.h"
#include "sp_fip.h"
#include "sp_flash.h"


static int file_handle, mtd_handle;
static DWORD file_read_cnt, num_read, num_written;
static int flen;
static void *buf = NULL;

/*
void flash_output_progress(int cnt, int total)
{
	char buf[10];
	cnt = cnt * 100 / total;
	sprintf(buf, "%u", cnt);
	fip_write_string(buf);
}
*/

int flash_end()
{
	if (buf != NULL)
		SPSafeFree(buf);
	close(file_handle);
	close(mtd_handle);
	return 0;
}


int flash_file(char *fname, DWORD address)
{
	if (buf != NULL)
	{
		flash_end();
	}
	int addr = address;
	DWORD length = 0x400000 - addr;
	struct erase_info_user eiu;
	int mtdpos;

	file_handle = open(fname, O_RDONLY);
	if (file_handle == -1)
		return -1;
	flen = lseek(file_handle, 0, SEEK_END);
	if (flen < (int)length)
		length = flen;
	lseek(file_handle, 0, SEEK_SET);
	
	mtd_handle = open("/dev/mtd/0", O_RDWR);
	if (mtd_handle == -1)
	{
		mtd_handle = file_handle;
		goto err0;
	}
	mtdpos = lseek(mtd_handle, addr, SEEK_SET);
	if (mtdpos == -1)
		goto err;

	buf = SPmalloc(0x1000);
	if (buf == NULL)
		goto err;
	
	if (addr < 0x4000)
	{
		length += addr;
		addr = 0;
	} else if (addr < 0x8000)
	{
		length += (DWORD)addr % 0x2000;
		addr &= ~0x1fff;
	} else if (addr < 0x10000)
	{
		length += (DWORD)addr - 0x8000;
		addr = 0x8000;
	} else
	{
		length += (DWORD)addr & 0xffff;
		addr = (DWORD)addr / 0x10000 * 0x10000;
	}

	if (addr + (int)length <= 0x4000) 
	{
		length = 0x4000;
	} else if (addr + (int)length <= 0x8000) 
	{
		length = ((length - 1) / 0x2000) * 0x2000 + 0x1fff;
		length = length - 1;
	} else if (addr + (int)length <= 0x10000) 
	{
		length = 0x10000 - addr;
	} else
	{
		length = ((((DWORD)(addr+length)-1) / 0x10000+1	) * 0x10000) -1;
		length = length + 1 - (DWORD)addr;
	}

	// erase FLASH memory
	eiu.start = addr;
	eiu.length = length;
	if (ioctl(mtd_handle, MEMERASE, &eiu) != 0)
		goto err;

	file_read_cnt = 0;
	return 0;

err:
	if (buf != NULL)
		SPfree(buf);
	close(file_handle);
err0:
	close(mtd_handle);
	return -1;
}

int flash_cycle()
{
	if (buf == NULL)
	{
		return -1;
	}
	num_read = read(file_handle, buf, 0x1000);
	if (num_read <= 0)
	{
		flash_end();
		return 100;
	}
	num_written = write(mtd_handle, buf, num_read);
	if (num_read != num_written)
		goto err;
	file_read_cnt += num_read;
	
	if ((int)file_read_cnt != flen)
		goto err;
	
	return file_read_cnt * 100 / flen;
err:
	flash_end();
	return -1;
}

