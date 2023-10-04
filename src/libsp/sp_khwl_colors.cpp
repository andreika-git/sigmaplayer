//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - color space support functions source file
 *  \file       sp_khwl_colors.c
 *  \author     bombur
 *  \version    0.1
 *  \date       4.07.2004
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
#include <fcntl.h>

#include "sp_misc.h"
#include "sp_khwl.h"



#define USE_NEW_YUV
//#define USE_GAMMA


#define START 0
#define END 65536
#define PRECISION 20

#define SCALEBITS_INT	8
#define FIX_INT(x)		((WORD) ((x) * (1L<<SCALEBITS_INT) + 0.5))

#define Y_R_INT			FIX_INT(0.257)
#define Y_G_INT			FIX_INT(0.504)
#define Y_B_INT			FIX_INT(0.098)
#define Y_ADD_INT		16

#define U_R_INT			FIX_INT(0.148)
#define U_G_INT			FIX_INT(0.291)
#define U_B_INT			FIX_INT(0.439)
#define U_ADD_INT		128

#define V_R_INT			FIX_INT(0.439)
#define V_G_INT			FIX_INT(0.368)
#define V_B_INT			FIX_INT(0.071)
#define V_ADD_INT		128

#define RGB2Y(r,g,b)	(((Y_R_INT * r + Y_G_INT * g + Y_B_INT * b) >> SCALEBITS_INT) + Y_ADD_INT)
#define RGB2U(r,g,b)    (((- U_R_INT * r - U_G_INT * g + U_B_INT * b) >> (SCALEBITS_INT + 2)) + U_ADD_INT)
#define RGB2V(r,g,b)	(((V_R_INT * r	- V_G_INT * g	- V_B_INT * b) >> (SCALEBITS_INT + 2)) + V_ADD_INT)


//	yuv -> rgb constants
#define RGB_Y_INT		1.164

#define B_U_INT			2.018
#define G_U_INT			0.391

#define G_V_INT			0.813
#define R_V_INT			1.596

#define RGB555(R,G,B)	((clip[R] << 7) & 0x7c00) | ((clip[G] << 2) & 0x03e0) | ((clip[B] >> 3) & 0x001f)
#define RGB565(R,G,B)	((clip[R] << 8) & 0xf800) | ((clip[G] << 3) & 0x07e0) | ((clip[B] >> 3) & 0x001f)

// yuv -> rgb lookup tables
static DWORD RGB_Y_tab[256];
static DWORD B_U_tab[256];
static DWORD G_U_tab[256];
static DWORD G_V_tab[256];
static DWORD R_V_tab[256];

//smart clipping table
static BYTE clip_tab[2048];
static BYTE *clip = NULL;

void khwl_inityuv() 
{
	DWORD i;

	for (i = 0; i < 512; i++)
		clip_tab[i] = 0;
	for (i = 512; i < 512+256; i++)
		clip_tab[i] = (BYTE)(i - 512);
	for (i = 512+256; i < 2048; i++)
		clip_tab[i] = 255;

	clip = clip_tab + 512;

    for (i = 0; i < 256; i++) 
	{
		RGB_Y_tab[i] = FIX_INT(RGB_Y_INT) * (i - Y_ADD_INT);
		B_U_tab[i] = FIX_INT(B_U_INT) * (i - U_ADD_INT);
		G_U_tab[i] = FIX_INT(G_U_INT) * (i - U_ADD_INT);
		G_V_tab[i] = FIX_INT(G_V_INT) * (i - V_ADD_INT);
		R_V_tab[i] = FIX_INT(R_V_INT) * (i - V_ADD_INT);
	}
}

void new_rgbtoyuv(BYTE R, BYTE G, BYTE B, BYTE *y, BYTE *u, BYTE *v)
{
	*y = (BYTE)RGB2Y(R,G,B); 
	*u = (BYTE)RGB2U(R,G,B);
	*v = (BYTE)RGB2V(R,G,B);
}

