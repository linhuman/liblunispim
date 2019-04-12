/*	词库处理函数组。
 *	系统中的词库可能有多个，因此需要统一管理。
 *		函数列表：
 *		LoadWordLib			装载词库文件到共享区
 *		SaveWordLib			保存词库文件
 *		CloseWordLib		关闭词库文件
 *		BackupWordLib		备份词库
 *		MergeWordLib		合并词库
 *		AddWord				增加一条词汇
 *		DelWord				删除一条词汇
 *		SetFuzzyMode		设定模糊模式
 *		GetWordCANDIDATE	获得词的候选
 *		GetAllItems			获得词库中的全部词条
 *		GetSystemItems		获得全部系统词汇条目
 *		GetUserItems		获得全部用户自定义条目
 */

/*	词库结构说明
 *	采用页表方式进行内存的申请，每次扩展词库都以一个页为单位。
 *	页：1024字节。
 *
 *	词库结构：
 *	1、词库信息
 *	2、词库索引表
 *	3、词库页内容
 */
#include <vc.h>
#include <assert.h>
#include <fcntl.h>
#include <wordlib.h>
#include <utility.h>
#include <ci.h>
#include <zi.h>
#include <config.h>
#include <share_segment.h>
#include <utf16char.h>
#include <string.h>
#include <unistd.h>
#include <unistr.h>
#include <stdlib.h>
#include <pthread.h> 
#include <dirent.h>
#include <shared_memory.h>
//词库维护外部接口
#define	ERR_FILE_NAME		"err.log"
#define	DEFAULT_FREQ		0
#define WORDLIB_DATA_DIR "/wordlib/"
#define WORDLIB_SUFFIX ".uwl"
#define WORDLIB_SYS "sys.uwl"
#define WORDLIB_USER "user.uwl"
pthread_mutex_t mutex ;  
//词库指针数组，等于0的时候，为该ID没有被分配。
static WORDLIB *wordlib_buffer[MAX_WORDLIBS * 2] = { 0, };

//词库文件是否可写
static int wordlib_can_write[MAX_WORDLIBS + MAX_WORDLIBS] = { 0 };
//上面的变量应该为每一个进程分配一个，不能够使用全局的，否则会出毛病（在IE7）中。



/**	在词库中建新页。
 *	参数：
 *		wordlib_id				词库句柄
 *	返回：
 *		成功创建：页号
 *		失败：-1
 */
static int NewWordLibPage(int wordlib_id)
{

    WORDLIB *wordlib = GetWordLibrary(wordlib_id);				//词库指针
    int	length = share_segment->wordlib_length[wordlib_id];		//词库的总长度
    int	new_length, new_page_no;								//新的词库长度、新页号

    if (!wordlib)			//没有这个词库
        return -1;

    //计算当前词库的Size是否已经到达词库的边界
    new_length = sizeof(wordlib->header_data) +							//词库头
            wordlib->header.page_count * WORDLIB_PAGE_SIZE + 		//页数据长度
            WORDLIB_PAGE_SIZE;										//新页数据长度
    if (new_length > length)			//超出内存边界，无法分配
        return -1;

    //对页初始化
    new_page_no = wordlib->header.page_count;
    wordlib->pages[new_page_no].next_page_no = PAGE_END;
    wordlib->pages[new_page_no].data_length  = 0;


    wordlib->pages[new_page_no].page_no		 = new_page_no;
    wordlib->pages[new_page_no].length_flag  = 0;

    wordlib->header.page_count++;

    return new_page_no;
}

/**	导出词库词条
 *	参数：
 *		wordlib_name			被导出词库名称
 *		text_name				导出词条文件名称
 *		ci_count				被导出词计数
 *		err_file_name			错误文件名称
 *		export_all				是否输出全部词条（包含被删除词？）
 *	返回：
 *		1：成功
 *		0：失败
 */
int ExportWordLibrary(const char *wordlib_file_name, const char *text_file_name, int *ci_count, char *err_file_name, int	export_all, void *call_back)
{
#define	MAX_DELETED_COUNT	10000

    WORDLIBITEM *item;										//词项
    WORDLIB *wl;											//被导出词库
    PAGE	*page;											//当前页
    FILE *fw, *ferr;										//导出文件
    int  wl_id;												//被导出词库标识
    TCHAR ci_str[MAX_WORD_LENGTH + 1];
    TCHAR pinyin_str[MAX_WORD_LENGTH * MAX_PINYIN_LENGTH + 1];
    int	 pinyin_index;
    int  i, j, err;											//返回错误
    TCHAR bom[1] = {0xfeff};

    //progress_indicator pi = (progress_indicator)call_back;

    fw = ferr = 0;
    *ci_count = 0;

    //创建err文件，在text文件所在目录中
    for (i = 0, j = 0; text_file_name[i]; i++)
    {
        if (text_file_name[i] == '/')
            j = i;
    }

    strncpy_s(err_file_name, MAX_PATH, text_file_name, j + 1);
    if (j)
        strcpy_s(err_file_name + j + 1, MAX_PATH - j - 1, ERR_FILE_NAME);
    else
        strcpy_s(err_file_name, MAX_PATH - j, ERR_FILE_NAME);

    ferr = fopen(err_file_name, "wt");
    if (!ferr)
    {
        err_file_name[0] = 0;
        return 0;
    }

    //_setmode(_fileno(ferr), _O_U16TEXT);
    //_ftprintf(ferr, TEXT("%c"), 0xFEFF);

    //每次导出前，都要保存一下用户词库
    SaveWordLibrary(GetUserWordLibId());

    wl_id = LoadWordLibraryWithExtraLength(wordlib_file_name, 0, 0);
    wl = GetWordLibrary(wl_id);
    if (!wl)		//装载失败
    {
        fprintf(ferr, "词库<%s>装载失败\n", wordlib_file_name);
        fclose(ferr);
        return 0;
    }

    err = 1;
    do
    {
        /*忽略编辑标识
        if (!wl->header.can_be_edit)
        {
            fprintf(ferr, "词库<%s>不允许导出。\n", wordlib_file_name);
            break;
        }
        */
        fw = fopen(text_file_name, "wt");
        if (!fw)
        {
            fprintf(ferr, "文件<%s>无法创建。\n", text_file_name);
            break;			//无法打开导出文件
        }

        //_setmode(_fileno(fw), _O_U16TEXT);
        fwrite(bom, 2, 1, fw);
        //_ftprintf(fw, TEXT("%c"), 0xFEFF);


        utf16_minfprintf(fw, TEXT("名称=%s\n"), wl->header.name);
        utf16_minfprintf(fw, TEXT("作者=%s\n"), wl->header.author_name);
        utf16_minfprintf(fw, TEXT("编辑=1\n\n"), wl->header.name);

        for (i = 0; i < wl->header.page_count; i++)
        {	//遍历页表
            for (page = &wl->pages[i], item = (WORDLIBITEM*) page->data; (char*)item < (char*) &page->data + page->data_length; item = GetNextCiItem(item))
            {
                //是否有效
                if (!item->effective)
                    continue;

                memcpy(ci_str, GetItemHZPtr(item), item->ci_length * sizeof(HZ));
                ci_str[item->ci_length] = 0;

                pinyin_index = 0;
                for (j = 0; j < (int)item->syllable_length; j++)
                {
                    SYLLABLE s;
                    if (j)
                        pinyin_str[pinyin_index++] = SYLLABLE_SEPARATOR_CHAR;

                    s = item->syllable[j];
                    s.tone = TONE_0;
                    pinyin_index += GetSyllableString(s, pinyin_str + pinyin_index, _SizeOf(pinyin_str) - pinyin_index,/* 0,*/ 0);
                    pinyin_str[pinyin_index] = 0;
                }

                utf16_minfprintf(fw, TEXT("%s\t%s\t%d\n"), ci_str, pinyin_str, item->freq);
                (*ci_count)++;
                //if (pi && !(*ci_count % 0x100))
                //	(*pi)(wl->header.word_count, *ci_count);
            }
        }
        err = 0;

    }while(0);

    if (fw)
        fclose(fw);

    if (ferr)
        fclose(ferr);

    //if (pi)
    //	(*pi)(wl->header.word_count, *ci_count);

    CloseWordLibrary(wl_id);

    if (!err)		//没有错误
        unlink(err_file_name);

    return !err;
}
/**	向词库导入词条
 *	参数：
 *		wordlib_name
 *		text_name
 *		ok_count
 *		err_count
 *		err_file_name
 *	返回：
 *		1：成功
 *		0：失败
 */
