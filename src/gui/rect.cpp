//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - rectangle (incl. rounded) class impl.
 *  \file       rect.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       4.10.2006
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

#include <string.h>
#include <libsp/sp_misc.h>
#include <libsp/sp_memory.h>

#include <gui/rect.h>


////////////////////////////////////////////////////////

// some inline helper functions for painting the 'context':

static inline void Scanline(Context *context, int x1, int x2, int y, int bkcolor)
{
	if (x2 >= x1)
		memset(context->data + y * context->pitch + x1, bkcolor, x2 - x1 + 1);
}

static inline void SmallPoint1(Context *context, int x, int y, int color, int)
{
	*(context->data + y * context->pitch + x) = (char)color;
}

// right-bottom
static inline void BigPoint1(Context *context, int x, int y, int color, int width)
{
	for (int i = 0; i <= width; i++)
		Scanline(context, x, x + width, y++, color);
}

// right-top
static inline void BigPoint2(Context *context, int x, int y, int color, int width)
{
	for (int i = 0; i <= width; i++)
		Scanline(context, x, x + width, y--, color);
}

// left-bottom
static inline void BigPoint3(Context *context, int x, int y, int color, int width)
{
	for (int i = 0; i <= width; i++)
		Scanline(context, x - width, x, y++, color);
}

// left-top
static inline void BigPoint4(Context *context, int x, int y, int color, int width)
{
	for (int i = 0; i <= width; i++)
		Scanline(context, x - width, x, y--, color);
}

static inline void Rect(Context *context, int x1, int y1, int x2, int y2, int bkcolor)
{
	if (x2 >= x1)
	{
		for (int i = y1; i <= y2; i++)
			Scanline(context, x1, x2, i, bkcolor);
	}
}

/////////////////////////////////////////////////////////////////////////

// modified painting function for clipped case:

static inline void ClippedScanline(Context *context, int x1, int x2, int y, int bkcolor,
							int cx1, int cy1, int cx2, int cy2)
{
	if (y >= cy1 && y <= cy2)
	{
		x1 = MAX(x1, cx1);
		x2 = MIN(x2, cx2);
		if (x2 >= x1)
			memset(context->data + y * context->pitch + x1, bkcolor, x2 - x1 + 1);
	}
}

static inline void ClippedSmallPoint1(Context *context, int x, int y, int color, int,
							   int cx1, int cy1, int cx2, int cy2)
{
	if (x >= cx1 && y >= cy1 && x <= cx2 && y <= cy2)
		*(context->data + y * context->pitch + x) = (char)color;
}

// right-bottom
static inline void ClippedBigPoint1(Context *context, int x, int y, int color, int width,
							 int cx1, int cy1, int cx2, int cy2)
{
	for (int i = 0; i <= width; i++)
		ClippedScanline(context, x, x + width, y++, color, cx1, cy1, cx2, cy2);
}

// right-top
static inline void ClippedBigPoint2(Context *context, int x, int y, int color, int width,
					  int cx1, int cy1, int cx2, int cy2)
{
	for (int i = 0; i <= width; i++)
		ClippedScanline(context, x, x + width, y--, color, cx1, cy1, cx2, cy2);
}

// left-bottom
static inline void ClippedBigPoint3(Context *context, int x, int y, int color, int width,
							 int cx1, int cy1, int cx2, int cy2)
{
	for (int i = 0; i <= width; i++)
		ClippedScanline(context, x - width, x, y++, color, cx1, cy1, cx2, cy2);
}

// left-top
static inline void ClippedBigPoint4(Context *context, int x, int y, int color, int width,
							 int cx1, int cy1, int cx2, int cy2)
{
	for (int i = 0; i <= width; i++)
		ClippedScanline(context, x - width, x, y--, color, cx1, cy1, cx2, cy2);
}

//////////////////////////////////////////////////////////////

