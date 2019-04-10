#include <map_file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <utility.h>
#include <unistd.h>
/**	获得只读文件的句柄。注意只适合于只读文件！
 *	快速访问文件，用FileMapping方式对大文件的访问
 *	将会最为快捷。
 *	顺序访问数据
 */
FILEMAPHANDLE FileMapOpen(const char *file_name)
{
        FILEMAPHANDLE handle;

        handle = (FILEMAPHANDLE) malloc(sizeof(FILEMAPDATA));
        if (!handle)
                return 0;

        //打开文件
        //handle->h_file = CreateFile(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        handle->h_file = open(file_name, O_CREAT | O_RDWR, 0644);
        if (handle->h_file == -1)
        {
                free(handle);
                return 0;
        }

        handle->length = 0;
        //GetFileSizeEx(handle->h_file, (PLARGE_INTEGER)&handle->length);
	handle->length = GetFileLength(file_name);
        //创建映射
        /*
        handle->h_map = CreateFileMapping(handle->h_file, 0, PAGE_READONLY,	0, 0, 0);
        if (!handle->h_map)
        {
                CloseHandle(handle->h_file);
                free(handle);

                return 0;
        }
        */
       // handle->granularity = (int)sys_info.dwAllocationGranularity;	//linux没有虚拟内存颗粒度的概念，最小分配不会大于页面大小
        handle->pagesize = getpagesize();
        handle->offset		= 0;
        handle->view		= 0;
	handle->map_length	= 0;
        return handle;
}

/**	通过偏移获得文件的指针
 *	如果长度为0，则映射整个文件
 */
int FileMapGetBuffer(FILEMAPHANDLE handle, char **buffer, int length)
{
        int remains;
        long long offset;

        if (!handle || length < 0)
                return 0;

        if (handle->view)			//如果存在映射
        {
                munmap(handle->view, handle->map_length);
                handle->view = 0;
        }

        if (!length)
                length = (int)(handle->length - handle->offset);

        offset  = handle->offset;
        remains = (int)(offset & (handle->pagesize - 1));
        offset -= remains;

        if (length + handle->offset > handle->length)
                length = (int)(handle->length - handle->offset);

	
        //重新映射
        /*
        handle->view = (char*)MapViewOfFile(handle->h_map,
                                            FILE_MAP_READ,
                                            (DWORD)(offset >> 32),				//偏移地址
                                            (DWORD)(offset & 0xffffffff),
                                            length + remains);
        */
	handle->view = (char*)mmap(NULL,  length + remains, PROT_READ, MAP_SHARED, handle->h_file, (offset & 0xffffffff));
        if (!handle->view)
        {
                return 0;
        }
	handle->map_length = length + remains;
        *buffer = handle->view + remains;
        if (handle->length > handle->offset + length)
        {
                handle->offset += length;
                return length;
        }

        remains = (int)(handle->length - handle->offset);
        handle->offset = handle->length;

        return remains;
}

/**	设置文件指针
 */
int FileMapSetOffset(FILEMAPHANDLE handle, long long offset)
{
        if (!handle)
                return 0;

        handle->offset = offset;
        return 1;
}

/**	关闭文件映射
 */
int FileMapClose(FILEMAPHANDLE handle)
{
        if (handle)
        {
                munmap(handle->view, handle->map_length);
                close(handle->h_file);
                return 1;
        }

        return 0;
}
