//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - Port IO header. Stripped from <asm/io.h>
 *  \file       sp_io.h
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

#ifndef SP_IO_H
#define SP_IO_H
#ifndef __ASM_ARM_IO_H

#define __arch_getb(a)			(*(volatile unsigned char *)(a))
#define __arch_getw(a) 			(*(volatile unsigned short *)(a))
#define __arch_getl(a)			(*(volatile unsigned int  *)(a))


#define __arch_putb(v,a)		(*(volatile unsigned char *)(a) = (v))
#define __arch_putw(v,a) 		(*(volatile unsigned short *)(a) = (v))
#define __arch_putl(v,a)		(*(volatile unsigned int  *)(a) = (v))

#define __raw_writeb(v,a)		__arch_putb(v,a)
#define __raw_writew(v,a)		__arch_putw(v,a)
#define __raw_writel(v,a)		__arch_putl(v,a)

#define __raw_readb(a)			__arch_getb(a)
#define __raw_readw(a)			__arch_getw(a)
#define __raw_readl(a)			__arch_getl(a)

#define outb(v,p)			__raw_writeb(v,p)
#define outw(v,p)			__raw_writew(v,p)
#define outl(v,p)			__raw_writel(v,p)

#define inb(p)	__raw_readb(p)
#define inw(p)	__raw_readw(p)
#define inl(p)	__raw_readl(p)

#endif
#endif // of SP_IO_H
