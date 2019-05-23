#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <string>
#include <shared_memory.h>
#include <map>
#include <debug.h>
#include <utility.h>
#include <iostream>
#ifdef BOOST_RESIZE_FILE

#define RESIZE_FILE boost::filesystem::resize_file

#else

#ifdef _WIN32
#include <windows.h>
#define RESIZE_FILE(P,SZ) (resize_file_api(P, SZ) != 0)
static BOOL resize_file_api(const char* p, boost::uintmax_t size) {
    HANDLE handle = CreateFileA(p, GENERIC_WRITE, 0, 0, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, 0);
    LARGE_INTEGER sz;
    sz.QuadPart = size;
    return handle != INVALID_HANDLE_VALUE
            && ::SetFilePointerEx(handle, sz, 0, FILE_BEGIN)
            && ::SetEndOfFile(handle)
            && ::CloseHandle(handle);
}
#else
#include <unistd.h>
#define RESIZE_FILE(P,SZ) (::truncate(P, SZ) == 0)
#endif  // _WIN32

#endif  // BOOST_RESIZE_FILE
extern "C" { 

class SharedMemoryImpl{
public:
    enum OpenMode {
        kOpenReadOnly,
        kOpenReadWrite,
    };

    SharedMemoryImpl(const char* shared_name, int shared_type, OpenMode mode, int size) {
        shared_type_ = shared_type;
        shared_name_ = shared_name;
        boost::interprocess::mode_t file_mapping_mode =
                (mode == kOpenReadOnly) ? boost::interprocess::read_only
                                        : boost::interprocess::read_write;
        if(shared_type == SHARED_TYPE_SHARED_MEMORY){
            shared_memory_.reset(new boost::interprocess::shared_memory_object(boost::interprocess::open_or_create, shared_name, file_mapping_mode));
            shared_memory_->truncate(size);
            region_.reset(new boost::interprocess::mapped_region(*shared_memory_, file_mapping_mode));
        }else if(shared_type == SHARED_TYPE_MAP_FILE){
            file_.reset(new boost::interprocess::file_mapping(shared_name, file_mapping_mode));
            region_.reset(new boost::interprocess::mapped_region(*file_, file_mapping_mode));
        }

    }
    ~SharedMemoryImpl() {
        if(shared_type_ == SHARED_TYPE_SHARED_MEMORY){
            shared_memory_->remove(shared_name_.c_str());
            shared_memory_.reset();
        }else if(shared_type_ == SHARED_TYPE_MAP_FILE){
            file_.reset();
        }
        region_.reset();
    }
    bool Flush() {
        return region_->flush();
    }
    bool remove()
    {
        if(shared_type_ == SHARED_TYPE_SHARED_MEMORY){
            shared_memory_->remove(shared_name_.c_str());
        }else if(shared_type_ == SHARED_TYPE_MAP_FILE){
            return boost::interprocess::file_mapping::remove(shared_name_.c_str());
        }
        return true;
    }
    void* get_address() const {
        return region_->get_address();
    }
    size_t get_size() const {
        return region_->get_size();
    }
    int get_shared_type()
    {
        return shared_type_;
    }
private:
    int shared_type_;
    std::shared_ptr<boost::interprocess::shared_memory_object> shared_memory_;
    std::shared_ptr<boost::interprocess::file_mapping> file_;
    std::shared_ptr<boost::interprocess::mapped_region> region_;
    std::string shared_name_;


};
using shared_map = std::map<const std::string, SharedMemoryImpl>;
static std::map<const std::string, SharedMemoryImpl>* _shared_map;
void CreateSharedMap()
{
    _shared_map = new shared_map;
}
void FreeSharedMap()
{
    delete _shared_map;
}
void* CreateSharedMemoryReadOnly(const char* shared_name, int shared_type, int size)
{
    if(shared_type == SHARED_TYPE_MAP_FILE && !FileExists(shared_name)) return nullptr;
    if(_shared_map->find(shared_name) != _shared_map->end()){
        auto& shared_memory_obj = _shared_map->at(shared_name);
        return shared_memory_obj.get_address();
    }
    SharedMemoryImpl shared_memory_obj(shared_name,
                                       shared_type,
                                       SharedMemoryImpl::kOpenReadOnly,
                                       size);
    _shared_map->insert(std::make_pair(shared_name, shared_memory_obj));
    return shared_memory_obj.get_address();

}
void* CreateSharedMemoryReadWrite(const char* shared_name, int shared_type, int size)
{
    if(shared_type == SHARED_TYPE_MAP_FILE && !FileExists(shared_name)) return nullptr;
    if(_shared_map->find(shared_name) != _shared_map->end()){
        auto& shared_memory_obj = _shared_map->at(shared_name);
        return shared_memory_obj.get_address();
    }
    SharedMemoryImpl shared_memory_obj(shared_name,
                                       shared_type,
                                       SharedMemoryImpl::kOpenReadWrite,
                                       size);
    _shared_map->insert(std::make_pair(shared_name, shared_memory_obj));
    return shared_memory_obj.get_address();

}
void* GetSharedMemoryAddress(const char* shared_name)
{
    if(_shared_map->find(shared_name) == _shared_map->end()) return nullptr;
    auto& shared_memory_obj = _shared_map->at(shared_name);
    return shared_memory_obj.get_address();
}
int FlushSaredMemory(const char* shared_name)
{
    if(_shared_map->find(shared_name) == _shared_map->end()) return FAILED;
    auto& shared_memory_obj = _shared_map->at(shared_name);
    return shared_memory_obj.Flush();
}

int RemoveSharedMemory(const char* shared_name)
{
    if(_shared_map->find(shared_name) == _shared_map->end()) return SHARED_MEMORY_NOT_EXISTS;
    auto& shared_memory_obj = _shared_map->at(shared_name);
    if(shared_memory_obj.remove()){
        _shared_map->erase(shared_name);
        return SUCCESS;
    }else{
        return FAILED;
    }
}
int CloseSharedMemory(const char* shared_name)
{
    int shared_type;
    if(_shared_map->find(shared_name) == _shared_map->end()) return SHARED_MEMORY_NOT_EXISTS;
    auto& shared_memory_obj = _shared_map->at(shared_name);
    shared_type = shared_memory_obj.get_shared_type();
    if(shared_type == SHARED_TYPE_SHARED_MEMORY){
        if(shared_memory_obj.remove()){
            _shared_map->erase(shared_name);
            return SUCCESS;
        }else{
            return FAILED;
        }
    }else if(shared_type == SHARED_TYPE_MAP_FILE){
        _shared_map->erase(shared_name);
        return SUCCESS;
    }
    return FAILED;
}

size_t GetSharedMemorySize(const char* shared_name)
{
    if(_shared_map->find(shared_name) == _shared_map->end()) return FAILED;
    auto& shared_memory_obj = _shared_map->at(shared_name);
    return shared_memory_obj.get_size();

}
void* CreateMappedFile(const char* file_name, int size)
{
    int res;
    if(FileExists(file_name)){
        res = ResizeMappedFile(file_name, size);
        if(res != 1) return nullptr;
    }else{
        std::filebuf fbuf;
        fbuf.open(file_name,
                  std::ios_base::in | std::ios_base::out |
                  std::ios_base::trunc | std::ios_base::binary);
        if (size > 0) {
            fbuf.pubseekoff(size - 1, std::ios_base::beg);
            fbuf.sputc(0);
        }
        fbuf.close();
    }
    return CreateSharedMemoryReadWrite(file_name, SHARED_TYPE_MAP_FILE, 0);
}
int ResizeMappedFile(const char* shared_name, int size)
{
    try {
        RESIZE_FILE(shared_name, size);
    }
    catch (...) {
        return FAILED;
    }
    return SUCCESS;

}
int SharedIsCreated(const char* shared_name)
{
    if(_shared_map->find(shared_name) == _shared_map->end())
        return FAILED;
    return SUCCESS;
}

void ShowSharedMemoryList()
{
    for(auto item = _shared_map->begin(); item != _shared_map->end(); item++){
        std::cout<<item->first<<std::endl;
    }
}

}
