//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - Bitstream reader header file
 *  \file       bitstream.h
 *  \author     bombur
 *  \version    0.1
 *  \date       1.04.2007
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

#ifndef SP_BITSTREAM_H
#define SP_BITSTREAM_H

#define MAX_BITSTREAM_BUFS 5

#define INLINE SP_INLINE
#define STATIC static

typedef enum
{
	BITSTREAM_MODE_DONE = 0,
	BITSTREAM_MODE_INPUT = 1,
	BITSTREAM_MODE_OUTPUT = 2,
} BITSTREAM_MODE;

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*bitstream_callback)(BITSTREAM_MODE status, BYTE *outbuf, int outlen);

void bitstream_set_callback(int N, bitstream_callback);

int bitstream_decode_buf_init(int N, BYTE *buf, int len, BOOL read_more);
//int bitstream_decode_eof(int N, DWORD &dec_v, int &dec_bitidx, int &dec_left);

int bitstream_encode_buf_init(BYTE *buf, int len);
int bitstream_flush_output_callback(BITSTREAM_MODE mode, int enc_len);

/////////////////////////////////////////////////////

extern WORD *dec_b[MAX_BITSTREAM_BUFS];
extern int dec_len[MAX_BITSTREAM_BUFS], dec_next_bitidx_shift[MAX_BITSTREAM_BUFS];
extern DWORD *enc_b;
extern int enc_maxlen;
extern BOOL enc_eof;

#ifdef __cplusplus
}
#endif

#define bitstream_show_bits(ret, dec_v, dec_bitidx, n) \
{\
	ret = dec_v >> (32 - n);\
}

#define bitstream_skip_bits(N, dec_v, dec_bitidx, dec_left, n) \
{ \
	dec_bitidx -= (n); \
	dec_v <<= (n); \
	if (dec_bitidx < 16) \
	{ \
		register DWORD next_b = *dec_b[N]++; \
		dec_v |= BWSWAP(next_b) << (31 - 16 - dec_bitidx); \
		dec_bitidx += 16; \
		dec_left -= 2; \
	} \
}

#define bitstream_get_bits(ret, N, dec_v, dec_bitidx, dec_left, n) \
{ \
	bitstream_show_bits(ret, dec_v, dec_bitidx, n); \
	bitstream_skip_bits(N, dec_v, dec_bitidx, dec_left, n); \
}

#define bitstream_decode_start(N, dec_v, dec_bitidx, dec_bitleft) \
{ \
	register DWORD v2; \
	dec_v = *dec_b[N]++; \
	v2 = *dec_b[N]++; \
	dec_v = (BWSWAP(dec_v) << 16) | BWSWAP(v2); \
	dec_bitidx = 31 - dec_next_bitidx_shift[N]; \
	dec_v <<= dec_next_bitidx_shift[N]; \
	dec_bitleft = dec_len[N] - 4; \
}

#define bitstream_get_bits1(ret, N, dec_v, dec_bitidx, dec_left) \
{ \
	bitstream_show_bits(ret, dec_v, dec_bitidx, 1); \
	bitstream_skip_bits(N, dec_v, dec_bitidx, dec_left, 1); \
}

#define bitstream_get_bits012(ret, N, dec_v, dec_bitidx, dec_left) \
{ \
	const DWORD rets[4] = { /*00b*/0, /*01b*/0, /*10b*/1, /*11b*/2 }; \
	const DWORD skips[4] = { 1, 1, 2, 2 }; \
	bitstream_show_bits(ret, dec_v, dec_bitidx, 2); \
	bitstream_skip_bits(N, dec_v, dec_bitidx, dec_left, skips[ret]); \
	ret = rets[ret]; \
}

/////////////////////////////////////////////////////////////////////////
#define bitstream_flush_output(mode, enc_v, enc_bitidx, enc_len) \
{ \
	DWORD enc_tmp = enc_v; \
	register int shift; \
	if (!enc_eof) \
	{ \
		BSWAP(enc_tmp); \
		*enc_b++ = enc_tmp; \
		for (shift = 24; enc_bitidx < 31; shift -= 8, enc_bitidx += 8) \
		{ \
			enc_len++; \
		} \
	} \
	\
	if (bitstream_flush_output_callback(mode, enc_len) < 0) \
	{ \
		enc_bitidx -= 32; \
		enc_len -= 4; \
		return -1; \
	} \
}

#define bitstream_encode_next(enc_v, enc_bitidx, enc_len) \
{ \
	enc_bitidx += 32; \
	enc_len += 4; \
	if (enc_len >= enc_maxlen) \
	{ \
		bitstream_flush_output(BITSTREAM_MODE_OUTPUT, enc_v, enc_bitidx, enc_len); \
		enc_len = 0; \
		enc_v = 0; \
		enc_bitidx = 31; \
	} \
	else \
	{ \
		BSWAP(enc_v); \
		*enc_b++ = enc_v; \
		enc_v = 0; \
	} \
}

#if 1
#define bitstream_put_bits(n, data, enc_v, enc_bitidx, enc_len) \
{ \
	register int shift = enc_bitidx + 1 - (n); \
	if (shift >= 0) \
	{ \
		enc_v |= (data) << shift; \
		enc_bitidx -= (n); \
		if (enc_bitidx < 0) \
			bitstream_encode_next(enc_v, enc_bitidx, enc_len); \
	} else \
	{ \
		shift = -shift; \
		enc_v |= (data) >> shift; \
		enc_bitidx -= (n) - shift; \
		if (enc_bitidx < 0) \
			bitstream_encode_next(enc_v, enc_bitidx, enc_len); \
		enc_v |= (data) << (32 - shift); \
		enc_bitidx -= shift; \
		if (enc_bitidx < 0) \
			bitstream_encode_next(enc_v, enc_bitidx, enc_len); \
	} \
}
#else
int FORCEINLINE bitstream_put_bits(int n, DWORD data, DWORD &enc_v, int &enc_bitidx, int &enc_len)
{
	register int shift = enc_bitidx + 1 - (n);
	if (shift >= 0)
	{ 
		enc_v |= (data) << shift; 
		enc_bitidx -= (n); 
		if (enc_bitidx < 0) 
		{ 
			if (bitstream_encode_next(enc_v, enc_bitidx, enc_len) < 0) 
				return -1; 
		} 
	} else 
	{ 
		shift = -shift; 
		enc_v |= (data) >> shift; 
		enc_bitidx -= (n) - shift; 
		if (enc_bitidx < 0) 
		{ 
			if (bitstream_encode_next(enc_v, enc_bitidx, enc_len) < 0) 
				return -1; 
		} 
		enc_v |= (data) << (32 - shift); 
		enc_bitidx -= shift; 
		if (enc_bitidx < 0) 
		{ 
			if (bitstream_encode_next(enc_v, enc_bitidx, enc_len) < 0) 
				return -1; 
		} 
	} 
	return 0;
}
#endif


#endif // of SP_BITSTREAM_H
