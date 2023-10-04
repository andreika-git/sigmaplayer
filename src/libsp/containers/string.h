//////////////////////////////////////////////////////////////////////////
/**
 * support lib string header
 *	\file		libsp/containers/string.h
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

#ifndef SP_STRING_H
#define SP_STRING_H

#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>

#define GROW(num) (9 * num / 8)

#ifdef _MSC_VER
#pragma warning (disable : 4239) 
#endif

/// String manipulation class
class SPString
{
public:
	/// empty ctor
	SPString ()
	{
		str = NULL;
		len = 0;
		max = 0;
	}

    /// dtor
   	~SPString ();

	/// copy ctor
	SPString (const SPString & src)
	{
		str = NULL;
		len = 0;
		max = 0;

		if (src.len > 0)
		{
			Add(src.len);
			if (src.str != NULL)
				strcpy(str, src.str);
		}
		// don't copy mbstr
	}

	/// from a single character
	SPString (char ch, int repeat = 1)
	{
		str = NULL;
		len = 0;
		max = 0;

		if (repeat < 0)
			repeat = 0;
		Add(repeat);
		int i;
		for (i = 0; i < repeat; i++)
			str[i] = ch;
		str[i] = '\0';
	}

	/// from a string (converts to char)
	SPString (char *src)
	{
		str = NULL;
		len = 0;
		max = 0;

		if (src != NULL)
		{
			Add(strlen(src));
			strcpy(str, src);
		}
	}

	/// from a string (converts to char)
	SPString (const char *src)
	{
		str = NULL;
		len = 0;
		max = 0;

		if (src != NULL)
		{
			Add(strlen(src));
			strcpy(str, src);
		}
	}

	/// subset of characters from an string (converts to char)
	SPString (char *pitch, int inLen)
	{
		str = NULL;
		len = 0;
		max = 0;

		if (pitch != NULL && inLen > 0)
		{
			Add(inLen);
			int i;
			for(i = 0; i < len; i++)
				str[i] = pitch[i];
			str[i] = '\0';
		}
	}

	// Attributes & Operations

	/// get data length
	int GetLength () const
	{
		return len;
	}

	/// TRUE if zero length
	BOOL IsEmpty () const
	{
		return !len;
	}

	/// clear contents to empty
	void Empty ()
	{
		len = 0;
	}

	/// cleans addition memory allocated
	void Shrink ()
	{
		max = len;
		Realloc();
	}

	/// return single character at zero-based index
	char GetAt (int index) const
	{
		if (index < 0 || index > len)
			return *str;
		return *(str + index);
	}

	/// return single character at zero-based index
	char & operator [] (int index) const
	{
		if (index < 0 || index > len)
			return *str;
		return *(str + index);
	}
	
	/// set a single character at zero-based index
	void SetAt (int index, char ch)
	{
		if (index < 0 || index >= len)
			*(str + index) = ch;
	}
	
	/// return pointer to const string (will be freed in dtor)
	const char* operator * () const
	{
		return len ? str : "";
	}

	/// return pointer to const string (will be freed in dtor)
	operator char* () const
	{
		return len ? str : (char *)"";
	}

	// overloaded assignment

	/// ref-counted copy from another SPString
	const SPString & operator = (const SPString & src)
	{
		if (this != &src)
		{
			len = max = src.len;
			Realloc();
			if (len > 0)
			{
				strcpy(str, src.str);
			}
		}
		return *this;
	}

	/// set string content to single character
	const SPString & operator = (char ch)
	{
		len = max = 1;
		Realloc();
		*str = ch;
		str[1] = '\0';
		return *this;
	}

	/// copy string content from string
	const SPString & operator = (const char *src)
	{
		if (str != src)
		{
			for (len = 0; *(src + len) != '\0'; len++)
				;
			max = len;
			Realloc();
			if (len != 0)
				strcpy(str, src);
		}
		return *this;
	}

	// string concatenation

	/// concatenate a character after converting it to char
	const SPString & operator += (const char *src)
	{
		if (max != 0)
		{
			int index = len;
			Add(strlen(src));
			strcpy(str + index, src);
		}
		else if (*src != '\0')
		{
			Add(strlen(src));
			strcpy(str, src);
		}
		return *this;

	}
	
	/// concatenate from another SPString
	const SPString & operator += (const SPString & src)
	{
		return operator+=(*src);
	}
	
	friend SPString operator + (const SPString & string1, const SPString & string2)
	{
		return SPString(string1) += string2;
	}

	friend SPString operator + (const SPString & string, const char *str)
	{
		return SPString(string) += str;
	}

	friend SPString operator + (const char *str, const SPString & string)
	{
		return SPString((char*)str) += string;
	}

	// string comparison

	/// straight character comparison
	int Compare (const SPString & string) const
	{
		if (len == 0)
		{
			if (string.str == NULL || string.len < 1)
				return 0;
			return string.str[0] == '\0' ? 0 : 1;
		}
		else if (string.len == 0 || string.str == NULL)
			return -1;
		return strcmp(str, string.str);
	}

	/// compare ignoring case
	int CompareNoCase (const SPString & string) const
	{
		if (len == 0)
		{
			if (string.str == NULL || string.len < 1)
				return 0;
			return string.str[0] == '\0' ? 0 : 1;
		}
		else if (string.len == 0 || string.str == NULL)
			return -1;
		return strcasecmp(str, string.str);
	}
	
	// simple sub-string extraction

	/// return count characters starting at zero-based first
	SPString Mid (int first, int count) const
	{
		if (first < 0)
			first = 0;
		if (first >= len)
			return SPString();
		if (first + count > len)
			count = len - first;
		return SPString(str + first, count);
	}

	/// return all characters starting at zero-based first
	SPString Mid (int first) const
	{
		if (first < 0)
			first = 0;
		if (first >= len)
			return SPString();
		return SPString(str + first);
	}

	/// return first count characters in string
	SPString Left (int count) const
	{
		if (count > len)
			return SPString(str, len);
		return SPString(str, count);
	}

	/// return nCount characters from end of string
	SPString Right (int count) const
	{
		if (count > len)
			return SPString();
		return SPString(str + len - count, count);
	}

	// upper/lower/reverse conversion
#ifdef _MSC_VER
	/// NLS aware conversion to uppercase
	void MakeUpper ()
	{
		if (str == NULL)
			return;
		_strupr(str);
	}

	/// NLS aware conversion to lowercase
	void MakeLower ()
	{
		if (str == NULL)
			return;
		_strlwr(str);
	}

	/// reverse string right-to-left
	void MakeReverse ()
	{
		if (str == NULL)
			return;
		_strrev(str);
	}
#endif
	// trimming anything (either side)

	/// remove continuous occurrences of target starting from right
	void TrimLeft (char target = ' ')
	{
		if (str == NULL)
			return;
		int i;
		for (i = 0; i < len && str[i] == target; i++)
			;
		if (i != 0)
		{
			memmove(str, str + i, (len - i + 1) * sizeof(char));
			len -= i;
		}
	}

	/// remove continuous occurrences of target starting from left
	void TrimRight (char target = ' ')
	{
		if (str == NULL)
			return;
		int i;
		for (i = len - 1; i > 0 && str[i] == target; i--)
			;
		len = i + 1;
		str[len] = '\0';
	}

	/// remove continuous occurrences of one of charSet starting from right
	void TrimLeft (const char *charSet)
	{
		if (str == NULL)
			return;
		int i;
		for (i = 0; i < len; i++)
		{
			bool found = false;
			for (int j = 0; charSet[j] != '\0'; j++)
			{
				if (str[i] == charSet[j])
				{
					found = true;
					break;
				}
			}
			if (!found)
				break;
		}

		if (i != 0)
		{
			memmove(str, str + i, (len - i + 1) * sizeof(char));
			len -= i;
		}
	}

	/// remove continuous occurrences of one of charSet starting from left
	void TrimRight (const char *charSet)
	{
		if (str == NULL)
			return;
		int i;
		for (i = len - 1; i; i--)
		{
			bool found = false;
			for (int j = 0; charSet[j]; j++)
			{
				if (str[i] == charSet[j])
				{
					found = true;
					break;
				}
			}
			if (!found)
				break;
		}
		len = i + 1;
		str[len] = '\0';
	}

	void TrimLines(int numlines)
	{
		if (str == NULL)
			return;
		int j = 0;
		for (int i = 0; str[j] && i < numlines; i++)
		{
			for(; j < len && str[j] != '\n'; j++)
				;
			if (str[j] == '\n')
				j++;
		}
		len = j;
		str[len] = '\0';
	}

	// advanced manipulation

	/// replace occurrences of oldS with newS
	int Replace (char oldS, char newS)
	{
		if (str == NULL)
			return 0;
		int ret = 0;
		for (int i = 0; i < len; i++)
		{
			if (str[i] == oldS)
			{
				str[i] = newS;
				ret++;
			}
		}
		return ret;
	}

	/// replace occurrences of substring oldString with newString;
	/// empty newString removes instances of oldString
	int Replace (const char *oldString, const char *newString)
	{
		if (str == NULL)
			return 0;
		int last = 0;
		int ret = 0;
		int oldLen = strlen(oldString);
        int newLen = strlen(newString);

		while (last < len && (last = Find(oldString, last)) != -1)
		{
			Delete(last, oldLen);
			Insert(last, newString);
			ret++;
			last += newLen;
		}
		return ret;
	}

	/// insert substring at zero-based index; concatenates
	/// if index is past end of string
	/// add to existing string
	int Insert (int index, const char *string)
	{
		if (index < 0)
			index = 0;

		int stringLen = strlen(string);
		if (len == 0)
		{
			Add(stringLen);
			strcpy(str, string);
			return index;
		}

		if (index >= len)
			index = len;

		int l = len - index + 1;

		Add(stringLen);
		
		memmove(str + index + stringLen, str + index, l * sizeof(char));
		memmove(str + index, string, stringLen * sizeof(char));

		return index;
	}

	/// delete count characters starting at zero-based index
	int Delete (int index, int count = 1)
	{
		if (index >= len || index < 0)
			return -1;
		if (index + count > len)
		{
			len = index;
			return count;
		}
		memmove(str + index, str + index + count, (len - index - count + 1) * sizeof(char));
		len -= count;
		return count;
	}

	// searching

	/// find character starting at left, -1 if not found
	int Find (char ch) const
	{
		for (int i = 0; i < len; i++)
		{
			if (str[i] == ch)
				return i;
		}
		return -1;
	}

	/// find character starting at right
	int ReverseFind (char ch) const
	{
		for(int i = len - 1; i >= 0; i--)
		{
			if (str[i] == ch)
				return i;
		}
		return -1;
	}

	/// find character starting at zero-based index and going right
	int Find (char ch, int start) const
	{
		for (int i = start; i < len; i++)
		{
			if (str[i] == ch)
				return i;
		}
		return -1;
	}

	/// find first instance of any character in passed string
	int FindOneOf (const char *charSet) const
	{
		int charSetLen = strlen(charSet);
		for (int i = 0; i<len; i++)
		{
			for (int j = 0; j < charSetLen; j++)
			{
				if (str[i] == charSet[j])
					return i;
			}
		}
		return -1;
	}

	/// find the first character not contained in passed string
	int FindNoneOf (const char *charSet, int start) const
	{
		int charSetLen = strlen(charSet);
		for (int i = start; i < len; i++)
		{
			bool notfound = true;
			for (int j = 0; j < charSetLen; j++)
			{
				if (str[i] == charSet[j])
				{
					notfound = false;
					break;
				}
			}
		    if (notfound)
		    	return i;
		}
		return -1;
	}

	/// reverse find first instance of any character in passed string
	int ReverseFindOneOf (const char *charSet) const
	{
		int charSetLen = strlen(charSet);
		for (int i = len - 1; i >= 0; i--)
		{
			for (int j = 0; j < charSetLen; j++)
			{
				if (str[i] == charSet[j])
					return i;
			}
		}
		return -1;
	}

	/// reverse find the first character not contained in passed string
	int ReverseFindNoneOf (const char *charSet, int start = -1) const
	{
		int charSetLen = strlen(charSet);
		if (start == -1)
			start = len - 1;
		for (int i = start; i >= 0; i--)
		{
			bool notfound = true;
			for (int j = 0; j < charSetLen; j++)
			{
				if (str[i] == charSet[j])
				{
					notfound = false;
					break;
				}
			}
		    if (notfound)
		    	return i;
		}
		return -1;
	}

	/// find first instance of substring starting at zero-based index
	int Find (const char *string, int start = 0) const
	{
		if (start < 0 || start >= len || string == NULL)
			return -1;

		char *ret = strstr(str + start, string);
		return  ret ? ret - str : -1;
	}

	/// find first instance of substring starting at zero-based index
	int FindNoCase (const char *string, int start = 0) const
	{
		if (start < 0 || start >= len || string == NULL)
			return -1;
        
		char *cp = str + start;
        char *s1, *s2;

        if (!*string)
            return start;

        while (*cp)
        {
            s1 = cp;
            s2 = (char *) string;

            while (*s1 && *s2 && !(tolower(*s1) - tolower(*s2)))
				s1++, s2++;
            if (!*s2)
				return cp - str;
            cp++;
        }
        return -1;
	}

	/// calculate djb2 (Bernstein) string hash
	DWORD Hash() const
	{
		const char *s = str;
        DWORD hash = 5381;
        int c;
		if (len > 0)
		{
			while ((c = *s++) != '\0')
				hash = ((hash << 5) + hash) + c; // = hash * 33 + c;
		}
        return hash;
	}

	// simple formatting

	/// printf-like formatting using passed string
	void Printf (const char *string, ...);
	void Strftime (const char *string, time_t tim);

	/// Set string from integer
	void FromInteger(int i)
	{
		if (max < 12)
		{
			max = 12;
			Realloc();
		}
		char *s = str;
		if (i < 0)
		{
			*(s++) = '-';
			i = -i;
		}
		char *b = s;
		do 
		{
			*s++ = (char)((i % 10) + '0');
            i /= 10;
        } while (i > 0);
		len = s - str;
        *s-- = '\0';
        do 
		{
            char temp = *s;
            *s = *b;
            *b = temp;
            --s;
            ++b;
        } while (b < s);
	}

	/// Calculate the similarity between two strings, in percents. [algorithm (c) Oliver, 1993]
	/// \Warning! Recursive!
	int Similar(const SPString & s2);

		
protected:
	char *str;	/// pointer to ref counted string data
	int	len;	/// length in chars
	int	max;	/// max length in chars

	/// reallocates memory
	void Realloc();

	/// adds count symbols, returns index of new first symbol
	int Add(int count)
	{
		if (count < 0 || len < 0 || max < len)
			return -1;

		int index = len;
		if ((len += count) >= max)
		{
			// allocate more memory
			max = GROW(len) + 8;
			Realloc();
		}
		return index;
	}

protected:
	friend bool operator == (const SPString &, const char *);
	friend bool operator != (const SPString &, const char *);
};

/*
static int SPListStringsCompare(SPString *e1, SPString *e2)
{
	return e1->Compare(*e2);
}

static int SPListStringsCompareNoCase(SPString *e1, SPString *e2)
{
	return e1->CompareNoCase(*e2);
}
*/

// Compare helpers
bool operator== (const SPString & s1, const SPString & s2);
bool operator== (const SPString & s1, const char *s2);
bool operator== (const char *s1, const SPString & s2);
bool operator!= (const SPString & s1, const SPString & s2);
bool operator!= (const SPString & s1, const char *s2);
bool operator!= (const char *s1, const SPString & s2);
bool operator<  (const SPString & s1, const SPString & s2);
bool operator<  (const SPString & s1, const char *s2);
bool operator<  (const char *s1, const SPString & s2);
bool operator>  (const SPString & s1, const SPString & s2);
bool operator>  (const SPString & s1, const char *s2);
bool operator>  (const char *s1, const SPString & s2);
bool operator<= (const SPString & s1, const SPString & s2);
bool operator<= (const SPString & s1, const char *s2);
bool operator<= (const char *s1, const SPString & s2);
bool operator>= (const SPString & s1, const SPString & s2);
bool operator>= (const SPString & s1, const char *s2);
bool operator>= (const char *s1, const SPString & s2);

#endif // of SP_STRING_H
