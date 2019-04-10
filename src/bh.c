/**	笔划输入模块
 *	提供两个笔划输入的功能：
 *	1. 顺序笔划输入模式
 *	2. 无顺序笔划输入模式
 */

#include <zi.h>
#include <assert.h>
#include <string.h>
#include <spw.h>
#include <config.h>
#include <utility.h>
#include <string.h>
#include <vc.h>
#include <fontcheck.h>
#include <gbk_map.h>
#include <share_segment.h>
#include <stdint.h>
#include <utf16char.h>
static BHDATA *bh_data      = 0;  //笔画文件映射内存块
static char *bh_share_name = "HYPIM_BH_SHARED_NAME";
//static char bh_share_name[MAX_PATH];

/**	获得笔划输入的候选
 */
int GetBHCandidates(const TCHAR *input_string, CANDIDATE *candidates, int array_length)
{
	char bh_string[0x100];
	int  i, bh_length, min_bhs, hz_count, idx;
	intptr_t maxp;
	int *index1, *index2;
	BHITEM *data;

	extern int LoadBHResource();

	//没有输入或者输入不为B则直接返回
	if (!array_length || !input_string || *input_string != 'B' || !input_string[1])
		return 0;

	if(!share_segment->bh_loaded)
		LoadBHResource();

	if (!bh_data)
	{
        DEBUG_ECHO("");
        bh_data = GetSharedMemory(bh_share_name);
        //bh_data = GetMappedFileAddress(bh_share_name);
		//可能存在其他进程已经装载了，但是退出后共享内存被释放的问题
		if (!bh_data && share_segment->bh_loaded)
		{
			share_segment->bh_loaded = 0;
			LoadBHResource();
		}
	}

	if (!bh_data)
		return 0;

	//检查是否都为笔划字母与数字构成，否则返回；并确认是否为模糊匹配
	min_bhs = 0;
	for (bh_length = 0, i = 1; input_string[i] && i < sizeof(bh_string) - 1; i++)
	{
		switch (input_string[i])
		{
		case 'h':
			bh_string[bh_length] = '1';
			break;

		case 's':
			bh_string[bh_length] = '2';
			break;

		case 'p':
			bh_string[bh_length] = '3';
			break;

		case 'n':
			bh_string[bh_length] = '4';
			break;

		case 'd':
			bh_string[bh_length] = '4';
			break;

		case 'z':
			bh_string[bh_length] = '5';
			break;

		case '*':
			bh_string[bh_length] = '*';
			min_bhs--;
			break;

		case '?':
			bh_string[bh_length] = '?';
			break;

		default:
			return 0;
		}

		bh_length++;
	}

	bh_string[bh_length] = 0;
	if (!bh_length)
		return 0;

	min_bhs += bh_length;

	if (min_bhs <= 0)
		return 0;

	index1 = bh_data->index1;
	index2 = (int*)(bh_data->index1 + bh_data->maxstrockes);
	data   = (BHITEM*)(index2 + (bh_data->maxmcp - bh_data->minmcp + 1));
	idx    = bh_data->index1[min_bhs - 1];

	maxp = (intptr_t)data + sizeof(BHITEM) * bh_data->itemcount;

	//取得汉字的候选
	for (hz_count = 0, i = 0; i < bh_data->itemcount && hz_count <array_length; i++)
	{
		BHITEM *p = (BHITEM*)((intptr_t)bh_data + idx + i * sizeof(BHITEM));

		if((intptr_t)p >= maxp)
			break;

		if (pim_config->scope_gbk == HZ_SCOPE_UNICODE)
		{
            if (!FontCanSupport(p->zi))
				continue;
		}
		else
		{
			//if it is not gbk,then continue
			if(!IsGBK(p->zi))
				continue;
		}

		if (!strncmp(bh_string, (char*)((intptr_t)bh_data + (int)p->bh), bh_length) ||
				strMatch((char*)((intptr_t)bh_data + (int)p->bh), bh_string))
		{
			//delete duplicated
			if(hz_count > 0 && p->zi == candidates[hz_count-1].spw.hz)
				continue;
			//找到
			candidates[hz_count].type		= CAND_TYPE_SPW;
			candidates[hz_count].spw.type	= SPW_STRING_BH;
			candidates[hz_count].spw.hz		= p->zi;
			candidates[hz_count].spw.string = &candidates[hz_count].spw.hz;
			candidates[hz_count].spw.length = (int)utf16_strlen(candidates[hz_count].spw.string);

			hz_count++;
		}
	}

	return hz_count;
}

/**	获得笔划的候选显示
 */
void GetBHDisplayString(CANDIDATE *candidate, TCHAR *buffer, int length)
{
	if (!buffer || !length)
		return;

	if (candidate->type != CAND_TYPE_SPW ||	candidate->spw.type != SPW_STRING_BH)
	{
		*buffer = 0;
		return;
	}

	//依次寻找汉字的发音
	GetZiBHPinyin(candidate->spw.hz, buffer, length);
}

/**	加载笔划数据文件到内存。
 *	参数：
 *		file_name			笔划数据文件全路径
 *	返回：
 *		成功：1
 *		失败：0
 */
int LoadBHData(const char *file_name)
{
	int file_length;

	assert(file_name);

	if (share_segment->bh_loaded)
		return 1;

	file_length = GetFileLength(file_name);
	if (file_length <= 0)
		return 0;
   // strcpy_s(bh_share_name, MAX_PATH, file_name);

	bh_data = AllocateSharedMemory(bh_share_name, file_length);

	if (!bh_data)
		return 0;

	if ((file_length = LoadFromFile(file_name, bh_data, file_length)) == -1)
	{
		FreeSharedMemory(bh_share_name, bh_data);
		printf("bh.c : 笔划数据文件打开失败。name=%s\n", file_name);
		return 0;
	}

	if (!file_length)
		return 0;
/*
    DEBUG_ECHO("bh");
    bh_data = MapFileReadOnly(bh_share_name);
    if(!bh_data) return 0;
    */
	share_segment->bh_loaded = 1;

	return 1;
}
/**	释放笔划数据文件
 */
int FreeBHData()
{
	share_segment->bh_loaded = 0;

	if (bh_data)
	{
        DEBUG_ECHO("");
        FreeSharedMemory(bh_share_name, bh_data);
        //CloseMappedFile(bh_share_name);
		bh_data = 0;
	}

	return 1;
}