int ImportWordLibrary(const char *wordlib_file_name, const char *text_file_name, int *ok_count, int *err_count, char *err_file_name, void *call_back)
{
    TCHAR line[1024];				//一行1000个字节
    char u8_line[1536];
    TCHAR ci_str[256] = {0};				//词串
    char u8_ci_str[384] = {0};
    int  ci_length;					//词长
    TCHAR pinyin_str[256];			//拼音串
    char u8_pinyin_str[256];			//拼音串
    int  syllable_count;			//音节长度
    FILE *fr, *ferr;				//导入文件、错误文件
    SYLLABLE syllables[256];		//音节数组
    WORDLIB	*wl;					//被导出词库
    WORDLIBITEM *item;				//
    int  wl_id, user_wl_id;			//被导入词库标识、用户词库标识
    int  freq;						//词频
    int  i, j, line_count;			//行数
    int  old_ci_count;
    int  import_to_user_lib = 0;
    int  wl_reloaded;
    //progress_indicator pi = (progress_indicator)call_back;

    *ok_count = 0;
    *err_count = 0;

    //创建err文件，在text文件所在目录中
    for (i = 0, j = 0; text_file_name[i]; i++)
    {
        if (text_file_name[i] == '/')
            j = i;
    }

    strncpy_s(err_file_name, MAX_PATH, text_file_name, j + 1);
    if (j)
        strcpy_s(err_file_name + j + 1, MAX_PATH - j - 1, ERR_FILE_NAME);
    else
        strcpy_s(err_file_name, MAX_PATH - j, ERR_FILE_NAME);

    ferr = fopen(err_file_name, "wt");
    if (!ferr)
    {
        err_file_name[0] = 0;
        return 0;
    }

    //_setmode(_fileno(ferr), _O_U16TEXT);
    //fwrite(bom, 2, 1, ferr);	//写入utf16的bom
    //_ftprintf(ferr, TEXT("%c"), 0xFEFF);

    //装载词库，如果内存中有这个词库，不需要重新进行加载
    wl_reloaded = 0;
    wl_id = GetWordLibraryLoaded(wordlib_file_name);

    if (wl_id == -1)		//内存中没有这个库
    {

        wl_id = LoadWordLibraryWithExtraLength(wordlib_file_name, WORDLIB_EXTRA_LENGTH, 0);
        wl_reloaded = 1;

    }
    user_wl_id = GetUserWordLibId();
    if (user_wl_id == wl_id)
        import_to_user_lib = 1;

    wl = GetWordLibrary(wl_id);
    if (!wl)		//装载失败
    {
        fprintf(ferr, "词库<%s>装载失败。可能您装载的词库过多\n", wordlib_file_name);
        fclose(ferr);
        return 0;
    }

    old_ci_count = wl->header.word_count;

    if (!wl->header.can_be_edit)
    {
        fprintf(ferr, "词库<%s>不允许导入。\n", wordlib_file_name);
        fclose(ferr);
        return 0;
    }

    fr = fopen(text_file_name, "rb");
    if (!fr)
    {
        fprintf(ferr, "文件<%s>无法打开。\n", text_file_name);
        fclose(ferr);
        return 0;			//无法打开导出文件
    }

    //跳过FFFE
    fseek(fr, 2, SEEK_SET);

    line_count = 0;
    do
    {
        //读取一行数据
        if (!GetLineFromFile(fr, line, _SizeOf(line))){

            break;			//可能到结尾了
        }
        Utf16ToUtf8(line, u8_line, 1536);
        line_count++;

        //注释行或空行
        if (u8_line[0] == 0 || u8_line[0] == '#' || u8_line[0] == ';' || u8_line[0] == 0xd || u8_line[0] == 0xa)
            continue;

        if (strstr(u8_line, "="))			//跳过前面的作者等信息
            continue;

        if (1 != sscanf(u8_line, "%383[^\t]", u8_ci_str))
        {
            fprintf(ferr, "第<%d>行没有内容\n", line_count);
            (*err_count)++;
            continue;
        }

        if (2 != sscanf(u8_line, "%383[^\t]\t%255[^\t]", u8_ci_str, u8_pinyin_str))
        {
            fprintf(ferr, "第<%d>行没有拼音内容\n", line_count);
            (*err_count)++;
            continue;
        }

        if(strlen(u8_pinyin_str) > MAX_INPUT_LENGTH){
            fprintf(ferr, "第<%d>行拼音超过%d字符的限制\n", line_count, MAX_INPUT_LENGTH);
            (*err_count)++;
            continue;
        }

        if (3 != sscanf(u8_line, "%383[^\t]\t%255[^\t]\t%d", u8_ci_str, u8_pinyin_str, &freq))
            freq = DEFAULT_FREQ;



        Utf8ToUtf16(u8_pinyin_str, pinyin_str, 256);
        syllable_count = ParsePinYinStringReverse(pinyin_str, syllables, _SizeOf(syllables), 0, PINYIN_QUANPIN/*, 0, 0*/);

        if (!syllable_count)
        {
            fprintf(ferr, "第<%d>行拼音<%s>有错误\n", line_count, u8_pinyin_str);
            (*err_count)++;
            continue;
        }

        //continue;
        Utf8ToUtf16(u8_ci_str, ci_str, 256);
        ci_length = (int)utf16_strlen(ci_str);
        if (ci_length < syllable_count)
        {
            fprintf(ferr, "第<%d>行词条<%s>音节长度<%d>与词长度<%d>不匹配\n", line_count, ci_str, syllable_count, ci_length);
            (*err_count)++;
            continue;
        }

        //判断词是否都是汉字
        for (i = 0; i < ci_length; i++)
            if (!_CanInLibrary(ci_str[i]))
                break;

        if (i != ci_length)
        {
            fprintf(ferr, "第<%d>行词条<%s>不全是汉字\n", line_count, u8_ci_str);
            (*err_count)++;
            continue;
        }

        if (ci_length > MAX_WORD_LENGTH)
        {
            fprintf(ferr, "第<%d>行词条<%s>长度超出最大长度\n", line_count, u8_ci_str);
            (*err_count)++;
            continue;
        }

        if (syllable_count > MAX_WORD_LENGTH)
        {
            fprintf(ferr, "第<%d>行词条音节<%s>长度超出最大长度\n", line_count, u8_pinyin_str);
            (*err_count)++;
            continue;
        }

        //重复词汇
        if (item = GetCiInWordLibrary(wl_id, (HZ*)ci_str, ci_length, syllables, syllable_count))
        {
            if (item->effective)
                continue;
        }

        //向词库中追加
        if (AddCiToWordLibrary(wl_id, (HZ*)ci_str, ci_length, syllables, syllable_count, freq))
        {
            (*ok_count)++;
            //if (pi && !(*ok_count % 0x100))
            //	(*pi)(-1, *ok_count);
            continue;
        }

        //可能词库满，重新装载词库
        SaveWordLibrary(wl_id);					//保存词库数据
        if (wl_reloaded)						//必须在词库重装载后，进行Close操作
            CloseWordLibrary(wl_id);			//关闭词库
        wl_id = LoadWordLibraryWithExtraLength(wordlib_file_name, WORDLIB_EXTRA_LENGTH, 0);
        wl_reloaded = 1;
        wl = GetWordLibrary(wl_id);

        if (!wl) 		//装载失败
        {
            fprintf(ferr, "词库<%s>重新装载失败。\n", wordlib_file_name);
            fclose(fr);
            fclose(ferr);
            return 0;
        }

        if (AddCiToWordLibrary(wl_id, (HZ*)ci_str, ci_length, syllables, syllable_count, freq))
        {
            (*ok_count)++;
            //if (pi && !(*ok_count % 0x100))
            //	(*pi)(-1, *ok_count);
            continue;
        }

        fprintf(ferr, "发生未知错误，词库<%s>可能毁坏\n", wordlib_file_name);
        fclose(fr);
        fclose(ferr);
        return 0;

    }while(1);

    fclose(fr);

    if (wl)
        *ok_count = wl->header.word_count - old_ci_count;

    //if (pi)
    //	(*pi)(-1, *ok_count);

    SaveWordLibrary(wl_id);					//保存词库数据
    if (wl_reloaded)
    {
        CloseWordLibrary(wl_id);			//删除共享
        if (import_to_user_lib)
            share_segment->can_save_user_wordlib = 0;
    }
    fclose(ferr);
    //_tunlink(err_file_name);					//删除错误记录文件

    return 1;
}

