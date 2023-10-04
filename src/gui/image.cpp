//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - image class impl.
 *  \file       image.cpp
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

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_memory.h>

#include <gui/image.h>
#include <gui/giflib.h>

ImageManager *guiimg = NULL;


ImageData::ImageData()
{
	data = NULL;
	dt = NULL;
	num_frames = 0;
	width = height = 0;
	total_time = 0;

	unloaded = false;
}

ImageData::~ImageData()
{
	Unload();
}

BOOL ImageData::Load()
{
	Unload();
	GIFData *gifdata = new GIFData();
	if (gifdata == NULL)
		return FALSE;
	if (!gifdata->ReadFile(name))
	{
		delete gifdata;
		return FALSE;
	}

	data = (BYTE **)SPmalloc(sizeof(BYTE *) * (gifdata->GetNumImages() + 1));

#if 0
	msg("+LOAD %s...\n", name);
	SPMemoryManager &memManager = SPMemoryManager::GetHandle();
	FILE *fp = fopen("sp_log.txt", "at");
	memManager.DumpLastAlloc(fp);
	fclose(fp);
#endif


	if (data == NULL || gifdata->GetNumImages() < 1)
	{
		msg_error("Image: image data not found for %s.\n", name);
		delete gifdata;
		return FALSE;
	}
	num_frames = gifdata->GetNumImages();
	width = gifdata->GetWidth();
	height = gifdata->GetHeight();
	dt = new int [num_frames];
	total_time = 0;
	for (int i = 0; i < (int)gifdata->GetNumImages(); i++)
	{
		GIFImage *img = gifdata->GetImage(i);
		if (img == NULL)
		{
			delete gifdata;
			return FALSE;
		}
		if (img->GetWidth() != gifdata->GetWidth() || img->GetHeight() != gifdata->GetHeight())
		{
			delete gifdata;
			return FALSE;
		}
		data[i] = img->GetData();
		dt[i] = img->GetTimeDelay();
		total_time += dt[i];
	}
	delete gifdata;

//	msg("GIF %s loaded OK.\n", name);
	
	unloaded = false;
	return TRUE;
}

BOOL ImageData::Unload()
{
	if (data != NULL)
	{
#if 0
msg("-UNLOAD %s...\n", name);
#endif
		for (int i = 0; i < num_frames; i++)
		{
			SPSafeDeleteArray(data[i]);
		}
	}
	SPSafeFree(data);
	SPSafeDeleteArray(dt);
	unloaded = true;
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////

Image::Image()
{
	img = NULL;
	flipx = flipy = false;
	cur_frame = 0;
	last_time = -1;

	updateobj = NULL;
}

Image::~Image()
{
}

bool Image::SetSource(char *fname)
{
	img = guiimg->GetImageData(fname);
	if (img == NULL)
		return false;
	SetWidth(img->width);
	SetHeight(img->height);
	auto_width = img->width;
	auto_height = img->height;
	dirty = true;
	return true;
}

void Image::SetFlipX(bool fx)
{
	flipx = fx;
	dirty = true;
}

void Image::SetFlipY(bool fy)
{
	flipy = fy;
	dirty = true;
}

bool Image::Update(int curtime)
{
	int oldcf = cur_frame;
	if (last_time < 0 || img == NULL || img->total_time == 0)
	{
		cur_frame = 0;
		last_time = curtime;
	} else
	{
		int dt = (curtime - last_time) % img->total_time;
		for (int i = 0, cf = cur_frame; i < img->num_frames; i++)
		{
			cf = (cf + 1) % img->num_frames;
			int d = img->dt[cf];
			if (d > dt)
				break;
			cur_frame = cf;
			last_time = curtime;
			dt -= d;
			if (dt <= 0)
				break;
		}
	}

	if (cur_frame != oldcf)
		dirty = true;
	return true;
}

bool Image::Update(Context *context, int x1, int y1, int x2, int y2)
{
	if (img == NULL || context == NULL)
		return false;
	if (img->unloaded)
	{
		if (!img->Load())
			return false;
	}
	if (img->data == NULL || img->num_frames < 1)
		return false;
	if (cur_frame < 0)
		cur_frame = 0;
	cur_frame %= img->num_frames;

	if (img->width < 1 || img->height < 1)
		return false;
	if (x1 >= img->width)
		return false;
	if (y1 >= img->height)
		return false;
	if (x2 >= img->width) x2 = img->width - 1;
	if (y2 >= img->height) y2 = img->height - 1;

	BYTE *sd = img->data[cur_frame] + (flipy ? img->height-y1-1 : y1) * img->width + (flipx ? x2 : x1);
	BYTE *dd = context->data + y1 * context->pitch + x1;
	int len = x2 - x1 + 1;
	int iw = (flipy) ? -img->width : img->width;
	
	if (flipx)
	{
		for (int iy = y1; iy <= y2; iy++)
		{
			register BYTE *sdi = sd;
			register BYTE *ddi = dd;
			for (int ix = len - 1; ix >= 0; ix--)
				*ddi++ = *--sdi;
			dd += context->pitch;
			sd += iw;
		}
	} else
	{
		for (int iy = y1; iy <= y2; iy++)
		{
			memcpy(dd, sd, len);
			dd += context->pitch;
			sd += iw;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

ImageManager::ImageManager()
{
	datahash.SetN(resource_num_hash);
}

ImageManager::~ImageManager()
{
}

BOOL ImageManager::ClearImageData()
{
	ImageData *cur = datahash.GetFirst();
	while (cur != NULL)
	{
		cur->Unload();
		cur = datahash.GetNext(*cur);
	}
	return TRUE;
}

ImageData *ImageManager::GetImageData(char *fname)
{
	// first, search the hash
	static ImageData testdata;
	testdata.SetConstName(fname);
	ImageData *dat = datahash.Get(testdata);
	testdata.name = NULL;
	if (dat != NULL)
		return dat;
	ImageData *newdat = new ImageData();
	newdat->SetName(fname);
	if (!newdat->Load())
	{
		delete newdat;
		return NULL;
	}
	
	datahash.Add(newdat);

	return newdat;
}

void ImageManager::DumpImages()
{
	ImageData *cur = datahash.GetFirst();
	while (cur != NULL)
	{
		if (!cur->unloaded)
			msg("IMAGE %s\n", cur->name);
		cur = datahash.GetNext(*cur);
	}
}
