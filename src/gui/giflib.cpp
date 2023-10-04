//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - GIF (ani-gif) library source file
 *  \file       giflib.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       4.07.2004
 *
 *  Parts of the code taken from GIFLib, (C) Scott Heiman
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
#include <libsp/sp_msg.h>
#include <libsp/sp_memory.h>

#include <gui/giflib.h>
#include <gui/image.h>


const BYTE IMAGESEP                    = 0x2C;
const BYTE EXTENSIONINTRODUCER         = 0x21;
const BYTE EOGF                        = 0x3B;

const BYTE GRAPHIC_CONTROL_LABEL       = 0xF9;
const BYTE COMMENT_LABEL               = 0xFE;
const BYTE PLAIN_TEXT_LABEL            = 0x01;
const BYTE APPLICATION_EXTENSION_LABEL = 0xFF;

char errormsg[256];

static void err(const char *str)
{
	if (str != errormsg)
		strcpy(errormsg, str);
	msg_error("Gif error: %s\n", str);
}

/// inequality operator for ColorStruct
inline bool operator != (const ColorStruct &a, const ColorStruct &b) 
{
    return (a.Red != b.Red || a.Green != b.Green || a.Blue != b.Blue);
}

/// a nice, generic read function for binary files
template <class T>
bool SafeRead (T *data, unsigned int NumItems, FILE *file)
{
	if (fread((void *)data, sizeof(T), NumItems, file) != NumItems)
	{
		if (feof(file))
			err("Unexpected end-of-file in SafeRead");
		else
			err("Failed to read data in SafeRead");
		return false;
	}
	return true;
}

#if defined(__GNUC__)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define ATTRIBUTE_PACKED __attribute__ ((packed))
#endif
#endif
#ifdef WIN32
#pragma pack(1)
#define ATTRIBUTE_PACKED
#endif


typedef struct GraphicControlStruct
{
	BYTE BlockSize;
	BYTE PackedField;
	WORD DelayTime;
	BYTE TransparentColorIndex;
	BYTE BlockTerminator;
} ATTRIBUTE_PACKED GraphicControl;

typedef struct LogicalScreenDescriptorStruct
{
	WORD Width;
	WORD Height;
	BYTE  PackedField;  // see comments above in LSDField
	BYTE  BackgroundColorIndex;
	BYTE  PixelAspectRatio;
} ATTRIBUTE_PACKED LSD;

typedef struct ImageDescriptorStruct
{
	WORD Left;
	WORD Top;
	WORD Width;
	WORD Height;
	BYTE  Field;   // see comments above in IDField
} ATTRIBUTE_PACKED ImageDescriptor;

typedef struct ApplicationExtensionStruct
{
	BYTE BlockSize;  //should always be 11
	char		  Identifier[8];
	BYTE AuthenticationCode[3];
} ATTRIBUTE_PACKED ApplicationExtension;

typedef struct PlainTextStruct
{
	BYTE  BlockSize; // should always be 12
	WORD TextGridLeftPosition;
	WORD TextGridTopPosition;
	WORD TextGridTopWidth;
	WORD TextGridTopHeight;
	BYTE  CharacterCellWidth;
	BYTE  CharacterCellHeight;
	BYTE  TextForegroundColorIndex;
	BYTE  TextBackgroundColorIndex;
} ATTRIBUTE_PACKED PlainTextExtension;


#ifdef WIN32
#pragma pack()
#endif


