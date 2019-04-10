#include <utf16char.h>
#include <vc.h>
#include <stdio.h>  
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
TCHAR * _tcsncpy_s(TCHAR *dest, int dest_size, const TCHAR *src, int src_len)
{
	int length;
	if(src_len >= dest_size)
		length = dest_size - 1;
	else
		length = src_len;
	dest[length] = 0;
	return utf16_strncpy(dest, src, length);
}
TCHAR * _tcsncat_s(TCHAR *dest, int dest_size, const TCHAR *src, int src_len)
{
	
	int length;
	int dest_len = utf16_strlen(dest);
	if(src_len >= dest_size - dest_len)
		length = dest_size - dest_len - 1;
	else
		length = src_len;
	dest[length + dest_len] = 0;
	
	
	return utf16_strncat(dest, src, length);
}
TCHAR * _tcscpy_s(TCHAR *dest, int dest_size, const TCHAR *src)
{
	int length;
	int src_len = utf16_strlen(src);
	if(src_len >= dest_size)
		length = dest_size - 1;
	else
		length = src_len;
	dest[length] = 0;
	return utf16_strncpy(dest, src, length);
}

char * strncpy_s(char *dest, int dest_size, const char *src, int src_len)
{
	int length;
	if(src_len >= dest_size)
		length = dest_size - 1;
	else
		length = src_len;
	dest[length] = 0;	
	return strncpy(dest, src, length);
	
}
char * strcpy_s(char *dest, int dest_size, const char *src)
{
	int length;
	int src_len = strlen(src);
	if(src_len >= dest_size)
		length = dest_size - 1;
	else
		length = src_len;
	dest[length] = 0;
	return strncpy(dest, src, length);
}

char * strcat_s(char *dest, int dest_size, const char *src)
{
	int length;
	int dest_len = strlen(dest);
	int src_len = strlen(src);
	if(src_len >= dest_size - dest_len)
		length = dest_size - dest_len - 1;
	else
		length = src_len;
	dest[length + dest_len] = 0;	
	return strncat(dest, src, length);
}
/*
 * 函数只是简单的把tchar强制转换成char，然后连接到目标字符串，
 * 只能用于英文字符的操作
 */
char* u16tou8cat_s(char *dest, int dest_size, const TCHAR *src)
{
    int length;
    int dest_len = strlen(dest);
    int src_len = utf16_strlen(src);
    if(src_len >= dest_size - dest_len)
        length = dest_size - dest_len - 1;
    else
        length = src_len;
    dest[length + dest_len] = 0;
    char* dest_ptr = dest + strlen(dest);
    for(; length > 0 && (*dest_ptr = (char)*src) != 0; dest_ptr++, src++, length--)
        ;
    if(length == 0) *dest_ptr = 0;
    return dest;
}
TCHAR * _tcscat_s(TCHAR *dest, int dest_size, const TCHAR *src)
{
	int length;
	int src_len = utf16_strlen(src);
	int dest_len = utf16_strlen(dest);
	if(src_len >= dest_size - dest_len)
		length = dest_size - dest_len - 1;
	else
		length = src_len;
    dest[dest_len + length] = 0;
	return utf16_strncat(dest, src, length);
}
//创建文件夹，相当于mkdir -p指令
int SHCreateDirectoryEx(char *sPathName)
{
	char DirName[MAX_PATH];
	int i,len;
	strcpy(DirName,sPathName);
	len = strlen(DirName);
	if('/' != DirName[len-1]){
		strcat(DirName,"/");
		len++;
	}
	for(i = 1;i < len; i++){
		if('/' == DirName[i]){
			DirName[i] = '\0';
			if(access(DirName,F_OK) != 0){
				if(mkdir(DirName,0777) == -1){
					return 0;
				}
			}
			DirName[i] = '/';
		}
	}
	return 1;
}
int CopyFile(const char *from_file, const char *to_file)
{
	char buff[1024];
	FILE *in, *out;
    int cout = 0;
    size_t len;
	char const *src = from_file;
	char const *des = to_file;
    in = fopen(src, "r");
    if(!in){
        return -1;
	}
    out = fopen(des, "w+");
    if(!out){
        return -2;
    }
    while(len = fread(buff, sizeof(char), sizeof(buff), in)){
        cout += fwrite(buff, sizeof(char), len, out);
	}
    printf("cout:%d\n", cout);
	return cout;
}
int _tstoi(const char16_t *n)
{
	return utf16_atoi(n);
}

char * memcpy_s(char *dest, int dest_size, const char *src, int src_len)
{
	
	int length;
	if(src_len > dest_size)
		length = dest_size;
	else
		length = src_len;
	return memcpy(dest, src, length);
}

TCHAR * _tcslwr_s(TCHAR *str, int len)
{
	TCHAR *p = str;
	if(str == NULL)
		return NULL;
	
	while(*p != u'\0' && len > 0){
		if(*p >= u'A' && *p <= u'Z'){
			*p = (*p) + (TCHAR)0x20;
		}
		p++;
		len--;
	}
	return str;
}

int _tcsicmp(const TCHAR *s1, const TCHAR *s2)
{
	while(tolower(*s1) == tolower(*s2)){
		if(*s1 == u'\0'){
			return 0;
		}
		s1++;
		s2++;
	}
	return (tolower(*s1) - tolower(*s2));
}