void new_yuvtorgb(BYTE y, BYTE u, BYTE v, BYTE *R, BYTE *G, BYTE *B)
{
	int rgb_y = RGB_Y_tab[y];
	int b_u = B_U_tab[u];
	int g_uv = G_U_tab[u] + G_V_tab[v];
	int r_v = R_V_tab[v];

	register int r = (rgb_y + r_v) >> SCALEBITS_INT;
	register int g = (rgb_y - g_uv) >> SCALEBITS_INT;
	register int b = (rgb_y + b_u) >> SCALEBITS_INT;
	*R = clip[r];
	*G = clip[g];
	*B = clip[b];
}

#ifdef USE_GAMMA

static DWORD powertwodottwotable[PRECISION + 1] =
{
	0, 89, 413, 1008, 1899, 3104, 4636, 6507, 8729, 11312, 14263, 17590, 
	21301, 25403, 29901, 34802, 40112, 45835, 51977, 58542, 65536
};

static DWORD invertofpowertwodottwotable[PRECISION + 1]=
{
	0, 16792, 23010, 27667, 31533, 34899, 37914, 40666, 43211, 45587, 47824, 
	49941, 51956, 53881, 55727, 57502, 59214, 60869, 62471, 64025, 65536
};

// computes y/65536=f(x/65536) with 0<=x,y<65536 with f=^2.2 or ^(1/2.2) given as a DWORD table (have to code the 65536 case)
static WORD compute_f(DWORD beg, DWORD end, DWORD prec, DWORD *table, WORD x)
{
	long a,b,fa,fb,entry;
	
	entry = (x - beg) * prec / (end - beg);

	if ((entry < 0) || (entry >= 20)) 
	{
		printf("memory corrupted x=%hd beg=%ld end=%ld prec=%ld\n", x, beg, end, prec);
		return 0;
	}

	a = entry * (end - beg) / prec + beg;
	b = (entry + 1) * (end - beg) / prec + beg;
	fa = table[entry];
	fb = table[entry + 1];
	return (WORD)(fa + (fb - fa) * (x - a) / (b - a));
}
#endif


// see video demystified page 43

static void internal_rgbtoyuv(WORD R, WORD G, WORD B, WORD *y, WORD *u,WORD *v)
{
	long yraw, uraw, vraw;
	
	yraw = ( 257*R  +504*G + 98*B)/1000 + RANGE8TO16(16);
	uraw = (-148*R  -291*G +439*B)/1000 + RANGE8TO16(128);
	vraw = ( 439*R  -368*G - 71*B)/1000 + RANGE8TO16(128);
 
	*y = (WORD)MAX(MIN(yraw, RANGE8TO16(235)), RANGE8TO16(16)); 
	*u = (WORD)MAX(MIN(uraw, RANGE8TO16(240)), RANGE8TO16(16));
	*v = (WORD)MAX(MIN(vraw, RANGE8TO16(240)), RANGE8TO16(16));
}

/*
static void internal_yuvtorgb(WORD y, WORD u, WORD v, WORD *R, WORD *G, WORD *B)
{
	long Rraw, Graw, Braw;
	long ym = y - RANGE8TO16(16), um = u - RANGE8TO16(128), vm = v - RANGE8TO16(128);

	Rraw = (1164 * ym             + 1596 * vm)/1000;
	Graw = (1164 * ym + 392  * um - 813  * vm)/1000;
	Braw = (1164 * ym + 2017 * um            )/1000;

	*R = (WORD)MAX(0, MIN(Rraw, RANGE8TO16(255)));
	*G = (WORD)MAX(0, MIN(Graw, RANGE8TO16(255)));
	*B = (WORD)MAX(0, MIN(Braw, RANGE8TO16(255)));
}
*/