char *fname = NULL;
/// ReadFile opens a GIF and stores the information in memory
bool GIFData::ReadFile(const char *filename)
{
	FILE *file;
	{
	file = fopen(filename, "rb");
	if (file == NULL)
	{
		sprintf(errormsg, "Could not open %s", filename);
		err(errormsg);
		goto err;
	}

	fname = (char *)filename;
	// check for signature
	if (!SafeRead(Version, 3, file))
		return false;
	Version[3] = 0;
	if (strcmp(Version, "GIF") != 0)
	{
		sprintf(errormsg, "%s is not a .GIF file", filename);
		err(errormsg);
		return false;
	}

	// get Version
	if (!SafeRead(Version, 3, file))
		return false;

	// extract LogicalScreenDescriptor
	LSD lsd;
	if (!SafeRead(&lsd, 1, file))
		return false;
	GlobalWidth = lsd.Width;
	GlobalHeight = lsd.Height;
	PixelDepth = (BYTE)(((lsd.PackedField & 0x70)>>4) + 1);

	// get the global color table if 1 is present
	if ((lsd.PackedField & 0x80) > 0)
	{
		GlobalNumColors = (WORD)(1 << (int)((lsd.PackedField & 0x07) + 1));
		GlobalColorTable = new ColorStruct[GlobalNumColors];
		if (GlobalColorTable == NULL)
		{
			err("Could not allocate memory for color table");
			return false;
		}

		if (!SafeRead(GlobalColorTable, GlobalNumColors, file))
			return false;
		BackgroundColorIndex = lsd.BackgroundColorIndex;
	}
	else
	{
		BackgroundColorIndex = 0;//-1;
		GlobalNumColors = 0;
	}

	// practice has shown that if (1 << PixelDepth) is < GlobalNumColors
	// then PixelDepth needs to be corrected
	if ((1 << PixelDepth) < GlobalNumColors)
	{
		if (GlobalNumColors < 5)
			PixelDepth = 2;
		else if (GlobalNumColors < 9)
			PixelDepth = 3;
		else if (GlobalNumColors < 17)
			PixelDepth = 4;
		else if (GlobalNumColors < 33)
			PixelDepth = 5;
		else if (GlobalNumColors < 65)
			PixelDepth = 6;
		else if (GlobalNumColors < 129)
			PixelDepth = 7;
		else
			PixelDepth = 8;
	}
	// read images
	BYTE DataByte;
	if (!SafeRead(&DataByte, 1, file))
		return false;
	while (DataByte != EOGF)
	{
		switch (DataByte)
		{
		case EXTENSIONINTRODUCER:
			ReadExtension(file);
		   	break;
		case IMAGESEP:
			if (!ReadImage(file))
				goto err;
				break;
		default:
			/*err("Unknown instruction")*/;
		}
		if (!SafeRead(&DataByte, 1, file))
			// [bombur]: the file is broken but let's give it a chance...
			break;
	}
	

	// if more than 1 image exists then remove unneeded images
	/*vector<GIFImage>::iterator image = Images.begin();

	while (image != Images.end() && Images.size() > 1)
	{
		if (image->DelayTime == 0)
			image = Images.erase(image);
		else
			image++;
	}*/

	if (numImages < 1)
		goto err;
	fclose(file);
	return true;
	}
err:
	if (file != NULL)
	{
		fclose(file);
	}
	return false;
}


bool GIFData::ReadExtension(FILE *file)
{
	BYTE block;
	BYTE data[256];

	BYTE Label;
	if (!SafeRead(&Label, 1, file))
		return false;

	switch (Label)
	{
	case GRAPHIC_CONTROL_LABEL:
		{
			GraphicControl GC;
			UseGC = true;
			if (!SafeRead(&GC, 1, file))
				return false;
			TransparentColorFlag = (GC.PackedField & 0x01) > 0;
			if (TransparentColorFlag)
				TransparentColorIndex = GC.TransparentColorIndex;
			DelayTime = (WORD)(10 * GC.DelayTime);
			// [bombur]: I guess it's better...
			if (DelayTime < 10)
				DelayTime = 100;
			DisposalMethod = (WORD)((GC.PackedField & 0x1C) >> 2);
			break;
		}
	// comment labels are read, but ignored.
	case COMMENT_LABEL:
			if (!SafeRead(&block, 1, file))
				return false;
			while (block != 0)
			{
				if (!SafeRead(data, (unsigned)block, file))
					return false;
				if (!SafeRead(&block, 1, file))
					return false;
			}
			break;
	// plain text labels are read, but ignored.
	case PLAIN_TEXT_LABEL:
		{
			PlainTextExtension pte;
			if (!SafeRead(&pte, 1, file))
				return false;
			if (!SafeRead(&block, 1, file))
				return false;
			while (block > 0)
			{
				//memset((void *)data, 0, 256);
				if (!SafeRead(data, (unsigned)block, file))
					return false;
				if (!SafeRead(&block, 1, file))
					return false;
			}
			break;
		}
	// application extension labels are read, but ignored.
	case APPLICATION_EXTENSION_LABEL:
		{
			ApplicationExtension AE;
			if (!SafeRead(&AE, 1, file))
				return false;
			if (!SafeRead(&block, 1, file))
				return false;
			while (block != 0)
			{
				if (!SafeRead(data, (unsigned)block, file))
					return false;
				if (!SafeRead(&block, 1, file))
					return false;
			}
			break;
		}
	default:
		/*err("Unknown Extension")*/;
	}
	return true;
}


