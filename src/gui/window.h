//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - abstract window class header file
 *  \file       window.h
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

#ifndef SP_WINDOW_H
#define SP_WINDOW_H

/// Drawing context
class Context
{
public:
	/// 8-bit color data
	BYTE *data;
	/// data pitch
	int pitch;
	/// context dims
	int width, height;
};

//////////////////////////////////////////////////////////////////

enum WINDOW_ALIGN
{
	WINDOW_ALIGN_TOP = 0,
	WINDOW_ALIGN_LEFT = 0,
	WINDOW_ALIGN_CENTER = 1,
	WINDOW_ALIGN_RIGHT = 2,
	WINDOW_ALIGN_BOTTOM = 2
};

enum WINDOW_TVSTANDARD
{
	WINDOW_TVSTANDARD_UNKNOWN = -1,
	WINDOW_TVSTANDARD_NTSC = 0,
	WINDOW_TVSTANDARD_PAL,
	WINDOW_TVSTANDARD_480P,
	WINDOW_TVSTANDARD_576P,
	WINDOW_TVSTANDARD_720P,
	WINDOW_TVSTANDARD_1080I,
};

enum WINDOW_TVOUT
{
	WINDOW_TVOUT_UNKNOWN = -1,
	WINDOW_TVOUT_COMPOSITE = 0,
	WINDOW_TVOUT_YPBPR,
	WINDOW_TVOUT_RGB,
};

class ScriptTimerObject;

/// Abstract window class
class Window
{
public:
	/// ctor
	Window();
	/// dtor
	virtual ~Window() 
	{
		SPSafeDelete(group);
	}

	/// Update object's state for current time (in milliseconds)
	virtual bool Update(int curtime);

	/// Update part of window in LOCAL coords
	virtual bool Update(Context *context, int x1, int y1, int x2, int y2) = 0;

	/// Set window center using halign
	void SetX(int);
	/// Set window center using valign
	void SetY(int);
	/// Set window width using halign
	void SetWidth(int);
	/// Set window height using valign
	void SetHeight(int);
	
	void SetHAlign(WINDOW_ALIGN);
	void SetVAlign(WINDOW_ALIGN);

	/// Get window center using halign
	int GetX();
	/// Get window center using valign
	int GetY();
	int GetWidth();
	int GetHeight();

	void SetVisible(bool);

public:
	/// window coords
	int x, y;
	/// window size
	int width, height;

	WINDOW_ALIGN halign, valign;
	/// window is visible?
	bool visible, wasvisible;
	/// set this to update window
	bool dirty;
	/// set if window is being removed
	bool removing;
	/// if window is transparent
	bool transparent;

	/// saved coords & dims
	int old_x, old_y, old_width, old_height, auto_width, auto_height;

	SPString *group;
	
	int timer;
	/// update queue object (used by script)
	ScriptTimerObject *timerobj;

	Window *prev, *next;
};

/// Window manager
class WindowManager
{
public:
	/// ctor
	WindowManager();
	/// dtor
	~WindowManager();

	/// initialize
	bool Initialize();

	/// deinitialize
	bool DeInitialize();

	/// Turn display on/off
	bool SwitchDisplay(bool ison);
	bool IsDisplaySwitchedOn();
	bool SetTv(WINDOW_TVOUT = WINDOW_TVOUT_UNKNOWN, WINDOW_TVSTANDARD = WINDOW_TVSTANDARD_UNKNOWN);
	WINDOW_TVOUT GetTvOut();
	WINDOW_TVSTANDARD GetTvStandard();

	/// Add window to the list
	bool AddWindow(Window *);

	/// Remove window from the list
	bool RemoveWindow(Window *);

	/// Show/Hide window
	bool ShowWindow(Window *, bool);

	/// Update windows if needed
	bool Update();

	/// Enable/disable windows update
	void UpdateEnable(bool ison);
	bool IsUpdateEnabled();

	/// Clear screen
	bool Clear(bool updatenow = true, int x1 = 0, int y1 = 0, int x2 = -1, int y2 = -1);

	////////////////////////////////////////////
	bool TextOut(int x, int y, char *str, int nx = 0);

	////////////////////////////////////////////
	int GetColor();
	int GetBkColor();
	int GetTransparentColor();
	int GetWhiteColor();
	WINDOW_ALIGN GetHAlign();
	WINDOW_ALIGN GetVAlign();

	bool IsOsdFullscreen();
	void SetOsdFullscreen(bool);

	bool SetColor(int);
	bool SetBkColor(int);
	bool SetTransparentColor(int);
	void SetHAlign(WINDOW_ALIGN);
	void SetVAlign(WINDOW_ALIGN);

	bool LoadPalette(const SPString & fname);
	bool SetPalette(BYTE *argb, int offset, int num_colors);

	/// in ARGB form
	BYTE *GetPaletteEntry(int index);

	bool SetBackground(char *fname, int hscale = 100, int vscale = 100, int offsx = 0, int offsy = 0, int *rotate = NULL);

public:
	/// Use these freely
	int left, top, right, bottom;
	WINDOW_ALIGN halign, valign;

protected:
	/// windows list
	SPDLinkedListAbstract<Window, Window> windows;
	SPDLinkedListAbstract<Window, Window> removed_windows;

private:
	BYTE *data;
	BYTE *pal;
	/// won't be changed
	int width, height;

	// current def.object color & background color
	int def_color, def_bkcolor;
	// current transparent color
	int tr_color;
	// current white color
	int w_color;

	bool update_enabled;
	bool display_switched;

	bool osd_fullscreen;

	/// Update screen area for given window (or entire screen if NULL).
	/// (Not thread-safe!)
	bool UpdateForWindow(Window *win);

	/// Current values. See SETTING_TVOUT and SETTING_TVSTANDARD.
	WINDOW_TVOUT tvout;
	WINDOW_TVSTANDARD tvstandard;
};

/// User interface manager
extern WindowManager gui;

#endif // of SP_WINDOW_H
