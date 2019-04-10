/* 
   Copyright (C) 2003, 2006-2007, 2009-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2003.

   This program is free software: you can redistribute it and/or
   modify it under the terms of either:

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at your
       option) any later version.

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at your
       option) any later version.

   or both in parallel, as here.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#define CANON_ELEMENT(c) c
#include <stdlib.h>
#include <utf16char.h>
#include <utility.h>
#include <stdarg.h>
#include <unistr.h>
#include <malloca.h>
#include <str-kmp.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif

int utf16_atoi(const char16_t *utf16_str) {
	if (NULL == utf16_str)
		return 0;

	int value = 0;
	int sign = 1;
	size_t pos = 0;

	if ((char16_t)'-' == utf16_str[pos]) {
		sign = -1;
		pos++;
	}

	while ((char16_t)'0' <=  utf16_str[pos] &&
			(char16_t)'9' >= utf16_str[pos]) {
		value = value * 10 + (int)(utf16_str[pos] - (char16_t)'0');
		pos++;
	}

	return value*sign;
}

size_t utf16_strlen(const char16_t *utf16_str) {
    if (NULL == utf16_str)
        return 0;

	size_t size = 0;
    while ((char16_t)'\0' != utf16_str[size])
        size++;
	return size;
}


int utf16_strcmp(const char16_t* str1, const char16_t* str2) {
	size_t pos = 0;
	while (str1[pos] == str2[pos] && (char16_t)'\0' != str1[pos])
		pos++;

	return (int)(str1[pos]) - (int)(str2[pos]);
}

int utf16_strncmp(const char16_t *str1, const char16_t *str2, size_t size) {
	size_t pos = 0;
	while (pos < size && str1[pos] == str2[pos] && (char16_t)'\0' != str1[pos])
		pos++;

	if (pos == size)
		return 0;

	return (int)(str1[pos]) - (int)(str2[pos]);
}

// we do not consider overlapping
char16_t* utf16_strcpy(char16_t *dst, const char16_t *src) {
	if (NULL == src || NULL == dst)
		return NULL;

	char16_t* cp = dst;

	while ((char16_t)'\0' != *src) {
		*cp = *src;
		cp++;
		src++;
	}

	*cp = *src;

	return dst;
}

char16_t* utf16_strncpy(char16_t *dst, const char16_t *src, size_t size) {
	if (NULL == src || NULL == dst || 0 == size)
		return NULL;

	if (src == dst)
		return dst;

	char16_t* cp = dst;

	if (dst < src || (dst > src && dst >= src + size)) {
		while (size-- && (*cp++ = *src++))
			;
	} else {
		cp += size - 1;
		src += size - 1;
		while (size-- && (*cp-- == *src--))
			;
	}
	return dst;
}

char16_t * utf16_strchr(const char16_t *utf16_str, char16_t ch)
{
	for(; *utf16_str; utf16_str++){
		if(*utf16_str == ch){
			return (char16_t*)utf16_str;
		}
	}
	return NULL;
}

char16_t * utf16_strrchr(const char16_t *utf16_str, char16_t ch)
{
	char16_t *result = NULL;
	for(; *utf16_str; utf16_str++){
		if(*utf16_str == ch){
			result = (char16_t*)utf16_str;
		}
	}
	return result;
}
int utf16_strspn(const char16_t *utf16_str, const char16_t *accept)
{
	const char16_t *ptr = utf16_str;
	if(accept[0] == 0) return 0;
	for(; *ptr; ptr ++){
		if(!utf16_strchr(accept, *ptr))
			return ptr - utf16_str;
	}
	return utf16_strlen(utf16_str);

}
char16_t * utf16_strpbrk(const char16_t *utf16_str, const char16_t *accept)
{
	if(accept[0] == 0) return NULL;
	{
		if(accept[1] == 0)
			return utf16_strchr(utf16_str, accept[0]);
	}


	{
		for(; *utf16_str; utf16_str++){
			if(utf16_strchr(accept, *utf16_str))
				return (char16_t*)utf16_str;
		}
	}
	return NULL;

}
char16_t * utf16_strtok(char16_t *utf16_str, const char16_t *delim, char16_t **save_ptr)
{

	if(utf16_str == NULL){
		utf16_str = *save_ptr;
		if(utf16_str == NULL) return NULL;
	}
	utf16_str += utf16_strspn(utf16_str, delim);
	if(*utf16_str == 0){
		*save_ptr = NULL;
		return NULL;
	}

	{
		char16_t *token = utf16_strpbrk(utf16_str, delim);
		if(token){
			*save_ptr = token + 1;
			*token = 0;
		}else{
			*save_ptr = NULL;
		}

	}
	return utf16_str;
}
size_t utf16_strnlen(const char16_t *utf16_str, size_t maxlen)
{
	const char16_t *ptr = utf16_str;
	for(; maxlen > 0 && *ptr; ptr++, maxlen++)
		;
	return ptr - utf16_str;
}

char16_t * utf16_strstr(const char16_t *haystack, const char16_t *needle)
{
	if(needle[0] == 0) return (char16_t*) haystack;

	{
		if (needle[1] == 0){
			return utf16_strchr(haystack, needle[0]);
		}
	}

	{
		bool try_kmp = true;
		size_t outer_loop_count = 0;
		size_t comparison_count = 0;
		size_t last_ccount = 0;
		const char16_t *needle_last_ccount = needle;
		char16_t b = *needle++;
		for (;; haystack++)
		{
			if (*haystack == 0)
				/* No match.  */
				return NULL;

			/* See whether it's advisable to use an asymptotically faster
			   algorithm.  */
			if (try_kmp
					&& outer_loop_count >= 10
					&& comparison_count >= 5 * outer_loop_count)
			{
				/* See if needle + comparison_count now reaches the end of
			       needle.  */
				if (needle_last_ccount != NULL)
				{
					needle_last_ccount +=
							utf16_strnlen (needle_last_ccount,
								       comparison_count - last_ccount);
					if (*needle_last_ccount == 0)
						needle_last_ccount = NULL;
					last_ccount = comparison_count;
				}
				if (needle_last_ccount == NULL)
				{
					/* Try the Knuth-Morris-Pratt algorithm.  */
					const char16_t *result;
					bool success =
							knuth_morris_pratt (haystack,
									    needle - 1, utf16_strlen(needle - 1),
									    &result);
					if (success)
						return (char16_t *) result;
					try_kmp = false;
				}
			}

			outer_loop_count++;
			comparison_count++;
			if (*haystack == b)
				/* The first character matches.  */
			{
				const char16_t *rhaystack = haystack + 1;
				const char16_t *rneedle = needle;

				for (;; rhaystack++, rneedle++)
				{
					if (*rneedle == 0)
						/* Found a match.  */
						return (char16_t *) haystack;
					if (*rhaystack == 0)
						/* No match.  */
						return NULL;
					comparison_count++;
					if (*rhaystack != *rneedle)
						/* Nothing in this round.  */
						break;
				}
			}
		}

	}
}
char16_t * utf16_strcat(char16_t *dest, const char16_t *src)
{
	char16_t *dest_ptr = dest + utf16_strlen(dest);
	for(; (*dest_ptr = *src) != 0; dest_ptr++,src++)
		;
	return dest;
}
char16_t * utf16_strncat(char16_t *dest, const char16_t *src, size_t n)
{
	char16_t *dest_ptr = dest + utf16_strlen(dest);
	for(; n > 0 && (*dest_ptr = *src) != 0; dest_ptr++, src++, n--)
		;
	if(n == 0) *dest_ptr = 0;
	return dest;


}
char16_t * utf16_strdup(const char16_t *utf16_str)
{
	size_t n = utf16_strlen(utf16_str) + 1;
	char16_t *dest;
	dest = (char16_t*)malloc(n * sizeof(char16_t));
	if(dest != NULL)
		memcpy((char*)dest, (const char *)utf16_str, n * sizeof(char16_t));

	return dest;
}

