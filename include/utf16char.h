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

#ifndef _UTF16CHAR_H_
#define _UTF16CHAR_H_

#include <stdlib.h>
#include <uchar.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

// Get a token from utf16_str,
// Returned pointer is a '\0'-terminated utf16 string, or NULL
// *utf16_str_next returns the next part of the string for further tokenizing

char16_t * utf16_strtok(char16_t *utf16_str, const char16_t *delim, char16_t **save_ptr);

int utf16_atoi(const char16_t *utf16_str);

size_t utf16_strlen(const char16_t *utf16_str);
size_t utf16_strlen_test(const char16_t *utf16_str);
int utf16_strcmp(const char16_t *str1, const char16_t *str2);
int utf16_strncmp(const char16_t *str1, const char16_t *str2, size_t size);

char16_t* utf16_strcpy(char16_t *dst, const char16_t *src);
char16_t* utf16_strncpy(char16_t *dst, const char16_t *src, size_t size);

char16_t * utf16_strrchr(const char16_t *utf16_str, char16_t ch);
char16_t * utf16_strchr(const char16_t *utf16_str, char16_t ch);
char16_t * utf16_strpbrk(const char16_t *utf16_str, const char16_t *accept);
char16_t * utf16_strstr(const char16_t *haystack, const char16_t *needle);
char16_t * utf16_strcat(char16_t *dest, const char16_t *src);
char16_t * utf16_strncat(char16_t *dest, const char16_t *src, size_t n);
char16_t * utf16_strdup(const char16_t *utf16_str);

int utf16_minfprintf(FILE *stream, char16_t *fmt, ...);


#ifdef __cplusplus
}
#endif


#endif  