/*	获得词库的内存大小（用于保存词库数据）
 *	参数：
 *		wordlib				词库指针
 *	返回：
 *		词库的大小（头加上页长度）
 */
int GetWordLibSize(WORDLIB *wordlib)
{
    assert(wordlib);

    return sizeof(wordlib->header_data) + wordlib->header.page_count * WORDLIB_PAGE_SIZE;
}

/**	获得内存中已经装载的词库文件
 */
int GetWordLibraryLoaded(const char *lib_name)
{
    int  i;
    //遍历内存中的词库
    for (i = 0; i < MAX_WORDLIBS; i++){

        if (!share_segment->wordlib_deleted[i] && !strcmp(share_segment->wordlib_name[i], lib_name)){
            break;
        }
    }
    if (i != MAX_WORDLIBS)			//本词库已经在内存中
        return i;

    return -1;
}


/*	装载词库文件。模块内部使用。
 *	词库文件将装载到内存的共享区。对于同一个用户，所有的应用程序共享相同的
 *	词库（包括多个词库）。
 *	参数：
 *		lib_name			词库的完整文件名字（包含路径）
 *		extra_length		词库所需要的扩展数据长度
 *		check_exist			检查是否已经存在（用于词库的更新）
 *	返回值：
 *		成功：词库序号 >= 0
 *		失败：-1
 */