GIFImage *GIFData::GetImage(int index)
{
	if (index < 0 || index >= (int)numImages)
	{
		err("Invalid index in GetImage()");
		return NULL;
	}

	return Images[index];
}


bool GIFData::ReadImage(FILE *file)
{
	ImageDescriptor id;
	if (!SafeRead(&id, 1, file))
		return false;

	// [bombur]: check if file is corrupt
	if (id.Left > 1000 || id.Top > 1000 || id.Width > 5000 || id.Height > 5000)
		return false;

	GIFImage *image = new GIFImage(this, id.Left, id.Top, id.Width, id.Height,
					PixelDepth, (id.Field & 0x40) > 0,
					TransparentColorFlag && UseGC ? TransparentColorIndex : -1,
					UseGC ? DelayTime : 0, DisposalMethod);

	//if a local color table is present, then read it in, otherwise copy
	// the global color table.
	if (id.Field & 0x80)
	{
		DWORD NC = 1 << ((id.Field & 0x07) + 1);
		image->ReadColorTable(NC, file);
		msg_error("Gif: Multiple palettes not supported for file %s\n", fname);
	}
	else
		image->CopyColorTable(GlobalNumColors, GlobalColorTable);

	// decode the bits
	if (image->ReadBits(file))
	{

		// save the image in the array
		Images = (GIFImage **)SPrealloc((void *)Images, (numImages + 1) * sizeof(GIFImage *));
		Images[numImages] = image;
		numImages++;
	}
	UseGC = false;
	return true;
}


// GIFImage functions
GIFImage::GIFImage(const GIFImage &g)
{
	typedef BYTE *pUChar;

	ColorTable = NULL;
	ppBits = NULL;

	Parent = g.Parent;
	Left = g.Left;
	Top = g.Top;
	Height = g.Height;
	Width = g.Width;
	ImageHeight = g.ImageHeight;
	ImageWidth = g.ImageWidth;
	PixelDepth = g.PixelDepth;
	ImagePixelDepth = g.ImagePixelDepth;
	NumColors = g.NumColors;
	ScanLineWidth = g.ScanLineWidth;
	Interlaced = g.Interlaced;
	DelayTime = g.DelayTime;
	DisposalMethod = g.DisposalMethod;
	BackgroundColorIndex = g.BackgroundColorIndex;
	TransparentColorIndex = g.TransparentColorIndex;

	// copy color table if 1 exists
	if (g .ColorTable)
	{
		ColorTable = new ColorStruct[NumColors];
		if (ColorTable == NULL)
			err("Could not allocate memory for color table in copy ctor");
		else
			memcpy((void *)ColorTable, (void *)g.ColorTable,
				NumColors*sizeof(ColorStruct));
	}

	// copy bits
	// assumes complete memory has been allocated for g.ppBits
	if (g.ppBits)
	{
		ppBits = new pUChar[ImageHeight];
		if (ppBits != NULL)
		{
			MakeBitmap();
			memcpy((void *)ppBits[0], (void *)g.ppBits[0], ImageWidth*ImageHeight);
		}
	}
	else
	{
		ppBits = NULL;
	}
}