/// Paints filled circle slices for rounded rect.
static inline void DrawRounds(Context *context, int center_x1, int center_y1, int center_x2, int center_y2, 
						int radius, int linewidth, int color, int bkcolor)
{ 
    int x; 
    int y = radius;
	// circle decision variable 
    int d = 3 - (radius << 1);  
	// treat the line width as the additive to the scan coordinates
	linewidth--;

	// draw filled part (generate scan sections of the circle)
	if (bkcolor >= 0)
	{
		for (x = 0; x <= y; x++)
		{ 
			// draw the y-dependent scans
			if (bkcolor >= 0)
			{
     			Scanline(context, center_x1 - y, center_x1, center_y2 + x, bkcolor); 
				Scanline(context, center_x2, center_x2 + y, center_y2 + x, bkcolor); 
     			if (x >= 0) 
				{
     				Scanline(context, center_x1 - y, center_x1, center_y1 - x, bkcolor); 
					Scanline(context, center_x2, center_x2 + y, center_y1 - x, bkcolor); 
				}
			}

     		// iterate the differential circle drawing code 
     		if (d < 0) 
			{
				d += (x << 2) + 6; 
			}
     		else 
     		{ 
      			d += ((x - y) << 2) + 10; 

      			// draw the x-dependent scans - done not so frequently 
      			if (x != y && bkcolor >= 0) 
      			{ 
       				Scanline(context, center_x1 - x, center_x1, center_y1 - y, bkcolor); 
					Scanline(context, center_x2, center_x2 + x, center_y1 - y, bkcolor); 

       				Scanline(context, center_x1 - x, center_x1, center_y2 + y, bkcolor); 
					Scanline(context, center_x2, center_x2 + x, center_y2 + y, bkcolor); 
      			} 
				
      			y--; 
     		} 
		}
		// restore values
		y = radius;
		d = 3 - (radius << 1);
	}
	
	// draw border frame
	if (color >= 0)
	{
		for (x = 0; x <= y; x++)  // Generate scan sections of the circle 
		{

#define CIRCLE_POINTS(size, dir1, dir2, dir3, dir4) \
			size##Point##dir1(context, center_x1 - x, center_y1 - y, color, linewidth); \
			size##Point##dir1(context, center_x1 - y, center_y1 - x, color, linewidth); \
			size##Point##dir2(context, center_x1 - x, center_y2 + y, color, linewidth); \
			size##Point##dir2(context, center_x1 - y, center_y2 + x, color, linewidth); \
			size##Point##dir3(context, center_x2 + x, center_y1 - y, color, linewidth); \
			size##Point##dir3(context, center_x2 + y, center_y1 - x, color, linewidth); \
			size##Point##dir4(context, center_x2 + x, center_y2 + y, color, linewidth); \
			size##Point##dir4(context, center_x2 + y, center_y2 + x, color, linewidth);

			if (linewidth < 1)
			{
				CIRCLE_POINTS(Small,1,1,1,1);
			} else
			{
				CIRCLE_POINTS(Big,1,2,3,4);
			}

     		// Iterate the differential circle drawing code 
     		if (d < 0) 
				d += (x << 2) + 6; 
     		else 
     		{ 
      			d += ((x - y) << 2) + 10; 
      			y--; 
     		} 
		}
	}
} 

/// Paints clipped filled circle slices for rounded rect.
static inline void DrawClippedRounds(Context *context, int center_x1, int center_y1, int center_x2, int center_y2, 
						int radius, int linewidth, int color, int bkcolor,
						int x1, int y1, int x2, int y2)
{ 
    int x; 
    int y = radius;

	// circle decision variable 
    int d = 3 - (radius << 1);  
	// treat the line width as the additive to the scan coordinates
	linewidth--;

    // draw filled part (generate scan sections of the circle)
	if (bkcolor >= 0)
	{
		for (x = 0; x <= y; x++)
		{ 
			// draw the y-dependent scans
			if (bkcolor >= 0)
			{
     			ClippedScanline(context, center_x1 - y, center_x1, center_y2 + x, bkcolor, x1, y1, x2, y2); 
				ClippedScanline(context, center_x2, center_x2 + y, center_y2 + x, bkcolor, x1, y1, x2, y2); 
     			if (x >= 0) 
				{
     				ClippedScanline(context, center_x1 - y, center_x1, center_y1 - x, bkcolor, x1, y1, x2, y2); 
					ClippedScanline(context, center_x2, center_x2 + y, center_y1 - x, bkcolor, x1, y1, x2, y2); 
				}
			}

     		// iterate the differential circle drawing code 
     		if (d < 0) 
			{
				d += (x << 2) + 6; 
			}
     		else 
     		{ 
      			d += ((x - y) << 2) + 10; 

      			// draw the x-dependent scans - done not so frequently 
      			if (x != y && bkcolor >= 0) 
      			{ 
       				ClippedScanline(context, center_x1 - x, center_x1, center_y1 - y, bkcolor, x1, y1, x2, y2); 
					ClippedScanline(context, center_x2, center_x2 + x, center_y1 - y, bkcolor, x1, y1, x2, y2); 

       				ClippedScanline(context, center_x1 - x, center_x1, center_y2 + y, bkcolor, x1, y1, x2, y2); 
					ClippedScanline(context, center_x2, center_x2 + x, center_y2 + y, bkcolor, x1, y1, x2, y2); 
      			} 
				
      			y--; 
     		} 
		}
		// restore values
		y = radius;
		d = 3 - (radius << 1);
	}
	
	// draw border frame
	if (color >= 0)
	{
		for (x = 0; x <= y; x++)  // Generate scan sections of the circle 
		{

#define CLIPPED_CIRCLE_POINTS(size, dir1, dir2, dir3, dir4) \
			Clipped##size##Point##dir1(context, center_x1 - x, center_y1 - y, color, linewidth, x1, y1, x2, y2); \
			Clipped##size##Point##dir1(context, center_x1 - y, center_y1 - x, color, linewidth, x1, y1, x2, y2); \
			Clipped##size##Point##dir2(context, center_x1 - x, center_y2 + y, color, linewidth, x1, y1, x2, y2); \
			Clipped##size##Point##dir2(context, center_x1 - y, center_y2 + x, color, linewidth, x1, y1, x2, y2); \
			Clipped##size##Point##dir3(context, center_x2 + x, center_y1 - y, color, linewidth, x1, y1, x2, y2); \
			Clipped##size##Point##dir3(context, center_x2 + y, center_y1 - x, color, linewidth, x1, y1, x2, y2); \
			Clipped##size##Point##dir4(context, center_x2 + x, center_y2 + y, color, linewidth, x1, y1, x2, y2); \
			Clipped##size##Point##dir4(context, center_x2 + y, center_y2 + x, color, linewidth, x1, y1, x2, y2);

			if (linewidth < 1)
			{
				CLIPPED_CIRCLE_POINTS(Small,1,1,1,1);
			} else
			{
				CLIPPED_CIRCLE_POINTS(Big,1,2,3,4);
			}

     		// Iterate the differential circle drawing code 
     		if (d < 0) 
				d += (x << 2) + 6; 
     		else 
     		{ 
      			d += ((x - y) << 2) + 10; 
      			y--; 
     		} 
		}
	}
} 

