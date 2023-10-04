//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - JPEG file display source file
 *  \file       jpeg.cpp
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
#include <unistd.h>
#include <math.h>
#include <setjmp.h>
#include <sys/mman.h>

#include <jpeglib.h>
#include <jerror.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_khwl.h>

#include <settings.h>
#include "jpeg.h"

//#define JPEG_USE_MMAP

const int YUV_BUFFER_MARGIN = 16;
const int EXIF_JPEG_MARKER = JPEG_APP0 + 1;
const char *EXIF_IDENT_STRING = "Exif\000\000";
enum
{
	JPEG_BIG_ENDIAN = 0,
	JPEG_LITTLE_ENDIAN
};

static BYTE *Y = NULL, *UV = NULL;
static int cur_w = 0, cur_h = 0;
static jmp_buf setjmp_buffer;
static FILE *jpegfile = NULL;

const int frame_width = 720, frame_height = 480;

/// \WARNING! Works only on little-endian systems!
static inline WORD jpeg_exif_get16(BYTE *ptr, DWORD endian)
{
	if (endian == JPEG_BIG_ENDIAN)
		return (WORD)((ptr[0] << 8) | ptr[1]);
	return (WORD)((ptr[1] << 8) | ptr[0]);
}

/// \WARNING! Works only on little-endian systems!
static inline DWORD jpeg_exif_get32(BYTE *ptr, DWORD endian)
{
	if (endian == JPEG_BIG_ENDIAN)
		return (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
	return (ptr[3] << 24) | (ptr[2] << 16) | (ptr[1] << 8) | ptr[0];
}


void jpeg_dealloc()
{
	if (Y != NULL)
	{
#ifdef JPEG_USE_MMAP
		munmap(Y - YUV_BUFFER_MARGIN, cur_w * cur_h + YUV_BUFFER_MARGIN);
#else
		SPfree(Y - YUV_BUFFER_MARGIN);
#endif
		Y = NULL;
	}
	if (UV != NULL)
	{
#ifdef JPEG_USE_MMAP
		munmap(UV - YUV_BUFFER_MARGIN, cur_w * cur_h / 2 + YUV_BUFFER_MARGIN);
#else
		SPfree(UV - YUV_BUFFER_MARGIN);
#endif
		UV = NULL;
	}
}

BOOL jpeg_alloc_yuv(int w, int h)
{
	if (w != cur_w || h != cur_h || Y == NULL || UV == NULL)
	{
		jpeg_dealloc();
#ifdef JPEG_USE_MMAP
		Y = (BYTE *)mmap(0, (w * h) + YUV_BUFFER_MARGIN, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
		UV = (BYTE *)mmap(0, (w * h / 2) + YUV_BUFFER_MARGIN, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
#else
		Y = (BYTE *)SPmalloc((w * h) + YUV_BUFFER_MARGIN);
		UV = (BYTE *)SPmalloc((w * h / 2) + YUV_BUFFER_MARGIN);
#endif
		if (Y == NULL || UV == NULL)
			return FALSE;
		Y += YUV_BUFFER_MARGIN;
		UV += YUV_BUFFER_MARGIN;
		cur_w = w;
		cur_h = h;
	}
	return TRUE;
}

BOOL jpeg_init()
{
	return TRUE;
}

BOOL jpeg_deinit()
{
	return TRUE;
}

//jmp_buf jpg_err;
bool was_error = false;

void jpeg_error_exit (j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	// Create the message
	(*cinfo->err->format_message) (cinfo, buffer);
	msg_error("Jpeg error: %s\n", buffer);
	was_error = true;
	longjmp(setjmp_buffer, 1);
}

BOOL jpeg_show (char *filename, int hscale, int vscale, int offsx, int offsy, int *param_rot)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	
	JSAMPARRAY scanline;
	int nb_lines_buffer, nb_lines_read, line_size;
	int i, j, k;

#ifdef WIN32
	if (filename[0] == '/') filename++;
#endif

	bool is_external_img = (memcmp(filename, "/img/", 5) != 0 && memcmp(filename, "img/", 4) != 0);

	int frame_left, frame_top, frame_right, frame_bottom;
	khwl_get_window_frame(&frame_left, &frame_top, &frame_right, &frame_bottom);
	int frame_width, frame_height;
	
	KHWL_WINDOW maxdst;
	khwl_getproperty(KHWL_VIDEO_SET, evMaxDisplayWindow, sizeof(maxdst), &maxdst);
	
	frame_width = ::frame_width;
	frame_height = ::frame_height;
	
	int frame_w = frame_right - frame_left + 1;
	int frame_h = frame_bottom - frame_top + 1;

	bool no_rescale = frame_w == frame_width && frame_h == frame_height, partial_rescale = false;
#if 0
	if (no_rescale && is_external_img)
	{
		khwl_display_clear();
	}
#endif
	
	was_error = false;

	jpegfile = fopen(filename, "rb");
	if (jpegfile == NULL) 
	{
		printf("Can't open source file %s\n", filename);
		return FALSE;
	}

	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = jpeg_error_exit;

	if (setjmp(setjmp_buffer)) 
	{
		fclose(jpegfile);
		return FALSE;
	}

	jpeg_create_decompress(&cinfo);
    jpeg_save_markers(&cinfo, EXIF_JPEG_MARKER, 0xffff);
	jpeg_stdio_src(&cinfo, jpegfile);
	jpeg_read_header(&cinfo, TRUE);

	const WORD orientation_tag_id = 0x112;

	// get Exif data (orientation)
	int rot = param_rot != NULL ? *param_rot : 0;
	for (jpeg_saved_marker_ptr mark = cinfo.marker_list; mark != NULL; mark = mark->next) 
	{
		switch (mark->marker)
		{
		case EXIF_JPEG_MARKER:
			if (mark->data_length < (6+4*3+2+12))	// 1 tag at least...
				break;
			if (memcmp (mark->data, EXIF_IDENT_STRING, 6) != 0) 
				break;

			BYTE *d = mark->data + 6;
			for (DWORD i = 6; i < 16; d++, i++) 
			{
				DWORD endian = 0;
				static const char leth[]  = {0x49, 0x49, 0x2a, 0x00};
				static const char beth[]  = {0x4d, 0x4d, 0x00, 0x2a};

				if (memcmp (d, leth, 4) == 0)
					endian = JPEG_LITTLE_ENDIAN;
				else if (memcmp (d, beth, 4) == 0)
					endian = JPEG_BIG_ENDIAN;
				else 
					continue;
			
				i += jpeg_exif_get32(d + 4, endian);
				d = mark->data + i;
				if ((i + 2) > mark->data_length)
					return 0;
				WORD tags = jpeg_exif_get16(d, endian);
				if ((i + 2 + tags * 12) > mark->data_length)
					return 0;
				for (d += 2; tags--; d += 12)
				{
					WORD tag_id = jpeg_exif_get16(d, endian);
					if (tag_id == orientation_tag_id)
					{ 
						WORD type   = jpeg_exif_get16(d + 2, endian);
						DWORD count  = jpeg_exif_get32(d + 4, endian);
					
						if (type == 3 && count == 1)
						{
							int orientation = jpeg_exif_get16(d + 8, endian);
							if (rot < 0)	// set only if rotation is undefined
							{
								switch (orientation)
								{
								case 1: rot = 0; break;
								case 3: rot = 2; break;
								case 6: rot = 1; break;
								case 8: rot = 3; break;
								}
							}
							break;
						}
					}
				}
				break;
			}
			break;
		}
    }
	if (rot < 0 || rot > 3)
		rot = 0;
	
	cinfo.out_color_space = JCS_YCbCr;
	cinfo.dct_method = JDCT_IFAST;
	cinfo.scale_num = 1;
	cinfo.quantize_colors = FALSE;

	// Set it to 2 to use down-scaling for all big JPEGs (more quality), or set to 1 for default up-scaling.
	int mul = settings_get(SETTING_HQ_JPEG) ? 2 : 1;
	//if (rot == 0) mul = 1;

	int rotated_frame_w = (rot == 0 || rot == 2) ? frame_w : frame_h;
	int rotated_frame_h = (rot == 0 || rot == 2) ? frame_h : frame_w;
	int rotated_frame_width = (rot == 0 || rot == 2) ? frame_width : frame_height;
	int rotated_frame_height = (rot == 0 || rot == 2) ? frame_height : frame_width;

	int scalefac = 1;
	while (scalefac < 32 && (((int)cinfo.image_width / (mul*scalefac)) > rotated_frame_w
			|| ((int)cinfo.image_height / (mul*scalefac)) > rotated_frame_h)) 
	{
		scalefac = scalefac * 2;
	}
	if (scalefac > 8)
	{
		scalefac = 8;
		if ((int)cinfo.image_width / scalefac > rotated_frame_width || 
			(int)cinfo.image_height / scalefac > rotated_frame_height)
		{
			return FALSE;
		}
	}
	
	cinfo.scale_denom = scalefac;

	jpeg_start_decompress(&cinfo);
	
	if (was_error)
		return FALSE;
	
	if ((int)cinfo.output_width > rotated_frame_width * mul || (int)cinfo.output_height > rotated_frame_height * mul
		|| (int)cinfo.output_width < 1 || (int)cinfo.output_height < 1)
	{
		return FALSE;
	}
	if ((int)cinfo.output_width > rotated_frame_width || (int)cinfo.output_height > rotated_frame_height)
	{
		no_rescale = false;
		partial_rescale = true;
	}
	
	int scaled_width  = (int)cinfo.image_width;
	int scaled_height = (int)cinfo.image_height;

	int rescale_width, rescale_height;
	if (!no_rescale)
	{
		if ((int)cinfo.output_width < 3 || (int)cinfo.output_height < 3)
			return FALSE;
		rescale_width = rotated_frame_h * scaled_width / scaled_height;
		rescale_height = rotated_frame_w * scaled_height / scaled_width;
		
		if (rescale_width > rotated_frame_w)
			rescale_width = rotated_frame_w;
		else
			rescale_height = rotated_frame_h;

		while (scaled_width < rescale_width / 5 || scaled_height < rescale_height / 5)
		{
			rescale_width /= 2;
			rescale_height /= 2;
		}
		

		scaled_width = rescale_width;
		scaled_height = rescale_height;
	} else
	{
		rescale_width = cinfo.output_width;
		rescale_height = cinfo.output_height;
	}

	if (!jpeg_alloc_yuv(frame_width, frame_height))
	{
		msg("jpeg: Memory ERROR!\n");
		msg_sysinfo();
		jpeg_dealloc();
		return FALSE;
	}

	line_size = cinfo.output_width * cinfo.output_components;
	nb_lines_buffer = 1;

	scanline = (JSAMPARRAY) SPmalloc(nb_lines_buffer * sizeof(JSAMPROW));
	if (scanline == NULL)
		return FALSE;
	for (i = 0; i < nb_lines_buffer; i++)
	{
		scanline[i] = (JSAMPROW) SPmalloc((line_size + 1) * sizeof(JSAMPLE));
		if (scanline[i] == NULL)
			return FALSE;
	}

	int jmul, imgwidth, imgheight, imglf = 0, imgtp = 0;
	int maxj = rotated_frame_h;

	imgtp = MAX((rotated_frame_w - rescale_width) / 2, 0);
	imglf = MAX((rotated_frame_h - rescale_height) / 2, 0);

	if (partial_rescale)
	{
		imgwidth = (int)cinfo.output_width;
		imgheight = (int)cinfo.output_height;
	} else
	{
		if (rot == 0 || rot == 2)
		{
			imgwidth = MIN((int)cinfo.output_width, frame_width);
			imgheight = MIN((int)cinfo.output_height, frame_height);
		} else
		{
			imgwidth = MIN((int)cinfo.output_width, frame_height);
			imgheight = MIN((int)cinfo.output_height, frame_width);
		}
	}

	if (rot == 0 || rot == 3)
	{
		j = 0;
		jmul = 1;
	}
	else
	{
		j = maxj - 1;
		jmul = -1;
	}

	int jd = 0, oldj = -1, jstep = 1;
	bool read_scan = true;

	for (i = 0; j >= 0 && j < maxj; )
	{
		int imgw = 0;
		if (i >= imglf && i < imgheight + imglf)
		{
			if (read_scan)
			{
				nb_lines_read = jpeg_read_scanlines(&cinfo, scanline, nb_lines_buffer);
				if (nb_lines_read < 1)
					break;
			}
			imgw = imgwidth;
		} 
		if (read_scan)
			i++;

		BYTE *y;
		BYTE *uv;
		JSAMPROW sl = *scanline;

		if (j != oldj)
		{
			if (rot == 0)
			{
				y = Y + j * frame_width;
				uv = UV + j / 2 * frame_width;
				BYTE *scl = sl;
				for (k = 0; k < imgtp; k++) 
				{
    				*y++ = 0;
					if (((j & 1) == 0) && ((k & 1) == 0))
					{
						*uv++ = 128; 
						*uv++ = 128;
					} 
				}
				if (no_rescale)
				{
					for (int kk = 0; kk < imgw; kk++, k++) 
					{
    					*y++ = *scl;
						if (((j & 1) == 0) && ((k & 1) == 0))
						{
							*uv++ = scl[1]; 
							*uv++ = scl[2];
						} 
						scl += 3;
					}
				} else
				{
					int d = 0;
					for (int kk = 0; kk < imgw; k++) 
					{
						*y++ = *scl;
						if (((j & 1) == 0) && ((k & 1) == 0))
						{
							*uv++ = scl[1]; 
							*uv++ = scl[2];
						} 
						d += imgw;
						while (d >= rescale_width)
						{
							scl += 3;
							kk++;
							d -= rescale_width;
						}
					}
				}
				for ( ; k < frame_w; k++) 
				{
    				*y++ = 0;
					if (((j & 1) == 0) && ((k & 1) == 0))
					{
						*uv++ = 128; 
						*uv++ = 128;
					} 
				}
			} 
			else if (rot == 1)
			{
				y = Y + j;
				uv = UV + j;
				for (k = 0; k < imgtp; k++) 
				{
    				*y = 0;
					y += frame_width;
					if (((j & 1) == 0) && ((k & 1) == 0))
					{
						*uv = 128; 
						*(uv+1) = 128;
						uv += frame_width;
					} 
				}
				if (no_rescale)
				{
					for (int kk = 0; kk < imgw; kk++, k++) 
					{
    					BYTE *scl = sl + kk * 3;
    					*y = *scl;
						y += frame_width;
						if (((j & 1) == 0) && ((k & 1) == 0))
						{
							*uv = scl[1]; 
							*(uv+1) = scl[2];
							uv += frame_width;
						} 
					}
				} else
				{
					BYTE *scl = sl;
					int d = 0;
					for (int kk = 0; kk < imgw; k++) 
					{
						*y = *scl;
						y += frame_width;
						if (((j & 1) == 0) && ((k & 1) == 0))
						{
							*uv = scl[1]; 
							*(uv+1) = scl[2];
							uv += frame_width;
						}
						d += imgw;
						while (d >= rescale_width)
						{
							scl += 3;
							kk++;
							d -= rescale_width;
						}
					}
				}
				for ( ; k < frame_h; k++) 
				{
    				*y = 0;
					y += frame_width;
					if (((j & 1) == 0) && ((k & 1) == 0))
					{
						*uv = 128; 
						*(uv+1) = 128;
						uv += frame_width;
					} 
				}
			}
			else if (rot == 2)
			{
				y = Y + (j) * frame_width + frame_w;
				uv = UV + (j / 2) * frame_width + frame_w;

				k = frame_w;

				for ( ; k > frame_w - imgtp; ) 
				{
					k--;
    				*--y = 0;
					if (((j & 1) == 0) && ((k & 1) == 0))
					{
						*--uv = 128; 
						*--uv = 128;
					} 
				}
				
				if (no_rescale)
				{
					BYTE *scl = sl;
					for (int kk = 0; kk < imgw; kk++ ) 
					{
						k--;
    					*--y = *scl;
						if (((j & 1) == 0) && ((k & 1) == 0))
						{
							*--uv = scl[2];
							*--uv = scl[1]; 
						} 
						scl += 3;
					}
				} else
				{
					BYTE *scl = sl;
					int d = 0;
					for (int kk = 0; kk < imgw; ) 
					{
						*--y = *scl;
						k--;
						if (((j & 1) == 0) && ((k & 1) == 0))
						{
							*--uv = scl[2];
							*--uv = scl[1]; 
						} 
						d += imgw;
						while (d >= rescale_width)
						{
							scl += 3;
							kk++;
							d -= rescale_width;
						}
					}
				}

				for (; k > 0; k--) 
				{
    				*--y = 0;
					if (((j & 1) == 0) && ((k & 1) == 0))
					{
						*--uv = 128; 
						*--uv = 128;
					} 
				}

			} 
			else if (rot == 3)
			{
				y = Y + j;
				uv = UV + j;
				int iw = (int)cinfo.output_width;
				for (k = frame_h; k > frame_h - imgtp; ) 
				{
    				*y = 0;
					y += frame_width;
					k--;
					if (((j & 1) == 0) && ((k & 1) == 0))
					{
						*uv = 128; 
						*(uv+1) = 128;
						uv += frame_width;
					} 
				}
				if (no_rescale)
				{
					BYTE *scl = sl + (iw - 1) * 3;
					for (int kk = iw - 1; kk >= (iw - imgw); kk--) 
					{
    					*y = *scl;
						y += frame_width;
						k--;
						if (((j & 1) == 0) && ((k & 1) == 0))
						{
							*uv = scl[1]; 
							*(uv+1) = scl[2];
							uv += frame_width;
						} 
						scl -= 3;
					}
				} else
				{
					BYTE *scl = sl + (iw - 1) * 3;
					int d = 0;
					for (int kk = iw - 1; kk >= (iw - imgw); ) 
					{
						*y = *scl;
						y += frame_width;
						k--;
						if (((j & 1) == 0) && ((k & 1) == 0))
						{
							*uv = scl[1]; 
							*(uv+1) = scl[2];
							uv += frame_width;
						} 
						d += imgw;
						while (d >= rescale_width)
						{
							scl -= 3;
							kk--;
							d -= rescale_width;
						}
					}
				}
				for (; k > 0; ) 
				{
    				*y = 0;
					y += frame_width;
					k--;
					if (((j & 1) == 0) && ((k & 1) == 0))
					{
						*uv = 128; 
						*(uv+1) = 128;
						uv += frame_width;
					} 
				}
			}
			oldj = j;
		}

		if (!no_rescale)
		{
			if (imgw == 0)
				jstep = 1;
			else
			{
				jstep--;
				read_scan = false;
				if (jstep <= 0)
				{
					jstep = 0;
					jd += rescale_height;
					while (jd >= imgheight)
					{
						jd -= imgheight;
						jstep++;
					}
				}
				if (jstep <= 1)
					read_scan = true;
			}
		} else
		{
			// if no rescale, then we use hardware scaling
			//if (i >= imgheight)
			//	break;
		}

		if (jstep > 0)
		{
			j += jmul;
		}
	}

	for (i = 0; i < nb_lines_buffer; i++)
		SPfree(scanline[i]);
	SPfree(scanline);

	KHWL_YUV_FRAME f;

	if (no_rescale || partial_rescale)
	{
		int scaleto_width, scaleto_height;
		if (partial_rescale)
		{
			scaleto_width = frame_w;
			scaleto_height = frame_h;
		} else
		{
			scaleto_width = MAX((int)cinfo.output_width, frame_width / 5);
			scaleto_height = MAX((int)cinfo.output_height, frame_height / 4);
#if 0
			if (scaleto_width > scaleto_height)
				scaleto_height = scaleto_width * frame_height / frame_width;
			else
				scaleto_width = scaleto_height * frame_width / frame_height;
#endif
			if (rot == 1 || rot == 3)
				Swap(scaleto_width, scaleto_height);

		}

		KHWL_VIDEOMODE vmode = KHWL_VIDEOMODE_NORMAL;
		// for external images, use aspect-ratio correction
		if (is_external_img)
		{
			int tvtype = settings_get(SETTING_TVTYPE);
			if (tvtype == 2 || tvtype == 3)
				vmode = KHWL_VIDEOMODE_WIDE;
		} else
			vmode = KHWL_VIDEOMODE_NONE;
		khwl_setvideomode(vmode, FALSE);
		khwl_set_window_zoom(KHWL_ZOOMMODE_YUV);
		khwl_set_window(scaleto_width, scaleto_height,
			frame_width, frame_height, hscale, vscale, offsx, offsy);
	}

	int maxi = frame_width * (frame_bottom - frame_top + 1);
	int chunk_n = (frame_right - frame_left + 1);
	int chunk_width = frame_width;
	if (no_rescale)
	{
		chunk_n *= 32;
		chunk_width *= 32;
	}
	
	int offs = frame_width * frame_top + frame_left;
	int offs2 = frame_width / 2 * frame_top + frame_left;
	int maxi2 = maxi / 2;

	for (i = 0; i < maxi; i += chunk_width)
	{
		f.y_buf = Y + i;
		f.y_offs = offs + i;
		f.y_num = maxi - i < chunk_n ? maxi - i : chunk_n;
		f.uv_buf = NULL;
		f.uv_offs = 0;
		f.uv_num = 0;
		khwl_displayYUV(&f);

		if (i < maxi2)
		{
			f.y_buf = NULL;
			f.y_offs = 0;
			f.y_num = 0;
			f.uv_buf = UV + i;
			f.uv_offs = offs2 + i;
			f.uv_num = maxi2 - i < chunk_n ? maxi2 - i : chunk_n;
			khwl_displayYUV(&f);
		}
		
	}

#ifdef WIN32
	khwl_osd_update();
#endif

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	fclose(jpegfile);
	
	fflush(stdout);

	jpeg_dealloc();

	if (param_rot != NULL)
		*param_rot = rot;

	return TRUE;
}