int LoadWordLibraryWithExtraLength(const char *lib_name, int extra_length, int check_exist)
{
    int  length,length1;						//词库长度
    char *buffer;						//词库的指针
    int  i, error;						//是否出错
    int  empty_id;						//未使用的ID
    WORDLIB *wl;						//词库指针
    error = 1;						//默认出错

    DEBUG_ECHO("装载词库<%s>文件\n", lib_name);

    //需要加锁
    pthread_mutex_lock(&mutex);

    do
    {
        //首先检查内存中是否已经存入了词库的数据
        empty_id = -1;
        if (!check_exist)			//不检查已有词库，则从剩余的共享中寻找
        {
            for (i = MAX_WORDLIBS; i < 2 * MAX_WORDLIBS; i++){
                if (wordlib_buffer[i] == 0)		//可以使用的ID
                {
                    empty_id = i;
                    break;
                }
            }

        }
        else
        {							//检查内存中存在的词库
            for (i = 0; i < MAX_WORDLIBS; i++)
            {
                if (check_exist && !share_segment->wordlib_deleted[i] &&
                        !strcmp(share_segment->wordlib_name[i], lib_name))
                    break;

                if (empty_id == -1 && wordlib_buffer[i] == 0)		//可以使用的ID
                    empty_id = i;
            }

            if (i != MAX_WORDLIBS)			//本词库已经在内存中
            {
                buffer = GetSharedMemory(share_segment->wordlib_shared_name[i]);
                if (!buffer)				//没有找到，系统错误！
                {
                    //可能由于IE7的优先级问题造成只能读取这个词库，所以要获得ReadOnly内存映射
                    buffer = GetSharedMemory(share_segment->wordlib_shared_name[i]);
                    if (buffer)
                    {
                        wordlib_can_write[i] = 0;
                        wordlib_buffer[i] = (WORDLIB*)buffer;
                        return i;
                    }
                    //Sandboxie等程序会造成无法获取共享内存(共享内存的名字被修改了)
                    //因此重新加载共享内存
                    else
                        empty_id = i;
                }
                else
                {
                    wordlib_can_write[i] = 1;
                    wordlib_buffer[i] = (WORDLIB*)buffer;		//设定本地址空间内的词库指针
                }

                if (buffer)
                {
                    pthread_mutex_unlock(&mutex);
                    return i;		//返回已经找到的词库标识
                }
            }
        }

        if (empty_id == -1)		//无法再装载新的词库，已满
            break;

        //判断是否为V6当前词库，如果不是能够升级进行升级，否则返回
        if (!CheckAndUpdateWordLibraryFast(lib_name))
        {

            error = 1;
            break;
        }

        //内存中没有词库数据，需要进行装载
        length = GetFileLength(lib_name);				//获得词库的长度
        if (length <= 0)									//文件不存在？
            break;
        //临时解决系统词库清零问题，固定系统词库内存为2M+1M，不足2M的直接申请2M，外挂1M作为扩展（不断的增加单词
        //大概能保存20W左右的数据
        if(!strcmp(lib_name + strlen(lib_name) - 8,"user.uwl") && length < 0x200000)
            length1 = 0x200000;
        else
            length1 = length;

        //分配共享内存
        buffer = AllocateSharedMemory(share_segment->wordlib_shared_name[empty_id], length1 + extra_length);
        if (!buffer){			//分配失败
            DEBUG_ECHO("分配内存失败");
            break;
        }
        //判断词库内容为何被加载了其他的内容
        if (empty_id == 1)		//系统词库
        {
            int len = (int)strlen(lib_name);
            if (strcmp(lib_name + len - 7, "sys.uwl"))
                DEBUG_ECHO("!!!!!!!!!!系统词库正在被覆盖, 覆盖文件:%s\n", lib_name);
        }
        if (!LoadFromFile(lib_name, buffer, length))
        {	//装载失败
            FreeSharedMemory(share_segment->wordlib_shared_name[empty_id], buffer);
            break;
        }
        wl = (WORDLIB*)buffer;
        if (wl->header.signature != HYPIM_WORDLIB_V66_SIGNATURE)
        {
            error = 1;
            FreeSharedMemory(share_segment->wordlib_shared_name[empty_id], buffer);
            break;
        }

        //对词库相关数据进行记录
        strncpy(share_segment->wordlib_name[empty_id], lib_name, sizeof(share_segment->wordlib_name[0]));

        share_segment->wordlib_length[empty_id]	= length1 + extra_length;
        wordlib_buffer[empty_id]	= (WORDLIB*)buffer;
        wordlib_can_write[empty_id] = 1;

        error = 0;

    }while(0);

    //需要解锁
    pthread_mutex_unlock(&mutex);

    if (error)
    {
        printf("加载失败\n");
        return -1;
    }

    return empty_id;
}
/*	获得用户词库的标识。
 *	参数：无
 *	返回：
 *		用户词库标识，没有装载则返回-1。
 */
int GetUserWordLibId()
{
    //extern int resource_thread_finished;

    //while (!resource_thread_finished)
    //	Sleep(0);						//此处引发了死机问题
    return share_segment->user_wordlib_id;
}
/*	判断词是否已经在词库中
 *	参数：
 *	返回：
 *		在词库中：指向词条的指针
 *		不在：0
 */
WORDLIBITEM *GetCiInWordLibrary(int wordlib_id, HZ *hz, int hz_length, SYLLABLE *syllable, int syllable_length)
{
    int count, i;
    //CANDIDATE candidates[MAX_CANDIDATES];
    CANDIDATE	*candidates;
    WORDLIBITEM	*item = 0;

    candidates = malloc(sizeof(CANDIDATE) * MAX_CANDIDATES);

    count = GetCiCandidates(wordlib_id, syllable, syllable_length, candidates, MAX_CANDIDATES, FUZZY_CI_SYLLABLE_LENGTH);

    for (i = 0; i < count; i++)
        if (candidates[i].word.item->ci_length == hz_length && !memcmp(candidates[i].word.hz, hz, hz_length * sizeof(HZ)))
        {
            item = candidates[i].word.item;			//找到
            break;
        }

    free(candidates);

    return item;
}

/*	向词库中增加词汇（用户自造词）。
 *	要求：词的音节必须与字一一对应，不能出现“分隔符”以及“通配符”。
 *	参数：
 *		wordlib_id	词库标识
 *		hz			词
 *		length		词长度
 *		freq		词频
 *	返回：
 *		成功加入：1
 *		失败：0
 */
int AddCiToWordLibrary(int wordlib_id, HZ *hz, int hz_length, SYLLABLE *syllable, int syllable_length, int freq)
{
    WORDLIBITEM	*item;							//词项指针
    WORDLIB		*wordlib;						//词库指针
    PAGE		*page;							//页指针
    int			page_no, new_page_no;			//页号
    int			item_length;					//词项长度
    int			i;

    //检查词频，避免越界
    if (freq > WORDLIB_MAX_FREQ)
        freq = WORDLIB_MAX_FREQ;

    if (wordlib_id < 0 || wordlib_id >= MAX_WORDLIBS * 2)
        return 0;

    //不能进行写的词库文件
    if (!wordlib_can_write[wordlib_id])
        return 0;

    if (syllable_length < 2 || syllable_length > MAX_WORD_LENGTH ||	hz_length < 2 || hz_length > MAX_WORD_LENGTH)
    {
        fprintf(stdout, "增加词汇长度错误。syllable_length = %d, ci_length = %d", syllable_length, hz_length);
        return 0;				//音节过少或者过大
    }

    //判断是否都是汉字
    if (!IsAllCanInLibrary(hz, hz_length))
        return 0;

    //进行插入
    wordlib = GetWordLibrary(wordlib_id);
    if (!wordlib)				//没有这个词库
        return 0;

    //判断该词是否在词库中存在，如果存在不做插入，但将有效置为1，并且增加一次词频
    if ((item = GetCiInWordLibrary(wordlib_id, hz, hz_length, syllable, syllable_length)) != 0)
    {
        if (!item->effective)
        {
            item->effective = 1;
            wordlib->header.word_count++;

        }

        if (wordlib_id == GetUserWordLibId())
        {
            if (freq > (int)item->freq)
                item->freq = (unsigned int)freq;
            else
                item->freq++;

            share_segment->user_wl_modified = 1;
            return 1;
        }

        if (freq > (int)item->freq)
            item->freq = (unsigned int)freq;

        return 1;
    }

    item_length = GetItemLength(hz_length, syllable_length);

    //找出音节序列的词库页索引
    page_no = wordlib->header.index[syllable[0].con][syllable[1].con];
    if (page_no == PAGE_END)							//索引没有指向页
    {
        new_page_no = NewWordLibPage(wordlib_id);		//分配新页
        if (new_page_no == -1)							//未能分配成功，只好返回
            return 0;

        wordlib->header.index[syllable[0].con][syllable[1].con] = new_page_no;		//索引联接
        page_no = new_page_no;
    }

    //遍历页表找出最后一页。
    //不进行已删除词汇空洞的填补工作，省力（好编）并且省心（程序健壮）。
    while(wordlib->pages[page_no].next_page_no != PAGE_END)
        page_no = wordlib->pages[page_no].next_page_no;

    //获得页
    page = &wordlib->pages[page_no];

    //如果本页的数据不能满足加入要求
    if (page->data_length + item_length > WORDLIB_PAGE_DATA_LENGTH)
    {
        //需要分配新页
        new_page_no = NewWordLibPage(wordlib_id);
        if (new_page_no == -1)		//未能分配成功，只好返回
            return 0;
        //分配成功，维护页链表
        page->next_page_no	= new_page_no;
        page_no				= new_page_no;
        page				= &wordlib->pages[page_no];
    }

    assert(page->data_length + item_length <= WORDLIB_PAGE_DATA_LENGTH);

    //词汇长度
    page->length_flag |= (1 << syllable_length);

    //在本页中插入输入
    item = (WORDLIBITEM*)&page->data[page->data_length];

    item->effective		  = 1;					//有效
    item->ci_length		  = hz_length;			//词长度
    item->syllable_length = syllable_length;	//音节长度
    item->freq			  = freq;				//词频

    for (i = 0; i < syllable_length; i++)
        item->syllable[i] = syllable[i];		//音节

    for (i = 0; i < hz_length; i++)
        GetItemHZPtr(item)[i] = hz[i];			//汉字

    //增加页的数据长度
    page->data_length += item_length;

    //增加了一条记录
    wordlib->header.word_count++;

    if (wordlib_id == share_segment->user_wordlib_id)
        share_segment->user_wl_modified = 1;

    //成功插入
    return 1;
}
/*	依据词库标识，获得内存中词库的指针。
 *	参数：
 *		wordlib_id			词库标识
 *	返回：
 *		找到：词库指针
 *		未找到：0
 */
