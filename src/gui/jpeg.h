//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - JPEG file display header file
 *  \file       jpeg.h
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

#ifndef SP_JPEG_H
#define SP_JPEG_H

#ifdef __cplusplus
extern "C" {
#endif

/// Initialize (allocate buffers)
BOOL jpeg_init();

/// Deinitialize (release buffers)
BOOL jpeg_deinit();

/// Display JPEG file
/// rotation is auto-detected and returned, if Exif data is present.
BOOL jpeg_show (char *filename, int hscale = 0, int vscale = 0, int offsx = 0, int offsy = 0, int *rot = NULL);


#ifdef __cplusplus
}
#endif

#endif // of SP_JPEG_H
