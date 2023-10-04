//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - I2C data transfer functions source file.
 * 								For Technosonic-compatible players ('MP')
 *  \file       sp_i2c.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       1.02.2005
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
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sp_misc.h"
#include "sp_io.h"
#include "sp_i2c.h"

#include "arch-jasper/hardware.h"


BOOL i2c_data_in(BYTE addr, BYTE idx, BYTE *data, int num)
{
	outl(375, JASPER_I2C_MASTER_BASE+I2C_MASTER_CLK_DIV);
	while ((inl(JASPER_I2C_MASTER_BASE+I2C_MASTER_STATUS) & 1) == 0)
		;
	
	for (int i = 0; i < num; i++)
	{
		outl(250, JASPER_I2C_MASTER_BASE+I2C_MASTER_CONFIG);
		outl(0, JASPER_I2C_MASTER_BASE+I2C_MASTER_BYTE_COUNT);
		outl(addr/2, JASPER_I2C_MASTER_BASE+I2C_MASTER_DEV_ADDR);
		outl(idx++, JASPER_I2C_MASTER_BASE+I2C_MASTER_DATAOUT);
		outl(4, JASPER_I2C_MASTER_BASE+I2C_MASTER_STARTXFER);
		
		while ((inl(JASPER_I2C_MASTER_BASE+I2C_MASTER_STATUS) & 2) == 0)
			;
		while ((inl(JASPER_I2C_MASTER_BASE+I2C_MASTER_STATUS) & 1) == 0)
			;
		
		outl(250, JASPER_I2C_MASTER_BASE+I2C_MASTER_CONFIG);
		outl(0, JASPER_I2C_MASTER_BASE+I2C_MASTER_BYTE_COUNT);
		outl(addr/2, JASPER_I2C_MASTER_BASE+I2C_MASTER_DEV_ADDR);
		outl(1, JASPER_I2C_MASTER_BASE+I2C_MASTER_STARTXFER);

		while ((inl(JASPER_I2C_MASTER_BASE+I2C_MASTER_STATUS) & 4) == 0)
			;
		while ((inl(JASPER_I2C_MASTER_BASE+I2C_MASTER_STATUS) & 1) == 0)
			;
		
		*data++ = inl(JASPER_I2C_MASTER_BASE+I2C_MASTER_DATAIN);
	}
	return TRUE;
}

BOOL i2c_data_out(BYTE addr, BYTE idx, BYTE *data, int num)
{
	outl(248, JASPER_I2C_MASTER_BASE+I2C_MASTER_CONFIG);
	outl(375, JASPER_I2C_MASTER_BASE+I2C_MASTER_CLK_DIV);
	outl(addr/2, JASPER_I2C_MASTER_BASE+I2C_MASTER_DEV_ADDR);

	while ((inl(JASPER_I2C_MASTER_BASE+I2C_MASTER_STATUS) & 1) == 0)
			;

	for (int i = 0; i < num; i++)
	{
		outl(idx++, JASPER_I2C_MASTER_BASE+I2C_MASTER_ADR);
		outl(0, JASPER_I2C_MASTER_BASE+I2C_MASTER_BYTE_COUNT);
		outl(*data++, JASPER_I2C_MASTER_BASE+I2C_MASTER_DATAOUT);
		outl(0, JASPER_I2C_MASTER_BASE+I2C_MASTER_STARTXFER);

		while ((inl(JASPER_I2C_MASTER_BASE+I2C_MASTER_STATUS) & 2) == 0)
			;
	}
	return TRUE;
}

