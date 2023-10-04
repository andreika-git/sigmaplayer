//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - abstract window class impl.
 *  \file       window.cpp
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
#include <string.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_memory.h>
#include <libsp/sp_khwl.h>

#include <gui/window.h>
#include <gui/image.h>
#include <gui/font.h>
#include <gui/jpeg.h>

#include <gui/console.h>

#include <settings.h>

WindowManager gui;


////////////////////////////////////////////////////////////

Window::Window()
{
	x = 0;
	y = 0;
	width = 0;
	height = 0;
	auto_width = auto_height = 0;

	old_x = old_y = old_width = old_height = 0;

	dirty = false;
	visible = true;
	wasvisible = false;
	transparent = false;
	removing = false;

	group = NULL;

	halign = gui.halign;
	valign = gui.valign;

	timer = 0;
	timerobj = NULL;

	prev = next = NULL;
}

bool Window::Update(int)
{
	return false;
}

void Window::SetX(int newx)
{
	if (halign == WINDOW_ALIGN_RIGHT)
	{
		x = newx - width;
	}
	else if (halign == WINDOW_ALIGN_CENTER)
	{
		x = newx - width/2;
	}
	else
		x = newx;
	dirty = true;
}

int Window::GetX()
{
	if (halign == WINDOW_ALIGN_RIGHT)
	{
		return x + width;
	}
	else if (halign == WINDOW_ALIGN_CENTER)
	{
		return x + width/2;
	}
	return x;
}

void Window::SetY(int newy)
{
	if (valign == WINDOW_ALIGN_BOTTOM)
	{
		y = newy - height;
	}
	else if (valign == WINDOW_ALIGN_CENTER)
	{
		y = newy - height/2;
	}
	else
		y = newy;
	dirty = true;
}

int Window::GetY()
{
	if (valign == WINDOW_ALIGN_BOTTOM)
	{
		return y + height;
	}
	else if (valign == WINDOW_ALIGN_CENTER)
	{
		return y + height/2;
	}
	return y;
}

void Window::SetWidth(int newwidth)
{
	if (newwidth <= 0)	// use auto-width
		newwidth = auto_width;
	
	if (halign == WINDOW_ALIGN_RIGHT)
	{
		x = x + width - newwidth;
	}
	else if (halign == WINDOW_ALIGN_CENTER)
	{
		x = x + width/2 - newwidth/2;
	}
	
	width = newwidth;
	dirty = true;
}

int Window::GetWidth()
{
	return width;
}

void Window::SetHeight(int newheight)
{
	if (newheight < 0)	// use auto-width
		newheight = auto_height;
	if (valign == WINDOW_ALIGN_BOTTOM)
	{
		y = y + height - newheight;
	}
	else if (valign == WINDOW_ALIGN_CENTER)
	{
		y = y + height/2 - newheight/2;
	}
	height = newheight;
	dirty = true;
}

int Window::GetHeight()
{
	return height;
}

void Window::SetVisible(bool newv)
{
	visible = newv;
	dirty = true;
}

void Window::SetHAlign(WINDOW_ALIGN newha)
{
	int user_x = GetX();
	halign = newha;
	SetX(user_x);
	dirty = true;
}

void Window::SetVAlign(WINDOW_ALIGN newva)
{
	int user_y = GetY();
	valign = newva;
	SetY(user_y);
	dirty = true;
}

////////////////////////////////////////////////////////////

WindowManager::WindowManager()
{
	halign = WINDOW_ALIGN_LEFT;
	valign = WINDOW_ALIGN_TOP;

	update_enabled = true;
	display_switched = true;

	osd_fullscreen = false;
}

WindowManager::~WindowManager()
{
	DeInitialize();
}

bool WindowManager::Initialize()
{
	if (!jpeg_init())
		return false;
	
	// init OSD
	KHWL_OSDSTRUCT osd = { 0 };
	khwl_osd_switch(&osd, FALSE/*TRUE*/);

	// bpp is 8, i guess?
	if (osd.bpp != 8)
		return false;

	data = (BYTE *)osd.addr + 8 + 1024;
	pal = (BYTE *)osd.addr + 8;
	width = osd.width;
	height = osd.height;

	int offsx = 40, offsy = 30;
	left = offsx;
	top = offsx;
	right = width - 1 - offsy;
	bottom = height - 1 - offsy;
	
	// set default 'b/w' pal
	BYTE bw_colors[1024] = { 0, 0, 0, 0, 255, 255, 255, 255, 
		/////////// ------------ this is for test only!
		255, 55, 5, 255 };
	SetPalette(bw_colors, 0, 256);
	def_color = 1;
	def_bkcolor = 0;
	
	tr_color = 0;
	w_color = 1;

	tvout = (WINDOW_TVOUT)settings_get(SETTING_TVOUT);
	tvstandard = (WINDOW_TVSTANDARD)settings_get(SETTING_TVSTANDARD);

	Clear();

	guiimg = new ImageManager();
	guifonts = new FontManager();

	return true;
}