WORDLIB *GetWordLibrary(int wordlib_id)
{
    if (wordlib_id < 0)				//非法标识
        return 0;

    if (wordlib_buffer[wordlib_id] == 0)		//没有这个词库的指针（可能被释放掉了）
    {
        fprintf(stdout, "获取词库指针出错。id=%d", wordlib_id);
        return 0;
    }

    return wordlib_buffer[wordlib_id];
}

/**	快速检查并升级用户的词汇。
 *	注：V3/V5的词汇将自动加入到用户词库中
 *	    V6B1/V6B2的词库将升级成为V6词库
 *	参数：
 *		wl_name		词库名称
 *	返回：
 *		成功升级或加入到用户词库：1
 *		失败：0
 */
int CheckAndUpdateWordLibraryFast(const char *wl_name)
{
    int version = 0, ret = 0;

    GetWordLibInformationFast(wl_name, &version, 0, 0, 0, 0);

    switch(version)
    {
    case WORDLIB_V66:
        return 1;

    case WORDLIB_V6:
        return UpgradeWordLibFromV6ToV66(wl_name);

    case WORDLIB_V3:
        //case WORDLIB_V5:
        //	return UpgradeWordLibFromV5ToV6(wl_name);

    case WORDLIB_V6B1:
        ret = UpgradeWordLibFromV6B1ToV6(wl_name);
        if (ret)
            ret = UpgradeWordLibFromV6ToV66(wl_name);

        return ret;

    case WORDLIB_V6B2:
        ret = UpgradeWordLibFromV6B2ToV6(wl_name);
        if (ret)
            ret = UpgradeWordLibFromV6ToV66(wl_name);

        return ret;
    }

    return 0;
}

/**	词库从V6升级到V6.6
 *	参数：
 *		wl_file_name			原始词库文件名称
 *	返回：
 *		成功	1
 *		失败	0
 *	V6和V6.6词库上的差别主要是中文从GBK改成了Unicode
 */
int UpgradeWordLibFromV6ToV66(const char *wl_file_name)
{
    int length, i, err;
    char *data;
    char backup_name[MAX_PATH];
    char wordlib_name_ansi[WORDLIB_NAME_LENGTH * sizeof(HZ) + 1] = {0};
    TCHAR wordlib_name_uc[WORDLIB_NAME_LENGTH] = {0};
    char author_name_ansi[WORDLIB_AUTHOR_LENGTH * sizeof(HZ) + 1] = {0};
    TCHAR author_name_uc[WORDLIB_AUTHOR_LENGTH] = {0};

    PAGE			*page;
    WORDLIB			*wordlib;
    WORDLIBHEADER	*header;
    WORDLIBITEM		*item;

    printf("正在升级V6用户词库......");

    err = 1;
    do
    {
        //装载V6词库到内存
        length = GetFileLength(wl_file_name);
        if (length <= 0)
            break;

        //分配内存
        data = (char*)malloc(length);
        if (!data)
            break;

        //装载词库数据
        if (length != LoadFromFile(wl_file_name, data, length))
            break;

        wordlib = (WORDLIB*)data;
        header = &wordlib->header;
        if (header->signature == HYPIM_WORDLIB_V66_SIGNATURE)		//已经为V6.6的词库，不需要升级
        {
            err = 0;
            break;
        }

        if (header->signature != HYPIM_WORDLIB_V6_SIGNATURE)		//判断版本号
            break;

        memcpy(wordlib_name_ansi, header->name, WORDLIB_NAME_LENGTH * sizeof(HZ));
        memcpy(author_name_ansi, header->author_name, WORDLIB_AUTHOR_LENGTH * sizeof(HZ));

        AnsiToUtf16(wordlib_name_ansi, wordlib_name_uc, WORDLIB_NAME_LENGTH);
        AnsiToUtf16(author_name_ansi, author_name_uc, WORDLIB_AUTHOR_LENGTH);

        //避免名称超出
        if (wordlib_name_uc[WORDLIB_NAME_LENGTH - 1])
            wordlib_name_uc[WORDLIB_NAME_LENGTH - 1]  = 0;

        if (author_name_uc[WORDLIB_AUTHOR_LENGTH - 1])
            author_name_uc[WORDLIB_AUTHOR_LENGTH - 1]  = 0;

        //设置新的版本号
        header->signature = HYPIM_WORDLIB_V66_SIGNATURE;

        memcpy(header->name, wordlib_name_uc, WORDLIB_NAME_LENGTH * sizeof(HZ));
        memcpy(header->author_name, author_name_uc, WORDLIB_AUTHOR_LENGTH * sizeof(HZ));

        //遍历页
        for (i = 0; i < header->page_count; i++)
        {
            page = &wordlib->pages[i];

            for (item = (WORDLIBITEM*)page->data;
                 (char*)item < (char*)&page->data + page->data_length;
                 item = (WORDLIBITEM*)GetNextCiItem((WORDLIBITEM*)item))
            {
                char ci_str_ansi[MAX_WORD_LENGTH * sizeof(HZ) + 1] = {0};
                TCHAR ci_str_uc[MAX_WORD_LENGTH + 1] = {0};

                memcpy(ci_str_ansi, GetItemHZPtr(item), item->ci_length * sizeof(HZ));

                AnsiToUtf16(ci_str_ansi, ci_str_uc, _SizeOf(ci_str_uc));

                memcpy(GetItemHZPtr(item), ci_str_uc, item->ci_length * sizeof(HZ));
            }
        }

        //先备份文件，再保存文件
        strcpy_s(backup_name, sizeof(backup_name), wl_file_name);
        strcat_s(backup_name, sizeof(backup_name), ".bk");
        unlink(backup_name);
        rename(wl_file_name, backup_name);

        if (SaveToFile(wl_file_name, data, length))
        {
            unlink(backup_name);
            err = 0;
            break;
        }

        //保存失败，恢复原来的文件
        unlink(wl_file_name);
        rename(backup_name, wl_file_name);
        break;

    }while(0);

    //ShowWaitingMessage(0, 0, 500);

    if (err)
        return 0;

    return 1;
}


