//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - image class header file
 *  \file       image.h
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

#ifndef SP_IMAGE_H
#define SP_IMAGE_H

#include <gui/res.h>
#include <gui/window.h>

/// Image data class (incl. animation frames)
class ImageData : public Resource
{
public:
	/// ctor
	ImageData();
	/// dtor
	~ImageData();

	BOOL Load();
	BOOL Unload();

public:	
	BYTE **data;	// BYTE *data[num_frames];
	int num_frames;
	int width, height;	// all frames have the same size

	/// delta time for each frame, in milliseconds
	int *dt;
	int total_time;

	bool unloaded;
};

/// Image window class
class Image : public Window
{
public:
	/// ctor
	Image();
	/// dtor
	virtual ~Image();

	virtual bool Update(int curtime);

	/// Update part of image (rect in LOCAL window coords)
	virtual bool Update(Context *context, int x1, int y1, int x2, int y2);

public:

	bool SetSource(char *fname);

	void SetFlipX(bool);
	void SetFlipY(bool);

	/// Pointer to the image data used
	ImageData *img;
	/// Current frame index
	int cur_frame;
	int last_time;
	bool flipx, flipy;

	/// update queue object (used by script)
	friend class ScriptTimerObject;
	ScriptTimerObject *updateobj;
};

/// Image manager class (contains all image data and other info)
class ImageManager
{
public:
	/// ctor
	ImageManager();
	/// dtor
	~ImageManager();

public:

	// Clear image cache
	BOOL ClearImageData();

	ImageData *GetImageData(char *fname);

	SPHashListAbstract<ImageData, ImageData> datahash;

	void DumpImages();
};

extern ImageManager *guiimg;

#endif // of SP_IMAGE_H
