//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - CSS auth. functions source file (Win32)
 *  \file       sp_css.cpp
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

#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <io.h>

#include "sp_misc.h"
#include "sp_khwl.h"
#include "sp_css.h"

typedef unsigned char uint8_t;

#include <contrib/libdvdcss/src/csstables.h>

void CryptKey( int i_key_type, int i_variant,
                      unsigned char const *p_challenge, unsigned char *p_key )
{
    /* Permutation table for challenge */
    unsigned char pp_perm_challenge[3][10] =
            { { 1, 3, 0, 7, 5, 2, 9, 6, 4, 8 },
              { 6, 1, 9, 3, 8, 5, 7, 4, 0, 2 },
              { 4, 0, 3, 5, 7, 2, 8, 6, 1, 9 } };

    /* Permutation table for variant table for key2 and buskey */
    unsigned char pp_perm_variant[2][32] =
            { { 0x0a, 0x08, 0x0e, 0x0c, 0x0b, 0x09, 0x0f, 0x0d,
                0x1a, 0x18, 0x1e, 0x1c, 0x1b, 0x19, 0x1f, 0x1d,
                0x02, 0x00, 0x06, 0x04, 0x03, 0x01, 0x07, 0x05,
                0x12, 0x10, 0x16, 0x14, 0x13, 0x11, 0x17, 0x15 },
              { 0x12, 0x1a, 0x16, 0x1e, 0x02, 0x0a, 0x06, 0x0e,
                0x10, 0x18, 0x14, 0x1c, 0x00, 0x08, 0x04, 0x0c,
                0x13, 0x1b, 0x17, 0x1f, 0x03, 0x0b, 0x07, 0x0f,
                0x11, 0x19, 0x15, 0x1d, 0x01, 0x09, 0x05, 0x0d } };

    unsigned char p_variants[32] =
            {   0xB7, 0x74, 0x85, 0xD0, 0xCC, 0xDB, 0xCA, 0x73,
                0x03, 0xFE, 0x31, 0x03, 0x52, 0xE0, 0xB7, 0x42,
                0x63, 0x16, 0xF2, 0x2A, 0x79, 0x52, 0xFF, 0x1B,
                0x7A, 0x11, 0xCA, 0x1A, 0x9B, 0x40, 0xAD, 0x01 };

    /* The "secret" key */
    unsigned char p_secret[5] = { 0x55, 0xD6, 0xC4, 0xC5, 0x28 };

    unsigned char p_bits[30], p_scratch[10], p_tmp1[5], p_tmp2[5];
    unsigned char i_lfsr0_o;  /* 1 bit used */
    unsigned char i_lfsr1_o;  /* 1 bit used */
    unsigned char i_css_variant, i_cse, i_index, i_combined, i_carry;
    unsigned char i_val = 0;
    unsigned long i_lfsr0, i_lfsr1;
    int i_term = 0;
    int i_bit;
    int i;

    for (i = 9; i >= 0; --i)
        p_scratch[i] = p_challenge[pp_perm_challenge[i_key_type][i]];

    i_css_variant = (unsigned char)(( i_key_type == 0 ) ? i_variant :
                    pp_perm_variant[i_key_type-1][i_variant]);

    /*
     * This encryption engine implements one of 32 variations
     * one the same theme depending upon the choice in the
     * variant parameter (0 - 31).
     *
     * The algorithm itself manipulates a 40 bit input into
     * a 40 bit output.
     * The parameter 'input' is 80 bits.  It consists of
     * the 40 bit input value that is to be encrypted followed
     * by a 40 bit seed value for the pseudo random number
     * generators.
     */

    /* Feed the secret into the input values such that
     * we alter the seed to the LFSR's used above,  then
     * generate the bits to play with.
     */
    for( i = 5 ; --i >= 0 ; )
    {
        p_tmp1[i] = (unsigned char)(p_scratch[5 + i] ^ p_secret[i] ^ p_crypt_tab2[i]);
    }

    /*
     * We use two LFSR's (seeded from some of the input data bytes) to
     * generate two streams of pseudo-random bits.  These two bit streams
     * are then combined by simply adding with carry to generate a final
     * sequence of pseudo-random bits which is stored in the buffer that
     * 'output' points to the end of - len is the size of this buffer.
     *
     * The first LFSR is of degree 25,  and has a polynomial of:
     * x^13 + x^5 + x^4 + x^1 + 1
     *
     * The second LSFR is of degree 17,  and has a (primitive) polynomial of:
     * x^15 + x^1 + 1
     *
     * I don't know if these polynomials are primitive modulo 2,  and thus
     * represent maximal-period LFSR's.
     *
     *
     * Note that we take the output of each LFSR from the new shifted in
     * bit,  not the old shifted out bit.  Thus for ease of use the LFSR's
     * are implemented in bit reversed order.
     *
     */

    /* In order to ensure that the LFSR works we need to ensure that the
     * initial values are non-zero.  Thus when we initialise them from
     * the seed,  we ensure that a bit is set.
     */
    i_lfsr0 = ( p_tmp1[0] << 17 ) | ( p_tmp1[1] << 9 ) |
              (( p_tmp1[2] & ~7 ) << 1 ) | 8 | ( p_tmp1[2] & 7 );
    i_lfsr1 = ( p_tmp1[3] << 9 ) | 0x100 | p_tmp1[4];

    i_index = sizeof(p_bits);
    i_carry = 0;

    do
    {
        for( i_bit = 0, i_val = 0 ; i_bit < 8 ; ++i_bit )
        {

            i_lfsr0_o = (unsigned char)(( ( i_lfsr0 >> 24 ) ^ ( i_lfsr0 >> 21 ) ^
                        ( i_lfsr0 >> 20 ) ^ ( i_lfsr0 >> 12 ) ) & 1);
            i_lfsr0 = ( i_lfsr0 << 1 ) | i_lfsr0_o;

            i_lfsr1_o = (unsigned char)(( ( i_lfsr1 >> 16 ) ^ ( i_lfsr1 >> 2 ) ) & 1);
            i_lfsr1 = ( i_lfsr1 << 1 ) | i_lfsr1_o;

            i_combined = (unsigned char)(!i_lfsr1_o + i_carry + !i_lfsr0_o);
            /* taking bit 1 */
            i_carry = (unsigned char)(( i_combined >> 1 ) & 1);
            i_val |= ( i_combined & 1 ) << i_bit;
        }

        p_bits[--i_index] = i_val;
    } while( i_index > 0 );

    /* This term is used throughout the following to
     * select one of 32 different variations on the
     * algorithm.
     */
    i_cse = (unsigned char)(p_variants[i_css_variant] ^ p_crypt_tab2[i_css_variant]);

    /* Now the actual blocks doing the encryption.  Each
     * of these works on 40 bits at a time and are quite
     * similar.
     */
    i_index = 0;
    for( i = 5, i_term = 0 ; --i >= 0 ; i_term = p_scratch[i] )
    {
        i_index = (unsigned char)(p_bits[25 + i] ^ p_scratch[i]);
        i_index = (unsigned char)(p_crypt_tab1[i_index] ^ ~p_crypt_tab2[i_index] ^ i_cse);

        p_tmp1[i] = (unsigned char)(p_crypt_tab2[i_index] ^ p_crypt_tab3[i_index] ^ i_term);
    }
    p_tmp1[4] ^= p_tmp1[0];

    for( i = 5, i_term = 0 ; --i >= 0 ; i_term = p_tmp1[i] )
    {
        i_index = (unsigned char)(p_bits[20 + i] ^ p_tmp1[i]);
        i_index = (unsigned char)(p_crypt_tab1[i_index] ^ ~p_crypt_tab2[i_index] ^ i_cse);

        p_tmp2[i] = (unsigned char)(p_crypt_tab2[i_index] ^ p_crypt_tab3[i_index] ^ i_term);
    }
    p_tmp2[4] ^= p_tmp2[0];

    for( i = 5, i_term = 0 ; --i >= 0 ; i_term = p_tmp2[i] )
    {
        i_index = (unsigned char)(p_bits[15 + i] ^ p_tmp2[i]);
        i_index = (unsigned char)(p_crypt_tab1[i_index] ^ ~p_crypt_tab2[i_index] ^ i_cse);
        i_index = (unsigned char)(p_crypt_tab2[i_index] ^ p_crypt_tab3[i_index] ^ i_term);

        p_tmp1[i] = (unsigned char)(p_crypt_tab0[i_index] ^ p_crypt_tab2[i_index]);
    }
    p_tmp1[4] ^= p_tmp1[0];

    for( i = 5, i_term = 0 ; --i >= 0 ; i_term = p_tmp1[i] )
    {
        i_index = (unsigned char)(p_bits[10 + i] ^ p_tmp1[i]);
        i_index = (unsigned char)(p_crypt_tab1[i_index] ^ ~p_crypt_tab2[i_index] ^ i_cse);

        i_index = (unsigned char)(p_crypt_tab2[i_index] ^ p_crypt_tab3[i_index] ^ i_term);

        p_tmp2[i] = (unsigned char)(p_crypt_tab0[i_index] ^ p_crypt_tab2[i_index]);
    }
    p_tmp2[4] ^= p_tmp2[0];

    for( i = 5, i_term = 0 ; --i >= 0 ; i_term = p_tmp2[i] )
    {
        i_index = (unsigned char)(p_bits[5 + i] ^ p_tmp2[i]);
        i_index = (unsigned char)(p_crypt_tab1[i_index] ^ ~p_crypt_tab2[i_index] ^ i_cse);

        p_tmp1[i] = (unsigned char)(p_crypt_tab2[i_index] ^ p_crypt_tab3[i_index] ^ i_term);
    }
    p_tmp1[4] ^= p_tmp1[0];

    for(i = 5, i_term = 0 ; --i >= 0 ; i_term = p_tmp1[i] )
    {
        i_index = (unsigned char)(p_bits[i] ^ p_tmp1[i]);
        i_index = (unsigned char)(p_crypt_tab1[i_index] ^ ~p_crypt_tab2[i_index] ^ i_cse);

        p_key[i] = (unsigned char)(p_crypt_tab2[i_index] ^ p_crypt_tab3[i_index] ^ i_term);
    }

    return;
}
