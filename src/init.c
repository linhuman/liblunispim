#include <pim_resource.h>
#include <config.h>
#include <share_segment.h>
#include <debug.h>
#include <shared_memory.h>
#include <vc.h>
#include <utility.h>
#include <config.h>
#include <init.h>
#include <wordlib.h>
#include <stdlib.h>
#define ENGLISH_DATA_DIR "/english/"
#define ENGLISH_DAT "english.dat"
#define ENGLISH_TRANS_DAT "english_trans.dat"

#define INI_DATE_DIR "/ini/"
#define TOP_ZI_INI "固顶字.ini"
#define SHUANGPIN_INI "双拼.ini"
#define CHINESE_SYMBOLS_INI "中文符号.ini"

#define PHRASE_DATA_DIR "/phrase/"
#define PARASE_INI "系统短语库.ini"

#define WORDLIB_DATA_DIR "/wordlib/"
#define BIGRAM_DAT "bigram.dat"
#define SYS_WORDLIB "sys.uwl"

#define ZI_DATA_DIR "/zi/"
#define HZBH_DAT "hzbh.dat"
#define HZBS_DAT "hzbs.dat"
#define HZJF_DAT "hzjf.dat"
#define HZPY_DAT "hzpy.dat"
#define J2F_DAT "j2f.dat"
#define ERR_INFO_LENGTH 1024
static char initialize_err_info[ERR_INFO_LENGTH] = {0};
void DeployResources()
{
    char des_path[MAX_PATH] = {0};
    char res_path[MAX_PATH] = {0};
    int arr_size = 5;
    char* data_dir[5] = {"/english/", "/ini/", "/phrase/", "/wordlib/", "/zi/"};
    char* data_file[5][5] = {
        {"english.dat", "english_trans.dat", 0, 0, 0},
        {"固顶字.ini", "双拼.ini", "中文符号.ini", 0, 0},
        {"系统短语库.ini", 0, 0, 0, 0},
        {"bigram.dat", "sys.uwl", 0, 0, 0},
        {"hzbh.dat", "hzbs.dat", "hzjf.dat", "hzpy.dat", "j2f.dat"}
    };

    int res;
    if(strlen(pim_config->user_data_dir) == 0){
        char* home = getenv("HOME");
        strcat_s(pim_config->user_data_dir, MAX_PATH, home);
        strcat_s(pim_config->user_data_dir, MAX_PATH, "/.lunispim");
    }
    if(!FileExists(pim_config->user_data_dir)){
        if(!SHCreateDirectoryEx(pim_config->user_data_dir)){
            sprintf(initialize_err_info, "创建用户数据文件夹(%s)失败", pim_config->user_data_dir);
            return;
        }
    }
    for(int i=0; i<arr_size; i++){
        printf("data_dir[%d]:%s\n", i, data_dir[i]);
        for(int j=0; j<arr_size; j++){
            if(data_file[i][j] == 0) break;
            strcpy_s(des_path, MAX_PATH, pim_config->user_data_dir);
            strcat_s(des_path, MAX_PATH, data_dir[i]);
            if(!FileExists(des_path)){
                if(!SHCreateDirectoryEx(des_path)){
                    sprintf(initialize_err_info, "创建用户数据文件夹(%s)失败", des_path);
                }
            }
            strcat_s(des_path, MAX_PATH, data_file[i][j]);
            if(!FileExists(des_path)){
                strcpy_s(res_path, MAX_PATH, pim_config->resources_data_dir);
                strcat_s(res_path, MAX_PATH, data_dir[i]);
                strcat_s(res_path, MAX_PATH, data_file[i][j]);
                res = CopyFile(res_path, des_path);
                DEBUG_ECHO("res_path: %s", res_path);
                DEBUG_ECHO("des_path: %s", des_path);
                if(res == -1){
                    sprintf(initialize_err_info, "读取资源文件(%s)失败", res_path);
                    return;
                }
                if(res == -2){
                    sprintf(initialize_err_info, "创建用户数据文件(%s)失败", des_path);
                    return;
                }
            }
        }
    }
}
void Initialize()
{
    if(pim_config == 0) LoadDefaultConfig();
    DeployResources();
    if(initialize_err_info[0] != 0) return;
    SearchWordLib();
    CreateSharedMap();
    LoadSharedSegment();
    PIM_LoadResources();
    PIM_ReloadINIResource();
    ResetContext(&demo_context);
    ShowSharedMemoryList();
}
char* GetInitializeErrorInfo()
{
    return initialize_err_info;
}

void __attribute__ ((constructor)) init()
{

}
void __attribute__ ((destructor)) finit()
{
    DEBUG_ECHO("finit");
    //FreeWordLibraryResource();
    //FreeSpwResource();
    //FreeSharedSegment();
    //FreeSharedMap();
} 