bool WindowManager::DeInitialize()
{
	SPSafeDelete(guiimg);
	SPSafeDelete(guifonts);
	removed_windows.Delete();
	windows.Delete();
	jpeg_deinit();
	return true;
}

////////////////////////////////////////////////////

bool WindowManager::IsOsdFullscreen()
{
	return osd_fullscreen;
}

void WindowManager::SetOsdFullscreen(bool is)
{
	osd_fullscreen = is;
	khwl_osd_setfullscreen(osd_fullscreen);
}

int WindowManager::GetColor()
{
	return def_color;
}

int WindowManager::GetBkColor()
{
	return def_bkcolor;
}

int WindowManager::GetTransparentColor()
{
	return tr_color;
}

int WindowManager::GetWhiteColor()
{
	return w_color;
}

WINDOW_ALIGN WindowManager::GetHAlign()
{
	return halign;
}

WINDOW_ALIGN WindowManager::GetVAlign()
{
	return valign;
}

////////////////////////////////////////////////////

bool WindowManager::SetColor(int c)
{
	if (c >= 0 && c < 256)
	{
		def_color = c;
		return true;
	}
	return false;
}

bool WindowManager::SetBkColor(int c)
{
	if (c < 256)
	{
		def_bkcolor = c;
		return true;
	}
	return false;
}

bool WindowManager::SetTransparentColor(int c)
{
	if (c >= 0 && c < 256)
	{
		tr_color = c;
		return true;
	}
	return false;
}

void WindowManager::SetHAlign(WINDOW_ALIGN a)
{
	halign = a;
}

void WindowManager::SetVAlign(WINDOW_ALIGN a)
{
	valign = a;
}

////////////////////////////////////////////////////

bool WindowManager::SetPalette(BYTE *argb, int offset, int num_colors)
{
	// check if current tr_color is valid
	if (tr_color >= offset && tr_color < offset + num_colors)
	{
		if (*(pal + (tr_color - offset) * 4) != 0)
			tr_color = -1;
	}
	BYTE *p = pal + offset * 4;
	int min_a = 255, max_a = 128, max_sum = 0;
	for (int i = 0; i < num_colors; i++)
	{
		BYTE Y, U, V;
		khwl_vgargbtotvyuv(argb[1], argb[2], argb[3], &Y, &U, &V);
		*p++ = argb[0];
		*p++ = Y;
		*p++ = U;
		*p++ = V;
		if (tr_color < 0 && argb[0] < min_a)
		{
			tr_color = i;
			min_a = argb[0];
		}
		int s = argb[1] + argb[2] + argb[3];
		if (argb[0] >= max_a && s > max_sum)
		{
			max_a = argb[0];
			max_sum = s;
			w_color = i;
		}
		argb += 4;
	}

	return true;
}

bool WindowManager::LoadPalette(const SPString & fname)
{
	FILE *fp = fopen(fname, "rb");
	if (fp == NULL)
	{
		msg_error("Cannot load palette from %s.\n", *fname);
		return false;
	}
	fseek(fp, 0, SEEK_END);
	int siz = ftell(fp);
	rewind(fp);

	if (siz < 256 * 3)
	{
		msg_error("Palette error: file %s is truncated.\n", *fname);
		fclose(fp);
		return false;
	}

	int maxsiz = 256 * 4 + 24;
	BYTE *pal = new BYTE [maxsiz];
	fread(pal, Min(siz, maxsiz), 1, fp);
	fclose(fp);

	if (siz == 256*3 || siz == 256*3+4)
	{
		BYTE *p = pal + 256*4;
		for (int i = 255; i >= 0; i--)
		{
			*--p = pal[i * 3 + 2];
			*--p = pal[i * 3 + 1];
			*--p = pal[i * 3];
			*--p = 0xff;
		}
	} 
	else if (pal[0] == 'R' && pal[1] == 'I' && pal[2] == 'F' && pal[3] == 'F')
	{
		BYTE *p = pal;
		int i, numc = (pal[17] << 8) | pal[16];
		for (i = 6; i < numc+6; i++)
		{
			*p++ = 0xff;// pal[i * 4 + 3];
			*p++ = pal[i * 4 + 0];
			*p++ = pal[i * 4 + 1];
			*p++ = pal[i * 4 + 2];
		}
		for (i = numc; i < 256; i++)
		{
			*p++ = 0;
			*p++ = 0;
			*p++ = 0;
			*p++ = 0;
		}
	}

	SetPalette(pal, 0, 256);

	delete [] pal;

	return true;
}

