//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - Bitstream reader source file.
 *  \file       bitstream.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       1.04.2007
 *
 * Portions of code taken from libavcodec.
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

#include <bitstream.h>

WORD *dec_buf[MAX_BITSTREAM_BUFS], *dec_b[MAX_BITSTREAM_BUFS];
int dec_len[MAX_BITSTREAM_BUFS] = { 0 }, dec_next_bitidx_shift[MAX_BITSTREAM_BUFS] = { 0 };
bool dec_eof[MAX_BITSTREAM_BUFS] = { false };

static DWORD *enc_buf = NULL;
BOOL enc_eof = FALSE;
DWORD *enc_b = NULL;
int enc_maxlen = 0;

bitstream_callback callback_func[MAX_BITSTREAM_BUFS] = { NULL };

void bitstream_set_callback(int N, bitstream_callback c)
{
	callback_func[N] = c;
}

int bitstream_decode_buf_init(int N, BYTE *buf, int len, BOOL read_more)
{
	int bytes = (DWORD)buf & 1;
	WORD *b = (WORD *)((DWORD)buf & (~1));
	dec_b[N] = dec_buf[N] = b;

	dec_next_bitidx_shift[N] = bytes * 8;
	dec_len[N] = len + bytes;
	dec_eof[N] = !read_more;

	return 0;
}

int bitstream_decode_next(int N, int &dec_left)
{
	if (callback_func[N](BITSTREAM_MODE_INPUT, NULL, 0) < 0)
	{
		dec_eof[N] = true;
		return -1;
	}
	dec_left = dec_len[N];
	return 0;
}


int bitstream_decode_eof(int N, DWORD &dec_v, int &dec_bitidx, int &dec_left)
{
	if (dec_eof[N]) 
		return -1;
	register DWORD next_b;

	if (dec_left == 0)
	{
		if (bitstream_decode_next(N, dec_left) < 0)
			return -1;
		next_b = *dec_b[N]++;

		if (dec_next_bitidx_shift[N] == 0)
		{
			// v = cc dd XX YY
			// b2 = 34 12 | 78 56
			dec_v |= BWSWAP(next_b) << (31 - 16 - dec_bitidx);
			dec_bitidx += 16;
			dec_left -= 2;

		} else // dec_next_bitidx_shift == 8
		{
			// v = cc dd XX YY
			// b2 = 12 cd | 56 34

			if (dec_bitidx < 8)
			{
				dec_v |= (next_b >> 8) << (31 - 8 - dec_bitidx);
				next_b = *dec_b[N]++;
				dec_v |= BWSWAP(next_b) << (31 - 24 - dec_bitidx);
				dec_bitidx += 24;
				dec_left -= 4;
			}
			else // dec_bitidx >= 8
			{
				dec_v |= (next_b >> 8) << (31 - 8 - dec_bitidx);
				dec_bitidx += 8;
				dec_left -= 2;
			}
		}
	} else	// dec_left == 1
	{
		next_b = *dec_b[N];
		if (bitstream_decode_next(N, dec_left) < 0)
			return -1;

		if (dec_next_bitidx_shift[N] == 0)
		{
			// v = cc dd XX YY
			// b1 = cd 12
			// b2 = 56 34 | 9a 78

			if (dec_bitidx < 8)
			{
				dec_v |= (next_b & 0xff) << (31 - 8 - dec_bitidx);
				next_b = *dec_b[N]++;
				dec_v |= BWSWAP(next_b) << (31 - 24 - dec_bitidx);
				dec_bitidx += 24;
				dec_left -= 2;
			}
			else  // dec_bitidx >= 8
			{
				dec_v |= (next_b & 0xff) << (31 - 8 - dec_bitidx);
				dec_bitidx += 8;
			}
		}
		else // dec_next_bitidx_shift == 8
		{
			// v = cc dd XX YY
			// b1 = cd 12
			// b2 = 34 cd | 78 56

			dec_v |= (next_b & 0xff) << (31 - 8 - dec_bitidx);
			next_b = *dec_b[N]++;
			dec_v |= (next_b >> 8) << (31 - 16 - dec_bitidx);
			dec_bitidx += 16;
			dec_left -= 2;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////

int bitstream_encode_buf_init(BYTE *buf, int len)
{
	int bytes = (4 - ((DWORD)buf & 3)) & 3;
	enc_b = enc_buf = (DWORD *)(((DWORD)buf + 3) & (~3));
	enc_maxlen = (len & (~3)) - 4 - bytes;
	
	if (enc_maxlen < 1)
		return -1;
	
	enc_eof = false;
	return 0;
}


int bitstream_flush_output_callback(BITSTREAM_MODE mode, int enc_len)
{
	// [0] ???
	if (callback_func[0](mode, (BYTE *)enc_buf, enc_len) < 0)
	{
		enc_eof = true;
		return -1;
	}
	return 0;
}