//一个简单的fprintf功能函数，只满足项目需求，实现简单的格式化指令
int utf16_minfprintf(FILE *stream, char16_t *fmt, ...)
{
	va_list ap;
	char16_t buffer[1024] = {0};
	char u8_buf[3076] = {0};
	char number[30] = {0};
	char16_t *p, *sval, *bp, ch;
	char *np;
	int ival, result;
	double dval;
	bp = buffer;
	va_start(ap, fmt);

	for(p = fmt; *p; p++){
		if(*p != u'%' && bp < (buffer + 1023)){
			*bp++ = *p;
			continue;
		}else if(!(bp < (buffer + 1023))){
			break;
		}
		switch (*++p) {
		case 'd':
			ival = va_arg(ap, int);
			sprintf(number, "%d", ival);
			for(np = number; *np && bp < (buffer + 1023); np++){
				*bp++ = (char16_t)*np;
			}
			break;
		case 'c':
			ch = (char16_t)va_arg(ap, int);
			*bp++ = ch;
			break;
		case 'f':
			dval = va_arg(ap, double);
			sprintf(number, "%f", dval);
			for(np = number; *np && bp < (buffer + 1023); np++){
				*bp++ = (char16_t)*np;
			}
			break;
		case 's':
			for(sval = va_arg(ap, char16_t*); sval && *sval && bp < (buffer + 1023); sval++){
				*bp++ = *sval;
			}
			break;
		case 'x':
			ival = va_arg(ap, int);
			sprintf(number, "%x", ival);
			for(np = number; *np && bp < (buffer + 1023); np++){
				*bp++ = (char16_t)*np;
			}
			break;
		default:
			break;
		}
	}
	if (stream == stdout || stream == stderr) {
		Utf16ToUtf8(buffer, u8_buf, 3072);
		result = fprintf(stream, u8_buf);
	} else {
		result = fwrite(buffer, 2, utf16_strlen(buffer), stream);
	}
	return result;
}

#ifdef __cplusplus
}
#endif

