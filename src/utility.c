#include <sys/types.h>
#include <sys/ipc.h>  
#include <sys/shm.h>  
#include <utility.h>
#include <utf16char.h>
#include <sys/stat.h>  
#include <fcntl.h>
#include <unistd.h>
#include <vc.h>
#include <stdlib.h> 
#include <time.h>
#include <map_file.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistdio.h>
#include <unistr.h>
#include <string.h>
#include <shared_memory.h>
#include <debug.h>
#include <config.h>
#include <errno.h>
#define READ_LENGTH 1024
//static SHAREDMEMORYINFO shared_info[MAX_SHARED_MEMORY_COUNT] = { 0, };

/*	存储共享内存句柄。
 *	参数：
 * 		shared_name		共享内存名称
 *		pointer			指针
 * 		length			共享大小
 *	返回：无
 */
/*
static void StoreSharedMemoryHandle(const TCHAR *shared_name, void *pointer, int length)
{
	int i;
	//找到一个空位
	for (i = 0; i < MAX_SHARED_MEMORY_COUNT; i++)
		if (shared_info[i].pointer == 0)
		{
            //strcpy_s(shared_info[i].shared_name, MAX_PATH, shared_name);
            utf16_strcpy(shared_info[i].shared_name, shared_name);
			//strncpy(shared_info[i].lib_name, lib_name, WORDLIB_FILE_NAME_LENGTH);
			//shared_info[i].handle = handle;
			shared_info[i].pointer = pointer;
			shared_info[i].length = length;
			return;
		}
}
*/
/*	获得文件长度。
 *	参数：
 *		file_name			文件名称
 *	返回：
 *		文件长度，-1标识出错。
 */
int GetFileLength(const char *file_name)
{
	struct stat f_data;

	if (stat(file_name, &f_data))
		return -1;

	return (int) f_data.st_size;
}


/*	获得共享内存区域指针。
 *	参数：
 *		shared_name			共享内存名称
 *	返回：
 *		没有找到：0
 *		找到：指向内存的指针
 */
void *GetSharedMemory(const char *shared_name)
{


	char *p = NULL;
    p = GetSharedMemoryAddress(shared_name);
    return p;
    /*
    int shmkey = 0, shmid = -1;
	shmkey = GetShmKey(shared_name);

	if( (shmid = shmget(shmkey, 0, (SHM_R | SHM_W) | IPC_CREAT)) < 0){
		#if __DEBUG
        U16_DEBUG_ECHO("获取共享内存失败, 共享名:%s, shmkey:%x, shmid:%d\n", shared_name, shmkey, shmid);
		#endif
		return 0;
	}
	if( (intptr_t)(p = shmat(shmid, 0, 0)) < 0 ){
		#if __DEBUG
        U16_DEBUG_ECHO("获取共享内存失败，共享名:%s, p:%d\n", shared_name, p);

		#endif
		return 0;
    }
*/
	/*
	for (i = 0; i < MAX_SHARED_MEMORY_COUNT; i++){
		if(!utf16_strcmp(shared_info[i].shared_name, shared_name)){
			p = shared_info[i].pointer;
			break;
		}
	}
	*/
}

/*	创建共享内存区
 *	参数：
 * 		lib_name			词库的完整文件名字（包含路径）
 *		shared_name			共享内存名称
 *		length				共享内存长度
 *	返回：
 *		创建失败：0
 *		成功：指向内存的指针
 */
