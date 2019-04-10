#include <config.h>
#include <string.h>
#include <stdlib.h>
PIMCONFIG *pim_config = 0;					//全局使用的config指针
 
PIMCONFIG default_config =				//默认的输入法配置数据
{
	//输入风格
	STYLE_CSTAR,
//	STYLE_ABC,

	//启动的输入方式：中文、英文
	STARTUP_CHINESE,
//	STARTUP_ENGLISH,

	//拼音模式
	PINYIN_QUANPIN,

	//是否显示双拼的提示
	0,

	//候选选择方式，字母、数字。
	SELECTOR_DIGITAL,
//	SELECTOR_LETTER,

	//总是清除上下文
	0,

	//是否使用汉字音调辅助
	1,

	//是否使用ICW（智能组词）
	1,

	//是否保存ICW的结果到词库
	1,

	//首字母输入的最小汉字数目
	4,

	//在输入V后以英文方式进行输入
	0,			//允许在V后面输入空格

    //是否在数字键之后，输入英文符号 english_symbol_follow_number
	0,

    //使用TAB扩展汉字的候选  use_tab_expand_candidates
	1,

	//汉字输出的方式：简体、繁体。同时繁体字包含未分类汉字。
	HZ_OUTPUT_SIMPLIFIED,
//	HZ_OUTPUT_HANZI_ALL,
//	HZ_TRADITIONAL_EQU_OTHERS,		//繁体字包含属性为其他的字（日文/韩文汉字）

    //输入字的选项 hz_option
	HZ_RECENT_FIRST			|		//最近输入的字优先（默认）
	HZ_ADJUST_FREQ_FAST		|		//字输入调整字频（默认）
	HZ_USE_TAB_SWITCH_SET	|		//使用TAB切换汉字的集合
	HZ_USE_TWO_LEVEL_SET	|		//使用两种集合切分方式
	HZ_SYMBOL_CHINESE		|		//使用汉字符号
	HZ_SYMBOL_HALFSHAPE		|		//半角符号
	HZ_USE_FIX_TOP,					//使用固顶字

    //词输入选项 ci_option
	CI_AUTO_FUZZY			|		//输入词的时候，自动使用z/zh, c/ch, s/sh的模糊（默认）
	CI_SORT_CANDIDATES		|		//候选词基于词频进行排序（默认）
	CI_ADJUST_FREQ_FAST		|		//快速调整词频（默认）
	CI_WILDCARD				|		//输入词的时候，使用通配符（默认）
	CI_USE_FIRST_LETTER		|		//使用首字母输入词（默认）
	CI_RECENT_FIRST			|		//最新输入的词优先（默认）
	0,				//使用韵母自动匹配

    //是否使用模糊音 use_fuzzy
	1,

    //模糊音选项 fuzzy_mode
	1,								//全部不模糊

    //词库文件名称 wordlib_name
	{
		"/wordlib/user.uwl",
		"/wordlib/sys.uwl",
	},

    //词库数量 wordlib_count
    2,

    //一次能够输入的最大汉字数目 max_hz_per_input
	20,

    //显示在候选页中的候选的个数 candidates_per_line
	8,

    //在数字之后，"."作为英文符号 english_dot_follow_number
	1,

    //扩展候选的行数 expand_candidate_lines
	4,

    //将用户使用过的词汇记录到用户词库中，默认为0 insert_used_word_to_user_wl
	0,

    //备份目录 backup_dir
	TEXT(""),

    //配置版本 config_version
	CONFIG_VERSION,					//配置版本

    //记忆最近输入过的词 trace_recent
	0,

    //输入过程中忽略全角模式 ignore_fullshape
	1,

    //使用自定义短语 use_special_word
	1,

    //使用自定义符号 shuangpin_selItem
	1,

    //候选包含英文单词 candidate_include_english
    1,

    //英文单词后自动补空格 space_after_english_word
	1,

    //启动时进入英文输入法 startup_with_english_input
	0,

    //短语文件名称 spw_name
	{
		SYS_SPW_FILE_NAME,
		/*TEXT("unispim6\\phrase\\表情符号.ini"),
		TEXT("unispim6\\phrase\\数字符号.ini"),
		TEXT("unispim6\\phrase\\图形符号.ini"),
		TEXT("unispim6\\phrase\\中文字符.ini"),
		TEXT("unispim6\\phrase\\外文符号.ini"),
		TEXT("unispim6\\phrase\\字符画.ini"),
		TEXT("unispim6\\phrase\\用户短语.ini")*/
	},

    //使用英文输入 use_english_input
	1,

    //使用英文翻译 use_english_translate
	1,

	//use url hint
	//1,
	
	//单个短语候选时，显示位置；使用上面废弃的配置项；默认为3
    //spw_position spw_position
	13,

    //使用词语联想 use_word_suggestion
	1,

    //从第x个音节开始联想 suggest_syllable_location
	4,

    //联想词位于候选词第x位 suggest_word_location
	2,

    //联想词个数 suggest_word_count
	2,

    //use_u_hint use_u_hint
	1,

	//只输出GBK集合，scope_gbk
	HZ_SCOPE_GBK,

    //B模式开关 B_mode_enabled
	1,

    //I模式开关 I_mode_enabled
	1,

    //i模式开关 i_mode_enabled
	1,

    //u模式开关 u_mode_enabled
	1,

    //v模式开关 v_mode_enabled
	1,

    //中文字体名称 chinese_font_name
	TEXT("WenQuanYi Zen Hei"),

    "/usr/share/lunispim",

    "",

};
#pragma data_seg()

int no_main_show = 0;
int no_status_show = 0;
int	no_transparent = 0;
int no_ime_aware = 0;
int no_gdiplus_release = 0;
int no_multi_line = 0;
int no_virtual_key = 0;
int host_is_console = 0;
int no_end_composition = 0;
int no_ppjz = 0;//不是偏旁部首检字

int program_id;

/**	装载默认配置
 */
void LoadDefaultConfig()
{	
	pim_config = malloc(sizeof(PIMCONFIG));
	memcpy(pim_config, &default_config, sizeof(PIMCONFIG));
}

/**	清理配置
 */
void CleanConfig()
{
	free(pim_config);
}
/**	获得扩展候选的行数
 */
int GetExpandCandidateLine()
{
	return pim_config->expand_candidate_lines;
}
