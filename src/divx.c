//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - DivX3->MPEG-4 transcoder source file.
 *  \file       divx.cpp
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>

#define DIVX_INTERNAL
#include "bitstream.h"
#include "divx.h"
#include "divx-tables.h"

//#define USE_SLOW_METHOD_LIMIT

// UNCOMMENT ALL!!!
#define USE_SLOW_METHOD
//#define USE_AC_CORRECTION
//#define USE_ONLY_SLOW_METHOD
//#define DUMP

//!!!!!!!!!!!!!!!!!!!!
//static int CNT = 0;

#ifdef DUMP
#define DEBUG_MSG msg
//#define DEBUG_MSG printf
#endif


#define INLINE SP_INLINE
#define STATIC static
//#define INLINE 
//#define STATIC

//////////////////////////////////////////////////////////////////

#ifdef USE_SLOW_METHOD_LIMIT
const int max_allowed_frame_size_for_correct_method = 3750*4;
#endif

int frame_number;

static int width, height;
static DWORD pict_type;

static DWORD qscale, chroma_qscale;
static int enc_y_dc_scale, enc_c_dc_scale;
static BYTE *enc_y_dc_scale_table, *enc_c_dc_scale_table;
static int dec_y_dc_scale, dec_c_dc_scale;
static BYTE *dec_y_dc_scale_table, *dec_c_dc_scale_table;
static BYTE *chroma_qscale_table;

#ifdef USE_AC_CORRECTION
static int *ac_val_base, *ac_val;
static int ac_size;
static int qmul, qadd;
static DWORD use_acpred;
#endif

static int mb_width, mb_height, mb_num;
static int slice_height;

static int use_skip_mb_code;
static int f_code;
static int rl_table_index;
static int rl_chroma_table_index;
static int dc_table_index;
static int mv_table_index;
static int no_rounding;
static BOOL flipflop_rounding;

static int b8_stride, mb_stride;
static int block_index[6], (*block_index_tbl)[6];
static BYTE *coded_block_base, *coded_block;
static signed short (*motion_val_base)[2], (*motion_val)[2];

static BOOL mb_intra;
static int mb_x, mb_y;
static BOOL mb_skip;
static int first_slice_line;
static int motion_x, motion_y;
static int ac_pred;
static int block_wrap[6];
static signed short *enc_dc_val_base, *enc_dc_val[3];
static signed short *dec_dc_val_base, *dec_dc_val[3];
static signed short old_enc_dc_val, old_dec_dc_val;

static DWORD block_flags[64], block_flag;
//static int out_block[64];

#ifdef DUMP
DWORD block_idx[64], block_run[64];
#endif

static DIVX_SCAN_TABLE inter_scantable, intra_scantable;
static DIVX_SCAN_TABLE intra_h_scantable, intra_v_scantable;

static LONGLONG pts;
static int last_time_div, time_incr_res;

static DIVX_VLC mb_intra_vlc;
static DIVX_VLC mb_non_intra_vlc;
static DIVX_VLC dc_lum_vlc[2];
static DIVX_VLC dc_chroma_vlc[2];
static BYTE uni_DCtab_lum_len[512];
static BYTE uni_DCtab_chrom_len[512];
static WORD uni_DCtab_lum_bits[512];
static WORD uni_DCtab_chrom_bits[512];

static DIVX_BITS_LEN *bits_len_tab_cur[3], *bits_len[9];
static DWORD *bits_len_tab2_cur[3], *bits_len2[9];

static DWORD *level_run_tab_cur[3], *level_run[6];
static DWORD *level_run_tab2_cur[3], *level_run2[6];

static DWORD *uni_table[2];

/*
#include <stdarg.h>
void DUMP_FRAME(char *text, ...)
{
	if (frame_number == 772)
	{
		static FILE *fp = NULL;
		if (fp == NULL)
			fp = fopen("out.log", "wb");
		va_list args;
		va_start(args, text);
		vfprintf(fp, text, args);
		va_end(args);
		//fclose(fp);
	}
}
*/
//#include <libsp/sp_fip.h>
//#define DUMP_FRAME gui_update();while(fip_read_button(TRUE) != FIP_KEY_ENTER);msg


////////////////////////////////////////////////////

STATIC void divx_init_scantable(DIVX_SCAN_TABLE *st, const BYTE *src_scantable)
{
    DWORD i;
    int end;
    st->scantable = src_scantable;

    for (i = 0; i < 64; i++)
	{
        st->permutated[i] = src_scantable[i];
		st->inv_permutated[src_scantable[i]] = (BYTE)i;
    }
    
    end = -1;
    for (i = 0; i < 64; i++)
	{
        int j = st->permutated[i];
        if (j > end) 
			end = j;
        st->raster_end[i] = (BYTE)end;
    }
}

static int divx_alloc_vlc_table(DIVX_VLC *vlc, int size)
{
    int index;
    index = vlc->table_size;
    vlc->table_size += size;
    if (vlc->table_size > vlc->table_allocated) 
	{
        vlc->table_allocated += (1 << vlc->bits);
        vlc->table = (signed short (*)[2])SPrealloc(vlc->table,
                                sizeof(signed short) * 2 * vlc->table_allocated);
        if (!vlc->table)
            return -1;
    }
    return index;
}

STATIC INLINE DWORD divx_get_vlc_data(const void *table, int i, int wrap, int size)
{
    const BYTE *ptr = (const BYTE *)table + i * wrap;
    switch(size) 
	{
    case 1:
        return *(const BYTE *)ptr;
    case 2:
        return *(const WORD *)ptr;
    default:
        return *(const DWORD *)ptr;
    }
}

static int divx_build_vlc_table(DIVX_VLC *vlc, int table_nb_bits,
                       int nb_codes,
                       const void *bits, int bits_wrap, int bits_size,
                       const void *codes, int codes_wrap, int codes_size,
                       DWORD code_prefix, int n_prefix)
{
    int i, j, k, n, table_size, table_index, nb, n1, index;
    DWORD code;
    signed short (*table)[2];

    table_size = 1 << table_nb_bits;
    table_index = divx_alloc_vlc_table(vlc, table_size);
    if (table_index < 0)
        return -1;
    table = &vlc->table[table_index];

    for (i = 0; i < table_size; i++) 
	{
        table[i][1] = 0; //bits
        table[i][0] = -1; //codes
    }

    // first pass: map codes and compute auxillary table sizes
    for (i = 0; i < nb_codes; i++) 
	{
        n = divx_get_vlc_data(bits, i, bits_wrap, bits_size);
        code = divx_get_vlc_data(codes, i, codes_wrap, codes_size);
        // we accept tables with holes
        if (n <= 0)
            continue;
        // if code matches the prefix, it is in the table
        n -= n_prefix;
        if (n > 0 && (code >> n) == code_prefix) 
		{
            if (n <= table_nb_bits) 
			{
                // no need to add another table
                j = (code << (table_nb_bits - n)) & (table_size - 1);
                nb = 1 << (table_nb_bits - n);
                for (k = 0; k < nb; k++) 
				{
                    if (table[j][1] != 0)  // bits
					{
                        return -1;
                    }
                    table[j][1] = (signed short)n; //bits
                    table[j][0] = (signed short)i; //code
                    j++;
                }
            } else 
			{
                n -= table_nb_bits;
                j = (code >> n) & ((1 << table_nb_bits) - 1);
                // compute table size
                n1 = -table[j][1]; //bits
                if (n > n1)
                    n1 = n;
                table[j][1] = (signed short)-n1; //bits
            }
        }
    }

    // second pass : fill auxiliary tables recursively
    for (i = 0; i < table_size; i++) 
	{
        n = table[i][1]; //bits
        if (n < 0) 
		{
            n = -n;
            if (n > table_nb_bits) 
			{
                n = table_nb_bits;
                table[i][1] = (signed short)-n; //bits
            }
            index = divx_build_vlc_table(vlc, n, nb_codes,
                                bits, bits_wrap, bits_size,
                                codes, codes_wrap, codes_size,
                                (code_prefix << table_nb_bits) | i,
                                n_prefix + table_nb_bits);
            if (index < 0)
                return -1;
            // note: realloc has been done, so reload tables
            table = &vlc->table[table_index];
            table[i][0] = (signed short)index; //code
        }
    }
    return table_index;
}

static int divx_init_vlc(DIVX_VLC *vlc, int nb_bits, int nb_codes,
             const void *bits, int bits_wrap, int bits_size,
             const void *codes, int codes_wrap, int codes_size)
{
    vlc->bits = nb_bits;
    vlc->table = NULL;
    vlc->table_allocated = 0;
    vlc->table_size = 0;

    if (divx_build_vlc_table(vlc, nb_bits, nb_codes,
                    bits, bits_wrap, bits_size,
                    codes, codes_wrap, codes_size, 0, 0) < 0) 
	{
        SPSafeFree(vlc->table);
        return -1;
    }
    return 0;
}


static int divx_init_rl(DIVX_RL_TABLE *rl)
{
    signed char max_level[DIVX_MAX_RUN + 1], max_run[DIVX_MAX_LEVEL + 1];
    BYTE index_run[DIVX_MAX_RUN + 1];
    int last, run, level, start, end, i;

    // compute max_level[], max_run[] and index_run[]
    for (last = 0; last < 2; last++) 
	{
        if (last == 0) 
		{
            start = 0;
            end = rl->last;
        } else 
		{
            start = rl->last;
            end = rl->n;
        }

        memset(max_level, 0, DIVX_MAX_RUN + 1);
        memset(max_run, 0, DIVX_MAX_LEVEL + 1);
        memset(index_run, rl->n, DIVX_MAX_RUN + 1);
        for (i = start; i < end; i++) 
		{
            run = rl->table_run[i];
            level = rl->table_level[i];
            if (index_run[run] == rl->n)
                index_run[run] = (BYTE)i;
            if (level > max_level[run])
                max_level[run] = (signed char)level;
            if (run > max_run[level])
                max_run[level] = (signed char)run;
        }
        rl->max_level[last] = (signed char *)SPmalloc(DIVX_MAX_RUN + 1);
		if (rl->max_level[last] == NULL)
			return -1;
        memcpy(rl->max_level[last], max_level, DIVX_MAX_RUN + 1);
        
		rl->max_run[last] = (signed char *)SPmalloc(DIVX_MAX_LEVEL + 1);
		if (rl->max_run[last] == NULL)
			return -1;
        memcpy(rl->max_run[last], max_run, DIVX_MAX_LEVEL + 1);
        
		rl->index_run[last] = (BYTE *)SPmalloc(DIVX_MAX_RUN + 1);
		if (rl->index_run[last] == NULL)
			return -1;
        memcpy(rl->index_run[last], index_run, DIVX_MAX_RUN + 1);
    }

	return 0;
}

