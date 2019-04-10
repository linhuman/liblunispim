/*	工具头文件。
 *	装载log，file，以及内存管理的头文件。
 */

#ifndef	_UTILITY_H_
#define	_UTILITY_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <kernel.h>
#include <vc.h>
#include <wordlib.h>
#include <iconv.h>
#include <utf16char.h>
#include <string.h>
#include <debug.h>
#define _SizeOf(x)		(sizeof((x)) / sizeof((x)[0]))
#define _IsNoneASCII(x)	((WORD)x >= 0x2B0)
#define _HanZiLen		1

//最多申请的共享内存数量
#define	MAX_SHARED_MEMORY_COUNT			1024			//最多的共享对象

//键盘定义
#define	KEY_LSHIFT				(1 << 0)
#define	KEY_RSHIFT				(1 << 1)
#define	KEY_SHIFT				(1 << 2)
#define	KEY_LCONTROL			(1 << 3)
#define	KEY_RCONTROL			(1 << 4)
#define	KEY_CONTROL				(1 << 5)
#define	KEY_LALT				(1 << 6)
#define	KEY_RALT				(1 << 7)
#define	KEY_ALT					(1 << 8)
#define	KEY_CAPITAL				(1 << 9)

//共享内存的信息
typedef struct tagSHAREDMEMORYINFO
{
    //char	shared_name[MAX_PATH];		//词库的完整文件名字（包含路径）
	//int	handle;		//打开的文件句柄
	int	length;		//共享大小
    TCHAR	shared_name[0x20];	//词库共享名称
	void	*pointer;
} SHAREDMEMORYINFO;


//LOG相关结束

//文件处理相关
//从文件读入数据
extern int LoadFromFile(const char *file_name, void *buffer, int buffer_length);

//保存数据文件
extern int SaveToFile(const char *file_name, void *buffer, int buffer_length);

//获得文件长度
extern int GetFileLength(const char *file_name);

//文件处理结束

//获得当前系统tick
extern unsigned int GetCurrentTicks();

//复制部分字符串
extern void CopyPartString(TCHAR *target, const TCHAR *source, int length);

//输出汉字
extern void OutputHz(HZ hz);

//获得共享内存区域指针
extern void *GetSharedMemory(const char *shared_name);
extern void *GetReadOnlySharedMemory(const char *shared_name);

//创建共享内存区
extern void *AllocateSharedMemory(const char *shared_name, int length);

//释放共享内存区
void FreeSharedMemory(const char *shared_name, void *pointer);
/*
//窗口移动开始
extern void DragStart(HWND window);

//窗口移动
extern void DragMove(HWND window);

//窗口移动结束
extern void DragEnd(HWND window);

//获得当前屏幕的坐标
extern RECT GetMonitorRectFromPoint(POINT point);

//将第一个矩形放入第二个矩形中
extern void MakeRectInRect(RECT *in_rect, RECT out_rect);

//转换VK
extern void TranslateKey(UINT virtual_key, UINT scan_code, CONST LPBYTE key_state, int *key_flag, TCHAR *ch, int no_virtual_key);
*/
//判断文件是否存在
extern int FileExists(const char *file_name);

//获得当前用户的Application目录
extern TCHAR *GetUserAppDirectory(TCHAR *dir);

//获得文件的路径
char *GetFileFullName(const char *file_name, char *result);

//Ansi字符串转换到UTF16
extern int AnsiToUtf16(const char *chars, TCHAR *wchars, int nSize);

extern int Utf16ToAnsi(const TCHAR *uchars, char *chars, int nSize);

extern void UCS32ToUCS16(const UC UC32Char, TCHAR *buffer);

//判断一个4字节TChar数组，是由几个汉字组成的。返回值：0，1，2
extern int UCS16Len(TCHAR *buffer);

//从ucs16转为ucs32，只转一个汉字，多于一个汉字，返回0。
extern UC UCS16ToUCS32(TCHAR *buffer);

//在文件中读取一行数据
extern int GetLineFromFile(FILE *fr, TCHAR *line, int length);

//在文件中读取一个字符串（没有分隔符）
extern int GetStringFromFile(FILE *file, TCHAR *string, int length);

//除掉字符串首尾的空白符号
extern void TrimString(TCHAR *line);

//使字符串符合缓冲区大小的限制
extern void MakeStringFitLength(TCHAR *string, int length);

//获得当前系统时间
extern void GetTimeValue(int *year, int *month, int *day, int *hour, int *minute, int *second);

//执行程序
extern void ExecuateProgram(const TCHAR *program_name, const TCHAR *args, const int is_url);
extern void ExecuateProgramWithArgs(const TCHAR *cmd_line);

//对GZip文件进行解压缩
extern int UncompressFile(const TCHAR *name, const TCHAR *tag_name, int stop_length);
extern int CompressFile(const char *name, const char *tag_name);

//字符串/16进制字符串转换函数
extern int ArrayToHexString(const char *src, int src_length, char *tag, int tag_length);
extern int HexStringToArray(const char *src, char *tag, int tag_length);
extern int GetSign(const char *buffer, int buffer_length);

extern int IsFullScreen();

extern const TCHAR *GetProgramName();

extern void Lock();
extern void Unlock();

extern int PackStringToBuffer(TCHAR *str, int str_len, TCHAR *buffer, int buf_len);
extern int IsNumberString(TCHAR *candidate_str);
extern int LastCharIsAtChar(TCHAR *str);
extern char strMatch(char *src,char * pattern);
extern int IsNumpadKey(int virtual_key);
//原本的转码函数保留，新增编码转换相关函数IconvOpen, CodeConvert iconvClose，把iconv_open的函数独立出来，避免过多重复调用消耗时间
iconv_t IconvOpen(const char *from_charset, const char *to_charset);

int CodeConvert(iconv_t cd, char *inbuf, size_t inlen, char *outbuf, size_t outlen);

int IconvClose(iconv_t cd);

int code_convert(char *from_charset,char *to_charset, char *inbuf, size_t inlen, char *outbuf, size_t outlen);

int Utf16ToUtf8(const TCHAR *wchars, char *chars, int nSize);

int Utf8ToUtf16(const char *chars, TCHAR *wchars, int nSize);

//判断是否为64位系统
extern BOOL IsIME64();

int GetSharedMemoryLength(const char *shared_name);

#ifndef __arm__
inline unsigned long long GetCycleCount()
{
   __asm ("RDTSC");
}
#endif
//16进制字符串转整数
int htoi(TCHAR *s);
#ifdef __cplusplus
}
#endif

#endif