GIFImage::GIFImage(GIFData *parent, int l, int t, int w, int h, int pd,
					bool inter, int TCIndex, int dt, WORD dm,
					bool /*bmp*/)
{
	Parent = parent;
	Left = (WORD)l;
	Top = (WORD)t;
	Width = (WORD)w;
	Height = (WORD)h;
	PixelDepth = (BYTE)pd;
	ImageWidth = Parent->GlobalWidth;
	ImageHeight = Parent->GlobalHeight;
	Interlaced = inter;
	ppBits = 0;
	ColorTable = 0;
	DelayTime = (WORD)dt;
	TransparentColorIndex = (short)TCIndex;
	BackgroundColorIndex = 0;//-1;
	DisposalMethod = dm;

	NumColors = (WORD)(1 << pd);
	ColorTable = new ColorStruct[NumColors];
	if (ColorTable == NULL)
		err("Could not allocate memory for color table in ctor");
	else
		memset((void *)ColorTable, 0, NumColors*sizeof(ColorStruct));

	// calculate the width of a scan line in bytes
	switch (PixelDepth)
	{
	case 1:
		ImagePixelDepth = 1;
		break;
	case 2:
	case 3:
	case 4:
		ImagePixelDepth = 4;
		break;
	default:
		ImagePixelDepth = 8;
		break;
	}

	ScanLineWidth = (ImageWidth * ImagePixelDepth + 7) / 8;
	// [bombur]: we don't need pitch align
	/*
	if (ScanLineWidth % 4)
		ScanLineWidth += (4 - ScanLineWidth % 4);
	*/

	// allocate memory for bitmap pixels
	typedef BYTE *pUChar;
	ppBits = new pUChar[ImageHeight];
	if (ppBits == NULL)
		 err("Could not allocate memory for bitmap in ctor");
}

/// dtor
GIFImage::~GIFImage()
{
	if (ColorTable)
		delete [] ColorTable;

	if (ppBits)
	{
		if (ppBits[0] != NULL)
			delete [] ppBits[0];
		delete [] ppBits;
	}
}

BYTE *GIFImage::GetScanLine(int index) const
{
	if (index < 0 || index >= (int)Height)
	{
		// err("Illegal index in GIFImage::GetScanLine");
		return 0;
	}

	return ppBits[index];
}

// taken out from ReadBits() to fit the stack...
#define MaxStackSize  4096
#define NullCode  (-1)

static short prefix[MaxStackSize];

static BYTE block_size, data_size, first, packet[256],
		pixel_stack[MaxStackSize+1], suffix[MaxStackSize], *top_stack,
		*q_buffer;