void* AllocateSharedMemory(const char *shared_name, int length)
{
	
	//int handle = 0;
	char *p = NULL;
    /*
	int key = 0, shmid = 0;
	key = GetShmKey(shared_name);
	if( (shmid = shmget(key, length, (SHM_R | SHM_W) | IPC_CREAT)) < 0 ){
		U16_DEBUG_ECHO("创建共享失败，获取shmid失败，共享名称:%s，长度:%d, shmid:%d", shared_name, length, shmid);
		return 0;
	}
	if( (intptr_t)(p = shmat(shmid, 0, 0)) < 0 ){
		U16_DEBUG_ECHO("创建共享失败，获取指针失败，共享名称:%s，长度:%d, 指针:%p", shared_name, length, p);
		return 0;
	}
	U16_DEBUG_ECHO("创建共享成功，共享名称:%s，长度:%d, shmkey:0x%x, shmid:%d", shared_name, length, key, shmid);
    */
    //StoreSharedMemoryHandle(shared_name, p, length);
	/*
	//handle = open(lib_name, O_RDWR | O_CREAT, 0644);
	if (handle == -1)
	{
		printf("创建共享失败，共享名称:%s，长度:%d, err=%d", shared_name, length, handle);
		return 0;
	}
	lseek(handle, length - 1, SEEK_SET);
	write(handle, "", 1);
	p = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, handle, 0);  
	
	if (!p)
	{
		printf("文件映射失败，共享名称:%s，句柄:%d, 长度:%d\n", shared_name, handle, length);
	}
	else
	{
		StoreSharedMemoryHandle(lib_name, handle, shared_name, p, length);
		printf("分配共享内存成功，长度:%d\n", length);
	}
	*/
    p = CreateSharedMemoryReadWrite(shared_name, SHARED_TYPE_SHARED_MEMORY, length);

	return p;
}

int GetSharedMemoryLength(const char *shared_name)
{

    int size;
    size = GetSharedMemorySize(shared_name);
    return size;
}
/*	释放共享内存区
 *	参数：
 *		shared_name			共享内存名称
 *		pointer				共享内存指针
 *	返回：无
 */
void FreeSharedMemory(const char *shared_name, void *pointer)
{
/*
    int i, shmkey, shmid;
	shmkey = GetShmKey(shared_name);
	shmid = shmget(shmkey, 0, (SHM_R | SHM_W) | IPC_CREAT);			
	shmctl(shmid, IPC_RMID, 0);
    */
    /*
	for (i = 0; i < MAX_SHARED_MEMORY_COUNT; i++)
        if (!utf16_strcmp(shared_info[i].shared_name, shared_name))
		{
            memset(shared_info[i].shared_name, 0, sizeof(shared_info[i].shared_name[0])*MAX_PATH);	//shared_name是char16_t类型
			shared_info[i].pointer = 0;
			shared_info[i].length = 0;
			return;
		}
        */
    DEBUG_ECHO("CloseSharedMemory(shared_name): %s", shared_name);
    CloseSharedMemory(shared_name);
    DEBUG_ECHO("CloseSharedMemory(shared_name): %d", CloseSharedMemory(shared_name));
}

/*	Ansi字符串转换到UTF16
 */
int AnsiToUtf16(const char *chars, TCHAR *wchars, int nSize)
{
	size_t len;
	int result;
    TCHAR temp[nSize + 2];
    len = strlen(chars);
    result = code_convert("GB18030","UTF-16",(char*)chars, len, (char*)temp, (nSize + 2)*2);    //转换成utf16后，字符串开头会有个0xFEFF字符，而结尾需要存结束符u'\0',所以nSize需要加2
    if(temp[0] == 0xFEFF){
	utf16_strcpy(wchars, (temp + 1));
    }
    return result;
}

/**	获得一行文本，并且除掉尾回车
 */
int GetLineFromFile(FILE *fr, TCHAR *line, int length)
{
	TCHAR *p = line, ch;
	ch = (unsigned short)fgetc(fr) % 0x100 + (unsigned short)fgetc(fr) * 0x100;
	if (TEOF == ch)
		return 0;

	while (length-- && ch != TEOF)
	{
		
		*p = ch;
		if (0xa == ch)
			break;

		ch = (unsigned short)fgetc(fr) % 0x100 + (unsigned short)fgetc(fr) * 0x100;
		if (0xd == *p && 0xa == ch)	//0xa是回车
			break;

		p++;
	}

	*p = 0;

	return 1;
}

/**	从文件中读出一个字符串
 *	参数：
 *		file		文件指针
 *		string		读入的字符串缓冲区
 *		length		缓冲区长度
 *	返回：
 *		字符串的长度
 */
int GetStringFromFile(FILE *file, TCHAR *string, int length)
{
	TCHAR ch = 0x09;
	int count = 0;

	*string = 0;

	/* 过滤分隔符 */
	while(0x09 == ch || 0xd == ch || 0xa == ch)
		ch = (unsigned short)fgetc(file) % 0x100 + (unsigned short)fgetc(file) * 0x100;

	if (TEOF == ch)
		return 0;

	while (length-- && ch != TEOF && 0x09 != ch && 0xd != ch && 0xa != ch)
	{
		*string++ = ch;
		count++;

		ch = (unsigned short)fgetc(file) % 0x100 + (unsigned short)fgetc(file) * 0x100;
	}

	*string = 0;

	return count;
}

