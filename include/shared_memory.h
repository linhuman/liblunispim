#ifndef _SHARED_MEMORY_H_
#define _SHARED_MEMORY_H_ 
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#define SHARED_MEMORY_NOT_EXISTS -1
#define FILE_NOT_EXISTS -2
#define FILE_NOT_MAPPED -3
#define SUCCESS 1
#define FAILED 0
#define SHARED_TYPE_MAP_FILE 1
#define SHARED_TYPE_SHARED_MEMORY 2
void CreateSharedMap();
void FreeSharedMap();
void* CreateSharedMemoryReadOnly(const char* shared_name, int shared_type, int size);
void* CreateSharedMemoryReadWrite(const char* shared_name, int shared_type, int size);
void* GetSharedMemoryAddress(const char* shared_name);
int FlushSaredMemory(const char* shared_name);
int RemoveSharedMemory(const char* shared_name);
int CloseSharedMemory(const char* shared_name);
size_t GetSharedMemorySize(const char* shared_name);
int ResizeMappedFile(const char* shared_name, int size);
void* CreateMappedFile(const char* file_name, int size);
int SharedIsCreated(const char* shared_name);
void ShowSharedMemoryList();
#ifdef __cplusplus
}
#endif 
#endif 