/**	词库从V6B2升级到V6
 *	参数：
 *		wl_file_name			原始词库文件名称
 *	返回：
 *		成功	1
 *		失败	0
 */
int UpgradeWordLibFromV6B2ToV6(const char *wl_file_name)
{
    int length, i, err;
    char *data;
    char backup_name[MAX_PATH];

    PAGE			*page;
    WORDLIB			*wordlib;
    WORDLIBHEADER	*header;
    WORDLIBITEMV6B2	*item;
    WORDLIBITEM		*new_item;

    printf("正在升级V6B2用户词库......");

    err = 1;
    do
    {
        //装载V6B2词库到内存
        length = GetFileLength(wl_file_name);
        if (length <= 0)
            break;

        //分配内存
        data = (char*)malloc(length);
        if (!data)
            break;

        //装载词库数据
        if (length != LoadFromFile(wl_file_name, data, length))
            break;

        wordlib = (WORDLIB*)data;
        header = &wordlib->header;
        if (header->signature == HYPIM_WORDLIB_V6_SIGNATURE)		//已经为V6的词库，不需要升级
        {
            err = 0;
            break;
        }

        if (header->signature != HYPIM_WORDLIB_V6B2_SIGNATURE)		//判断版本号
            break;

        //设置新的版本号
        header->signature = HYPIM_WORDLIB_V6_SIGNATURE;

        //遍历页
        for (i = 0; i < header->page_count; i++)
        {
            page = &wordlib->pages[i];
            for (item = (WORDLIBITEMV6B2*) page->data;
                 (char*)item < (char*) &page->data + page->data_length;
                 item = (WORDLIBITEMV6B2*)GetNextCiItem((WORDLIBITEM*)item))		//此时Item已经为V6的item，所以使用V6的函数！！
            {
                int effective, ci_length, syllable_length, freq;

                //V6B2与V6的区别在于音节长度与词汇长度可以到达32
                ci_length		= (unsigned) item->ci_length;
                syllable_length = (unsigned) item->syllable_length;
                freq			= item->freq;
                effective		= item->effective;

                //freq需要经过调整
                if (freq > WORDLIB_MAX_FREQ)
                    freq = WORDLIB_MAX_FREQ;

                new_item = (WORDLIBITEM*)item;

                new_item->ci_length		  = ci_length;
                new_item->syllable_length = syllable_length;
                new_item->freq			  = freq;
                new_item->effective		  = effective;
            }
        }

        //先备份文件，再保存文件
        strcpy_s(backup_name, sizeof(backup_name), wl_file_name);
        strcat_s(backup_name, sizeof(backup_name), ".bk");
        unlink(backup_name);
        rename(wl_file_name, backup_name);

        if (SaveToFile(wl_file_name, data, length))
        {
            unlink(backup_name);
            err = 0;
            break;
        }

        //保存失败，恢复原来的文件
        unlink(wl_file_name);
        rename(backup_name, wl_file_name);
        break;

    }while(0);

    //ShowWaitingMessage(0, 0, 500);

    if (err)
        return 0;

    return 1;
}


/**	快速获得词库信息（不检查V3V5的备份文件）
 *	当参数为0的时候，不对参数进行赋值
 *	参数：
 *		name		词库文件
 *		version		词库版本号
 *		wl_name		词库文件名称
 *		author_name	词库作者名称
 *		can_be_edit	是否可以被编辑
 *		items		词库中的词条数目
 *	返回：
 *		成功：1
 *		失败：0（词库可能不存在）
 */
int GetWordLibInformationFast(const char *name, int *version, TCHAR *wl_name, TCHAR *author_name, int *can_be_edit, int *items)
{
    WORDLIBHEADER header;

    if (LoadFromFile(name, (char*)&header, sizeof(header)) < sizeof(header))
        return 0;

    if (version)
    {
        switch (header.signature)
        {
        case HYPIM_WORDLIB_V66_SIGNATURE:
            *version = WORDLIB_V66;
            break;

        case HYPIM_WORDLIB_V6_SIGNATURE:
            *version = WORDLIB_V6;
            break;

        case HYPIM_WORDLIB_V6B1_SIGNATURE:
            *version = WORDLIB_V6B1;
            break;

        case HYPIM_WORDLIB_V6B2_SIGNATURE:
            *version = WORDLIB_V6B2;
            break;

        case HYPIM_WORDLIB_V5_SIGNATURE:
            *version = WORDLIB_V5;
            return 1;

        default:
            *version = WORDLIB_WRONG;
            return 1;
        }
    }

    if (wl_name)
        _tcscpy_s(wl_name, WORDLIB_NAME_LENGTH, header.name);

    if (author_name)
        _tcscpy_s(author_name, WORDLIB_AUTHOR_LENGTH, header.author_name);

    if (can_be_edit)
        *can_be_edit = header.can_be_edit;

    if (items)
        *items = header.word_count;

    return 1;
}

/**	词库从V6B1升级到V6
 *	参数：
 *		wl_file_name			原始词库文件名称
 *	返回：
 *		成功	1
 *		失败	0
 */