/////////////////////////////////////////////////////////////////////////

Rectangle::Rectangle()
{
	round = 0;
	linewidth = 1;

	color = gui.GetColor();
	bkcolor = gui.GetBkColor();
}

void Rectangle::SetLineWidth(int lw)
{
	linewidth = lw;
	dirty = true;
}

void Rectangle::SetRound(int r)
{
	round = r;
	dirty = true;
}

void Rectangle::SetColor(int c)
{
	if (c > 255)
		c = -1;
	color = c;
	dirty = true;
}
void Rectangle::SetBkColor(int bc)
{
	if (bc > 255)
		bc = -1;
	bkcolor = bc;
	dirty = true;
}

bool Rectangle::Update(Context *context, int x1, int y1, int x2, int y2)
{
	if (context == NULL)
		return false;
	// fix some values for invalid rects
	int lw = color >= 0 ? MIN(linewidth, MIN(width, height) / 2) : 0;
	int clr = lw > 0 ? color : -1;
	int rnd = MIN(round, lw <= 4 ? MIN(width, height) / 2 : MIN(width, height) / 2 - lw);
	
	// draw rounded parts
	if (rnd > 1)
	{
		if (x1 == 0 && y1 == 0 && x2 == width - 1 && y2 == height - 1)
			DrawRounds(context, rnd, rnd, width - 1 - rnd, 
						height - 1 - rnd, rnd, lw, clr, bkcolor);
		else
			DrawClippedRounds(context, rnd, rnd, width - 1 - rnd, 
								height - 1 - rnd, rnd, lw, clr, bkcolor, 
								x1, y1, x2, y2);
		
		// draw upper and lower filled rect. parts
		if (bkcolor >= 0)
		{
			Rect(context, MAX(rnd, x1), MAX(lw, y1), 
					MIN(width - 1 - rnd, x2), MIN(rnd - 1, y2), bkcolor);
			Rect(context, MAX(rnd, x1), MAX(height - rnd, y1), 
					MIN(width - 1 - rnd, x2), MIN(height - 1 - lw, y2), bkcolor);
		}
	}
	
	// draw top-left borders
	if (clr >= 0)
	{
		Rect(context, MAX(rnd, x1), MAX(0, y1), 
				MIN(width - 1 - rnd, x2), MIN(lw - 1, y2), clr);
		Rect(context, MAX(0, x1), MAX(MAX(lw, rnd), y1), 
				MIN(lw - 1, x2), MIN(height - 1 - MAX(lw, rnd), y2), clr);
	}

	// draw main filled rect
	if (bkcolor >= 0)
		Rect(context, MAX(lw, x1), MAX(MAX(lw, rnd), y1), 
			MIN(width - 1 - lw, x2), MIN(height - 1 - MAX(lw, rnd), y2), bkcolor);
	
	// draw bottom-right borders
	if (clr >= 0)
	{
		Rect(context, MAX(width - lw, x1), MAX(MAX(lw, rnd), y1), 
				MIN(width - 1, x2), MIN(height - 1 - MAX(lw, rnd), y2), clr);
		Rect(context, MAX(rnd, x1), MAX(height - lw, y1), 
				MIN(width - 1 - rnd, x2), MIN(height - 1, y2), clr);
	}

	return true;
}