static void divx_free_rl(DIVX_RL_TABLE *rl)
{
	SPSafeFree(rl->max_level[0]);
	SPSafeFree(rl->max_level[1]);
	SPSafeFree(rl->max_run[0]);
	SPSafeFree(rl->max_run[1]);
	SPSafeFree(rl->index_run[0]);
	SPSafeFree(rl->index_run[1]);
}

static int divx_build_ac_tables()
{
	DWORD i, k, data_mask[16];
	for (i = 0; i < 16; i++)
		data_mask[i] = ((1 << i) - 1) << (16 - i);
	for (i = 0; i < 9; i++)
	{
		DIVX_BITS_LEN *bits_len_tab = bits_len[i];
		DWORD *bits_len_tab2 = bits_len2[i];
		DWORD *uni_tbl = uni_table[i >= 6 ? 1 : 0];
		DWORD I = (i >= 6) ? i - 3 : i;
		DWORD *level_run_tab = level_run[I];
		DWORD *level_run_tab2 = level_run2[I];

		DWORD ext_idx = 0;
		DWORD rt_idx = 4, j;

		int max_level[64*2], max_run[64*2];
		int num = rl_table[I].n, rl_last = rl_table[I].last;

		DWORD ext_next_k = 0, rt_next_k = 0;
		DWORD ext_cur_j = 0;
		BOOL ext_next_esc4 = FALSE;
		int idx;
		//const WORD (*vlc)[2] = rl_table[I].table_vlc;

		memset(max_level, 0, 64 * 2 * sizeof(int));
		memset(max_run, 0, 64 * 2 * sizeof(int));

		for (idx = 0; idx < num; idx++)
		{
			// no need to add another table
			int lev = rl_table[I].table_level[idx];
			int run = rl_table[I].table_run[idx];
			int last = (idx >= rl_last) ? 64 : 0;
			
			if (max_level[last + run] < lev)
				max_level[last + run] = lev;
			if (max_run[last + lev] < run)
				max_run[last + lev] = run;
/*
			DWORD nb = (15 - vlc[idx][1]);
			DWORD j = vlc[idx][0] << (nb + 1);
			for (int sign = 0; sign <= 1; sign++)
			{
				DWORD jj = j | (sign << nb);
				for (int k = 1 << nb; k > 0; k--, jj++) 
				{
					bits_len_tab[jj>>2].bits = (WORD)idx;
				}
			}
*/
		}

		for (j = 0; j < (1<<14); j++)
		{
/*
			short idx0 = bits_len_tab[j].bits;
*/
			for (k = 0; k < 4; k++)
			{
				DWORD data = (j << 2) | k;

				int idx, left = 16, len = 0;
				BOOL found = FALSE, use_esc4 = FALSE, use_1 = FALSE;
				DWORD total_num = 0, total_inlen = 0, total_outlen = 0, total_last = 0;
				int first_level = 0;
				DWORD first_len = 0, first_outlen = 0, first_run = 0, first_last = 0, first_data = 0;
				DWORD out_data = 0, esc;
				DWORD lrt;
/*
				if (idx0 >= 0)
				{
					idx = idx0;
					goto found;
				}
*/
				do
				{
					found = FALSE;
					len = 0;
					for (idx = 0; idx < num; idx++)
					{
/*
found:
*/
						const WORD *vlc = rl_table[I].table_vlc[idx];
						len = vlc[1];
						if (len + 1 > left)
							continue;
						if ((data & data_mask[len]) == (((DWORD)vlc[0]) << (16 - len)))
						{
							int level = rl_table[I].table_level[idx];
							int run = rl_table[I].table_run[idx];
							int last = (idx >= rl_table[I].last) ? 1 : 0;
							register int index, outlen;

							len++;

							if (data & (1 << (16 - len)))
								level = -level;

							index = last*128*64 + run*128 + level + 64;
							outlen = uni_tbl[index] >> 24;

							left -= len;
							data <<= len;
							total_inlen += len;

							total_outlen += outlen;
							out_data <<= outlen;
							out_data |= uni_tbl[index] & 0xffffff;

							total_last = last;

							if (first_len == 0) 
							{
								first_len = len;
								first_outlen = outlen;
								first_level = level;
								first_run = run;
								first_data = out_data;
								first_last = last;
							}

							total_num++;

							if (outlen > 22)
							{
								use_esc4 = TRUE;
								break;
							}

							if (total_num > 0 && (total_outlen > 15 || total_inlen > 14))
							{
								use_1 = TRUE;
								break;
							}
							
							if (last)
								break;

							
							found = TRUE;
							break;
						}
					}
				}
				while (found && total_outlen < 22);

				esc = 0;
				if (!use_esc4 && total_num == 0)	// search ESC codes
				{
					const WORD *vlc = rl_table[I].table_vlc[num];
					int len = vlc[1];
					if ((data & data_mask[len]) == (((DWORD)vlc[0]) << (16 - len)))
					{
						int esc_shift, esc123;
						
						len++;
						esc_shift = 15 - len;
						esc123 = (data >> esc_shift) & 3;

						if (esc123 == 0)
						{
							esc = 3;
							len++;
						}
						else if (esc123 == 1)
						{
							esc = 2;
							len++;
						}
						else
							esc = 1;
						first_len = len;
					}
				}

				// truncate too long seqs
				if (use_1 || (total_num == 2 && total_outlen > 15 && first_outlen <= 15))
				{
					total_outlen = first_outlen;
					total_inlen = first_len;
					total_num = 1;
					total_last = first_last;
					out_data = first_data;
				}

				if (i < 6 && !esc)
				{
					DWORD add_level = max_level[first_last * 64 + (first_run & 63)];
					DWORD add_run = max_run[first_last * 64 + Abs(first_level)];

					lrt = ((DWORD)first_level & 127) << 25;
					lrt |= ((first_len - 1) & 0xf) << 18;
					lrt |= (first_last & 1) << 17;
					lrt |= (add_level & 31) << 12;
					lrt |= (add_run & 63) << 6;
					lrt |= (first_run & 63);
					
					if (first_len > 14)
					{
						if (k != rt_next_k)
							rt_idx = (((rt_idx >> 2) + 1) << 2) | k;

						level_run_tab2[rt_idx] = lrt;
						level_run_tab[j] = (rt_idx & ~3);
						rt_idx++;
						rt_next_k = (k + 1) & 3;
					}
					else
						level_run_tab[j] = lrt;
				}

				// esc1-4
				if (esc)
				{
					bits_len_tab[j].len = (BYTE)esc;
					bits_len_tab[j].bits = (WORD)first_len;
					level_run_tab[j] = 0;
				}
				// extended
				else if (total_inlen > 14 || total_outlen > 15 || use_esc4)
				{
					// if one of four extended records has ESC4, then all others should too

					if (ext_cur_j == j)
					{
						if (!use_esc4 && ext_next_esc4)
							use_esc4 = TRUE;
					}
					
					if (k != ext_next_k)
						ext_idx = (((ext_idx >> 2) + 1) << 2) | k;
					bits_len_tab2[ext_idx] = ((DWORD)total_last << 31) | ((out_data & 0x3FFFFF) << 9)
						| ((total_inlen - 1) << 5) | total_outlen;
					bits_len_tab[j].len = use_esc4 ? (BYTE)4 : (BYTE)0;
					bits_len_tab[j].bits = (WORD)(ext_idx & ~3);
					ext_idx++;
					ext_next_k = (k + 1) & 3;
					
					ext_next_esc4 = use_esc4;
					ext_cur_j = j;

				}
				// normal
				else 
				{
					bits_len_tab[j].len = (BYTE)(((total_outlen & 0xf) << 4) | (total_inlen & 0xf));
					bits_len_tab[j].bits = (WORD)(out_data | (total_last << 15));
				}
			}
		}
		msg("DivX3: AC-init[%d]: ext_num=%d, rt_num=%d\n", i, ext_idx, rt_idx);
		gui_update();
	}
	

	return 0;
}

static void divx_init_uni_dc_tab(void)
{
    int level, uni_code, uni_len;

    for (level = -256; level < 256; level++)
	{
        int size, v, l;
        // find number of bits
        size = 0;
        v = Abs(level);
        while (v)
		{
            v >>= 1;
			size++;
        }

        if (level < 0)
            l = (-level) ^ ((1 << size) - 1);
        else
            l = level;

        // luminance
        uni_code = DCtab_lum[size][0];
        uni_len = DCtab_lum[size][1];

        if (size > 0) 
		{
            uni_code <<= size; uni_code |= l;
            uni_len += size;
            if (size > 8)
			{
                uni_code <<= 1; uni_code |= 1;
                uni_len++;
            }
        }
        uni_DCtab_lum_bits[level + 256] = (WORD)uni_code;
        uni_DCtab_lum_len [level + 256] = (BYTE)uni_len;

        // chrominance
        uni_code = DCtab_chrom[size][0];
        uni_len = DCtab_chrom[size][1];
        
        if (size > 0) 
		{
            uni_code <<= size; uni_code |= l;
            uni_len += size;
            if (size > 8)
			{
                uni_code <<= 1; uni_code |= 1;
                uni_len++;
            }
        }
        uni_DCtab_chrom_bits[level + 256] = (WORD)uni_code;
        uni_DCtab_chrom_len [level + 256] = (BYTE)uni_len;
    }
}

STATIC INLINE int divx_get_rl_index(const DIVX_RL_TABLE *rl, int last, int run, int level)
{
    DWORD index;
    index = rl->index_run[last][run];
    if (index >= rl->n)
        return rl->n;
    if (level > rl->max_level[last][run])
        return rl->n;
    return (int)index + level - 1;
}