int UpgradeWordLibFromV6B1ToV6(const char *wl_file_name)
{
    int length, i, err;
    char *data;
    char backup_name[MAX_PATH];

    PAGE			*page;
    WORDLIB			*wordlib;
    WORDLIBHEADER	*header;
    WORDLIBITEMV6B1	*item;
    WORDLIBITEM		*new_item;

    printf("正在升级V6B1用户词库......");

    err = 1;
    do
    {
        //装载V6B1词库到内存
        length = GetFileLength(wl_file_name);
        if (length <= 0)
            break;

        //分配内存
        data = (char*)malloc(length);
        if (!data)
            break;

        //装载词库数据
        if (length != LoadFromFile(wl_file_name, data, length))
            break;

        wordlib = (WORDLIB*)data;
        header = &wordlib->header;
        if (header->signature == HYPIM_WORDLIB_V6_SIGNATURE)		//已经为V6的词库，不需要升级
        {
            err = 0;
            break;
        }

        if (header->signature != HYPIM_WORDLIB_V6B1_SIGNATURE)		//判断版本号
            break;

        //设置新的版本号
        header->signature = HYPIM_WORDLIB_V6_SIGNATURE;

        //遍历页
        for (i = 0; i < header->page_count; i++)
        {
            page = &wordlib->pages[i];
            for (item = (WORDLIBITEMV6B1*) page->data;
                 (char*)item < (char*) &page->data + page->data_length;
                 item = (WORDLIBITEMV6B1*)GetNextCiItem((WORDLIBITEM*)item)) //此时Item已经为V6的item，所以使用V6的函数！！
            {
                int effective, length, freq;

                //V6B1与V6的区别在于音节长度与词汇长度可以变化
                length	  = (unsigned) item->length;
                freq	  = item->freq;
                effective = item->effective;

                new_item = (WORDLIBITEM*)item;

                new_item->ci_length		  = length;
                new_item->syllable_length = length;
                new_item->freq			  = freq;
                new_item->effective		  = effective;
            }
        }

        //先备份文件，再保存文件
        strcpy_s(backup_name, sizeof(backup_name), wl_file_name);
        strcat_s(backup_name, sizeof(backup_name), ".bk");
        unlink(backup_name);
        rename(wl_file_name, backup_name);

        if (SaveToFile(wl_file_name, data, length))
        {
            unlink(backup_name);
            err = 0;
            break;
        }

        //保存失败，恢复原来的文件
        unlink(wl_file_name);
        rename(backup_name, wl_file_name);
        break;

    }while(0);

    //ShowWaitingMessage(0, 0, 500);

    if (err)
        return 0;

    return 1;
}
/*	保存词库文件。
 *	参数：
 *		wordlib_id			词库标识
 *	返回：
 *		成功：1
 *		失败：0
 */
int SaveWordLibrary(int wordlib_id)
{
    WORDLIB *wordlib;
    int length;			//词库长度

    if (wordlib_id == share_segment->user_wordlib_id &&
            (!share_segment->can_save_user_wordlib || !share_segment->user_wl_modified))
    {
        DEBUG_ECHO("用户词库没有改变或禁止改变，不保存词库\n");
        return 0;
    }

    //词库指针获取
    wordlib = GetWordLibrary(wordlib_id);
    if (!wordlib)
    {
        DEBUG_ECHO("未找到词库，id=%d\n", wordlib_id);
        return 0;
    }
    //用户词库，需要做一下备份
    if (GetUserWordLibId() == wordlib_id)
    {
        char bak_wordlib_name[MAX_PATH] = {0};

        strcpy_s(bak_wordlib_name, sizeof(bak_wordlib_name), share_segment->wordlib_name[wordlib_id]);
        strcat_s(bak_wordlib_name, sizeof(bak_wordlib_name), ".bak");
        unlink(bak_wordlib_name);
        //备份用户文件
        rename(share_segment->wordlib_name[wordlib_id], bak_wordlib_name);

    }

    length = GetWordLibSize(wordlib);
    if (!SaveToFile(share_segment->wordlib_name[wordlib_id], wordlib, length))
    {
        DEBUG_ECHO("保存词库失败，id = %d, lib_ptr = %p, length = %d\n", wordlib_id, wordlib, length);
        return 0;
    }

    if (wordlib_id == share_segment->user_wordlib_id)
        share_segment->user_wl_modified = 0;

    return 1;
}

/*	释放词库数据。
 *	参数：无
 *	返回：无
 */
void CloseWordLibrary(int wordlib_id)
{
    if (wordlib_id < 0 || wordlib_id >= MAX_WORDLIBS * 2)
        return;

    //加锁
    pthread_mutex_lock(&mutex);

    FreeSharedMemory(share_segment->wordlib_shared_name[wordlib_id], wordlib_buffer[wordlib_id]);

    wordlib_buffer[wordlib_id]					  = 0;		//指针清零
    share_segment->wordlib_length[wordlib_id]	  = 0;		//长度清零
    share_segment->wordlib_name[wordlib_id][0]    = 0;		//文件名字清零
    wordlib_can_write[wordlib_id]				  = 0;		//禁止写入
    share_segment->wordlib_deleted[wordlib_id]	  = 0;		//被删除标志

    //解锁
    pthread_mutex_unlock(&mutex);
}

/**	装载全部词库文件。
 *	其中，用户词库与系统词库为默认加载；外挂词库基于config中的设定进行加载
 *	返回：
 *		0：失败，一般为系统词库或者用户词库设置错误
 *		1：成功，但可能存在某些外挂词库错误的情况
 */
int LoadAllWordLibraries()
{

    int i, has_syslib = 0;
    char name[MAX_PATH] = {0};
    int used_wordlib[MAX_WORDLIBS + 1];

    if (!pim_config->wordlib_count)
        return 0;
    memset(used_wordlib, 0, sizeof(used_wordlib));

    //装载用户词库文件，一般不会失败，如果没有则创建一个用户词库文件
    GetFileFullName(pim_config->wordlib_name[0], name);

    if (0 > LoadUserWordLibrary(name))		//失败
        ;//		return 0;					//如果失败的话，也要进行下面的装载，否则可能发生丢失词库现象。

    used_wordlib[0] = 1;

    //检查系统词库是否被加载
    for (i = 1; i < pim_config->wordlib_count; i++)
    {
        if (!strcmp(pim_config->wordlib_name[i], WORDLIB_SYS_FILE_NAME))
        {
            has_syslib = 1;
            break;
        }
    }

    //如果系统词库没有被加载，就加载之，避免出现异常导致系统词库没有加载，而无法输入常用词
    if ((!has_syslib) && (pim_config->wordlib_count < MAX_WORDLIBS))
    {
        strcpy_s(pim_config->wordlib_name[pim_config->wordlib_count], MAX_FILE_NAME_LENGTH, WORDLIB_SYS_FILE_NAME);
        pim_config->wordlib_count++;
    }

    for (i = 1; i < pim_config->wordlib_count; i++)
    {
        int id;

        GetFileFullName(pim_config->wordlib_name[i], name);
        if ((id = LoadWordLibrary(name)) >= 0)
        {
            used_wordlib[id] = 1;
            continue;
        }

        GetFileFullName(pim_config->wordlib_name[i], name);
        if ((id = LoadWordLibrary(name)) >= 0)
            used_wordlib[id] = 1;
    }

    //此时要处理不再需要加载的词库文件
    for (i = 0; i < MAX_WORDLIBS; i++)
        if (share_segment->wordlib_name[i][0])
            share_segment->wordlib_deleted[i] = !used_wordlib[i];

    return 1;
}

/*	装载用户词库文件。
 *	词库文件将装载到内存的共享区。对于同一个用户，所有的应用程序共享相同的
 *	词库（包括多个词库）。
 *	由于用户词库将存储自行制造的词汇，因此需要加入扩展的数据长度。
 *	参数：
 *		wordlib_name		词库的完整文件名字（包含路径）
  *	返回值：
 *		如果成功，返回词库标识；失败返回-1。
 */
