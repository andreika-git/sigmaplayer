//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - I2C data transfer functions header file.
 *  \file       sp_i2c.h
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

#ifndef SP_I2C_H
#define SP_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

/// Reads data from the I2C bus
BOOL i2c_data_in(BYTE addr, BYTE idx, BYTE *data, int num);

/// Writes data to the I2C bus
BOOL i2c_data_out(BYTE addr, BYTE idx, BYTE *data, int num);

#ifdef __cplusplus
}
#endif

#endif // of SP_I2C_H
