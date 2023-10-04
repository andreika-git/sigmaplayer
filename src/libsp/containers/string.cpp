//////////////////////////////////////////////////////////////////////////
/**
 * support lib string implementation
 *	\file		libsp/containers/string.cpp
 *	\author		bombur et al.
 *	\version	x.xx
 *	\date		xx.xx.xxxx
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

#include <stdio.h>
#include <alloca.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <malloc.h>

//#define USE_MEMBIN

#ifndef USE_MEMBIN
#define membin_alloc(s, n) SPrealloc(s, n)
#define membin_free(s) SPfree(s)
#else
#include <libsp/containers/membin.h>
#endif

#ifdef _MSC_VER

#pragma warning(disable : 4706)

#else

char * strlwr(char *string)
{
    char *str = string;
    if (str == NULL)
        return NULL;

    while (*str != 0)
    {
        *str = tolower(*str);
        str++;
    }

    return string;
}

char *  strupr    (char * string)
{
    char *str = string;
    if (str == NULL)
        return NULL;

    while (*str != 0)
    {
        *str = toupper(*str);
        str++;
    }

    return string;
}

char *  strrev    (char * string)
{
    char *start = string;
    char *left = string;

    while (*string++)
        ;
    string -= 2;

    while (left < string)
    {
        char ch = *left;
        *left++ = *string;
        *string-- = ch;
    }

    return (start);
}

char *  strdup  (char * string)
{
	char *memory;
	if (!string)
		return(NULL);
	if ((memory = (char *)SPmalloc(strlen(string) + 1)) != NULL)
		return strcpy(memory, string);
	return NULL;
}

int strcasecmp (char *dst, char *src)
{
	int f, l;
	do 
	{
		f = tolower((unsigned char)(*(dst++)));
		l = tolower((unsigned char)(*(src++)));
	} while (f != 0 && (f == l));
	return(f - l);
}

#endif

SPString::~SPString ()
{
	if (str != NULL)
		membin_free(str);
	str = NULL;
}

void SPString::Printf (const char *string, ...)
{
	va_list l;
	va_start(l, string);
	char *tmp = (char *)SPalloca (4096);	// in stack
	vsprintf(tmp, string, l);
	Insert(GetLength()+1, tmp);
	va_end(l);
}

void SPString::Strftime (const char *string, time_t tim)
{
	struct tm *tmtim = localtime(&tim);
	char *tmp = (char *)SPalloca (4096);	// in stack
	strftime(tmp, 4096, string, tmtim);
	Insert(GetLength()+1, tmp);
}

void SPString::Realloc()
{
	str = (char *)membin_alloc(str, (max + 1) * sizeof(char));
}

// Compare helpers
bool operator== (const SPString & s1, const SPString & s2)
{
	return s1.Compare(*s2) == 0;
}
bool operator== (const SPString & s1, const char *s2)
{
	if (s2 == NULL)
	{
		return (s1.str == NULL);
	}
	return s1.Compare(s2) == 0;
}
bool operator== (const char *s1, const SPString & s2)
{
	return s2.Compare(s1) == 0;
}

bool operator!= (const SPString & s1, const SPString & s2)
{
	return s1.Compare(*s2) != 0;
}
bool operator!= (const SPString & s1, const char *s2)
{
	if (s2 == NULL)
		return s1.str != NULL;
	return s1.Compare(s2) != 0;
}
bool operator!= (const char *s1, const SPString & s2)
{
	return s2.Compare(s1) != 0;
}

bool operator< (const SPString & s1, const SPString & s2)
{
	return s1.Compare(*s2) < 0;
}
bool operator< (const SPString & s1, const char *s2)
{
	return s1.Compare(s2) < 0;
}
bool operator< (const char *s1, const SPString & s2)
{
	return s2.Compare(s1) > 0;
}

bool operator> (const SPString & s1, const SPString & s2)
{
	return s1.Compare(*s2) > 0;
}
bool operator> (const SPString & s1, const char *s2)
{
	return s1.Compare(s2) > 0;
}
bool operator> (const char *s1, const SPString & s2)
{
	return s2.Compare(s1) < 0;
}

bool operator<= (const SPString & s1, const SPString & s2)
{
	return s1.Compare(*s2) <= 0;
}
bool operator<= (const SPString & s1, const char *s2)
{
	return s1.Compare(s2) <= 0;
}
bool operator<= (const char *s1, const SPString & s2)
{
	return s2.Compare(s1) >= 0;
}

bool operator>= (const SPString & s1, const SPString & s2)
{
	return s1.Compare(*s2) >= 0;
}
bool operator>= (const SPString & s1, const char *s2)
{
	return s1.Compare(s2) >= 0;
}
bool operator>= (const char *s1, const SPString & s2)
{
	return s2.Compare(s1) <= 0;
}


//////////////////////////////////////////////////////////

static void similar_str(const char *txt1, int len1, const char *txt2, int len2, int *pos1, int *pos2, int *max)
{
	char *end1 = (char *)txt1 + len1;
	char *end2 = (char *)txt2 + len2;
	int l;
	
	*max = 0;
	for (char *p = (char *)txt1; p < end1; p++) 
	{
		for (char *q = (char *)txt2; q < end2; q++) 
		{
			for (l = 0; p + l < end1 && q + l < end2 && p[l] == q[l]; l++)
				;
			if (l > *max) 
			{
				*max = l;
				*pos1 = p - txt1;
				*pos2 = q - txt2;
			}
		}
	}
}

static int similar_char(const char *txt1, int len1, const char *txt2, int len2)
{
	int sum;
	int pos1, pos2, max;
	
	similar_str(txt1, len1, txt2, len2, &pos1, &pos2, &max);
	if ((sum = max))
	{
		if (pos1 && pos2) 
		{
			sum += similar_char(txt1, pos1, txt2, pos2);
		}
		if (pos1 + max < len1 && pos2 + max < len2)
		{
			sum += similar_char(txt1 + pos1 + max, len1 - pos1 - max, txt2 + pos2 + max, len2 - pos2 - max);
		}
	}
	return sum;
}
	
int SPString::Similar(const SPString & s2)
{
	if (len == 0 || s2.len == 0)
		return 0;
	int num = similar_char(str, len, s2.str, s2.len);
	return num * 200 / (len + s2.len);
}