int LoadUserWordLibrary(const char *wordlib_name)
{
    char new_wordlib_name[MAX_PATH];
    char bak_wordlib_name[MAX_PATH];

    DEBUG_ECHO(" 加载用户词库<%s>", wordlib_name);

    if (GetFileLength(wordlib_name) <= 0){		//没有词库文件，创建一份
        if (!CreateEmptyWordLibFile(wordlib_name, DEFAULT_USER_WORDLIB_NAME, DEFAULT_USER_WORDLIB_AUTHOR, 1))
            return -1;			//彻底失败
    }


    //用户词库需要扩充内存
    share_segment->user_wordlib_id = LoadWordLibraryWithExtraLength(wordlib_name, WORDLIB_EXTRA_LENGTH, 1);

    //无法加载，可能是词库文件出错了，备份之前的文件，然后重新创建用户词库
    if (-1 == share_segment->user_wordlib_id)
    {
        strcpy_s(new_wordlib_name, sizeof(new_wordlib_name), wordlib_name);
        strcat_s(new_wordlib_name, sizeof(new_wordlib_name), ".bad");

        strcpy_s(bak_wordlib_name, sizeof(bak_wordlib_name), wordlib_name);
        strcat_s(bak_wordlib_name, sizeof(bak_wordlib_name), ".bak");

        if (FileExists(new_wordlib_name))
        {
            unlink(new_wordlib_name);
            //SetFileAttributes(new_wordlib_name, FILE_ATTRIBUTE_NORMAL);
            //DeleteFile(new_wordlib_name);
        }

        //备份错误的文件
        rename(wordlib_name, new_wordlib_name);

        //恢复备份文件
        if (FileExists(bak_wordlib_name))
            CopyFile(bak_wordlib_name, wordlib_name);

        //装载备份文件
        if (FileExists(wordlib_name))
            share_segment->user_wordlib_id = LoadWordLibraryWithExtraLength(wordlib_name, WORDLIB_EXTRA_LENGTH, 1);

        //备份文件也不对，创建一个新的用户词库文件
        if (-1 == share_segment->user_wordlib_id)
        {
            CreateEmptyWordLibFile(wordlib_name, DEFAULT_USER_WORDLIB_NAME, DEFAULT_USER_WORDLIB_AUTHOR, 1);
            share_segment->user_wordlib_id = LoadWordLibraryWithExtraLength(wordlib_name, WORDLIB_EXTRA_LENGTH, 1);
        }
    }

    return share_segment->user_wordlib_id;
}

/*	装载词库文件。
 *	词库文件将装载到内存的共享区。对于同一个用户，所有的应用程序共享相同的
 *	词库（包括多个词库）。
 *	参数：
 *		wordlib_name		词库的完整文件名字（包含路径）
 *	返回值：
 *		成功：词库序号 >= 0
 *		失败：-1
 */
int LoadWordLibrary(const char *wordlib_name)
{
    return LoadWordLibraryWithExtraLength(wordlib_name, WORDLIB_NORMAL_EXTRA_LENGTH, 1);
}


/*	创建新的空的词库文件。
 *	参数：
 *		wordlib_file_name		词库文件名字（全路径）
 *		name					词库名字（放在词库内部）
 *		author					作者
 *		can_be_edit				是否允许编辑
 *	返回：
 *		成功创建：1
 *		失败：0
 */
int CreateEmptyWordLibFile(const char *wordlib_file_name, const TCHAR *name, const TCHAR *author, int can_be_edit)
{
    WORDLIB		wordlib;
    int i, j;

    DEBUG_ECHO("创建新词库<%s>, name:%s, author:%s, can_be_edit:%d",
               wordlib_file_name, name, author, can_be_edit);

    //清零
    memset(&wordlib, 0, sizeof(wordlib));

    //作者名字
    utf16_strncpy(wordlib.header.author_name, author, _SizeOf(wordlib.header.author_name));

    //词库名字
    utf16_strncpy(wordlib.header.name, name, _SizeOf(wordlib.header.name));

    wordlib.header.can_be_edit = can_be_edit;

    //将Index置为没有页
    for (i = CON_NULL; i < CON_END; i++)
        for (j = CON_NULL; j < CON_END; j++)
            wordlib.header.index[i][j] = PAGE_END;

    //没有已分配的页
    wordlib.header.page_count	= 0;
    wordlib.header.pim_version	= HYPIM_VERSION;
    wordlib.header.signature	= HYPIM_WORDLIB_V66_SIGNATURE;
    wordlib.header.word_count	= 0;

    if (!SaveToFile(wordlib_file_name, &wordlib, sizeof(wordlib)))
        return 0;

    return 1;
}

/**	释放所有的词库数据。
 *	参数：无
 *	返回：无
 */
void CloseAllWordLibrary()
{
    int i;

    for (i = 0; i < MAX_WORDLIBS; i++)
        CloseWordLibrary(i);
}
/**	获得下一个词库标识，用于词库的遍历。
 *	参数：
 *		cur_id				当前的ID（第一次的时候输入-1）
 *	返回：
 *		下一个词库标识：>0
 *		没有下一个词库的标识：-1
 */
int GetNextWordLibId(int cur_id)
{
    int i;

    if (cur_id < 0)
        cur_id = -1;

    for (i = cur_id + 1; i < MAX_WORDLIBS; i++){
        if (!share_segment->wordlib_deleted[i] && wordlib_buffer[i])
            return i;
    }
    return -1;
}
//检索wordlib文件夹中的词库文件，并记录入pim_config->wordlib_name数组
void SearchWordLib()
{
    DIR* dir = NULL;
    struct dirent* entry;
    char wordlib_dir[MAX_PATH] = {0};
    strcat_s(wordlib_dir, MAX_PATH, pim_config->user_data_dir);
    strcat_s(wordlib_dir, MAX_PATH, WORDLIB_DATA_DIR);
    if((dir = opendir(wordlib_dir)) == NULL)
    {
        return;
    }else{
        while(entry=readdir(dir)){
            size_t file_name_length = strlen(entry->d_name);
            size_t suffix_length = strlen(WORDLIB_SUFFIX);
            if((file_name_length > suffix_length) &&
                    strcmp(entry->d_name, WORDLIB_SYS) &&
                    strcmp(entry->d_name, WORDLIB_USER) &&
                    !strcmp(&entry->d_name[file_name_length - suffix_length], WORDLIB_SUFFIX) &&
                    pim_config->wordlib_count < MAX_WORDLIBS &&
                    entry->d_type == 8  //文件类型，4为文件夹
                    ){
                strcpy_s(pim_config->wordlib_name[pim_config->wordlib_count], MAX_FILE_NAME_LENGTH, WORDLIB_DATA_DIR);
                strcat_s(pim_config->wordlib_name[pim_config->wordlib_count], MAX_FILE_NAME_LENGTH, entry->d_name);
                pim_config->wordlib_count++;
            }
        }
        closedir(dir);
    }
}
