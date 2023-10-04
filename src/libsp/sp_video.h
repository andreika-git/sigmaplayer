//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - KHWL video functions/enums properties.
 *  \file       sp_video.h
 *  \author     bombur
 *  \version    0.1
 *  \date       14.09.2004
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

#ifndef SP_SPVIDEO_H
#define SP_SPVIDEO_H

#ifdef __cplusplus
extern "C" {
#endif


// Video mode
typedef enum 
{
	KHWL_VIDEOMODE_NONE = 0,

	KHWL_VIDEOMODE_NORMAL = 1,	// source = 4:3, display = 4:3, normal
	KHWL_VIDEOMODE_VCENTER,		// source = 4:3, display = 16:9, crop top/bottom
	KHWL_VIDEOMODE_HCENTER,		// source = 4:3, display = 16:9, black band on left/right
	KHWL_VIDEOMODE_WIDE,		// source = 16:9, display = 16:9, wide
	KHWL_VIDEOMODE_PANSCAN,		// source = 16:9, display = 4:3, panscan (crop left/right)
	KHWL_VIDEOMODE_LETTERBOX,	// source = 16:9, display = 4:3, letterbox (black band on top/bottom)

} KHWL_VIDEOMODE;

typedef enum
{
	KHWL_ZOOMMODE_YUV = 0,
	KHWL_ZOOMMODE_DVD,
	KHWL_ZOOMMODE_ASPECT,
} KHWL_ZOOMMODE;

/// Set display output -1=turn off else = set tvoutput mode
/// and standard.
int khwl_setdisplay(KHWL_TV_OUTPUT_FORMAT_TYPE mode, KHWL_TV_STANDARD_TYPE standard);

/// Clear YUV screen
int khwl_display_clear();

/// Set video mode (change aspect ratio)
/// Use frame width/height if set.
void khwl_setvideomode(KHWL_VIDEOMODE mode, BOOL set_hw);

/// Transform coord (used for anamorphic modes and overlays)
void khwl_transformcoord(KHWL_VIDEOMODE mode, int *x, int *y, int width, int height);

/// Get screen width
void khwl_getscreensize(int *width, int *height);

void khwl_set_window(int src_width, int src_height, int frame_width, int frame_height, 
					 int hscale, int vscale, int offsx, int offsy);

void khwl_set_window_zoom(KHWL_ZOOMMODE);

void khwl_set_window_source(int src_width, int src_height);

/// Set Window Frame in OSD coordinates!
void khwl_set_window_frame(int fr_left, int fr_top, int fr_right, int fr_bottom);
/// Get Window Frame in OSD coordinates!
void khwl_get_window_frame(int *fr_left, int *fr_top, int *fr_right, int *fr_bottom);

/// Set display settings (values 0..1000)
void khwl_set_display_settings(int brightness, int contrast, int saturation);

/// Get display settings (values 0..1000)
void khwl_get_display_settings(int *brightness, int *contrast, int *saturation);

/// Used for start-up procedure only...
void khwl_setwide(BOOL wide);

#ifdef __cplusplus
}
#endif

#endif // of SP_SPVIDEO_H
