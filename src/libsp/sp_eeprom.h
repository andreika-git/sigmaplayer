//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - EEPROM interface header.
 *  \file       sp_eeprom.h
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

#ifndef SP_EEPROM_H
#define SP_EEPROM_H


#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////
/// Functions:

/// Sets EEPROM value at given address.
BOOL eeprom_set_value(DWORD addr, DWORD val, int size = 4);

/// Gets EEPROM value from given address.
DWORD eeprom_get_value(DWORD addr, int size = 4);


#ifdef __cplusplus
}
#endif


#endif // of SP_EEPROM_H