void khwl_vgargbtotvyuv(BYTE R, BYTE G, BYTE B, BYTE *y, BYTE *u, BYTE *v)
{
//#ifdef USE_NEW_YUV
//	new_rgbtoyuv(R, G, B, y, u, v);
//#else
#ifdef USE_GAMMA
	WORD Rc = compute_f(START, END, PRECISION, powertwodottwotable, (WORD)RANGE8TO16(R));
	WORD Gc = compute_f(START, END, PRECISION, powertwodottwotable, (WORD)RANGE8TO16(G));
	WORD Bc = compute_f(START, END, PRECISION, powertwodottwotable, (WORD)RANGE8TO16(B));
#else
	WORD Rc = (WORD)RANGE8TO16(R);
	WORD Gc = (WORD)RANGE8TO16(G);
	WORD Bc = (WORD)RANGE8TO16(B);
#endif
	WORD wy, wu, wv;
	internal_rgbtoyuv(Rc, Gc, Bc, &wy, &wu, &wv);
	*y = (BYTE)(wy >> 8);
	*u = (BYTE)(wu >> 8);
	*v = (BYTE)(wv >> 8);
//#endif
}

void khwl_tvyuvtovgargb(BYTE y, BYTE u, BYTE v, BYTE *R, BYTE *G, BYTE *B)
{
#ifdef USE_NEW_YUV
	new_yuvtorgb(y, u, v, R, G, B);
#else
	WORD Ruc, Guc, Buc;
	internal_yuvtorgb((WORD)RANGE8TO16(y), (WORD)RANGE8TO16(u), (WORD)RANGE8TO16(v), &Ruc, &Guc, &Buc);
#ifdef USE_GAMMA
	*R = (BYTE)(compute_f(START, END, PRECISION, invertofpowertwodottwotable, Ruc) >> 8);
	*G = (BYTE)(compute_f(START, END, PRECISION, invertofpowertwodottwotable, Guc) >> 8);
	*B = (BYTE)(compute_f(START, END, PRECISION, invertofpowertwodottwotable, Buc) >> 8);
#else
	*R = (BYTE)(Ruc >> 8);
	*G = (BYTE)(Guc >> 8);
	*B = (BYTE)(Buc >> 8);
#endif
#endif
}


void khwl_jpegyuvtorgb(BYTE y, BYTE u, BYTE v, BYTE *R, BYTE *G, BYTE *B)
{
	long Rraw, Graw, Braw;
	long ym = RANGE8TO16(y), um = RANGE8TO16(u) - RANGE8TO16(128);
	long vm = RANGE8TO16(v) - RANGE8TO16(128);

	Rraw = (1000 * ym             + 1000*vm)/1000;
	Graw = (1000 * ym - 509 * um - 194 * vm)/1000;
	Braw = (1000 * ym + 1000*um            )/1000;

	Rraw = (WORD)MAX(0, MIN(Rraw, RANGE8TO16(255)));
	Graw = (WORD)MAX(0, MIN(Graw, RANGE8TO16(255)));
	Braw = (WORD)MAX(0, MIN(Braw, RANGE8TO16(255)));
	*R = (BYTE)(Rraw >> 8);
	*G = (BYTE)(Graw >> 8);
	*B = (BYTE)(Braw >> 8);
}


/*		
		float y = (float)ybuf[i] / 255.0f;
		float u = (float)uvbuf[ui*2+1] / 255.0f -0.5f;
		float v = (float)uvbuf[ui*2] / 255.0f -0.5f;
		
		b = (y + u) * 255.0f;
		if (b < 0) b = 0;
		if (b > 255)b = 255;
		
		g = (y - 0.509 * u - 0.194*v) * 255.0f;
		if (g < 0)g = 0;
		if (g > 255)g = 255;
		
		r = (y + v) * 255.0f;
		if (r < 0) r = 0;
		if (r > 255)r = 255;
		
		backbuf[vi] = (b  << 16) | (g << 8) | r;
*/		