BYTE *WindowManager::GetPaletteEntry(int index)
{
	if (pal == NULL || index < 0 || index > 255)
		return NULL;
	return pal + index * 4;
}

bool WindowManager::SetBackground(char *fname, int hscale, int vscale, int offsx, int offsy, int *rotate)
{
	if (fname == NULL || fname[0] == '\0')
	{
		msg("Clearing background.\n");
		khwl_display_clear();
		khwl_set_window_source(-1, -1);
		return true;
	}
	msg("Setting background to %s\n", fname);
	//khwl_setvideomode(KHWL_VIDEOMODE_NORMAL, TRUE);
	if (!jpeg_show(fname, hscale, vscale, offsx, offsy, rotate))
	{
		msg_error("Cannot show JPEG '%s'.\n", fname);
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////

bool WindowManager::AddWindow(Window *w)
{
	if (w == NULL)
		return false;
	if (w == console)
		windows.Add(w);
	else
		windows.InsertBefore(console, w);
#if 0
printf("Windows = %d\n", windows.GetNum());
#endif
	return true;
}

bool WindowManager::RemoveWindow(Window *w)
{
	if (w == NULL)
		return false;
	if (w->removing)
		return true;
	w->removing = true;
	windows.Remove(w);
	removed_windows.Add(w);
	return true;
}

bool WindowManager::ShowWindow(Window *w, bool ison)
{
	if (w == NULL)
		return false;
	if (ison != w->visible)
	{
		w->SetVisible(ison);
	}
	return true;
}

void WindowManager::UpdateEnable(bool ison)
{
	update_enabled = ison;
}

bool WindowManager::IsUpdateEnabled()
{
	return update_enabled;
}

bool WindowManager::SetTv(WINDOW_TVOUT tvo, WINDOW_TVSTANDARD tvs)
{
	if (tvo != WINDOW_TVOUT_UNKNOWN)
		tvout = tvo;
	if (tvs != WINDOW_TVSTANDARD_UNKNOWN)
		tvstandard = tvs;
	return SwitchDisplay(true);
}

WINDOW_TVOUT WindowManager::GetTvOut()
{
	return tvout;
}

WINDOW_TVSTANDARD WindowManager::GetTvStandard()
{
	return tvstandard;
}

bool WindowManager::SwitchDisplay(bool ison)
{
	static const KHWL_TV_OUTPUT_FORMAT_TYPE khwl_tvout[] = { evTvOutputFormat_COMPOSITE, 
											evTvOutputFormat_COMPONENT_YUV, 
											evTvOutputFormat_COMPONENT_RGB_SCART };
	static const KHWL_TV_STANDARD_TYPE khwl_standard[] = 
	{ 
		evTvStandard_NTSC, evTvStandard_PAL,  
		evTvStandard_480P, evTvStandard_576P, evTvStandard_720P, evTvStandard_1080I,
	};

	static int old_out = -1, old_standard = -1, old_tvtype = -1;

	display_switched = ison;

	if (display_switched)
	{
		int tvtype = settings_get(SETTING_TVTYPE);
		if (tvout != old_out || tvstandard != old_standard || tvtype != old_tvtype)
		{
			khwl_setwide((tvtype == 2 || tvtype == 3) ? TRUE : FALSE);
			khwl_setdisplay(khwl_tvout[tvout], khwl_standard[tvstandard]);
			old_out = tvout;
			old_standard = tvstandard;
			old_tvtype = tvtype;
		}
	}
	else
	{
		khwl_setdisplay(evTvOutputFormat_OUTPUT_OFF, khwl_standard[tvstandard]);
		old_out = -1; old_standard = -1;
	}

	return true;
}

bool WindowManager::IsDisplaySwitchedOn()
{
	return display_switched;
}

bool WindowManager::Update()
{
	if (!update_enabled)
		return true;

	bool need_update = false;
	// clean-up after deleted windows too
	Window *win = removed_windows.GetFirst();
	while (win != NULL)
	{
		win->visible = false;
		if (UpdateForWindow(win))
			need_update = true;

		Window *next = win->next;
		removed_windows.Delete(win);
		win = next;
	}

	win = windows.GetFirst();
	while (win != NULL)
	{
		if (win->dirty && (win->visible || win->wasvisible))
		{
			if (UpdateForWindow(win))
				need_update = true;

			win->old_x = win->x;
			win->old_y = win->y;
			win->old_width = win->width;
			win->old_height = win->height;
			win->dirty = false;
			win->wasvisible = win->visible;
		}
		win = win->next;
	}
	if (need_update)
		return khwl_osd_update() == TRUE;
	return true;
}

bool WindowManager::UpdateForWindow(Window *win)
{
	static Context c;
	bool ret = false;
	int upd_x1, upd_y1, upd_x2, upd_y2;
	if (win != NULL)
	{
		upd_x1 = Min(win->old_x, win->x); 
		if (upd_x1 < 0) upd_x1 = 0;
		
		upd_y1 = Min(win->old_y, win->y); 
		if (upd_y1 < 0) upd_y1 = 0;
		
		upd_x2 = Max(win->old_x + win->old_width, win->x + win->width); 
		if (upd_x2 >= width) upd_x2 = width - 1;
		
		upd_y2 = Max(win->old_y + win->old_height, win->y + win->height); 
		if (upd_y2 >= height) upd_y2 = height - 1;
	} else
	{
		upd_x1 = 0;
		upd_y1 = 0;
		upd_x2 = width - 1;
		upd_y2 = height - 1;
	}

	// clear background if window was moved/sized or transparent/hidden
	if (win == NULL || win->x != win->old_x || win->y != win->old_y
		|| win->width != win->old_width || win->height != win->old_height
		|| win->transparent || !win->visible)
	{
		if (Clear(false, upd_x1, upd_y1, upd_x2, upd_y2))
			ret = true;
	}

	Window *winj = windows.GetFirst();
	while (winj != NULL)
	{
		if (winj->visible)
		{
			c.data = data + winj->y * width + winj->x;
			c.pitch = width;
			c.width = winj->width;
			c.height = winj->height;
			// apply update region
			int lupd_x1 = Max(upd_x1, winj->x);
			int lupd_y1 = Max(upd_y1, winj->y);
			int lupd_x2 = Min(upd_x2, winj->x + winj->width - 1);
			int lupd_y2 = Min(upd_y2, winj->y + winj->height - 1);
			if (lupd_x1 <= lupd_x2 && lupd_y1 <= lupd_y2)
			{
				lupd_x1 -= winj->x;
				lupd_y1 -= winj->y;
				lupd_x2 -= winj->x;
				lupd_y2 -= winj->y;
				if (winj->Update(&c, lupd_x1, lupd_y1, lupd_x2, lupd_y2))
					ret = true;
			}
		}
		winj = winj->next;
	}
	return ret;
}

bool WindowManager::TextOut(int x, int y, char *str, int nx)
{
	Context c;
	c.data = data + y * width + x;
	c.pitch = width;
	c.width = width;
	c.height = height;
	return console->TextOut(&c, str, nx);
}

bool WindowManager::Clear(bool updatenow, int x1, int y1, int x2, int y2)
{
	Context c;
	c.data = data;
	c.pitch = width;
	c.width = width;
	c.height = height;
	if (x1 < 0) x1 = 0;
	if (y1 < 0) y1 = 0;
	if (x2 < 0 || x2 >= width) x2 = width - 1;
	if (y2 < 0 || y2 >= height) y2 = height - 1;

	if (x2 < x1 || y2 < y1)
		return false;
	
	BYTE *d = c.data + y1 * c.pitch + x1;
	int len = (x2 - x1 + 1) * sizeof(BYTE);
	for (int iy = y1; iy <= y2; iy++)
	{
		memset(d, tr_color, len);
		d += c.pitch;
	}
	return updatenow ? (khwl_osd_update() == TRUE) : true;
}
