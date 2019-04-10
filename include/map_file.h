#ifndef	_MAP_FILE_H_
#define	_MAP_FILE_H_

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct tagFILEMAPDATA
{
	int		h_file;			//文件句柄
	long long   length;			//文件的长度
	long long   map_length;			//映射大小
	long long	offset;			//文件当前偏移
	//int			granularity;	//映射颗粒度	//linux没有虚拟内存颗粒度的概念，最小分配不会大于页面大小
	int		pagesize;
	char		*view;			//当前的视图
}FILEMAPDATA, *FILEMAPHANDLE;

FILEMAPHANDLE FileMapOpen(const char *file_name);
int FileMapGetBuffer(FILEMAPHANDLE handle, char **buffer, int length);
int FileMapSetOffset(FILEMAPHANDLE handle, long long offset);
int FileMapClose(FILEMAPHANDLE handle);

#ifdef	__cplusplus
}
#endif

#endif 
