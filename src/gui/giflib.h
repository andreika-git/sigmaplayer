//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - GIF (ani-gif) library header file
 *  \file	   giflib.h
 *  \author    bombur
 *  \version   0.1
 *  \date	   4.07.2004
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

#ifndef SP_GIFLIB_H
#define SP_GIFLIB_H

#include <stdio.h>

#ifdef WIN32
#pragma pack(1)
#endif

typedef struct ColorStruct
{
	BYTE Red;
	BYTE Green;
	BYTE Blue;

} ATTRIBUTE_PACKED ColorStruct;

#ifdef WIN32
#pragma pack()
#endif

class GIFData;

/// Contains GIF image
class GIFImage
{
public:
	friend class GIFData;
	/// ctor
	GIFImage()
	{
		Parent = 0;
		Left = Top = 0;
		Height = Width = 0;
		ImagePixelDepth = PixelDepth = 0;
		NumColors = 0;
		ScanLineWidth = 0;
		ColorTable = 0;
		ppBits = 0;
		Interlaced = false;
		TransparentColorIndex = BackgroundColorIndex = -1;
		DelayTime = 0;
		DisposalMethod = 0;
	}
	
	/// ctor used by GIFData to create a GIFImage
	GIFImage (GIFData *parent, int l, int t, int w, int h, int pd,
			  bool inter, int TCIndex = -1, int dt = 0, WORD dm = 0,
			  bool bmp = true);
	/// copy ctor
	GIFImage (const GIFImage &g);
	/// dtor
	~GIFImage();

	/// Access functions
	WORD GetWidth() const { return (WORD)ImageWidth; }
	WORD GetHeight() const { return (WORD)ImageHeight; }
	BYTE BitsPerPixel() const { return PixelDepth; }
	BYTE ImageBitsPerPixel() const { return ImagePixelDepth; }
	WORD GetNumColors() const { return NumColors; }
	ColorStruct *GetColorTable() const { return ColorTable; }
	
	BYTE *GetData() { BYTE *d = ppBits[0]; ppBits[0] = NULL; return d; }
	
	BYTE *GetScanLine (int index) const;
	DWORD GetScanLineWidth() const { return ScanLineWidth; }
	WORD GetTimeDelay() const { return DelayTime; }

	void GetUnpackedBitArray (BYTE **bits);

	short GetBackgroundColorIndex() const { return BackgroundColorIndex; }
	short GetTransparentColorIndex() const { return TransparentColorIndex; }

	/// Access functions used by the multimedia timer callback function
	BOOL EraseBkgnd() const { return DisposalMethod == 2 ? TRUE : FALSE; }

protected:
	/// the GIFData object that owns this image
	GIFData *Parent;
	/// bytes needed to store 1 line of data in an HBITMAP
	DWORD  ScanLineWidth;
	/// Width and height of the subimage in pixels
	DWORD Height;
	DWORD Width;
	/// Width and height of the merged image in pixels
	DWORD ImageHeight;
	DWORD ImageWidth;
	/// the location of the upper, left corner of the image relative to Image[0]
	WORD Left;
	WORD Top;
	/// the number of bits needed to define all colors in the image
	BYTE  PixelDepth;
	/// the number of bits needed to define all of the colors in the HBITMAP.
	/// must be 1, 4, or 8 (I have not seen any 24 or 32 bit GIFs)
	BYTE  ImagePixelDepth;
	/// number of colors in the local color table
	WORD NumColors;
	/// the local color table
	ColorStruct	*ColorTable;
	/// an 2-D array of bits that define the image.  If UseBitmap == true then
	/// the array is dimensioned as required by an HBITMAP; otherwise, the
	/// dimensions are ppBits[ImageHeight][ImageWidth]
	BYTE  **ppBits;
	/// true if the image is interlaced
	bool		   Interlaced;
	/// the index in the local color table that is transparent.
	/// set to -1 if there is no transparent color
	short		  TransparentColorIndex;
	/// background color index
	/// set to -1 if there is no background color
	short		   BackgroundColorIndex;
	/// the time that this image is displayed if it is an animated GIF
	WORD DelayTime;
	/// the disposal method.  Determines how the image is drawn if the GIF
	/// is animated.
	/// DisposalMethod	  Action
	///	  0			  Nothing special.  Treat as DisposalMethod == 1
	///	  1			  image remains in place
	///	  2			  background is restored before merging w/ nxt img
	///	  3			  image is retained and merged with next image
	WORD DisposalMethod;

	/// create hBitmap
	bool MakeBitmap();
	/// used to copy the GIFData's global color table into the local color table
	void CopyColorTable (WORD GlobalNumColors,
						 ColorStruct *GlobalColorTable);
	/// reads the local color table from the GIF file
	void ReadColorTable (DWORD NC, FILE *file);
	/// decodes the bit data from the GIF file
	bool ReadBits (FILE *file);
};

/// Contains entire GIF data (from file)
class GIFData
{
public:
	/// ctor
	GIFData()
	{
		Images = (GIFImage **)SPmalloc(sizeof(GIFImage *));
		numImages = 0;
		GlobalWidth = GlobalHeight = 0;
		PixelDepth = 0;
		GlobalNumColors = 0;
		GlobalColorTable = 0;
		TransparentColorIndex = BackgroundColorIndex = 0;
		TransparentColorFlag = false;
		UseGC = false;
		DelayTime = 0;
		ScreenColor.Red   = 0/*GetRValue( color )*/;
		ScreenColor.Green = 0/*GetGValue( color )*/;
		ScreenColor.Blue  = 0/*GetBValue( color )*/;
	}

	/// dtor
	~GIFData()
	{
		if (GlobalColorTable != NULL)
			delete [] GlobalColorTable;
		for (int i = 0; i < numImages; i++)
			delete Images[i];
		SPfree(Images);
	}

	/// access functions
	WORD GetWidth() const { return GlobalWidth; }
	WORD GetHeight() const { return GlobalHeight; }
	BYTE BitsPerPixel() const { return PixelDepth; }
	BYTE GetNumColors() const { return (char)GlobalNumColors; }
	ColorStruct *GetColorTable() const { return GlobalColorTable; }
	
	GIFImage *GetImage(int index);
	DWORD GetNumImages() const { return numImages; }

	/// reads a GIF and stores the contents in memory
	bool ReadFile (const char *filename);

	friend class GIFImage;

protected:
	/// the height and width of the largest image in the group
	WORD   GlobalHeight;
	WORD   GlobalWidth;
	/// the number of pixels needed to access all colors used by the image
	BYTE	PixelDepth;
	/// the number of colors in the global color table
	WORD   GlobalNumColors;
	/// the global color table
	ColorStruct	 *GlobalColorTable;
	/// the version number
	char			 Version[4];
	/// the index to the background color
	BYTE	BackgroundColorIndex;
	/// an array to hold all of the images in the GIF
	GIFImage **Images;
	int numImages;
	/// --- the following variables are used to hold graphic control information
	/// true if a graphic control extension was be used with the next image
	bool			 UseGC;
	/// the index to the transparent color
	BYTE	TransparentColorIndex;
	/// true if a transparent color is present
	bool			 TransparentColorFlag;
	/// the time that an image will be displayed
	WORD   DelayTime;
	/// the disposal method for the next GIF image
	WORD   DisposalMethod;
	/// screen color
	ColorStruct ScreenColor;

	/// reads GIF extensions
	bool ReadExtension (FILE *file);
	/// reads the color table and bits for each image
	bool ReadImage (FILE *file);
};


#endif // of SP_GIFLIB_H
