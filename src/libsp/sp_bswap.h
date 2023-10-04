//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - optimized byte-swap header file
 *  \file       sp_bswap.h
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

#ifndef SP_BSWAP_H
#define SP_BSWAP_H


#ifdef SP_ARM

#define BSWAP(d) \
	asm volatile ("eor r3, %0, %0, ROR #16\n" \
                  "bic r3, r3, #0xff0000\n" \
                  "mov %0, %0, ROR #8\n" \
                  "eor %0, %0, r3, LSR #8\n" \
                  : "=r" (d) : "0" (d) : "r3");

#else

#define BSWAP(d) \
	((d) = (((d) & 0xff) << 24)  | (((d) & 0xff00) << 8) | \
		(((d) >> 8) & 0xff00) | (((d) >> 24) & 0xff))

#endif

#define BWSWAP(w) (((w) >> 8) | (((w) & 0xff) << 8))

#endif // of SP_BSWAP_H
