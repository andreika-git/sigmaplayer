//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - EEPROM interface functions source file
 * 								For Technosonic-compatible players ('MP')
 *  \file       sp_eeprom.cpp
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

#include "sp_misc.h"
#include "sp_khwl.h"
#include "sp_eeprom.h"

BOOL eeprom_set_value(DWORD addr, DWORD val, int size)
{
	KHWL_ADDR_DATA data;
	/// \warning: not usual byte order used!
	for (int i = 0; i < size; i++)
	{
		data.Addr = addr + i;
		data.Data = (val >> (i * 8)) & 0xff;
		khwl_setproperty(KHWL_EEPROM_SET, eEepromAccess, sizeof(data), &data);
	}
	return TRUE;
}

DWORD eeprom_get_value(DWORD addr, int size)
{
	KHWL_ADDR_DATA data;
	DWORD val = 0;
	/// \warning: not usual byte order used!
	for (int i = 0; i < size; i++)
	{
		data.Addr = addr + i;
		data.Data = 0;
		khwl_getproperty(KHWL_EEPROM_SET, eEepromAccess, sizeof(data), &data);
		val |= (data.Data & 0xff) << (i * 8);
	}	
	return val;
}

