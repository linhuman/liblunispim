#ifndef	_VC_H_
#define	_VC_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <uchar.h>
typedef unsigned char BYTE;
typedef unsigned char byte;
typedef unsigned int UINT;
typedef int HANDLE ;
typedef int BOOL ;
typedef char16_t TCHAR;
#define TEOF (TCHAR)0xFFFF
#define TRUE 0
#define FALSE 1
#define TEXT(quote) u##quote
#define MAX_PATH 390
#define _MAX_PATH MAX_PATH
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

char * strncpy_s(char *dest, int dest_len, const char *src, int src_len);

char * strcpy_s(char *dest, int dest_len, const char *src);

TCHAR * _tcscpy_s(TCHAR *dest, int dest_len, const TCHAR *src);

TCHAR * _tcsncpy_s(TCHAR *dest, int dest_len, const TCHAR *src, int src_len);

char * strcat_s(char *dest, int dest_len, const char *src);

TCHAR * _tcscat_s(TCHAR *dest, int dest_len, const TCHAR *src);
//创建文件夹，相当于mkdir -p指令
int SHCreateDirectoryEx(char *sPathName);

int CopyFile(const char *from_file, const char *to_file);

int _tstoi(const char16_t *n);

TCHAR * _tcsncat_s(TCHAR *dest, int dest_len, const TCHAR *src, int src_len);

char * memcpy_s(char *dest, int dest_len, const char *src, int src_len);
//大写转小写
TCHAR * _tcslwr_s(TCHAR *str, int len);

int _tcsicmp(const TCHAR *s1, const TCHAR *s2);

char* u16tou8cat_s(char *dest, int dest_size, const TCHAR *src);
#ifdef __cplusplus
}
#endif
#endif
 
