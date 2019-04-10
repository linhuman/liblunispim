/**	fontmap模块
 *	fontmap的读取以及检索功能。
 */

#include <assert.h>
#include <utility.h>
#include <fontcheck.h>
#include <vc.h>
#include <pim_resource.h>
#include <share_segment.h>
#include <utf16char.h>
static byte *fontmap = 0;

static char *fontmap_share_name					= "HYPIM_FONTMAP_SHARED_NAME";
//static char fontmap_share_name[MAX_PATH];

/**	加载fontmap到内存。
 *	参数：
 *		fontmap_file_name			fontmap文件全路径
 *	返回：
 *		成功：1
 *		失败：0
 */
int LoadFontMapData(const char *file_name)
{
	int file_length;

	assert(file_name);

    if ( pim_config->scope_gbk == HZ_SCOPE_GBK )
	{
		share_segment->fontmap_loaded = 0;
		return 0;
	}

	if (share_segment->fontmap_loaded)
		return 1;

	file_length = GetFileLength(file_name);
	if (file_length <= 0)
		return 0;
//    strcpy_s(fontmap_share_name, MAX_PATH, file_name);

	fontmap = AllocateSharedMemory(fontmap_share_name, file_length);
	if (!fontmap)
		return 0;

	if ((file_length = LoadFromFile(file_name, fontmap, file_length)) == -1)
	{
		FreeSharedMemory(fontmap_share_name, fontmap);
		printf("fontcheck.c : FONT MAP文件打开失败。name=%s\n", file_name);
		return 0;
	}

	if (!file_length)
		return 0;

    /*
    DEBUG_ECHO("fontcheck");
    fontmap = MapFileReadOnly(file_name);
    if (!fontmap)
        return 0;
        */
	share_segment->fontmap_loaded = 1;

	return 1;
}

/**	释放FONT MAP文件
 */
int FreeFontMapData()
{
	share_segment->fontmap_loaded = 0;

	if (fontmap)
	{
		FreeSharedMemory(fontmap_share_name, fontmap);
		fontmap = 0;
	}

	return 1;
}

int FontCanSupport(UC zi)
{
	int pos_offset;
	byte pos_mask,*pos;
	TCHAR *fontname,*pos1,*pos2;
	extern int LoadFontMapResource();

	if(!share_segment->fontmap_loaded)
		return 1;
	//if font map data isn't loaded,return 1(support)
	if(!fontmap)
	{
		fontmap = GetSharedMemory(fontmap_share_name);

		//可能存在其他进程已经装载了，但是退出后共享内存被释放的问题
		if (!fontmap && share_segment->fontmap_loaded)
		{
			share_segment->fontmap_loaded = 0;
			LoadFontMapResource();
		}
	}
	if(!fontmap)
		return 1;

	fontname = (TCHAR *)(fontmap + 0x6000);
	if (!fontname)
		return 1;
	pos1 = utf16_strstr( fontname, pim_config->chinese_font_name );
	pos2 = utf16_strstr( fontname, TEXT("{") );
	if (!pos1 || pos1 > pos2)
		return 1;

	if (!fontmap || zi > 0x2FFFF)
		return 1;

	//PUA
	if ( !utf16_strcmp(pim_config->chinese_font_name,TEXT("微软雅黑")) && zi >= 0xE000 && zi <= 0xF8FF )
		return 0;

	pos_offset = zi >> 3;
	pos_mask = 1 << (zi % 8);

	pos = (byte*)(fontmap + pos_offset);

	return (*pos) & pos_mask;
} 