/*	带有长度的复制字符串（为了进行移植的需要）。
 *	参数：
 *		target				目标地址
 *		source				源地址
 *	返回：无
*/
void CopyPartString(TCHAR *target, const TCHAR *source, int length)
{
	int i;

	for (i = 0; i < length && source[i]; i++)
		target[i] = source[i];

	target[i] = 0;
}

int GetShmKey(const TCHAR *shared_name)
{
	int shmid = 0;
	int i;
	for(i=0; shared_name[i]; i++){
		shmid += (short unsigned)shared_name[i];
	}
	return shmid;
}
/*	获得文件的绝对路径。
 *	存储于Config中的文件名字，可以为相对路径名字。
 *	如："theme/abc.jpg"，在/usr/app/unispim6目录中无法找到
 *	的话，则在/alluser/app/unispim6/中寻找，如果还找不到，则返回0
 *	如果文件为绝对路径，则找到直接返回。
 *	参数：
 *		file_name
 *		result;
 *	返回：
 *		0：没有找到文件（/usr、/allusr下皆没有）
 *		其他：新文件名（即原来的result）
 */
char *GetFileFullName(const char *file_name, char *result)
{
    char dir[MAX_PATH] = {0};
    strcpy(dir, pim_config->user_data_dir);
    strcat(dir, "/");
    strcat(dir, file_name);
    strcpy(result, dir);
	return result;
}

/*	返回系统的Tick数（从系统启动开始的时钟步数）
 *	参数：无
 *	返回：
 *		当前系统的TickCount
 */