/// This function decodes the GIF data.  The logic was taken from
/// the ImageMagick source code. ;-)
bool GIFImage::ReadBits(FILE *file)
{
	DWORD dbgcnt;
	int index, available, bits, code, clear, code_mask, code_size,
		count, end_of_information, in_code, old_code;

	register DWORD i;

	register BYTE *c, *q;

	register DWORD datum;

	// [bombur]:
	int pitch = ImageWidth;// (ImageWidth + 7)/8*8;
 
	// Initialize GIF data stream decoder.
	q_buffer = new BYTE[Height*pitch];
	if (q_buffer == NULL)
	{
		err("Memory Allocation Error in ReadBits");
		return false;
	}

	dbgcnt = 0;
	if (!SafeRead(&data_size, 1, file))
		return false;
	clear = 1 << data_size;
	end_of_information = clear + 1;
	available = clear + 2;
	old_code = NullCode;
	code_size = data_size + 1;
	code_mask = (1 << code_size) - 1;
	for (code = 0; code < clear; code++)
	{
		prefix[code] = 0;
		suffix[code] = (BYTE) code;
	}
	// Decode GIF pixel stream.
	datum = 0;
	bits = 0;
	c = 0;
	count = 0;
	first = 0;
	top_stack = pixel_stack;
	q = q_buffer;

	int qline = 0;
	while (q < q_buffer + Height*pitch)
	{
		if (top_stack == pixel_stack)
		{
			if (bits < code_size)
			{
				// Load bytes until there is enough bits for a code.
				if (count == 0)
				{
				  	// Read a new data block.
					if (!SafeRead(&block_size, 1, file))
						return false;
					count = block_size;
					if (!SafeRead(packet, (unsigned)block_size, file))
						return false;
					if (count <= 0)
						break;
					c = packet;
				}
				datum += (*c) << bits;
				bits += 8;
				c++;
				count--;
				continue;
			}
			// Get the next code.
			code = datum & code_mask;
			datum >>= code_size;
			bits -= code_size;
			// Interpret the code
			if ((code > available) || (code == end_of_information))
				break;
			if (code == clear)
			{
				// Reset decoder.
				code_size = data_size + 1;
				code_mask = (1 << code_size) - 1;
				available = clear + 2;
				old_code = NullCode;
				continue;
			}
			if (old_code == NullCode)
			{
				*top_stack++ = suffix[code];
				old_code = code;
				first = (BYTE) code;
				continue;
			}
			in_code = code;
			if (code >= available)
			{
				*top_stack++ = first;
				code = old_code;
			}
			while (code >= clear)
			{
				*top_stack++ = suffix[code];
				code = prefix[code];
			}
			first = suffix[code];
			// Add a new string to the string table,
			if (available >= MaxStackSize)
				break;
			*top_stack++ = first;
			prefix[available] = (short)old_code;
			suffix[available] = first;
			available++;
			if (((available & code_mask) == 0) && (available < MaxStackSize))
			{
				code_size++;
				code_mask+=available;
			}
			old_code = in_code;
		}
		// Pop a pixel off the pixel stack.
		top_stack--;
		index = (*top_stack);
		*q = (BYTE)index;
		q++;
		qline++;
		if (qline >= (int)Width)
		{
			q += pitch - Width;
			qline = 0;
		}
		dbgcnt++;
	}

	if (dbgcnt < Width * Height)
	{
		delete [] q_buffer;
		err("Corrupt .GIF image");
		return false;
	}

	if (Interlaced)
	{
		static const int
		interlace_rate[4] = { 8, 8, 4, 2 },
		interlace_start[4] = { 0, 4, 2, 1 };

		BYTE *i_buffer = new BYTE[Height*pitch];
		if (!i_buffer)
		{
			delete [] q_buffer;
			err("Memory allocation error");
			return false;
		}

		memcpy((void *)i_buffer, (void *)q_buffer, Height*pitch);
		memset((void *)q_buffer, 0, Height*pitch);

		int row = 0;
		for (int pass = 0; pass < 4; pass++)
		{
			DWORD j =  interlace_start[pass];
			while (j < Height)
			{
				BYTE *p = i_buffer + row * pitch;
				q = q_buffer + j * pitch;
				memcpy((void *)q, (void *)p, Width);
				row++;
				j += interlace_rate[pass];
			}
		}

		delete [] i_buffer;
	}

	// create the array to store the entire unpacked image
	BYTE **unpackedBits = ppBits;

	// if this is not the 1st image...
	if (Parent->numImages > 0)
	{
		int last;
		for (last = Parent->numImages - 1; last >= 0 && Parent->Images[last]->DisposalMethod == 3; last--)
			;
		GIFImage *lastImage = Parent->Images[last];

		// unpackedBits is initialized to the previous image
		lastImage->GetUnpackedBitArray(unpackedBits);
		if (lastImage->DisposalMethod == 2)
		{
			for (DWORD i = lastImage->Top; i < lastImage->Height + lastImage->Top; i++)
			{
				BYTE *p = unpackedBits[i];
				memset((void *)(p+lastImage->Left), BackgroundColorIndex,
					lastImage->Width);
			}
		}
	}
	else /*if (UseBitmap)*/
	{
		memset((void *)unpackedBits[0], BackgroundColorIndex, pitch * ImageHeight);
	}

	// add the subimage
	q = q_buffer;
	// if no transparent pixels are present then overwrite the existing image...
	if (TransparentColorIndex == -1)
	{
		for (i = Top; i < Top + Height; i++)
		{
			memcpy((void *)(unpackedBits[i]+Left), (void *)q, Width);
			q += pitch;
		}
	}
	// ... otherwise, only overwrite non-transparent pixels
	else
	{
		for (i = Top; i < Top + Height; i++)
		{
			BYTE *oldImage = unpackedBits[i]+Left;
			/*
			if (UseBitmap)*/
			{
				for (BYTE *newImage = q; newImage < q + Width; newImage++, oldImage++)
				{
					if ((short)(*newImage) != TransparentColorIndex)
						*oldImage = *newImage;
				}
			}
			/*else
				memcpy((void *)oldImage, (void *)q, Width);
			*/
			q += pitch;
		}
	}

	delete [] q_buffer;

	// read the terminator
	SafeRead(&block_size, 1, file);
	/*
	if (block_size != 0)
		err("Terminator was not found in the expected location");
	*/
	return true;
}