static void divx_init_uni_tabs()
{
	DWORD i;
    for (i = 0; i < 2; i++)
	{
		DIVX_RL_TABLE *rl = &rl_table[i == 0 ? 2 : 5];
		DWORD *uni_tab = uni_table[i];
		int slevel, run, last;

		for (run = 0; run < 64; run++)
		{
			for (last = 0; last <= 1; last++)
			{
				const int index = ((last)*128*64 + (run)*128 + 64);
				uni_tab[index] = 0;
			}
		}
    
		for (slevel = -64; slevel < 64; slevel++)
		{
			if (slevel == 0) 
				continue;
			for (run = 0; run < 64; run++)
			{
				for (last = 0; last <= 1; last++)
				{
					const int index = ((last)*128*64 + (run)*128 + (slevel+64));
					int level = slevel < 0 ? -slevel : slevel;
					int sign = slevel < 0 ? 1 : 0;
					DWORD bits, len, code;
					int level1, run1;
                
					DWORD out_len = 30;
					DWORD out_bits = 0;
					
					// ESC0
					code = divx_get_rl_index(rl, last, run, level);
					bits = rl->table_vlc[code][0];
					len =  rl->table_vlc[code][1] + 1;
					bits = bits * 2 + sign;
                
					if (code != rl->n && len < out_len)
					{
						out_bits = bits; out_len = len;
					}
					// ESC1
					bits = rl->table_vlc[rl->n][0];
					len =  rl->table_vlc[rl->n][1] + 1;
					bits = bits*2;
					level1 = level - rl->max_level[last][run];
					if (level1 > 0)
					{
						code = divx_get_rl_index(rl, last, run, level1);
						bits <<= rl->table_vlc[code][1];
						len  += rl->table_vlc[code][1];
						bits += rl->table_vlc[code][0];
						bits = bits * 2 + sign; 
						len++;
						
						if (code != rl->n && len < out_len)
						{
							out_bits = bits; out_len = len;
						}
					}
					// ESC2
					bits = rl->table_vlc[rl->n][0];
					len =  rl->table_vlc[rl->n][1];
					bits = bits * 4 + 2;    len+=2;
					run1 = run - rl->max_run[last][level] - 1;
					if (run1 >= 0)
					{
						code = divx_get_rl_index(rl, last, run1, level);
						bits <<= rl->table_vlc[code][1];
						len  += rl->table_vlc[code][1];
						bits += rl->table_vlc[code][0];
						bits = bits * 2 + sign; len++;
						
						if (code != rl->n && len < out_len)
						{
							out_bits = bits; out_len = len;
						}
					}

					uni_tab[index] = (out_len << 24) | (out_bits & 0xffffff);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////

#define divx_get_vlc2(code, dec_v, dec_bitidx, dec_left, table) \
{ \
    register DWORD index; \
    register int n; \
	\
	bitstream_show_bits(index, dec_v, dec_bitidx, 9); \
    code  = table[index][0]; \
    n     = table[index][1]; \
	\
    if (n < 0) \
	{ \
		bitstream_skip_bits(0, dec_v, dec_bitidx, dec_left, 9); \
        bitstream_show_bits(index, dec_v, dec_bitidx, (-n)); \
		index += code; \
        code  = table[index][0]; \
        n     = table[index][1]; \
    } \
	bitstream_skip_bits(0, dec_v, dec_bitidx, dec_left, n); \
}

#define divx_get_vlc3(code, dec_v, dec_bitidx, dec_left, table) \
{ \
    register DWORD index; \
    register int n; \
    \
	bitstream_show_bits(index, dec_v, dec_bitidx, 9); \
    code  = table[index][0]; \
    n     = table[index][1]; \
	\
    if (n < 0) \
	{ \
        register int nb_bits = -n; \
		bitstream_skip_bits(0, dec_v, dec_bitidx, dec_left, 9); \
		\
        bitstream_show_bits(index, dec_v, dec_bitidx, nb_bits); \
		index += code; \
        code  = table[index][0]; \
        n     = table[index][1]; \
        \
		if (n < 0) \
		{ \
			bitstream_skip_bits(0, dec_v, dec_bitidx, dec_left, nb_bits); \
            nb_bits = -n; \
            bitstream_show_bits(index, dec_v, dec_bitidx, nb_bits); \
			index += code; \
            code  = table[index][0]; \
            n     = table[index][1]; \
        } \
    } \
	bitstream_skip_bits(0, dec_v, dec_bitidx, dec_left, n); \
}

STATIC INLINE int divx_log2(unsigned int v)
{
    int n = 0;
    if (v & 0xffff0000) 
	{
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) 
	{
        v >>= 8;
        n += 8;
    }
    n += divx_log2_tab[v];

    return n;
}

STATIC INLINE int divx_coded_block_pred(int n, BYTE **coded_block_ptr)
{
    int xy, wrap, pred, a, b, c;

    xy = block_index[n];
    wrap = b8_stride;

    a = coded_block[xy - 1       ];
    b = coded_block[xy - 1 - wrap];
    c = coded_block[xy     - wrap];
    
    if (b == c)
        pred = a;
    else
        pred = c;
    
    *coded_block_ptr = &coded_block[xy];

    return pred;
}

STATIC INLINE int divx_pred_dc(int n, int level, int *dir_ptr, BOOL encoding)
{
    int a, b, c, wrap, pred, scale, ret;
    WORD *dc_val;

    // find prediction
    if (n < 4) 
	{
		scale = encoding ? enc_y_dc_scale : dec_y_dc_scale;
    } else 
	{
		scale = encoding ? enc_c_dc_scale : dec_c_dc_scale;
    }

    wrap = block_wrap[n];
	dc_val = (WORD *)((encoding ? enc_dc_val[0] : dec_dc_val[0]) + block_index[n]);

    /* B C
     * A X 
     */
    a = dc_val[ - 1];
    b = dc_val[ - 1 - wrap];
    c = dc_val[ - wrap];

    if (encoding)
	{
		if (Abs(a - b) < Abs(b - c)) 
		{
			pred = c;
			*dir_ptr = 1; // top
		} else 
		{
			pred = a;
			*dir_ptr = 0; // left
		}

		pred = -(pred + (scale >> 1)) / scale;
		
		ret = (level + pred);
		level *= scale;

		old_enc_dc_val = dc_val[0];
    } else
	{
		if (scale == 8) 
		{
			a = (a + (8 >> 1)) / 8;
			b = (b + (8 >> 1)) / 8;
			c = (c + (8 >> 1)) / 8;
		} else 
		{
			a = (a + (scale >> 1)) / scale;
			b = (b + (scale >> 1)) / scale;
			c = (c + (scale >> 1)) / scale;
		}
		
		if (Abs(a - b) <= Abs(b - c)) 
		{
			pred = c;
			*dir_ptr = 1; // top
		} else 
		{
			pred = a;
			*dir_ptr = 0; // left
		}

		ret = (level + pred);
		level = ret * scale;

		old_dec_dc_val = dc_val[0];
	}
    
    if (level & (~2047))
	{
        if (level < 0) 
            level = 0;
    }

	dc_val[0] = (WORD)level;

    return ret;
}

#ifdef USE_AC_CORRECTION

STATIC INLINE DWORD divx_pred_notcoded_ac(int block_i, int block[64], int dec_dc_pred_dir)
{
	int i = block_index[block_i] * 16;
	int *ac = ac_val + i;
	DWORD non_zero = 0;
	
	if (ac_pred) 
	{
        if (dec_dc_pred_dir == 0) 
		{
			int *a = ac - 16;
			for(i = 1; i < 8; i++) 
			{
#ifdef DUMP
//				DEBUG_MSG("block[%d] = %d\n", i<<3, *a);
#endif
				
				int level = *a++;
				level = (level > 0) ? (level - qadd) / qmul : (level + qadd) / qmul;
				block[i<<3] = level;
				non_zero |= (DWORD)level;
			}
        } else 
		{
			int *a = ac - 16 * block_wrap[block_i] + 8;
			for(i = 1; i < 8; i++)
			{
#ifdef DUMP
//				DEBUG_MSG("block[%d] = %d\n", i, *a);
#endif
				int level = *a++;
				level = (level > 0) ? (level - qadd) / qmul : (level + qadd) / qmul;
				block[i] = level;
				non_zero |= (DWORD)level;
			}
        }
	}

    int *a = ac, *a8 = a + 8;
	for(i = 1; i < 8; i++)
	{
		int level = block[i<<3];
		level = (level > 0) ? level * qmul + qadd : level * qmul - qadd;
		*a++  = level;
		
		level = block[i];
		level = (level > 0) ? level * qmul + qadd : level * qmul - qadd;
		*a8++ = level;
	}

	// non-zero if any coefs are non-zero
	return non_zero != 0 ? 1 : 0;
}

STATIC INLINE DWORD divx_pred_ac(int block_i, int *block, int dec_dc_pred_dir)
{
	int i = block_index[block_i] * 16;
	int *ac = ac_val + i;
	DWORD non_zero = 0;
	
    if (dec_dc_pred_dir == 0) 
	{
		int *a = ac - 16;
		for(i = 1; i < 8; i++) 
		{
#ifdef DUMP
//				DEBUG_MSG("block[%d] = %d + %d\n", i<<3, block[i<<3], *a);
#endif

			int level = block[i<<3];
			level = (level > 0) ? level * qmul + qadd : level * qmul - qadd;
			level += *a++;
			level = (level > 0) ? (level - qadd) / qmul : (level + qadd) / qmul;
			block[i<<3] = level;
			non_zero |= (DWORD)level;
		}
    } else 
	{
		int *a = ac - 16 * block_wrap[block_i] + 8;
		for(i = 1; i < 8; i++)
		{
#ifdef DUMP
//				DEBUG_MSG("block[%d] = %d + %d\n", i, block[i], *a);
#endif
			int level = block[i];
			level= (level > 0) ? level * qmul + qadd : level * qmul - qadd;
			level += *a++;
			level = (level > 0) ? (level - qadd) / qmul : (level + qadd) / qmul;
			block[i] = level;
			non_zero |= (DWORD)level;
		}
    }

    int *a = ac, *a8 = a + 8;
	for(i = 1; i < 8; i++)
	{
		int level = block[i<<3];
		level = (level > 0) ? level * qmul + qadd : level * qmul - qadd;
		*a++  = level;
		
		level = block[i];
		level = (level > 0) ? level * qmul + qadd : level * qmul - qadd;
		*a8++ = level;
	}
	
	// non-zero if any coefs are non-zero
	return non_zero != 0 ? 1 : 0;
}

STATIC INLINE void divx_pred_flaged_ac(int block_i, int *block, DWORD *block_f, DWORD block_flag, int dec_dc_pred_dir)
{
	int i = block_index[block_i] * 16;
	int *ac = ac_val + i;
	
    int *a = ac, *a8 = a + 8;
	for(i = 1; i < 8; i++)
	{
		if (block_f[i<<3] == block_flag)
		{
			int level = block[i<<3];
			level = (level > 0) ? level * qmul + qadd : level * qmul - qadd;
			*a++  = level;
		}
		else
			*a++  = 0;
		if (block_f[i] == block_flag)
		{
			int level = block[i];
			level = (level > 0) ? level * qmul + qadd : level * qmul - qadd;
			*a8++  = level;
		}
		else
			*a8++  = 0;
	}
}

#endif

#if 0
STATIC INLINE int divx_put_rl_vlc(DWORD &enc_v, int &enc_bitidx, int &enc_len, int level, int last, int run, DWORD *bits_tab, BYTE *len_tab)
{
	register int index = last*128*64 + (run-1)*128 + level + 64;
	if (index >= 0)
		return bitstream_put_bits(len_tab[index], bits_tab[index], enc_v, enc_bitidx, enc_len);
	return 0;
}
#endif

#define divx_put_rl_vlc_esc3(enc_v, enc_bitidx, enc_len, level, last, run) \
{ \
	DWORD d = ((3 << 23) + (3 << 21) + (1 << 13) + 1) + \
			(last << 20) + ((run - 1) << 14) + (((level) & 0xfff) << 1); \
	bitstream_put_bits((7+2+1+6+1+12+1), d, enc_v, enc_bitidx, enc_len); \
}

#define divx_encode_dc(enc_v, enc_bitidx, enc_len, level, n) \
{ \
	level += 256; \
    if (n < 4)  /* luminance */ \
	{ \
		bitstream_put_bits(uni_DCtab_lum_len[level], uni_DCtab_lum_bits[level], enc_v, enc_bitidx, enc_len); \
    } else		/* chrominance */ \
	{ \
		bitstream_put_bits(uni_DCtab_chrom_len[level], uni_DCtab_chrom_bits[level], enc_v, enc_bitidx, enc_len); \
    } \
}

STATIC INLINE int divx_mid_pred(int a, int b, int c)
{
    if (a > b)
	{
        if (c > b)
            b = (c > a) ? a : c;
    } else
	{
        if (b > c)
            b = (c > a) ? c : a;
    }
    return b;
}

STATIC INLINE signed short *divx_pred_motion(int *px, int *py)
{
    int wrap;
    signed short *A, *B, *C, (*mot_val)[2];
	
    wrap = b8_stride;
    mot_val = motion_val + block_index[0];

    A = mot_val[ - 1];
    if (first_slice_line) 
	{
        if (mb_x == 0)
		{
            *px = *py = 0;
        } else
		{
            *px = A[0];
            *py = A[1];
        }
    } else 
	{
        B = mot_val[ - wrap];
        C = mot_val[2 - wrap];
        *px = divx_mid_pred(A[0], B[0], C[0]);
        *py = divx_mid_pred(A[1], B[1], C[1]);
    }
    return *mot_val;
}

#define divx_decode_motion(dec_v, dec_bitidx, dec_left, mx_ptr, my_ptr) \
{ \
    DIVX_MV_TABLE *mv = &mv_tables[mv_table_index]; \
	\
    register int code; \
	int mx, my; \
    divx_get_vlc2(code, dec_v, dec_bitidx, dec_left, mv->vlc.table); \
    if (code < 0) \
        return -1; \
    if (code == mv->n)  \
	{ \
		register DWORD mxmy; \
		bitstream_get_bits(mxmy, 0, dec_v, dec_bitidx, dec_left, 12); \
        mx = mxmy >> 6; \
        my = mxmy & 63; \
    } else  \
	{ \
        mx = mv->table_mvx[code]; \
        my = mv->table_mvy[code]; \
    } \
	\
    mx += mx_ptr - 32; \
    my += my_ptr - 32; \
    /* \WARNING : they do not do exactly modulo encoding */ \
    if (mx <= -64) \
        mx += 64; \
    else if (mx >= 64) \
        mx -= 64; \
	\
    if (my <= -64) \
        my += 64; \
    else if (my >= 64) \
        my -= 64; \
	\
    mx_ptr = mx; \
    my_ptr = my; \
}

#define divx_encode_motion(enc_v, enc_bitidx, enc_len, val, pred, fc) \
{ \
    register int range, l, bit_size, sign, code, bits; \
	\
	int local_val = val - pred; \
    if (local_val == 0) \
	{ \
        code = 0; \
		/*DUMP_FRAME("MOTION ZERO code = %d\n", code);*/ \
		bitstream_put_bits(mvtab[code][1], mvtab[code][0], enc_v, enc_bitidx, enc_len); \
    } else  \
	{ \
		DWORD d; \
        bit_size = fc - 1; \
        range = 1 << bit_size; \
        /* modulo encoding (not divx3-like!) */ \
        l = range * 32; \
        local_val += l; \
        local_val &= 2*l-1; \
        local_val -= l; \
        sign = local_val >> 31; \
        local_val = (local_val^sign) - sign; \
        sign &= 1; \
        local_val--; \
        code = (local_val >> bit_size) + 1; \
        bits = local_val & (range - 1); \
		\
		/*DUMP_FRAME("MOTION code = %d sign=%d bits=%d bs=%d\n", code, sign, bits, bit_size);*/ \
		\
        d = ((mvtab[code][0] << 1) | sign); \
		bitstream_put_bits(mvtab[code][1] + 1, d, enc_v, enc_bitidx, enc_len); \
        if (bit_size > 0)  \
		{ \
			bitstream_put_bits(bit_size, bits, enc_v, enc_bitidx, enc_len); \
        } \
    } \
}

STATIC INLINE void divx_set_block_index()
{
    block_index[0] = block_index_tbl[mb_y][0];
	block_index[1] = block_index_tbl[mb_y][1];
	block_index[2] = block_index_tbl[mb_y][2];
	block_index[3] = block_index_tbl[mb_y][3];
	block_index[4] = block_index_tbl[mb_y][4];
	block_index[5] = block_index_tbl[mb_y][5];
}

STATIC INLINE void divx_update_block_index()
{
    block_index[0] += 2;
    block_index[1] += 2;
    block_index[2] += 2;
    block_index[3] += 2;
    block_index[4]++;
    block_index[5]++;
}

STATIC INLINE void divx_update_motion_val()
{
	register int xy = block_index[0];
	register int wrap = b8_stride;

	//DUMP_FRAME("--update_motion_val xy=%d = %d,%d\n", xy, motion_x, motion_y);

	register signed short smx = (signed short)motion_x;
	register signed short smy = (signed short)motion_y;
	motion_val[xy][0] = smx;
    motion_val[xy][1] = smy;
    motion_val[xy + 1][0] = smx;
    motion_val[xy + 1][1] = smy;
    motion_val[xy + wrap][0] = smx;
    motion_val[xy + wrap][1] = smy;
    motion_val[xy + 1 + wrap][0] = smx;
    motion_val[xy + 1 + wrap][1] = smy;
}

STATIC INLINE void divx_clean_intra_table_entries()
{
    register int wrap = b8_stride;
    register int xy = block_index[0];
	register int xy1 = xy + 1, xywrap = xy + wrap, xy1wrap = xy + 1 + wrap;
    
    coded_block[xy] = coded_block[xy1] = coded_block[xywrap] = coded_block[xy1wrap] = 0;

    dec_dc_val[0][xy] = dec_dc_val[0][xy1] = dec_dc_val[0][xywrap] = dec_dc_val[0][xy1wrap] = 1024;
	dec_dc_val[0][xy] = dec_dc_val[0][xy1] = dec_dc_val[0][xywrap] = dec_dc_val[0][xy1wrap] = 1024;
	
	enc_dc_val[0][xy] = enc_dc_val[0][xy1] = enc_dc_val[0][xywrap] = enc_dc_val[0][xy1wrap] = 1024;
	enc_dc_val[0][xy] = enc_dc_val[0][xy1] = enc_dc_val[0][xywrap] = enc_dc_val[0][xy1wrap] = 1024;
    
    wrap = mb_stride;
    xy = mb_x + mb_y * wrap;
    dec_dc_val[1][xy] = dec_dc_val[2][xy] = 1024;
	enc_dc_val[1][xy] = enc_dc_val[2][xy] = 1024;
}

/////////////////////////////////////////////////////////////////

#define divx_encode_mb_header(enc_v, enc_bitidx, enc_len, param_cbp, skip, ac_pred) \
{ \
	DWORD cbpc = param_cbp & 3; \
	DWORD cbpy = param_cbp >> 2; \
    if (pict_type == DIVX_I_TYPE)  \
	{ \
        bitstream_put_bits(intra_MCBPC_bits[cbpc], intra_MCBPC_code[cbpc], enc_v, enc_bitidx, enc_len); \
        \
		if (mb_intra) \
		{ \
			bitstream_put_bits(1, ac_pred, enc_v, enc_bitidx, enc_len); \
		} \
		else \
			cbpy ^= 0x0F; \
		/*DUMP_FRAME("cbpc=%d cbpy=%d\n", cbpc, cbpy);*/ \
		bitstream_put_bits(cbpy_tab[cbpy][1], cbpy_tab[cbpy][0], enc_v, enc_bitidx, enc_len); \
    } else \
	{ \
		bitstream_put_bits(1, (skip ? 1 : 0), enc_v, enc_bitidx, enc_len); \
		if (!skip) \
		{ \
			if (mb_intra) \
				cbpc |= 4; \
        	bitstream_put_bits(inter_MCBPC_bits[cbpc], inter_MCBPC_code[cbpc], enc_v, enc_bitidx, enc_len); \
			\
			if (mb_intra) \
			{ \
				bitstream_put_bits(1, ac_pred, enc_v, enc_bitidx, enc_len); \
			} \
			else \
				cbpy ^= 0x0F; \
			/*DUMP_FRAME("cbpc=%d cbpy=%d\n", cbpc, cbpy);*/ \
			bitstream_put_bits(cbpy_tab[cbpy][1], cbpy_tab[cbpy][0], enc_v, enc_bitidx, enc_len); \
		} \
	} \
}

#define divx_decode_picture_header(dec_v, dec_bitidx, dec_left) \
{ \
    bitstream_get_bits(pict_type, 0, dec_v, dec_bitidx, dec_left, 2); \
	pict_type += 1; \
    if (pict_type != DIVX_I_TYPE && pict_type != DIVX_P_TYPE) \
        return -1; \
    bitstream_get_bits(qscale, 0, dec_v, dec_bitidx, dec_left, 5); \
    if (qscale == 0) \
        return -1; \
	chroma_qscale = chroma_qscale_table[qscale]; \
    enc_y_dc_scale = enc_y_dc_scale_table[qscale]; \
    enc_c_dc_scale = enc_c_dc_scale_table[chroma_qscale]; \
	dec_y_dc_scale = dec_y_dc_scale_table[qscale]; \
    dec_c_dc_scale = dec_c_dc_scale_table[chroma_qscale]; \
	\
    if (pict_type == DIVX_I_TYPE)  \
	{ \
		DWORD code; \
        bitstream_get_bits(code, 0, dec_v, dec_bitidx, dec_left, 5); \
        if (code < 0x17) \
			return -1; \
		\
		slice_height = mb_height / (code - 0x16); \
		\
		bitstream_get_bits012(rl_chroma_table_index, 0, dec_v, dec_bitidx, dec_left); \
		bitstream_get_bits012(rl_table_index, 0, dec_v, dec_bitidx, dec_left); \
		bitstream_get_bits1(dc_table_index, 0, dec_v, dec_bitidx, dec_left); \
        no_rounding = 1; \
    } else  \
	{	\
		bitstream_get_bits1(use_skip_mb_code, 0, dec_v, dec_bitidx, dec_left); \
		bitstream_get_bits012(rl_table_index, 0, dec_v, dec_bitidx, dec_left); \
		rl_chroma_table_index = rl_table_index; \
		bitstream_get_bits1(dc_table_index, 0, dec_v, dec_bitidx, dec_left); \
		bitstream_get_bits1(mv_table_index, 0, dec_v, dec_bitidx, dec_left); \
		\
        if(flipflop_rounding) \
            no_rounding ^= 1; \
        else \
            no_rounding = 0; \
    } \
}

#define divx_encode_vos_header(enc_v, enc_bitidx, enc_len) \
{ \
	bitstream_put_bits(32, 0x000001B0, enc_v, enc_bitidx, enc_len); \
    bitstream_put_bits(8, 0xf5, enc_v, enc_bitidx, enc_len);	/* profile: f5=??? 01=MPEG4@SL1 */ \
}

#define divx_encode_vol_header(enc_v, enc_bitidx, enc_len) \
{ \
	const int vo_number = 0; \
	const int vol_number = 0; \
	const int vo_ver_id = 1; \
	const int vo_type = 1; \
	const int aspect_ratio_info = 1; \
	\
    bitstream_put_bits(32, 0x100 + vo_number, enc_v, enc_bitidx, enc_len); \
    bitstream_put_bits(32, 0x120 + vol_number, enc_v, enc_bitidx, enc_len); \
	\
    bitstream_put_bits(1, 0, enc_v, enc_bitidx, enc_len); \
    bitstream_put_bits(8, vo_type, enc_v, enc_bitidx, enc_len); \
    bitstream_put_bits(8, (1<<7)|(vo_ver_id<<3)|1, enc_v, enc_bitidx, enc_len); \
	\
    bitstream_put_bits(4, aspect_ratio_info, enc_v, enc_bitidx, enc_len); \
	bitstream_put_bits(8, 0xB1, enc_v, enc_bitidx, enc_len);  /*10110001b*/ \
    bitstream_put_bits(16, time_incr_res, enc_v, enc_bitidx, enc_len); \
    bitstream_put_bits(3, 5, enc_v, enc_bitidx, enc_len); /* 101b */ \
    bitstream_put_bits(13, width, enc_v, enc_bitidx, enc_len); \
    bitstream_put_bits(1, 1, enc_v, enc_bitidx, enc_len); \
    bitstream_put_bits(13, height, enc_v, enc_bitidx, enc_len); \
    bitstream_put_bits(1, 1, enc_v, enc_bitidx, enc_len); \
    bitstream_put_bits(1, 0 /*progressive=1*/, enc_v, enc_bitidx, enc_len); \
    \
	bitstream_put_bits(8, 0x8C, enc_v, enc_bitidx, enc_len); /*10001100b*/ \
    \
    bitstream_put_bits(3, 3, enc_v, enc_bitidx, enc_len); \
}

#if 0
static int divx_encode_gop_header(DWORD &enc_v, int &enc_bitidx, int &enc_len)
{
    bitstream_put_bits(32, 0x000001B3, enc_v, enc_bitidx, enc_len);
    
    LONGLONG time= pts;
	const int AV_TIME_BASE = 1000000;
    time= (time * time_incr_res + AV_TIME_BASE/2) / AV_TIME_BASE;

    int seconds = (int)(time / time_incr_res);
    int minutes = seconds / 60; seconds %= 60;
    int hours = minutes/60; minutes %= 60;  hours%=24;

    bitstream_put_bits(5, hours, enc_v, enc_bitidx, enc_len);
    bitstream_put_bits(6, minutes, enc_v, enc_bitidx, enc_len);
    bitstream_put_bits(1, 1, enc_v, enc_bitidx, enc_len);
    bitstream_put_bits(6, seconds, enc_v, enc_bitidx, enc_len);
    
    return bitstream_put_bits(6, 7, enc_v, enc_bitidx, enc_len); 
}
#endif


#define divx_encode_picture_header(enc_v, enc_bitidx, enc_len) \
{ \
	/* VOP header */ \
	int time_increment_bits; \
	int time_div = 0, time_mod = 0, time_incr; \
	\
	bitstream_put_bits(32, 0x000001b6, enc_v, enc_bitidx, enc_len); \
    bitstream_put_bits(2, pict_type - 1, enc_v, enc_bitidx, enc_len); \
	\
	time_increment_bits = time_incr_res != 0 ? divx_log2(time_incr_res - 1) + 1 : 4; \
	\
	if (time_incr_res > 0) \
	{ \
		time_div = (int)(pts / time_incr_res); \
		time_mod = (int)(pts % time_incr_res); \
	} \
    time_incr = time_div - last_time_div; \
	last_time_div = time_div; \
    \
	while (time_incr--) \
	{ \
        bitstream_put_bits(1, 1, enc_v, enc_bitidx, enc_len); \
    } \
    bitstream_put_bits(2, 1, enc_v, enc_bitidx, enc_len); \
    bitstream_put_bits(time_increment_bits, time_mod, enc_v, enc_bitidx, enc_len); \
    bitstream_put_bits(2, 3, enc_v, enc_bitidx, enc_len); \
    if (pict_type == DIVX_P_TYPE)  \
	{ \
		bitstream_put_bits(1, no_rounding, enc_v, enc_bitidx, enc_len); \
    } \
    \
    bitstream_put_bits(8, qscale & 31, enc_v, enc_bitidx, enc_len); \
	\
    if (pict_type != DIVX_I_TYPE) \
    { \
		bitstream_put_bits(3, f_code, enc_v, enc_bitidx, enc_len); \
	} \
}

/////////////////////////////////////////////////////////////////

int divx_transcode_preinit()
{
	int i;
	static const int bits_len2_size[9] = { 600, 800, 10, 1100, 600, 1700, 300, 500, 10 };
	static const int level_run2_size[6] = { 6, 130, 6, 100, 140, 6 };

	uni_table[0] = (DWORD *)SPmalloc(128*64*2 * sizeof(DWORD));
	uni_table[1] = (DWORD *)SPmalloc(128*64*2 * sizeof(DWORD));
	if (uni_table[0] == NULL || uni_table[1] == NULL)
		return -1;

	for (i = 0; i < 9; i++) 
	{
		bits_len[i] = (DIVX_BITS_LEN *)SPmalloc(16484 * sizeof(DIVX_BITS_LEN));
		if (bits_len[i] == NULL)
			return -1;
		//memset(bits_len[i], 0xff, 16484 * sizeof(DIVX_BITS_LEN));
		bits_len2[i] = (DWORD *)SPmalloc(bits_len2_size[i] * sizeof(DWORD));
		if (bits_len2[i] == NULL)
			return -1;
	}

	for (i = 0; i < 6; i++) 
	{
		level_run[i] = (DWORD *)SPmalloc(16484 * sizeof(DWORD));
		if (level_run[i] == NULL)
			return -1;
		level_run2[i] = (DWORD *)SPmalloc(level_run2_size[i] * sizeof(DWORD));
		if (level_run2[i] == NULL)
			return -1;
	}

	if (divx_init_rl(&rl_table[2]) < 0)
		return -1;
	if (divx_init_rl(&rl_table[5]) < 0)
		return -1;

	return 0;
}

int divx_transcode_predeinit()
{
    int i;
	divx_free_rl(&rl_table[5]);
	divx_free_rl(&rl_table[2]);

	SPSafeFree(uni_table[0]);
	SPSafeFree(uni_table[1]);

	for (i = 0; i < 6; i++) 
	{
		SPSafeFree(level_run[i]);
		SPSafeFree(level_run2[i]);
	}
	for (i = 0; i < 9; i++) 
	{
		SPSafeFree(bits_len[i]);
		SPSafeFree(bits_len2[i]);
	}

	return 0;
}


int divx_transcode_init(int w, int h, int t)
{
	int i, y_size, c_size, yc_size, b8_array_size;
	width = w;
	height = h;
	mb_width = (width  + 15) / 16;
    mb_height = (height  + 15) / 16;
    mb_num = mb_width * mb_height;
	no_rounding = 0;
	flipflop_rounding = 1;

	b8_stride = mb_width * 2 + 1;
	mb_stride = mb_width + 1;
	y_size = b8_stride * (2 * mb_height + 1);
	c_size = mb_stride * (mb_height + 1);
    yc_size = y_size + 2 * c_size;
	b8_array_size = b8_stride * mb_height * 2;

	enc_dc_val_base = (signed short *)SPcalloc(yc_size * sizeof(signed short));
	dec_dc_val_base = (signed short *)SPcalloc(yc_size * sizeof(signed short));
	coded_block_base = (BYTE *)SPcalloc(y_size);
	motion_val_base = (signed short (*)[2])SPcalloc(2 * (b8_array_size + 2) * sizeof(signed short));
	block_index_tbl = (int (*)[6])SPmalloc(6 * mb_height * sizeof(int));

	if (enc_dc_val_base == NULL || dec_dc_val_base == NULL || 
		coded_block_base == NULL || motion_val_base == NULL || block_index_tbl == NULL)
	{
		return -1;
	}

#ifdef USE_AC_CORRECTION
	ac_size = yc_size * sizeof(int) * 16;
	ac_val_base = (int *)SPcalloc(ac_size);
	if (ac_val_base == NULL)
		return -1;
	ac_val = ac_val_base + (b8_stride + 1) * 16;
#endif

	block_wrap[0] = block_wrap[1] = block_wrap[2] = block_wrap[3] = b8_stride;
    block_wrap[4] = block_wrap[5] = mb_stride;

    enc_dc_val[0] = enc_dc_val_base + b8_stride + 1;
	enc_dc_val[1] = enc_dc_val_base + y_size + mb_stride + 1;
	enc_dc_val[2] = enc_dc_val[1] + c_size;
	dec_dc_val[0] = dec_dc_val_base + b8_stride + 1;
	dec_dc_val[1] = dec_dc_val_base + y_size + mb_stride + 1;
	dec_dc_val[2] = dec_dc_val[1] + c_size;
	for (i = 0; i < yc_size; i++)
	{
		enc_dc_val_base[i] = 1024;
		dec_dc_val_base[i] = 1024;
	}

	for (i = 0; i < mb_height; i++)
	{
		block_index_tbl[i][0] = b8_stride * (i*2    ) - 2;
		block_index_tbl[i][1] = b8_stride * (i*2    ) - 1;
		block_index_tbl[i][2] = b8_stride * (i*2 + 1) - 2;
		block_index_tbl[i][3] = b8_stride * (i*2 + 1) - 1;
		block_index_tbl[i][4] = mb_stride * (i + 1)             + b8_stride * mb_height*2 - 1;
		block_index_tbl[i][5] = mb_stride * (i + mb_height + 2) + b8_stride * mb_height*2 - 1;
	}
	
    coded_block = coded_block_base + b8_stride + 1;

	mb_x = mb_y = 0;
	ac_pred = 0;
	f_code = 2;

	dec_y_dc_scale_table = old_y_dc_scale_table;
	dec_c_dc_scale_table = old_c_dc_scale_table;
	enc_y_dc_scale_table = mpeg4_y_dc_scale_table;
	enc_c_dc_scale_table = mpeg4_c_dc_scale_table;
	chroma_qscale_table = default_chroma_qscale_table;

	if (divx_init_vlc(&mb_intra_vlc, 9, 64, &table_mb_intra[0][1], 4, 2, &table_mb_intra[0][0], 4, 2) < 0)
		 return -1;

	if (divx_init_vlc(&mb_non_intra_vlc, 9, 128, &table_mb_non_intra[0][1], 8, 4, &table_mb_non_intra[0][0], 8, 4) < 0)
		 return -1;

	for (i = 0; i < 2; i++) 
	{
		DIVX_MV_TABLE *mv = &mv_tables[i];
		if (divx_init_vlc(&mv->vlc, 9, mv->n + 1, mv->table_mv_bits, 1, 
						1, mv->table_mv_code, 2, 2) < 0)
			return -1;
	}

	if (divx_init_vlc(&dc_lum_vlc[0], 9, 120, &table0_dc_lum[0][1], 8, 4, &table0_dc_lum[0][0], 8, 4) < 0)
		return -1;
	if (divx_init_vlc(&dc_chroma_vlc[0], 9, 120, &table0_dc_chroma[0][1], 8, 4, &table0_dc_chroma[0][0], 8, 4) < 0)
		return -1;
	if (divx_init_vlc(&dc_lum_vlc[1], 9, 120, &table1_dc_lum[0][1], 8, 4, &table1_dc_lum[0][0], 8, 4) < 0)
		return -1;
	if (divx_init_vlc(&dc_chroma_vlc[1], 9, 120, &table1_dc_chroma[0][1], 8, 4, &table1_dc_chroma[0][0], 8, 4) < 0)
		return -1;

	divx_init_scantable(&inter_scantable  , zigzag_direct);
    divx_init_scantable(&intra_scantable  , zigzag_direct);
    divx_init_scantable(&intra_h_scantable, alternate_horizontal_scan);
    divx_init_scantable(&intra_v_scantable, alternate_vertical_scan);

	pts = 0;
	time_incr_res = t;
	last_time_div = 0;

	divx_init_uni_dc_tab();

	divx_init_uni_tabs();
	divx_build_ac_tables();
	
	// be careful, don't use uni_table!
	SPSafeFree(uni_table[0]);
	SPSafeFree(uni_table[1]);

	motion_val = motion_val_base + 2;

	block_flag = 1;
	memset(block_flags, 0, sizeof(DWORD) * 64);

	frame_number = 0;
	return 0;
}

int divx_transcode_deinit()
{
	SPSafeFree(dc_lum_vlc[0].table);
	SPSafeFree(dc_chroma_vlc[0].table);
	SPSafeFree(dc_lum_vlc[1].table);
	SPSafeFree(dc_chroma_vlc[1].table);

	SPSafeFree(mv_tables[0].vlc.table);
	SPSafeFree(mv_tables[1].vlc.table);

	SPSafeFree(mb_intra_vlc.table);
	SPSafeFree(mb_non_intra_vlc.table);
	
#ifdef USE_AC_CORRECTION
	SPSafeFree(ac_val_base);
#endif

	SPSafeFree(block_index_tbl);
	SPSafeFree(motion_val_base);
	SPSafeFree(coded_block_base);
	SPSafeFree(dec_dc_val_base);
	SPSafeFree(enc_dc_val_base);

	return 0;
}

BOOL divx_is_key_frame()
{
	return pict_type == DIVX_I_TYPE;
}

//////////////////////////////////////////////////////
#ifdef USE_AC_CORRECTION

int divx_transcode_acpred_mb(DWORD &enc_v, int &ebitidx, int &enc_len, 
							DWORD &dec_v, int &bitidx, int &left, DWORD cbp)
{
	int blocks[6][64], *block;
	int dc_levels[6];
	DWORD block_i;
	DWORD block_coded[6];
	DWORD out_cbp = cbp & (~63);
	
	/// \TODO: move to the frame level

	for (block_i = 0; block_i < 6; block_i++) 
	{
		int ac_i, run_diff;
		int *dec_scan_table;
		int dec_dc_pred_dir;

		DIVX_BITS_LEN *bits_len_tab;
		DWORD *level_run_tab, *level_run_tab2;
		DWORD *bits_len_tab2, *uni_tbl;

		int coded = (cbp >> (5 - block_i)) & 1;
		int dc_level;

		register int level;
		register DWORD run;

		run_diff = 0;
		block = blocks[block_i];
		memset(block, 0, sizeof(int) * 64);

		// decode dc
		if (block_i < 4) 
			divx_get_vlc3(level, dec_v, bitidx, left, dc_lum_vlc[dc_table_index].table);
		else 
			divx_get_vlc3(level, dec_v, bitidx, left, dc_chroma_vlc[dc_table_index].table);

		if (level < 0)
			return -1;
		if (level == 119) 
		{
			register DWORD ret;
			bitstream_get_bits(level, 0, dec_v, bitidx, left, 8);
			bitstream_get_bits1(ret, 0, dec_v, bitidx, left)
			if (ret)
				level = -level;
		} 
		else if (level != 0) 
		{
			register DWORD ret;
			bitstream_get_bits1(ret, 0, dec_v, bitidx, left)
			if (ret)
				level = -level;
		}

		dc_level = divx_pred_dc(block_i, level, &dec_dc_pred_dir, FALSE);
		dc_levels[block_i] = dc_level;
		dec_scan_table = (dec_dc_pred_dir == 0) ? 
							intra_v_scantable.permutated : intra_h_scantable.permutated;

#ifdef DUMP
//		DEBUG_MSG("-----------------\n");
//		DEBUG_MSG("{%d}: DC LEVEL=%d / INTRA=%d\n", block_i, dc_level, mb_intra);
#endif
	
		if (!coded)
		{
			DWORD coded = divx_pred_notcoded_ac(block_i, block, dec_dc_pred_dir);
			out_cbp |= coded << (5 - block_i);
			block_coded[block_i] = coded;
			continue;
		}

		ac_i = 0;
#ifdef DUMP
		//DEBUG_MSG("  DIR: dec=%d, enc=%d\n", dec_dc_pred_dir, enc_dc_pred_dir);
#endif

		DWORD tbl_idx = block_i >> 2;
		bits_len_tab = bits_len_tab_cur[tbl_idx];
		bits_len_tab2 = bits_len_tab2_cur[tbl_idx];
		uni_tbl = uni_table[0];
		level_run_tab = level_run_tab_cur[tbl_idx];
		level_run_tab2 = level_run_tab2_cur[tbl_idx];
		
		while (ac_i < 64)
		{
			register DWORD idx, bits, len;
			bitstream_show_bits(idx, dec_v, bitidx, 16);

			bits = level_run_tab[idx>>2];
			
			if (bits == 0)	// esc123
			{
				bits = bits_len_tab[idx>>2].bits;
				len = bits_len_tab[idx>>2].len;
				bitstream_skip_bits(0, dec_v, bitidx, left, bits);
				
				if (len == 3)	// esc3
				{
					bitstream_get_bits(bits, 0, dec_v, bitidx, left, (1+6+8));
					level = (bits & 0x80) ? ((bits & 0xff) | 0xffffff00) : (bits & 0x7f);
					run = ((bits >> 8) & 63) + 1;
					if (bits & 0x4000)
						break;
				}
				else	// esc1,esc2
				{
					BOOL esc1 = (len & 1);
					bitstream_show_bits(idx, dec_v, bitidx, 16);
					bits = level_run_tab[idx>>2];
					if ((bits >> 16) == 0)
						bits = level_run_tab2[bits | (idx & 3)];
					len = ((bits >> 18) & 0xf) + 1;
					bitstream_skip_bits(0, dec_v, bitidx, left, len);
					
					level = ((int)bits) >> 25;
					run = (bits & 63) + 1;
					
					if (esc1)
					{
						level = level > 0 ? level + ((bits >> 12) & 31) * qmul : level - ((bits >> 12) & 31) * qmul;
					} else
					{
						run += (bits >> 6) & 63;
						run += run_diff;
					}
					
					if (bits & (1 << 17))
						break;
				}
			} else
			{
				if ((bits >> 16) == 0)
					bits = level_run_tab2[bits | (idx & 3)];
				len = ((bits >> 18) & 0xf) + 1;
				bitstream_skip_bits(0, dec_v, bitidx, left, len);
				level = ((int)bits) >> 25;
				
				run = (bits & 63) + 1;
				if (bits & (1 << 17))
					break;
			}

			ac_i += run;
			idx = dec_scan_table[ac_i];
			block[idx] = level;
#ifdef DUMP
			//DEBUG_MSG("-> level=%d, run=%d\n", level, run);
			block_idx[idx] = ac_i;
			block_run[idx] = run;
#endif
		}


		ac_i += run;

#ifdef DUMP
		//DEBUG_MSG("-> level=%d, run=%d\n", level, run);
		block_idx[dec_scan_table[ac_i]] = ac_i;
		block_run[dec_scan_table[ac_i]] = run;
#endif

		run = dec_scan_table[ac_i];
		block[run] = level;

		// we have reconstructed block at this moment...
		coded = divx_pred_ac(block_i, block, dec_dc_pred_dir);
		out_cbp |= coded << (5 - block_i);
		block_coded[block_i] = coded;
	}

	//////////////////////////////////////////////////////////////////////////////
	/// NOW ENCODE!!!

	divx_encode_mb_header(enc_v, ebitidx, enc_len, out_cbp, FALSE, 0);

	for (block_i = 0; block_i < 6; block_i++) 
	{
		register int level;
		register DWORD run, i;
		int last_non_zero = 0;
		int *enc_scan_table;
		int enc_dc_pred_dir;
		
		block = blocks[block_i];

#ifdef DUMP
		DEBUG_MSG("-----------------\n");
		DEBUG_MSG("{%d}: DC LEVEL=%d / INTRA=%d\n", block_i, dc_levels[block_i], mb_intra);

		for (i = 0; i < 64; i++)
		{
			if (block[i] != 0)
			{
				//DEBUG_MSG("[%d] = %d (%d,run=%d)\n", i, block[i], block_idx[i], block_run[i]);
				DEBUG_MSG("[%d] = %d\n", i, block[i]);
			}
		}
#endif

		level = divx_pred_dc(block_i, dc_levels[block_i], &enc_dc_pred_dir, TRUE);
		divx_encode_dc(enc_v, ebitidx, enc_len, level, block_i);

		if (!block_coded[block_i])
			continue;

		enc_scan_table = (enc_dc_pred_dir == 0) ? 
						intra_v_scantable.permutated : intra_h_scantable.permutated;

		// now output the table
		level = 0;
		run = 0;
		for (i = 0; i < 64; i++)
		{
			register DWORD eidx = enc_scan_table[i];
			
			if (block[eidx] != 0)
			{
				if (level && (int)run > last_non_zero)
				{
					divx_put_rl_vlc_esc3(enc_v, ebitidx, enc_len, level, 0, run - last_non_zero);
					last_non_zero = run;
				}
				
				level = block[eidx];
				run = i;
			}
		}
		
		divx_put_rl_vlc_esc3(enc_v, ebitidx, enc_len, level, 1, run - last_non_zero);
	}

	return 0;
}

#endif

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

int divx_transcode(bitstream_callback callback)
{
	int code;
	int block[64];

	register DWORD dec_v;
	register int bitidx, left;
	register DWORD enc_v = 0;
	register int ebitidx = 31;
	register int enc_len = 0;

	DWORD *block_f;

	bitstream_set_callback(0, callback);
	bitstream_decode_start(0, dec_v, bitidx, left);
	
	divx_decode_picture_header(dec_v, bitidx, left);

	if (divx_is_key_frame())
	{
		divx_encode_vos_header(enc_v, ebitidx, enc_len);
		divx_encode_vol_header(enc_v, ebitidx, enc_len);
		//if (divx_encode_gop_header(enc_v, ebitidx, len) < 0)
		//	return -1;
		bitstream_flush_output(BITSTREAM_MODE_OUTPUT, enc_v, ebitidx, enc_len);

		enc_v = 0;
		ebitidx = 31;
		enc_len = 0;
	}
	divx_encode_picture_header(enc_v, ebitidx, enc_len);

	// process VOP
	first_slice_line = 1;

	mb_x = 0;
	mb_y = 0;

	bits_len_tab_cur[0] = bits_len[rl_table_index];
	bits_len_tab_cur[1] = bits_len[3 + rl_chroma_table_index];
	bits_len_tab_cur[2] = bits_len[6 + rl_table_index];
	bits_len_tab2_cur[0] = bits_len2[rl_table_index];
	bits_len_tab2_cur[1] = bits_len2[3 + rl_chroma_table_index];
	bits_len_tab2_cur[2] = bits_len2[6 + rl_table_index];
	level_run_tab_cur[0] = level_run[rl_table_index];
	level_run_tab_cur[1] = level_run[3 + rl_chroma_table_index];
	level_run_tab_cur[2] = level_run[3 + rl_table_index];
	level_run_tab2_cur[0] = level_run2[rl_table_index];
	level_run_tab2_cur[1] = level_run2[3 + rl_chroma_table_index];
	level_run_tab2_cur[2] = level_run2[3 + rl_table_index];

	block_f = block_flags;

#ifdef USE_AC_CORRECTION
	memset(ac_val_base, 0, ac_size);
#endif

	for (; mb_y < mb_height; mb_y++)
	{
		divx_set_block_index();

		for (; mb_x < mb_width; mb_x++)
		{
			// process MB
#ifdef DUMP
			DEBUG_MSG("-[%d: %d %d] --------------------------------------------\n", frame_number, mb_x, mb_y);
#endif

			int pred_x, pred_y;
			DWORD cbp = 0;
			DWORD block_i;

#ifdef USE_AC_CORRECTION
			use_acpred = 1;
			
			if (!mb_intra)
			{
				qmul = qscale << 1;
				qadd = (qscale - 1) | 1;
				
#ifdef DUMP
				//DEBUG_MSG("qmul=%d, qadd=%d\n", qmul, qadd);
#endif
				
			} else
			{
				qmul = 1;
				qadd = 0;
			}

#endif

/*
if (frame_number == 0 && mb_x == 31 && mb_y == 16)
{
	int kk = 1;
}
*/
			pred_x = 0;
			pred_y = 0;
			mb_skip = FALSE;

			motion_x = 0;
			motion_y = 0;

			divx_update_block_index();

			if (pict_type == DIVX_I_TYPE)
			{
				BYTE *coded_val;
				int i;

				mb_intra = TRUE;
				divx_get_vlc2(code, dec_v, bitidx, left, mb_intra_vlc.table);
				
				for (i = 0; i < 6; i++) 
				{
					int val = ((code >> (5 - i)) & 1);
					if (i < 4) 
					{
						int pred = divx_coded_block_pred(i, &coded_val);
						val = val ^ pred;
						*coded_val = (BYTE)val;
					}
					cbp |= val << (5 - i);
				}
			}
			else if (pict_type == DIVX_P_TYPE)
			{
				if (use_skip_mb_code)
				{
					register DWORD ret;
					bitstream_get_bits1(ret, 0, dec_v, bitidx, left);
					if (ret) 
					{
						mb_intra = FALSE;
						mb_skip = TRUE;
					}
				}
				if (!mb_skip)
				{
					divx_get_vlc3(code, dec_v, bitidx, left, mb_non_intra_vlc.table);
					if (code < 0)
						return -1;
					mb_intra = ((~code & 0x40) >> 6) != 0;
					cbp = code & 0x3f;
				}
			}

			if (mb_intra)
			{
				bitstream_get_bits1(ac_pred, 0, dec_v, bitidx, left);
#ifdef DUMP
				DEBUG_MSG("CBP=%d, ACPRED=%d\n", cbp, ac_pred);
#endif

#ifdef USE_AC_CORRECTION
				use_acpred = 1;

				if (use_acpred && ac_pred)
				{
					if (divx_transcode_acpred_mb(enc_v, ebitidx, enc_len, dec_v, bitidx, left, cbp) < 0)
						return -1;
					continue;
				}
#endif

			} else
			{
#ifdef DUMP
				//DEBUG_MSG("CBP=%d, intra=%d\n", cbp, mb_intra);
#endif
				if (!mb_skip)
				{
					// predict motion
					divx_pred_motion(&pred_x, &pred_y);

					motion_x = pred_x;
					motion_y = pred_y;
					divx_decode_motion(dec_v, bitidx, left, motion_x, motion_y);
#ifdef DUMP					
					DEBUG_MSG("MOTION mx=%d my=%d\n", motion_x, motion_y);
#endif
				}

				if ((cbp | motion_x | motion_y) == 0)
					mb_skip = TRUE;

			}

			divx_encode_mb_header(enc_v, ebitidx, enc_len, cbp, mb_skip, ac_pred);

			if (!mb_intra && !mb_skip)
			{
				divx_encode_motion(enc_v, ebitidx, enc_len, motion_x, pred_x, f_code);
				divx_encode_motion(enc_v, ebitidx, enc_len, motion_y, pred_y, f_code);
			}

			for (block_i = 0; block_i < 6; block_i++) 
			{
				int ac_i, run_diff;
				int last_non_zero;
				int *dec_scan_table, *enc_scan_table;
				int *enc_inv_scan_table;

				DIVX_BITS_LEN *bits_len_tab;
				DWORD *level_run_tab, *level_run_tab2;
				DWORD *bits_len_tab2, *uni_tbl;

				DWORD last_scan = 0;
				DWORD coded = (cbp >> (5 - block_i)) & 1;
				BOOL the_same_scan_table = TRUE;
				int dec_dc_pred_dir, enc_dc_pred_dir;

				if (mb_intra)
				{
					int level, dc_level;
					DWORD tbl_idx;

					run_diff = 0;

					// decode dc
					if (block_i < 4) 
					{
						divx_get_vlc3(level, dec_v, bitidx, left, dc_lum_vlc[dc_table_index].table);
					}
					else 
					{
						divx_get_vlc3(level, dec_v, bitidx, left, dc_chroma_vlc[dc_table_index].table);
					}

					if (level < 0)
						return -1;
					if (level == 119) 
					{
						register DWORD ret;
						bitstream_get_bits(level, 0, dec_v, bitidx, left, 8);
						bitstream_get_bits1(ret, 0, dec_v, bitidx, left);
						if (ret)
							level = -level;
					} 
					else if (level != 0) 
					{
						register DWORD ret;
						bitstream_get_bits1(ret, 0, dec_v, bitidx, left);
						if (ret)
							level = -level;
					}

					dc_level = divx_pred_dc(block_i, level, &dec_dc_pred_dir, FALSE);
#ifdef DUMP
					DEBUG_MSG("-----------------\n");
					DEBUG_MSG("{%d}: DC LEVEL=%d / INTRA=%d\n", block_i, dc_level, mb_intra);
#endif
				
					level = divx_pred_dc(block_i, dc_level, &enc_dc_pred_dir, TRUE);
					divx_encode_dc(enc_v, ebitidx, enc_len, level, block_i);
					if (!coded) 
						continue;

					ac_i = 0;
					if (ac_pred && dec_dc_pred_dir != enc_dc_pred_dir)
						the_same_scan_table = FALSE;
#ifdef DUMP
					//DEBUG_MSG("  DIR: dec=%d, enc=%d\n", dec_dc_pred_dir, enc_dc_pred_dir);
#endif

					tbl_idx = block_i >> 2;
					bits_len_tab = bits_len_tab_cur[tbl_idx];
					bits_len_tab2 = bits_len_tab2_cur[tbl_idx];
					uni_tbl = uni_table[0];
					level_run_tab = level_run_tab_cur[tbl_idx];
					level_run_tab2 = level_run_tab2_cur[tbl_idx];
				} else
				{
					if (!coded) 
						continue;

					run_diff = 1;
					ac_i = -1;
					bits_len_tab = bits_len_tab_cur[2];
					bits_len_tab2 = bits_len_tab2_cur[2];
					uni_tbl = uni_table[1];
					level_run_tab = level_run_tab_cur[2];
					level_run_tab2 = level_run_tab2_cur[2];
				}
#ifdef USE_SLOW_METHOD_LIMIT
#ifdef USE_SLOW_METHOD
				if (frame_size > max_allowed_frame_size_for_correct_method)
#endif
					the_same_scan_table = TRUE;
#endif

#ifdef USE_ONLY_SLOW_METHOD
				if (the_same_scan_table)
				{
					enc_dc_pred_dir = dec_dc_pred_dir;
					the_same_scan_table = FALSE;
				}
#endif


				if (!the_same_scan_table)
				{
#ifdef USE_AC_CORRECTION
					// ????????????????????????????????
					dec_scan_table = ac_pred ? ((dec_dc_pred_dir == 0) ? 
						intra_v_scantable.permutated :
						intra_h_scantable.permutated) : intra_scantable.permutated;
					enc_scan_table = ac_pred ? ((enc_dc_pred_dir == 0) ? 
						intra_v_scantable.permutated :
						intra_h_scantable.permutated) : intra_scantable.permutated;
					enc_inv_scan_table = ac_pred ? ((enc_dc_pred_dir == 0) ? 
						intra_v_scantable.inv_permutated :
						intra_h_scantable.inv_permutated) : intra_scantable.inv_permutated;
#else
					dec_scan_table = (dec_dc_pred_dir == 0) ? 
						intra_v_scantable.permutated : // left
						intra_h_scantable.permutated; // top
					enc_scan_table = (enc_dc_pred_dir == 0) ? 
						intra_v_scantable.permutated : // left
						intra_h_scantable.permutated; // top
					enc_inv_scan_table = (enc_dc_pred_dir == 0) ? 
						intra_v_scantable.inv_permutated : // left
						intra_h_scantable.inv_permutated; // top
#endif
				}

				last_non_zero = ac_i;
				
				if (the_same_scan_table)
				{
					for(;;) 
					{
						register DWORD idx, bits, len;
						bitstream_show_bits(idx, dec_v, bitidx, 16);

						bits = bits_len_tab[idx>>2].bits;
						len = bits_len_tab[idx>>2].len;
						if (len >= 16)	// normal  (outlen < 16 bits, inlen < 14 bits)
						{
							bitstream_skip_bits(0, dec_v, bitidx, left, len & 0xf);
							len >>= 4;
							if (bits & 0x8000)	// last
							{
								bits &= ~0x8000;
								bitstream_put_bits(len, bits, enc_v, ebitidx, enc_len);
								break;
							}
							bitstream_put_bits(len, bits, enc_v, ebitidx, enc_len);
						}
						else	// extended || esc123
						{
							if (len == 0)	// extended (16 bits < outlen < 23 bits)
							{
								bits = bits_len_tab2[bits | (idx & 3)];
								len = ((bits >> 5) & 0xf) + 1;
								bitstream_skip_bits(0, dec_v, bitidx, left, len);
								len = (bits & 31);
								if (bits & 0x80000000)	// last
								{
									bits = (bits & ~(0x80000000)) >> 9;
									bitstream_put_bits(len, bits, enc_v, ebitidx, enc_len);
									break;
								}
								bitstream_put_bits(len, bits >> 9, enc_v, ebitidx, enc_len);
							}
							else if (len == 4)	// extended-"ESC4" (outlen >= 23 bits)
							{
								DWORD last;
								int level;
								bits = level_run_tab[idx>>2];
								// if extended (first VLC is longer than 14 bits)
								if ((bits >> 16) == 0)
									bits = level_run_tab2[bits | (idx & 3)];
								len = ((bits >> 18) & 0xf) + 1;
								bitstream_skip_bits(0, dec_v, bitidx, left, len);
								
								last = (bits & (1 << 17)) << 3;
								level = ((int)bits) >> 25;
								bits = ((bits & 63) << 14);
								bits += ((level & 0xfff) << 1) + last + ((3 << 23) + (3 << 21) + (1 << 13) + 1);
								bitstream_put_bits((7+2+1+6+1+12+1), bits, enc_v, ebitidx, enc_len);
								if (last)
									break;
							}
							else 
							{
								bitstream_skip_bits(0, dec_v, bitidx, left, bits);

								if (len == 3)	// esc3
								{
									int level;
									DWORD last;
									bitstream_get_bits(bits, 0, dec_v, bitidx, left, (1+6+8));

									level = (bits & 0x80) ? ((bits & 0xff) | 0xffffff00) : (bits & 0x7f);
									last = ((bits << 6) & 0x100000);
									
									bits = last + (((bits >> 8) & 63) << 14);
									bits += ((3 << 23) + (3 << 21) + (1 << 13) + 1) + ((level & 0xfff) << 1);

									bitstream_put_bits((7+2+1+6+1+12+1), bits, enc_v, ebitidx, enc_len);
									if (last)
										break;
								}
								else	// esc1,esc2
								{
									BOOL esc1 = (len & 1);
									int level;
									DWORD run, last;
									bitstream_show_bits(idx, dec_v, bitidx, 16);
									bits = level_run_tab[idx>>2];
									
									// if extended esc1/2 (first VLC is longer than 14 bits)
									if ((bits >> 16) == 0)
										bits = level_run_tab2[bits | (idx & 3)];

									len = ((bits >> 18) & 0xf) + 1;
									bitstream_skip_bits(0, dec_v, bitidx, left, len);

									/* bits = level(7)+reserved(3)+first_len(4)+first_last(1)+add_level(5)+add_run(6)+run(6) */
									level = ((int)bits) >> 25;
									run = (bits & 63) + 1;
									last = (bits & (1 << 17)) << 3;

									if (esc1)
									{
										level = level > 0 ? level + ((bits >> 12) & 31) : level - ((bits >> 12) & 31);
									} else
									{
										run += (bits >> 6) & 63;
										run += run_diff;
									}

									// now we have level, run, last - output in ESC3 mode
									bits = ((3 << 23) + (3 << 21) + (1 << 13) + 1) + last + ((run - 1) << 14) + ((level & 0xfff) << 1);
									
									// TODO: use uni_tab 
									bitstream_put_bits((7+2+1+6+1+12+1), bits, enc_v, ebitidx, enc_len);
									if (last)
										break;
								}
								continue;
							}
						}
					}
				}
				else	///////////////////////////////////////////////////////////
				{
#ifdef USE_SLOW_METHOD
					register int level;
					register DWORD run, i;

					while (ac_i < 64)
					{
						register DWORD idx, bits, len;
						bitstream_show_bits(idx, dec_v, bitidx, 16);

						bits = level_run_tab[idx>>2];
						
						if (bits == 0)	// esc123
						{
							bits = bits_len_tab[idx>>2].bits;
							len = bits_len_tab[idx>>2].len;
							bitstream_skip_bits(0, dec_v, bitidx, left, bits);
							
							if (len == 3)	// esc3
							{
								bitstream_get_bits(bits, 0, dec_v, bitidx, left, (1+6+8));
								level = (bits & 0x80) ? ((bits & 0xff) | 0xffffff00) : (bits & 0x7f);
								run = ((bits >> 8) & 63) + 1;
								if (bits & 0x4000)
									break;
							}
							else	// esc1,esc2
							{
								BOOL esc1 = (len & 1);
								bitstream_show_bits(idx, dec_v, bitidx, 16);
								bits = level_run_tab[idx>>2];
								if ((bits >> 16) == 0)
									bits = level_run_tab2[bits | (idx & 3)];
								len = ((bits >> 18) & 0xf) + 1;
								bitstream_skip_bits(0, dec_v, bitidx, left, len);
								
								level = ((int)bits) >> 25;
								run = (bits & 63) + 1;
								
								if (esc1)
								{
									level = level > 0 ? level + ((bits >> 12) & 31) : level - ((bits >> 12) & 31);
								} else
								{
									run += (bits >> 6) & 63;
									run += run_diff;
								}
								
								if (bits & (1 << 17))
									break;
							}
						} else
						{
							if ((bits >> 16) == 0)
								bits = level_run_tab2[bits | (idx & 3)];
							len = ((bits >> 18) & 0xf) + 1;
							bitstream_skip_bits(0, dec_v, bitidx, left, len);
							level = ((int)bits) >> 25;

							run = (bits & 63) + 1;
							if (bits & (1 << 17))
								break;
						}

						ac_i += run;
						idx = dec_scan_table[ac_i];
						bits = enc_inv_scan_table[idx];
						if (last_scan < bits)
							last_scan = bits;
						block[idx] = level;
						block_f[idx] = block_flag;
#ifdef DUMP
						//DEBUG_MSG("-> level=%d, run=%d\n", level, run);
						block_idx[idx] = ac_i;
						block_run[idx] = run;
#endif
					}


					ac_i += run;

#ifdef DUMP
					//DEBUG_MSG("-> level=%d, run=%d\n", level, run);
					block_idx[dec_scan_table[ac_i]] = ac_i;
					block_run[dec_scan_table[ac_i]] = run;
#endif

					run = dec_scan_table[ac_i];
					i = enc_inv_scan_table[run];
					if (last_scan < i)
						last_scan = i;
					block[run] = level;
					block_f[run] = block_flag;

					// we have reconstructed block at this moment...
#ifdef DUMP
					for (i = 0; i < 64; i++)
					{
						if (block_f[i] == block_flag)
						{
							//DEBUG_MSG("[%d] = %d (%d,run=%d)\n", i, block[i], block_idx[i], block_run[i]);
							DEBUG_MSG("[%d] = %d\n", i, block[i]);
						}
					}
#endif

					// now output the table
					level = 0;
					run = 0;

					for (i = 0; i <= last_scan; i++)
					{
						register DWORD eidx = enc_scan_table[i];

						if (block_f[eidx] == block_flag)
						{
							if (level && (int)run > last_non_zero)
							{
#if 0
								if (!((level + 64) & (~127))) // ESC3
								{
									idx = (DWORD)(level + 64) + (run - last_non_zero)*128;
									last_non_zero = run;
									idx = uni_tbl[idx];
									run = idx >> 24;
									idx &= 0xffffff;
									bitstream_put_bits(run, idx, enc_v, ebitidx, enc_len);
								}
								else
#endif
								{
									divx_put_rl_vlc_esc3(enc_v, ebitidx, enc_len, level, 0, run - last_non_zero);
									last_non_zero = run;
								}
							}
							level = block[eidx];
							run = i;
						}
					}
					
					if (level && (int)run > last_non_zero)
					{

#if 0
						if (!((level + 64) & (~127))) // ESC3
						{
							idx = 128*64 + (DWORD)(level + 64) + (run - last_non_zero)*128;
							idx = uni_tbl[idx];
							run = idx >> 24;
							idx &= 0xffffff;
							bitstream_put_bits(run, idx, enc_v, ebitidx, enc_len);
						} else
#endif
						{
							divx_put_rl_vlc_esc3(enc_v, ebitidx, enc_len, level, 1, run - last_non_zero);
						}
					}

#ifdef USE_AC_CORRECTION
					if (use_acpred && mb_intra)
					{
						divx_pred_flaged_ac(block_i, block, block_f, block_flag, dec_dc_pred_dir);
					}
#endif

					block_flag++;
#endif
				}
			}

			if (!mb_intra)
				divx_clean_intra_table_entries();
			divx_update_motion_val();
		}
		first_slice_line = 0;
		mb_x = 0;
	}
	// flush last output bits
	bitstream_flush_output(BITSTREAM_MODE_DONE, enc_v, ebitidx, enc_len);
	frame_number++;
	return 0;

}