unsigned int GetCurrentTicks()
{
	struct timespec ts = {0, 0};
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

int Utf16ToAnsi(const TCHAR *ucahrs, char *chars, int nSize)
{
	size_t len;
	int result;
	len = utf16_strlen(ucahrs);
	result = code_convert("UTF-16","GB18030",(char*)ucahrs, len*2, (char*)chars, nSize);
	return result;
}
void UCS32ToUCS16(const UC UC32Char, TCHAR *buffer)
{
	buffer[1] = 0;

	if (UC32Char > 0x10FFFF || (UC32Char >= 0xD800 && UC32Char <= 0xDFFF))
	{
		buffer[0] = '?';
		return;
	}

	if (UC32Char < 0x10000)
		buffer[0] = (TCHAR)UC32Char;
	else
	{
		buffer[0] = (UC32Char - 0x10000) / 0x400 + 0xD800;
		buffer[1] = (UC32Char - 0x10000) % 0x400 + 0xDC00;
		buffer[2] = 0;
	}
}

/**	将过长的字符串压缩到buffer中，压缩长度为缓冲区的大小
 *	压缩方式：在中间加入4个.两头保持。
 *	如：     北京紫光华宇软件股份有限公司
 *	压缩为： 北京紫光华....有限公司
 *	返回：
 *		压缩后的长度
 */
int PackStringToBuffer(TCHAR *str, int str_len, TCHAR *buffer, int buf_len)
{
	int index, last_index;
	int side_length;

	if (str_len <= buf_len)
	{
		_tcscpy_s(buffer, buf_len + 1, str);
		return str_len;
	}

	side_length = buf_len / 2 - 2;

	//找到边界
	last_index = index = 0;
	while(index < side_length)
	{
		last_index = index;
		index++;
	};

	if (index == side_length)
		last_index = index;

	for (index = 0; index < last_index; index++)
		*buffer++ = str[index];

	*buffer++ = '.';
	*buffer++ = '.';
	*buffer++ = '.';
	*buffer++ = '.';

	while(index < str_len - side_length)
		index++;

	for (; index < str_len; index++)
		*buffer++ = str[index];

	*buffer++ = 0;

	return (int)utf16_strlen(buffer);
}
/*	将文件装载到缓冲区。
 *	请注意：缓冲区的大小必须满足文件读取的要求，不能小于文件的长度。
 *	参数：
 *		file_name			文件全路径名称
 *		buffer				缓冲区
 *		buffer_length		缓冲区长度
 *	返回：
 *		成功：返回读取的长度，失败：-1
 */
int LoadFromFile(const char *file_name, void *buffer, int buffer_length)
{
	FILE *fd;
	int length;
	
	fd = fopen(file_name, "rb");
	if (!fd)
	{
        DEBUG_ECHO("文件打开失败: %s", strerror(errno));
		return 0;
	}

	length = (int)fread(buffer, 1, buffer_length, fd);
	fclose(fd);

	if (length < 0)
	{
        DEBUG_ECHO("文件读取失败: %s", strerror(errno));
		return 0;
	}
	return length;
}

/*	将内存保存到文件。如果目标存在，则覆盖。
 *	参数：
 *		file_name			文件全路径名称
 *		buffer				缓冲区指针
 *		buffer_length		文件长度
 *	返回：
 *		成功：1，失败：0
 */
int SaveToFile(const char *file_name, void *buffer, int buffer_length)
{
	FILE *fd;
	int length;
	
	DEBUG_ECHO("保存内存%p到文件%s，长度：%d", buffer, file_name, buffer_length);
	
	fd = fopen(file_name, "wb");
	if (!fd)
	{
		char dir_name[MAX_PATH];
		int  i, index, ret;

		printf("文件打开失败");

		//可能需要创建目录
		//1. 寻找当前文件的目录
		index = 0;
		strcpy(dir_name, file_name);
		for (i = 0; dir_name[i]; i++)
			if (dir_name[i] == '/')
				index = i;

		if (!index)
			return 0;

		dir_name[index] = 0;		//dir_name中包含有目录名字
		ret = SHCreateDirectoryEx(dir_name);
		if (ret != 1)
			return 0;

		//创建目录成功，再次打开
		fd = fopen(file_name, "wb");
		if (!fd)
			return 0;
	}
	length = (int)fwrite(buffer, 1, buffer_length, fd);
	fclose(fd);
	
	if (length != buffer_length)
	{
        DEBUG_ECHO("文件写入失败");
		return 0;
	}

    DEBUG_ECHO("文件写入成功");
	return length;
}
/*	判断文件是否存在。
 */
int FileExists(const char *file_name)
{
	if (access(file_name, F_OK) == 0)
		return 1;

	return 0;
}
 void GetTimeValue(int *year, int *month, int *day, int *hour, int *minute, int *second)
 {
     struct timespec time = {0, 0};
     struct tm nowtime;
     clock_gettime(CLOCK_REALTIME, &time);  //获取相对于1970到现在的秒数
     localtime_r(&(time.tv_sec), &nowtime);
     *year = nowtime.tm_year + 1900;
     *month = nowtime.tm_mon + 1;
     *day = nowtime.tm_mday;
     *hour = nowtime.tm_hour;
     *minute = nowtime.tm_min;
     *second = nowtime.tm_sec;
 }

//16进制字符串转整数
int htoi(TCHAR *s)  
{  
    unsigned int hexdigit, i, inhex, n;  
  
    i = 0;  
    if(s[i] == '0') //skip optional 0x or 0X  
    {  
        ++i;  
        if(s[i] == 'x' || s[i] == 'X')  
        {  
            ++i;  
        }  
    }  
    n = 0;  //interger value to be returned  
    inhex = 1;    //assume valid hexadecimal digit  
    for( ; inhex == 1; ++i)  
    {  
        if(s[i] >= '0' && s[i] <= '9')  
            hexdigit = s[i] - '0';  
        else if(s[i] >= 'a' && s[i] <= 'f')  
            hexdigit = s[i] - 'a' + 10;  
        else if(s[i] >= 'A' && s[i] <= 'F')  
            hexdigit = s[i] - 'A' + 10;  
        else  
            inhex = 0;  //not a valid hexadecimal digit  
        if(inhex == 1)  
            n = 16 * n + hexdigit;  
    }  
    return n;  
}

/*	输出汉字。
 *	参数：
 *		hz			汉字
 *	返回：无
 */
void OutputHz(HZ hz)
{
	printf("%c%c", hz % 0x100, hz / 0x100);
}
/**	删除字符串中的首尾空白字符
 */
void TrimString(TCHAR *line)
{
	TCHAR *p;
	TCHAR *start, *end;
	int  i;

	if (!*line)
		return;

	p = line;

	//除掉首部空白
	while(*p)
	{
		if (*p != 0xd && *p != 0xa && *p != ' ' && *p != 0x9)
			break;

		p++;
	}

	start = end = p;

	while(*p)
	{
		if (*p != 0xd && *p != 0xa && *p != ' ' && *p != 0x9)
			end = p;

		p++;
	}

	for (i = 0; i <= end - start; i++)
		line[i] = start[i];

	line[i] = 0;
}

char strMatch(char *src, char * pattern)
{
	int i, j, ilen1, ilen2, ipos, isfirst;

	//初始化各种变量
	ilen1 = (int)strlen(src);
	ilen2 = (int)strlen(pattern);

	i = j = isfirst = 0;
	ipos  = -1;

	//比对到字符串末尾退出
	while(i < ilen1 && j < ilen2)
	{
		//如果匹配，则i、j向后移动
		if(tolower(src[i]) == tolower(pattern[j]) || pattern[j] == '?')
        {
            ++i;
            ++j;
        }
		//如果遇到*，则j向后移动，记录*号出现位置，并设置isfirst为1（避免死循环）
		else if(pattern[j] == '*')
		{
			++j;
			ipos	= j;
			isfirst	= 1;
		}
		//在有星号的情况下，如果不匹配，则退回j，并i向后移动
		else if(ipos >= 0)
		{
			if (isfirst)
				isfirst = 0;
			else
				++i;

			j = ipos;
		}
		//不匹配、无*号，则退出
		else
			return 0;
	}

	//pattern用尽，则表示匹配
	if (j == ilen2)
		return 1;

	//pattern没用尽，src用尽了，如果pattern剩下的都是*，则匹配
	else if (i == ilen1)
	{
		//这里临时借用isfirst、ipos变量，不新定义变量了
		isfirst = 1;

		for (ipos = j; ipos < ilen2; ipos++)
		{
			if (pattern[ipos] != '*')
			{
				isfirst = 0;
				break;
			}
		}

		return isfirst;
	}
	else
		return 0;
}

//转码核心函数
int code_convert(char *from_charset, char *to_charset, char *inbuf, size_t inlen, char *outbuf, size_t outlen) {
	long long starttime, endtime;
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;
	size_t result;
	cd = iconv_open(to_charset, from_charset);

	if (cd == 0) {
		return 0;
	}
	memset(outbuf, 0, outlen);

	if ((result = iconv(cd, pin, &inlen, pout, &outlen)) == -1) {
		return 0;
	}
	iconv_close(cd);
	return result;
}

//原本的转码函数保留，新增编码转换相关函数IconvOpen, CodeConvert iconvClose，把iconv_open的函数独立出来，避免过多次重复调用消耗时间
iconv_t IconvOpen(const char *from_charset, const char *to_charset) {
	return iconv_open(to_charset, from_charset);
}

int CodeConvert(iconv_t cd, char *inbuf, size_t inlen, char *outbuf, size_t outlen) {
	char **pin = &inbuf;
	char **pout = &outbuf;
	size_t result;
	if (cd == 0) {
		return -2;
	}
	memset(outbuf, 0, outlen);
	if ((result = iconv(cd, pin, &inlen, pout, &outlen)) == -1) {
		return -1;
	}
	return result;
}

int IconvClose(iconv_t cd) {
	return iconv_close(cd);
}

int Utf16ToUtf8(const char16_t *wchars, char *chars, int nSize) {
	size_t len;
	len = utf16_strlen(wchars);
	return code_convert("UTF-16", "UTF-8", (char*) wchars, len * 2, chars, nSize);
}

int Utf8ToUtf16(const char *chars, TCHAR *wchars, int nSize) {
	size_t len;
	int result;
	TCHAR temp[nSize];
//	if (nSize > 1024) return 0;
	len = strlen(chars);
	result = code_convert("UTF-8", "UTF-16", (char*) chars, len, (char*) temp, nSize * 2);
	if (temp[0] == 0xFEFF) {
		utf16_strcpy(wchars, (temp + 1));
	}
	return result;
}