/// Read the color table from the GIF file and copy it to the bitmap (if one is present)
void GIFImage::ReadColorTable(DWORD NC, FILE *file)
{
	NumColors = (WORD)NC;
	SafeRead(ColorTable, NC, file);
/*
	// mess with transparency & background if we are creating a bitmap
	// set the transparent color to the screen color if this is the first image
	if (TransparentColorIndex > -1 && Parent->numImages == 0)
		ColorTable[TransparentColorIndex] = Parent->ScreenColor;

	// if the color table contains the screen color then set the
	// background color index to the appropriate value; otherwise,
	// set it to the parent's background color index
	bool NotFound = true;
	
	while (++BackgroundColorIndex < NumColors && NotFound)
		NotFound = ColorTable[BackgroundColorIndex] != Parent->ScreenColor;

	if (NotFound)
		BackgroundColorIndex = (int)Parent->BackgroundColorIndex;
	else
		BackgroundColorIndex--;
*/
	MakeBitmap();
}


/// This function copies the global color table to the image color table and
/// create the bitmap.
void GIFImage::CopyColorTable(WORD GlobalNumColors, ColorStruct *GlobalColorTable)
{
	NumColors = GlobalNumColors;
	memcpy((void *)ColorTable, (void *)GlobalColorTable,
		   GlobalNumColors*sizeof(ColorStruct));

	// mess with transparency & background if we are creating a bitmap
	// set the transparent color to the screen color if this is the first image
	// [bombur]:
	if (TransparentColorIndex > -1 ) // && Parent->numImages == 0
			ColorTable[TransparentColorIndex] = Parent->ScreenColor;
/*
	// if the color table contains the screen color then set the
	// background color index to the appropriate value; otherwise,
	// set it to the parent's background color index
	bool NotFound = true;
	while (++BackgroundColorIndex < NumColors && NotFound)
		NotFound = ColorTable[BackgroundColorIndex] != Parent->ScreenColor;
	
	if (NotFound)
		BackgroundColorIndex = (int)Parent->BackgroundColorIndex;
	else
		BackgroundColorIndex--;
*/

	MakeBitmap();
}

/// This function creates the bitmap.  This function assumes that the color
/// table has already been stored in memory.
bool GIFImage::MakeBitmap()
{
	ppBits[0] = new BYTE[ImageWidth*ImageHeight];
	if (ppBits[0] == NULL)
	{
		err("Memory Allocation Error");
		return false;
	}

	for (DWORD i = 1; i < ImageHeight; i++)
		ppBits[i] = ppBits[i-1] + ImageWidth;
	
	return true;
}

void GIFImage::GetUnpackedBitArray(BYTE **bits)
{
	if (bits == NULL)
		return;
	for (DWORD i = 0; i < ImageHeight; i++)
	{
		memcpy((void *)(bits[i]), (void *)(ppBits[i]), ImageWidth);
	}
}

