/**	用户自造词模块
 *	用户自造词的读取以及检索功能。
 *	用户自造词简称“短语”, spw(special word)
 *
 *	1. 20000条用户自造词
 *	2. 每条可以达到1024个字节
 *	3. 短语格式：
 *			name = item1
 *
 *	格式定义
 *		项名称：英文字母打头，可以是大写字母，下划线可以使用。
 *		如：hello, he_ll, Hell_
 *		用等号表达后续的内容，如：china=中华人民共和国
 *		项的名字可以重复：如：
 *		china=中国
 *		china=中华人民共和国
 *		内容约束：
 *			短语名称：第一列必须是英文字母并且满足名称的要求；如：^ abcd=不是合法短语名称，作为内容处理。
 *			短语名称定义行中如果没有内容，则这个回车被忽略；如：abcd=			这个回车将被忽略
 *			后续的内容行中的空格作为内容进行处理（不忽略）；如：abcd=ab def ggef
 *			后续内容最后的空行将被忽略（一般用于进行短语的分隔）。abcd=
 *																ABCD
 *																			<-这个空行将被忽略
 *																			<-这个空行将被忽略
 *																efgh=abddefef
 *
 *			内容中可以出现=，只要等号前面不是短语名称的合法组合。如：abcd=e=mc2。
 *
 *	SPW的存储设计
 *		从文件中逐条读取数据，存放在spw_buffer中，该缓冲区大小为1M(char)，使用spw_index指向
 *		spw_buffer的位置。
 *		缓冲区的结构为：短语名称，短语内容。这些字符串必须以0为结尾。
 *
 *	SPW的特殊设计：
 *	采纳sougou拼音的日期与时间输入方式进行候选选择，方便用户
 *	rq_, RQ, Rq, rQ->日期输入
 *		2007-4-26，2007.4.26，2007-04-26，2007年4月26日，二〇〇七年四月二十六日，
 *	sj_, SJ, Sj, sJ->时间输入
 *		2007-4-26 10:49，10:49，10:49:47, 2007年4月26日10:49:44，
 *
 */

/*	装载用户自造词库，如果已经在内存，则返回SPW词库指针，否则
 *	进行内存的加载。
 *	参数：
 *		spw_file_name			自造词库文件名
 *	返回：
 *		装载成功：spw词库指针
 *		失败：0
 */
#include <zi.h>
#include <assert.h>
#include <string.h>
#include <spw.h>
#include <config.h>
#include <utility.h>
#include <vc.h>
#include <pim_resource.h>
#include <fontcheck.h>
#include <gbk_map.h>
#include <share_segment.h>
#include <context.h>
#include <unistr.h>
#include <unistdio.h>
#include <ctype.h>
#include <utf16char.h>
//#include <shlwapi.h>

static TCHAR *spw_buffer	 = 0;
static char *spw_share_name = "HYPIM_SPW_SHARED_NAME";

static const TCHAR digit_hz_string[][4] = 
{	
	TEXT("〇"), TEXT("一"), TEXT("二"), TEXT("三"), TEXT("四"), TEXT("五"),
	TEXT("六"), TEXT("七"), TEXT("八"), TEXT("九"), TEXT("十")
};

//日期输入前缀
static const TCHAR date_spw_string[][8] =
{	TEXT("rq_"), TEXT("rq:"), TEXT("RQ"), TEXT("Rq"), TEXT("rQ"), TEXT("date"), };

//时间输入前缀
static const TCHAR time_spw_string[][8] =
{	TEXT("sj_"), TEXT("sj:"), TEXT("SJ"), TEXT("Sj"), TEXT("sJ"), TEXT("time"), };

static const char DEFAULT_SPW_FILENAME[][MAX_FILE_NAME_LENGTH]={
	SYS_SPW_FILE_NAME,
};

static const TCHAR special_hint[][MAX_SPW_HINT_STRING] =
{
	TEXT("i"),	TEXT("小写数字单位模式(I为大写)"),
	TEXT("U"),	TEXT("小写数字单位模式(I为大写)"),
	TEXT("U+"),	TEXT("UNICODE字符输入模式"),
	TEXT("I"),	TEXT("大写数字单位模式(i/U为小写)"),
	TEXT("E"),	TEXT("输入程序(文档)名，按TAB输入参数，按空格执行"),
	TEXT("u"),	TEXT("输入程序(文档)名，按TAB输入参数，按空格执行"),
	TEXT("v"),	TEXT("英文输入模式，TAB为输入空格"),
	TEXT("B"),	TEXT("笔画模式：h横、s竖、p撇、n捺、z折、d点"),
};

static const TCHAR special_hint_sp[][MAX_SPW_HINT_STRING] =
{
	TEXT("i"), TEXT("小写数字单位模式(I为大写)"),
	TEXT("U"), TEXT("小写数字单位模式(I为大写)"),
	TEXT("U+"),	TEXT("UNICODE字符输入模式"),
	TEXT("I"), TEXT("大写数字单位模式(U为小写)"),
	TEXT("E"), TEXT("输入程序(文档)名，按TAB输入参数，按空格执行"),
	TEXT("u"), TEXT("输入程序(文档)名，按TAB输入参数，按空格执行"),
	TEXT("v"), TEXT("英文输入模式，TAB为输入空格"),
	TEXT("B"), TEXT("笔画模式：h横、s竖、p撇、n捺、z折、d点"),
};

static TCHAR *hz_xx_numbers = TEXT("〇一二三四五六七八九十");
static TCHAR *hz_dx_numbers = TEXT("零壹贰叁肆伍陆柒捌玖拾");

static TCHAR *i_numcadidates[] = 
{
	TEXT("一二三四五六七八九十"),
	TEXT("①②③④⑤⑥⑦⑧⑨⑩"),
	TEXT("ⅰⅱⅲⅳⅴⅵⅶⅷⅸⅹ"),
	TEXT("➊➋➌➍➎➏➐➑➒➓"),
	TEXT("⒈⒉⒊⒋⒌⒍⒎⒏⒐⒑"),
	TEXT("⑴⑵⑶⑷⑸⑹⑺⑻⑼⑽"),
	TEXT("壹贰叁肆伍陆柒捌玖拾"),
	TEXT("ⅠⅡⅢⅣⅤⅥⅦⅧⅨⅩ"),
	TEXT("㈠㈡㈢㈣㈤㈥㈦㈧㈨㈩"),
	TEXT("㊀㊁㊂㊃㊄㊅㊆㊇㊈㊉")
};
static TCHAR *I_numcadidates[] = 
{
	TEXT("壹贰叁肆伍陆柒捌玖拾"),
	TEXT("ⅠⅡⅢⅣⅤⅥⅦⅧⅨⅩ"),
	TEXT("㈠㈡㈢㈣㈤㈥㈦㈧㈨㈩"),
	TEXT("㊀㊁㊂㊃㊄㊅㊆㊇㊈㊉"),
	TEXT("一二三四五六七八九十"),
	TEXT("①②③④⑤⑥⑦⑧⑨⑩"),
	TEXT("ⅰⅱⅲⅳⅴⅵⅶⅷⅸⅹ"),
	TEXT("➊➋➌➍➎➏➐➑➒➓"),
	TEXT("⒈⒉⒊⒋⒌⒍⒎⒏⒐⒑"),
	TEXT("⑴⑵⑶⑷⑸⑹⑺⑻⑼⑽")
};
static TCHAR *ext_unit[] = 
{ 
	TEXT(""), TEXT("万"), TEXT("亿"), TEXT("万亿"), TEXT("亿亿"), TEXT("万亿亿"),
	TEXT("亿亿亿"), TEXT("万亿亿亿"), TEXT("亿亿亿亿"),
};

static TCHAR *base_xx_unit[] = { TEXT(""), TEXT("十"), TEXT("百"), TEXT("千"), };
static TCHAR *base_dx_unit[] = { TEXT(""), TEXT("拾"), TEXT("佰"), TEXT("仟"), };

static TCHAR *digital_xx_string[] = 
{	
	TEXT("零"), TEXT("一"), TEXT("二"), TEXT("三"), TEXT("四"), TEXT("五"),
	TEXT("六"), TEXT("七"), TEXT("八"), TEXT("九")
};

static TCHAR *digital_dx_string[] = 
{	TEXT("零"), TEXT("壹"), TEXT("贰"), TEXT("叁"), TEXT("肆"), TEXT("伍"), 
	TEXT("陆"), TEXT("柒"), TEXT("捌"), TEXT("玖")
};

static TCHAR *money_unit[] = {TEXT("整"), TEXT("角"), TEXT("分")};

//转换数字串，简单转换(0-〇，1-一，2-二，3-三...)
void GetSimpleNumberString(const TCHAR *input, TCHAR *buffer, int length, int dx_number, int chinese_number)
{
	TCHAR *numbers, *p, *q, ch;
	int i;

	if (dx_number)
		numbers = hz_dx_numbers;
	else
		numbers = hz_xx_numbers;
	for (p = buffer, i = 0; p < buffer + length - 2 && i < (int)utf16_strlen(input); i++)
	{
		ch = input[i];
		if (ch >= '0' && ch <= '9' && chinese_number)
		{
			q = &numbers[(ch - '0')];
			*p = *q;
			p ++;
		}
		else if(ch == '.'){
			*p = TEXT('点');
			p++;
		}
		else
		{
			*p = ch;
			p++;
		}
	}
	if (p != buffer)
		*p = 0;
}

int LetterModeEnabled(TCHAR Letter)
{
	if (!pim_config->B_mode_enabled && Letter == 'B')
		return 0;

	if (!pim_config->i_mode_enabled && (Letter == 'i' || Letter == 'U'))
		return 0;

	if (!pim_config->I_mode_enabled && Letter == 'I')
		return 0;

	if (!pim_config->u_mode_enabled && (Letter == 'u' || Letter == 'E'))
		return 0;

	if (!pim_config->v_mode_enabled && Letter == 'v')
		return 0;

	return 1;
}

/**	获得短语输入的Hint信息
 */
const TCHAR *GetSPWHintString(const TCHAR *input_string)
{
	int i;

	if (!input_string)
		return 0;

	if (pim_config->pinyin_mode == PINYIN_QUANPIN)
	{
		for (i = 0; i < sizeof(special_hint) / _SizeOf(special_hint[0]) / sizeof(TCHAR); i += 2)
		{
			if (!LetterModeEnabled(input_string[0]))
				continue;

			if (!utf16_strcmp(input_string, special_hint[i]))
				return special_hint[i + 1];
		}
	}
	else if(pim_config->pinyin_mode == PINYIN_SHUANGPIN && input_string[0] != 'u')
	{
		for (i = 0; i < sizeof(special_hint_sp) / _SizeOf(special_hint_sp[0]) / sizeof(TCHAR); i += 2)
		{
			if (!LetterModeEnabled(input_string[0]))
				continue;

			if (!utf16_strcmp(input_string, special_hint_sp[i]))
				return special_hint_sp[i + 1];
		}
	}

	return 0;
}

/**	判断是否为日期输入的前缀
 */
static int IsDatePrefix(const TCHAR *input_string)
{
	int i;

	for (i = 0; i < sizeof(date_spw_string) / _SizeOf(date_spw_string[0]) / sizeof(TCHAR); i++)
		if (!utf16_strcmp(input_string, date_spw_string[i]))
			return 1;

	return 0;
}

/**	判断是否为时间输入前缀
 */
static int IsTimePrefix(const TCHAR *input_string)
{
	int i;

	for (i = 0; i < sizeof(time_spw_string) / _SizeOf(time_spw_string[0]) / sizeof(TCHAR); i++)
		if (!utf16_strcmp(input_string, time_spw_string[i]))
			return 1;

	return 0;
}

/**	生成日期的候选
 *	返回：候选数目
 */
static int GenerateDateCandidate(CANDIDATE *candidate_array, int array_length)
{
	static TCHAR date_candidate_string[10][0x20];
	static char date_candidate_string_u8[0x60];
	static int i;
	iconv_t cd;
	int year, month, day, hour, minute, second;

	GetTimeValue(&year, &month, &day, &hour, &minute, &second);

	//0.2007-04-26，1.2007.4.26，2.07-04-26, 3.2007年04月26日，4.二〇〇七年四月二十六日，
	cd = IconvOpen("UTF-8", "UTF-16");
	sprintf (date_candidate_string_u8, "%04d-%02d-%02d", year, month, day);
	CodeConvert(cd, date_candidate_string_u8, 0x60, (char*)date_candidate_string[0], 0x20*2);
	sprintf (date_candidate_string_u8, "%d%02d%02d", year, month, day);
	CodeConvert(cd, date_candidate_string_u8, 0x60, (char*)date_candidate_string[1], 0x20*2);
	sprintf (date_candidate_string_u8, "%02d-%02d-%02d", year % 100, month, day);
	CodeConvert(cd, date_candidate_string_u8, 0x60, (char*)date_candidate_string[2], 0x20*2);
	sprintf (date_candidate_string_u8, "%d年%d月%d日", year, month, day);
	CodeConvert(cd, date_candidate_string_u8, 0x60, (char*)date_candidate_string[3], 0x20*2);
	IconvClose(cd);
	date_candidate_string[4][0] = 0;
	_tcscat_s(date_candidate_string[4], _SizeOf(date_candidate_string[0]), digit_hz_string[(year / 1000) % 10]);
	_tcscat_s(date_candidate_string[4], _SizeOf(date_candidate_string[0]), digit_hz_string[(year / 100) % 10]);
	_tcscat_s(date_candidate_string[4], _SizeOf(date_candidate_string[0]), digit_hz_string[(year / 10) % 10]);
	_tcscat_s(date_candidate_string[4], _SizeOf(date_candidate_string[0]), digit_hz_string[year % 10]);
	_tcscat_s(date_candidate_string[4], _SizeOf(date_candidate_string[0]), TEXT("年"));

	if (month >= 10)
		_tcscat_s(date_candidate_string[4], _SizeOf(date_candidate_string[0]), TEXT("十"));

	if (month % 10)
		_tcscat_s(date_candidate_string[4], _SizeOf(date_candidate_string[0]), digit_hz_string[month % 10]);

	_tcscat_s(date_candidate_string[4], _SizeOf(date_candidate_string[0]), TEXT("月"));

	if (day >= 10)
	{
		if (day >= 20)
			_tcscat_s(date_candidate_string[4], _SizeOf(date_candidate_string[0]), digit_hz_string[day / 10]);

		_tcscat_s(date_candidate_string[4], _SizeOf(date_candidate_string[0]), TEXT("十"));
	}
	if (day % 10)
		_tcscat_s(date_candidate_string[4], _SizeOf(date_candidate_string[0]), digit_hz_string[day % 10]);

	_tcscat_s(date_candidate_string[4], _SizeOf(date_candidate_string[0]), TEXT("日"));

	for (i = 0; i < 5 && i < array_length; i++)
	{
		candidate_array[i].type			= CAND_TYPE_SPW;
		candidate_array[i].spw.type		= SPW_STRING_NORMAL;
		candidate_array[i].spw.string	= date_candidate_string[i];
		candidate_array[i].spw.hint		= 0;
		candidate_array[i].spw.length	= (int)utf16_strlen(date_candidate_string[i]);
	}

	return i;
}

/**	生成时间的候选
 *	返回：候选数目
 */
static int GenerateTimeCandidate(CANDIDATE *candidate_array, int array_length)
{
	static TCHAR time_candidate_string[10][0x20];
	static char time_candidate_string_u8[4][0x60];
	static int  i;
	iconv_t cd;
	int year, month, day, hour, minute, second;

	GetTimeValue(&year, &month, &day, &hour, &minute, &second);

	//0.2007-4-26 10:49，1.10:49，2.10:49:47, 3.2007年4月26日10:49:44，
	sprintf(time_candidate_string_u8[0], "%02d:%02d", hour, minute);
	sprintf(time_candidate_string_u8[1], "%04d-%02d-%02d %02d:%02d", year, month, day, hour, minute);
	sprintf(time_candidate_string_u8[2], "%02d:%02d:%02d", hour, minute, second);
	sprintf(time_candidate_string_u8[3], "%04d年%02d月%02d日 %02d:%02d:%02d", year, month, day, hour, minute, second);
	cd = IconvOpen("UTF-8", "UTF-16");
	for (i = 0; i < 4 && i < array_length; i++)
	{
		CodeConvert(cd, time_candidate_string_u8[i], 0x60, (char*)time_candidate_string[i], 0x20*2);
		candidate_array[i].type			= CAND_TYPE_SPW;
		candidate_array[i].spw.type		= SPW_STRING_NORMAL;
		candidate_array[i].spw.string	= time_candidate_string[i];
		candidate_array[i].spw.hint		= 0;
		candidate_array[i].spw.length	= (int)utf16_strlen(time_candidate_string[i]);
	}
	IconvClose(cd);
	return i;
}

/**	向短语缓冲区内插入短语。
 *	参数：
 *		name			短语名称
 *		content			短语内容
 *	返回：
 *		失败：0
 *		成功：1
 */
int InsertSpw(const TCHAR *name, TCHAR *hint, TCHAR *content)
{
	int name_length, content_length, hint_length;
	int i, index = 0;

	assert(name && content);

	name_length		= (int)utf16_strlen(name);
	content_length	= (int)utf16_strlen(content);
	hint_length		= (int)utf16_strlen(hint);

	//检查是否会发生越界
	if (share_segment->spw_length + name_length + content_length + 2 > SPW_BUFFER_SIZE)
	{
		printf("spw.c : 短语缓冲区长度不足，不能插入短语，length=%d, name=%s, content=%s\n", share_segment->spw_length, name, content);
		return 0;
	}

	if (share_segment->spw_count >= SPW_MAX_ITEMS)
	{
		printf("spw.c : 短语个数超过限制，不能插入短语，count=%d, name=%s, content=%s\n", share_segment->spw_count, name, content);
		return 0;
	}

	//如果长度小于128，则只用0d标识回车
	if (content_length < 128)
	{
		for (i = 0, index = 0; content[i]; i++)
			if (content[i] != 0xa)
				content[index++] = content[i];

		content[index] = 0;
		content_length = index;
	}

	if (!name[0])
		return 1;	//不存在名称，不插入，也不以错误返回。

	share_segment->spw_index[share_segment->spw_count++] = share_segment->spw_length;
	utf16_strcpy(spw_buffer + share_segment->spw_length, name);

	share_segment->spw_length += name_length + 1;

	if (hint_length)
	{
		utf16_strcpy(spw_buffer + share_segment->spw_length, hint);
		share_segment->spw_length += hint_length + 1;
		hint[0] = 0;
	}

	if (!hint_length && content_length && content[0] == SPW_HINT_LEFT_CHAR)
	{
		utf16_strcpy(spw_buffer + share_segment->spw_length, SPW_HINT_NULL_STR);
		share_segment->spw_length += (int)utf16_strlen(SPW_HINT_NULL_STR) + 1;
	}

	utf16_strcpy(spw_buffer + share_segment->spw_length, content);
	share_segment->spw_length += content_length + 1;

	return 1;
}

/**	获得数字的字符串，如：20005.00617	-> 两万零五点零零六一七
 */
void GetComplexNumberString(const TCHAR *input, TCHAR *buffer, int length, int dx_number)
{
	TCHAR tmp[0x10];
	TCHAR xs_string[0x100];						//小数字符串
	TCHAR zs_string[0x100];						//整数字符串
	TCHAR no_string[0x100];						//数字字符串
	int  zs_count;								//整数计数
	int  i, dot_index, input_length;
	int  is_zero, last_zero, all_zero;
	int  has_prefix_zero;
	TCHAR **base_unit, **digital_string;

	if (!input || !*input || !buffer || !length)
		return;

	if (dx_number)
	{
		base_unit = base_dx_unit;
		digital_string = digital_dx_string;
	}
	else
	{
		base_unit = base_xx_unit;
		digital_string = digital_xx_string;
	}

	*buffer = no_string[0] = zs_string[0] = xs_string[0] = 0;

	//检查符号
	if (input[0] == '-')
	{
		utf16_strcpy(no_string, TEXT("负"));
		input++;
	}

	//删除前导0
	has_prefix_zero = 0;
	while(*input && *input == '0')
	{
		input++;
		has_prefix_zero = 1;
	}

	//检查合法性
	input_length = (int)utf16_strlen(input);
	dot_index = -1;
	for (i = 0; i < input_length; i++)
	{
		if (input[i] >= '0' && input[i] <= '9')
			continue;

		if (input[i] != '.')		//小数点
			return;

		if (dot_index != -1)		//两个以上小数点
			return;

		dot_index = i;
	}

	if (dot_index == -1)		//没有小数点
		dot_index = input_length;

	//将小数部分提取
	for (i = dot_index; i < input_length && utf16_strlen(xs_string) < _SizeOf(xs_string) - 4; i++)
		utf16_strcat(xs_string, input[i] == '.' ? TEXT("点") : digital_string[input[i] - '0']);

	//将整数部分提取
	zs_count = dot_index;
	if (zs_count > 4 * (int)sizeof(ext_unit) / sizeof(ext_unit[0]))				//判断是否越过最大边界
		zs_count = 4 * (int)sizeof(ext_unit) / sizeof(ext_unit[0]) - 1;

	//形成整数字符串, 4个一组进行赋值
	last_zero = 0, all_zero = 1;
	for (i = dot_index - zs_count; i < zs_count; i++)
	{
		int index = zs_count - 1 - i;
		tmp[0] = 0;
		is_zero = input[i] == '0';

		if (!is_zero)
		{
			if (last_zero)
				utf16_strcat(tmp, digital_string[0]);

			if (index % 4 != 1 || input[i] != '1' || i)			//10只在前面没有值的时候为“十”
				utf16_strcat(tmp, digital_string[input[i] - '0']);

			utf16_strcat(tmp, base_unit[index % 4]);
			all_zero = 0;
		}

		//到“万”位
		if (index % 4 == 0 && !all_zero)
		{
			utf16_strcat(tmp, ext_unit[index / 4]);
			all_zero = 1;
		}

		last_zero = is_zero;
		utf16_strcat(zs_string, tmp);
	}

	//检查是否前面都为零
	if (!zs_string[0] && has_prefix_zero)
		utf16_strcpy(zs_string, TEXT("零"));

	utf16_strcat(no_string, zs_string);
	utf16_strcat(no_string, xs_string);

	utf16_strncpy(buffer, no_string, length / 2 * 2);		//避免越界
}

//private method
//append candidate
void AppendSPWCandidate(CANDIDATE *candidates, int *count, int length, const TCHAR *buffer){
	int i;
	//判断是否有重复候选
	for(i = 0 ; i < (*count); i++){
		if(!utf16_strcmp(candidates[i].spw.string, buffer))
			return;
	}
	candidates[*count].type		 = CAND_TYPE_SPW;
	candidates[*count].spw.type	 = SPW_STRING_SPECIAL;
	candidates[*count].spw.length = length;
	candidates[*count].spw.string = buffer;
	(*count)++;
}

/*--sunmd
字符串截取函数
*/
void substr(TCHAR *dest, const TCHAR* src, unsigned int start, unsigned int cnt)
{
	utf16_strncpy(dest, src + start, cnt);
	dest[cnt] = 0;
}

/**--sunmd
*获得时间的字符串，如：
*   "i20:09"-> 二十点九分、20点09分；
*   "i20:"->二十点、20点；
*   "i20:09:09"->二十点九分九秒、20点09分09秒
 */
void GetTimeString(const TCHAR *input, TCHAR *buffer, TCHAR *buffer1, TCHAR *buffer2, CANDIDATE *candidates, int *count, int dx_number)
{
	TCHAR chrSplit = ':';
	TCHAR *right_string,*left_string;
	TCHAR sHour[10];
	TCHAR sMinute[10];
	TCHAR sSecond[10];
	TCHAR temp[0x100];

	if (!input || !*input || !buffer)
		return;
	*buffer = *buffer1 = *buffer2 = sHour[0] = sMinute[0] = sSecond[0] = 0;
	left_string  =  utf16_strchr (input,chrSplit);
	right_string = utf16_strrchr(input,chrSplit);
	//有两个分隔符时
	if (left_string != right_string)
	{
		if (utf16_strlen(right_string) > 1)
			substr(sSecond, right_string, 1, (int)utf16_strlen(right_string) - 1);

		substr(sMinute, left_string, 1, (int)utf16_strlen(left_string) - (int)utf16_strlen(right_string) - 1);
		substr(sHour, input, 0, (int)utf16_strlen(input) - (int)utf16_strlen(left_string));
	}
	else
	{//只有一个分隔符
		if (utf16_strlen(left_string) > 1)
			substr(sMinute, left_string, 1, (int)utf16_strlen(left_string) - 1);
		substr(sHour, input, 0, (int)utf16_strlen(input) - (int)utf16_strlen(left_string));
	}

	if(!sHour[0])
		return;
	if (!dx_number)
	{
		utf16_strcat(buffer, sHour);
		utf16_strcat(buffer, TEXT("点"));
		utf16_strcat(buffer1, sHour);
		utf16_strcat(buffer1, TEXT("时"));
		if(sMinute[0])
		{
			utf16_strcat(buffer, sMinute);
			utf16_strcat(buffer, TEXT("分"));
			utf16_strcat(buffer1, sMinute);
			utf16_strcat(buffer1, TEXT("分"));
		}

		if(sSecond[0])
		{
			utf16_strcat(buffer, sSecond);
			utf16_strcat(buffer, TEXT("秒"));
			utf16_strcat(buffer1, sSecond);
			utf16_strcat(buffer1, TEXT("秒"));
		}
		else
		{
			utf16_strcat(buffer2, sHour);
			utf16_strcat(buffer2, TEXT("分"));
			if(sMinute[0]){
				utf16_strcat(buffer2, sMinute);
				utf16_strcat(buffer2, TEXT("秒"));
			}
		}
	}
	else
	{
		GetComplexNumberString(sHour, temp, _SizeOf(temp), 0);
		utf16_strcat(buffer, temp);
		utf16_strcat(buffer, TEXT("点"));
		utf16_strcat(buffer1, temp);
		utf16_strcat(buffer1, TEXT("时"));
		if (sMinute[0])
		{
			GetComplexNumberString(sMinute, temp, _SizeOf(temp), 0);
			utf16_strcat(buffer, temp);
			utf16_strcat(buffer, TEXT("分"));
			utf16_strcat(buffer1, temp);
			utf16_strcat(buffer1, TEXT("分"));
		}

		if(sSecond[0])
		{
			GetComplexNumberString(sSecond, temp, _SizeOf(temp), 0);
			utf16_strcat(buffer, temp);
			utf16_strcat(buffer, TEXT("秒"));
			utf16_strcat(buffer1, temp);
			utf16_strcat(buffer1, TEXT("秒"));
		}
		else
		{
			GetComplexNumberString(sHour, temp, _SizeOf(temp), 0);
			utf16_strcat(buffer2, temp);
			utf16_strcat(buffer2, TEXT("分"));
			if(sMinute[0]){
				GetComplexNumberString(sMinute, temp, _SizeOf(temp), 0);
				utf16_strcat(buffer2, temp);
				utf16_strcat(buffer2, TEXT("秒"));
			}
		}
	}
	//大写：如"i20:2:3"－》二十点二分三秒
	if (buffer[0])
		AppendSPWCandidate(candidates, count, (int)utf16_strlen(buffer), buffer);
	//大写：如"i20:2:3"－》二十时二分三秒
	if(buffer1[0])
		AppendSPWCandidate(candidates, count, (int)utf16_strlen(buffer1), buffer1);
	//大写：如"i20:2"－》二十分二秒
	if(buffer2[0])
		AppendSPWCandidate(candidates, count, (int)utf16_strlen(buffer2), buffer2);
}

/**--sunmd
*获得日期的字符串，如：
*   "i2005-09"-> 二〇〇五年九月、2005年9月；
*   "i2009-"->二〇〇九年、2009年；
*   "i2009-09-09"->二〇〇九年九月九日、2009年9月9日
*   "i20090807"->二〇〇九年八月七日、2009年08月07日
 */
void GetDateString(const TCHAR *input, TCHAR *buffer, TCHAR *buffer1, CANDIDATE *candidates, int *count, int dx_number)
{
	TCHAR chrSplit;
	TCHAR *right_string, *left_string;
	TCHAR sYear[10];					//年份
	TCHAR sMonth[10];					//月份
	TCHAR sDay[10];						//日期
	TCHAR temp[10];
	int iYear;

	if (!input || !*input || !buffer )
		return;
	*buffer = *buffer1 = sYear[0] = sMonth[0] = sDay[0]=0;
	if ( utf16_strchr(input, '-'))
		chrSplit = '-';
	else if (utf16_strchr(input, '/'))
		chrSplit = '/';
	else
		chrSplit = '0';

	//带分隔符的日期，如2009/09/09、2009-09-09
	if (chrSplit != '0')
	{
		left_string  = utf16_strchr(input, chrSplit);
		right_string = utf16_strrchr(input, chrSplit);

		//有两个分隔符时
		if(left_string != right_string)
		{
			if((int)utf16_strlen(right_string) > 1)
				substr(sDay, right_string, 1, (int)utf16_strlen(right_string) - 1);

			substr(sMonth, left_string, 1, (int)utf16_strlen(left_string) - (int)utf16_strlen(right_string) - 1);
			substr(sYear, input, 0, (int)utf16_strlen(input) - (int)utf16_strlen(left_string));
		}
		//只有一个分隔符
		else
		{
			if((int)utf16_strlen(left_string)>1)
				substr(sMonth, left_string, 1, (int)utf16_strlen(left_string) - 1);

			substr(sYear, input, 0, (int)utf16_strlen(input) - (int)utf16_strlen(left_string));
		}
	}
	//不带分隔符的日期，如20090807
	else
	{
		substr(sYear, input, 0, 4);
		substr(sMonth, input, 4, 2);
		substr(sDay, input, 6, 4);
	}
	if(!sYear[0])
		return;

	iYear = _tstoi(sYear);
	if (!dx_number)
	{
		utf16_strcat(buffer, sYear);
		utf16_strcat(buffer, TEXT("年"));
		if(sMonth[0])
		{
			utf16_strcat(buffer, sMonth);
			utf16_strcat(buffer, TEXT("月"));
		}

		if(sDay[0])
		{
			utf16_strcat(buffer, sDay);
			utf16_strcat(buffer, TEXT("日"));
		}
		else{
			if(iYear>0 && iYear<=12){
				utf16_strcat(buffer1, sYear);
				utf16_strcat(buffer1, TEXT("月"));
				if(sMonth[0]){
					utf16_strcat(buffer1, sMonth);
					utf16_strcat(buffer1, TEXT("日"));
				}
			}
		}
	}
	else
	{
		GetSimpleNumberString(sYear, temp, _SizeOf(temp), 0, 1);
		utf16_strcat(buffer, temp);
		utf16_strcat(buffer, TEXT("年"));
		if(sMonth[0])
		{
			GetComplexNumberString(sMonth, temp, _SizeOf(temp), 0);
			utf16_strcat(buffer, temp);
			utf16_strcat(buffer, TEXT("月"));
		}

		if(sDay[0])
		{
			GetComplexNumberString(sDay, temp, _SizeOf(temp), 0);
			utf16_strcat(buffer, temp);
			utf16_strcat(buffer, TEXT("日"));
		}
		else{
			if(iYear>0 && iYear<=12){
				GetComplexNumberString(sYear, temp, _SizeOf(temp), 0);
				utf16_strcat(buffer1, temp);
				utf16_strcat(buffer1, TEXT("月"));
				if(sMonth[0]){
					GetComplexNumberString(sMonth, temp, _SizeOf(temp), 0);
					utf16_strcat(buffer1, temp);
					utf16_strcat(buffer1, TEXT("日"));
				}
			}
		}
	}
	if (buffer1[0])
		AppendSPWCandidate(candidates, count, (int)utf16_strlen(buffer1), buffer1);
	if (buffer[0])
		AppendSPWCandidate(candidates, count, (int)utf16_strlen(buffer), buffer);
}

/*  --sunmd
*	获得元角分的字符串，如：
*     i123.-->一百二十三元、壹佰贰拾叁元、壹佰贰拾叁圆
*     i123.0或i123.00-->一百二十三元整、壹佰贰拾叁元整、壹佰贰拾叁圆整
*     i123.5-->一百二十三元五角、壹佰贰拾叁元伍角、壹佰贰拾叁圆伍角
*     i123.50-->一百二十三元五角整、壹佰贰拾叁元伍角整、壹佰贰拾叁圆伍角整
*     i123.51-->一百二十三元五角一分、壹佰贰拾叁元伍角壹分、壹佰贰拾叁圆伍角壹分
 */
void GetMoneyNumberString(const TCHAR *input, TCHAR *buffer, int length, int dx_number)
{
	TCHAR tmp[0x10];
	TCHAR xs_string[0x100];						//小数字符串
	TCHAR zs_string[0x100];						//整数字符串
	TCHAR no_string[0x100];						//数字字符串
	int  zs_count;								//整数计数
	int  i,j, dot_index, input_length;
	int  is_zero, last_zero, all_zero;
	int  has_prefix_zero;
	TCHAR **base_unit, **digital_string, *base_money_unit;

	if (!input || !*input || !buffer || !length)
		return;

	base_money_unit=TEXT("元");
	if (dx_number)
	{
		base_unit = base_dx_unit;
		if(dx_number==2)
			base_money_unit=TEXT("圆");
		digital_string = digital_dx_string;
	}
	else
	{
		base_unit = base_xx_unit;
		digital_string = digital_xx_string;
	}

	*buffer = no_string[0] = zs_string[0] = xs_string[0] =tmp[0]= 0;

	//删除前导0
	has_prefix_zero = 0;
	while(*input && *input == '0')
	{
		input++;
		has_prefix_zero = 1;
	}

	//判断“.”所在位置
	input_length = (int)utf16_strlen(input);
	for (i = 0; i < input_length; i++)
	{
		if (input[i] >= '0' && input[i] <= '9')
			continue;

		dot_index = i;
		break;
	}

	//如果第一位是“.”，则会生成“零元”
	if (dot_index==0)
		has_prefix_zero=1;

	//将小数部分提取
	j = input_length - dot_index;
	if (input[dot_index] == '.')
	{
		if ((j == 2 && input[dot_index + 1] == '0') ||
				(j==3 && input[dot_index + 1] == '0' && input[dot_index + 2] == '0'))
		{//i123.0或者i123.00
			utf16_strcat(xs_string, base_money_unit);
			utf16_strcat(xs_string, money_unit[0]);
		}
		else if (j == 3 && input[dot_index + 1] != '0' && input[dot_index + 2] == '0')
		{//i123.50
			utf16_strcat(xs_string, base_money_unit);
			utf16_strcat(xs_string, digital_string[input[dot_index+1] - '0']);
			utf16_strcat(xs_string, money_unit[1]);
			utf16_strcat(xs_string, money_unit[0]);
		}
		else if (j == 3 && input[dot_index + 1] == '0' && input[dot_index + 2] != '0')
		{//i123.01
			utf16_strcat(xs_string, base_money_unit);
			utf16_strcat(xs_string, digital_string[0]);
			utf16_strcat(xs_string, digital_string[input[dot_index + 2] - '0']);
			utf16_strcat(xs_string, money_unit[2]);
		}
		else
		{
			for (i = dot_index;i<input_length && utf16_strlen(xs_string) < _SizeOf(xs_string) - 4; i++)
			{
				utf16_strcat(xs_string, input[i] == '.' ? base_money_unit : digital_string[input[i] - '0']);

				if (input[i] != '.')
					utf16_strcat(xs_string,money_unit[i-dot_index]);
			}
		}
	}

	//将整数部分提取
	zs_count = dot_index;
	if (zs_count > 4 * (int)sizeof(ext_unit) / sizeof(ext_unit[0]))				//判断是否越过最大边界
		zs_count = 4 * (int)sizeof(ext_unit) / sizeof(ext_unit[0]) - 1;

	//形成整数字符串, 4个一组进行赋值
	last_zero = 0, all_zero = 1;
	for (i = dot_index - zs_count; i < zs_count; i++)
	{
		int index = zs_count - 1 - i;
		tmp[0] = 0;
		is_zero = input[i] == '0';

		if (!is_zero)
		{
			if (last_zero)
				utf16_strcat(tmp, digital_string[0]);

			if (index % 4 != 1 || input[i] != '1' || i || dx_number)			//10只在前面没有值的时候为“十”
				utf16_strcat(tmp, digital_string[input[i] - '0']);

			utf16_strcat(tmp, base_unit[index % 4]);
			all_zero = 0;
		}

		//到“万”位
		if (index % 4 == 0 && !all_zero)
		{
			utf16_strcat(tmp, ext_unit[index / 4]);
			all_zero = 1;
		}

		last_zero = is_zero;
		utf16_strcat(zs_string, tmp);
	}

	//检查是否前面都为零
	if (!zs_string[0] && has_prefix_zero)
		utf16_strcpy(zs_string, TEXT("零"));

	utf16_strcat(no_string, zs_string);
	utf16_strcat(no_string, xs_string);

	utf16_strncpy(buffer, no_string, length / 2 * 2);		//避免越界
}

/*
--sunmd
判断i输入的是否为时间，条件如下：
（1）输入串中包含1到2个“:”
（2）输入串中，除“:”外，其他都是数字
（3）“:”的前面最少有1个数字，最多有2位数字
（4）“:”的后面最多有两位数字
（5）如："i12:13:24-"->“十二点十三分二十四秒”、“12点13分24秒”，"i09:9"->"九点九分"、“09点9分”
*/
int IsTime4IPre(const TCHAR *input)
{
	TCHAR *right_string, *left_string;
	TCHAR chrSplit = ':';
	int iDisSplit = 0;
	int iTemp = 0;

	left_string  = utf16_strchr(input, chrSplit);
	right_string = utf16_strrchr(input, chrSplit);
	//存在“:”
	if (!left_string)
		return 0;
	//第一位不能是“:”
	if (input[1] == chrSplit)
		return 0;
	//第一个“:”前面不能超过两位数字
	if (utf16_strlen(input) - utf16_strlen(left_string) > 3)
		return 0;
	if (utf16_strlen(right_string) > 3)
		return 0;
	//多于一个":"
	if (left_string != right_string)
	{
		*left_string++;
		//多于两个':'
		if (utf16_strchr(left_string,chrSplit) != right_string)
			return 0;
		iDisSplit = (int)utf16_strlen(left_string) - (int)utf16_strlen(right_string);
		//两个‘:’相邻
		if (iDisSplit == 0)
			return 0;
		//两个'-'相隔太远，大于3
		if (iDisSplit > 2)
			return 0;
	}
	//除“:”外，应该全部为数字
	*input++;
	while (*input)
	{
		if(input[0] != chrSplit && !isdigit(*input))
			return 0;
		else
			*input++;
	}
	return 1;
}

/*--sunmd
*判断i输入的是否为日期
*条件：（1）串长度为9，除i外，全部是数字
*      （2）前四位转换为数字后，在1000到3000之间
*      （3）中间两位转换为数字后，在1到12之间
*      （4）最后两位转换为数字后，在1到31之间
*      （5）形如：i20090908－》二〇〇九年九月八日、2009年09月08日
*/
int IsUnsignDate4IPre(const TCHAR *input)
{
	TCHAR temp[0x100];
	int iTemp = 0;
	unsigned int i;

	temp[0] = 0;
	if (utf16_strlen(input)!=9)
		return 0;

	utf16_strcat(temp,input);

	//除“i”外，应该全部为数字
	for (i = 1; i < utf16_strlen(temp); i++)
	{
		if (!isdigit(temp[i]))
			return 0;
	}

	substr(temp, input, 1, 4);
	iTemp = _tstoi(temp);

	if (iTemp < 1000 || iTemp > 3000)
		return 0;

	substr(temp, input, 5, 2);
	iTemp = _tstoi(temp);

	if (iTemp < 1 || iTemp > 12)
		return 0;

	substr(temp, input, 7, 2);
	iTemp = _tstoi(temp);

	if (iTemp < 1 || iTemp > 31)
		return 0;

	return 1;
}

/*--sunmd
*判断i输入的是否为日期
*判断条件：（1）输入串中包含1到2个“-”
*          （2）输入串中，除“-”外，其他都是数字
*          （3）“-”的前面最少有1个数字,第一个“-”前面最多4位
*          （4）“-”的后面最多有两位数字
*          （5）如："i2009-"->“二〇〇九年”，"i2009-9"->"二〇〇九年九月"，"i2009-9-9"->"二〇〇九年九月九日"
*          （6）“-”和“/”均可，即支持"i2009/9/9"的形式
*
*/
int IsDate4IPre(const TCHAR *input_string, TCHAR chrSplit)
{
	TCHAR *right_string, *left_string;
	int iDisSplit = 0 , iTemp = 0;

	left_string = utf16_strchr(input_string, chrSplit);
	right_string = utf16_strrchr(input_string, chrSplit);

	//存在“-”
	if (!left_string)
		return 0;

	//第一位不能是“-”
	if (input_string[1] == chrSplit)
		return 0;

	//第一个“-”前面不能超过4位数字
	if (utf16_strlen(input_string) - utf16_strlen(left_string) > 5)
		return 0;

	if (utf16_strlen(right_string) > 3)
		return 0;

	//多于一个"-"
	if (left_string != right_string)
	{
		*left_string++;

		//多于两个'-'
		if (utf16_strchr(left_string, chrSplit) != right_string)
			return 0;

		iDisSplit = (int)utf16_strlen(left_string) - (int)utf16_strlen(right_string);

		//两个‘-’相邻
		if (iDisSplit == 0)
			return 0;

		//两个'-'相隔太远，大于3
		if (iDisSplit > 2)
			return 0;
	}

	//除“-”外，应该全部为数字
	*input_string++;
	while (*input_string)
	{
		if (input_string[0] != chrSplit && !isdigit(*input_string))
			return 0;
		else
			*input_string++;
	}

	return 1;
}

/*--sunmd
判断i输入的是否为金额
*判断方法：（1）输入串中只包含，且必须包含一个“.”
*          （2）点后面最多两个数字。
*          （3）“.”的前后如果有字符，则必须全都是数字。
*          （4）如i23.22，i23.0，省略写法包括：i.09，i23.
*/
int IsMoney4IPre(const TCHAR *input_string)
{
	TCHAR *temp_string;
	temp_string = utf16_strrchr(input_string, '.');

	//存在“.”
	if (!temp_string)
		return 0;

	//只能有一个“.”
	if (utf16_strchr(input_string, '.') != temp_string)
		return 0;

	//“.”后面不能多于2位
	if(temp_string && utf16_strlen(temp_string) > 3)
		return 0;

	//除“.”外，应该全部为数字
	*input_string++;
	while(*input_string)
	{
		if(input_string[0]!='.' && !isdigit(*input_string))
			return 0;
		else
			*input_string++;
	}

	return 1;
}

/**取带单位的数字(十、百、千、万...)
*   input_string    输入字符串
*   buffer_units    候选字符串（如i123->一百二十三)
*   buffer_special_units  候选特殊字符串，“十”自动变成“一十”
*   candidates      候选数组
*   count           候选个数指针
*   length          单个候选的内存长度
*   isCaps          是否是大写的（I123还是i123）
*/
void AppendComplexNumberString(const TCHAR *input_string, TCHAR *buffer_units, TCHAR *buffer_special_units, 
			       CANDIDATE *candidates, int *count, int length, int isCaps)
{
	buffer_units[0] = 0;
	buffer_special_units[0] = 0;
	GetComplexNumberString(input_string + 1, buffer_units, length, isCaps);
	if (buffer_units[0])
	{
		AppendSPWCandidate(candidates, count, (int)utf16_strlen(buffer_units), buffer_units);

		if (buffer_units[0] == TEXT('十') || buffer_units[0] == TEXT('拾'))
		{
			buffer_special_units[0] = 0;
			if(isCaps)
				utf16_strcat(buffer_special_units, TEXT("壹"));
			else
				utf16_strcat(buffer_special_units, TEXT("一"));
			utf16_strcat(buffer_special_units, buffer_units);
			AppendSPWCandidate(candidates, count, (int)utf16_strlen(buffer_special_units), buffer_special_units);
		}
	}
}

/**
* 取最普通的候选，如i123->一二三  或  壹贰叁
*   input_string    输入字符串
*   buffer          候选字符串（如i123->一二三)
*   candidates      候选数组
*   count           候选个数指针
*   length          单个候选的内存长度
*   isCaps          是否是大写的（I123还是i123）
*/
void AppendNumberStringWithUnit(const TCHAR *input_string, TCHAR *buffer, CANDIDATE *candidates, int *count, int length, int isCaps)
{
	int i;
	buffer[0] = 0;
	//检查是否都为数字构成，否则返回
	for (i = 1; i < (int)utf16_strlen(input_string); i++)
	{
		if (!(input_string[i] >= '0' && input_string[i] <= '9' || input_string[i] == '.'))
			return;
	}
	GetSimpleNumberString(input_string + 1, buffer, length, isCaps, 1);
	if (buffer[0])
		AppendSPWCandidate(candidates, count, (int)utf16_strlen(buffer), buffer);
}

/**	获取以I特殊输入时的候选结果
 */
int GetICandidates(const TCHAR *input_string, CANDIDATE *candidates, int array_length)
{
	static TCHAR buffer[0x100];//i123->一二三
	static TCHAR buffer1[0x100];//i123->壹贰叁
	static TCHAR buffer_units[0x100];//i123->一百二十三，I123->壹佰贰拾叁
	static TCHAR buffer_special_units[0x100];//“十”转为“一十”
	static TCHAR buffer_units1[0x100];//i123->一百二十三，I123->壹佰贰拾叁
	static TCHAR buffer_special_units1[0x100];//“拾”转为“壹拾”
	static TCHAR buffer_tmp1[0x100], buffer_tmp2[0x100], buffer_tmp3[0x100], buffer_tmp4[0x100], buffer_tmp5[0x100], buffer_tmp6[0x100];
	int i, count = 0;

	//没有输入或者输入不为i则直接返回
	if (!array_length || !input_string || (1 == utf16_strlen(input_string)) ||
			(*input_string != 'i' && *input_string != 'I' && *input_string != 'U'))
		return 0;
	if (*input_string == 'i' && share_segment->sp_used_i && pim_config->pinyin_mode == PINYIN_SHUANGPIN)
		return 0;
	if ((*input_string == 'i' || *input_string == 'U') && !pim_config->i_mode_enabled)
		return 0;
	if (*input_string == 'I' && !pim_config->I_mode_enabled)
		return 0;

	if (2 == utf16_strlen(input_string))
	{
		//特殊处理i1~i9
		if(input_string[1]>TEXT('0') && input_string[1]<=TEXT('9')){
			if(*input_string == 'I'){
				for(i=0;i < _SizeOf(I_numcadidates); i++)
					AppendSPWCandidate(candidates, &count, 1, &I_numcadidates[i][input_string[1]-'0'-1]);
			}
			else{
				for(i=0;i < _SizeOf(i_numcadidates); i++)
					AppendSPWCandidate(candidates, &count, 1, &i_numcadidates[i][input_string[1]-'0'-1]);
			}
			return count;
		}
		//特殊处理i0,I0，否则用下面的逻辑，总是输出顺序不正确
		else if((utf16_strcmp(input_string, TEXT("i0")) == 0) || (utf16_strcmp(input_string, TEXT("U0")) == 0)){
			AppendSPWCandidate(candidates, &count, 1, TEXT("〇"));
			AppendSPWCandidate(candidates, &count, 1, TEXT("零"));
			return count;
		}
		else if(utf16_strcmp(input_string, TEXT("I0")) == 0){
			AppendSPWCandidate(candidates, &count, 1, TEXT("零"));
			AppendSPWCandidate(candidates, &count, 1, TEXT("〇"));
			return count;
		}
	}

	//财务模式：快速输入元角分，不区分大小写的i，如i23.12，则候选为“二十三元一角二分”和“贰拾叁圆壹角贰分”
	if(IsMoney4IPre(input_string))
	{
		//大写：如i123.-->壹佰贰拾叁元
		GetMoneyNumberString(input_string + 1, buffer_tmp1, _SizeOf(buffer_tmp1), (*input_string == 'I'));
		if (buffer_tmp1[0])
			AppendSPWCandidate(candidates, &count, (int)utf16_strlen(buffer_tmp1), buffer_tmp1);
		//小写：如i123.-->一百二十三元
		GetMoneyNumberString(input_string + 1, buffer_tmp2, _SizeOf(buffer_tmp2), !(*input_string == 'I'));
		if (buffer_tmp2[0])
			AppendSPWCandidate(candidates, &count, (int)utf16_strlen(buffer_tmp2), buffer_tmp2);
		//大写：如i123.-->壹佰贰拾叁圆
		GetMoneyNumberString(input_string + 1, buffer_tmp3, _SizeOf(buffer_tmp3), 2);
		if (buffer_tmp3[0])
			AppendSPWCandidate(candidates, &count, (int)utf16_strlen(buffer_tmp3), buffer_tmp3);
	}
	//日期模式，如i2009/08/02或者i2009-08-02，输出为“二〇〇九年八月二日”和“2009年08月02日”
	//智能日期模式，形如：i20090908－》二〇〇九年九月八日、2009年09月08日
	else if (IsDate4IPre(input_string, '-') || IsDate4IPre(input_string, '/') || IsUnsignDate4IPre(input_string))
	{
		//大写：如"i2009-2-3"－》二〇〇九年二月三日
		GetDateString(input_string + 1, buffer_tmp1, buffer_tmp2, candidates, &count, (*input_string == 'I'));
		//小写：如"i2009/2/3"－》2009年2月3日
		GetDateString(input_string + 1, buffer_tmp3, buffer_tmp4, candidates, &count, !(*input_string == 'I'));

		if (!IsUnsignDate4IPre(input_string))
			return count;
	}
	//时间模式，如i20:23:56-->二十点二十三分五十六秒
	else if (IsTime4IPre(input_string))
	{
		GetTimeString(input_string + 1, buffer_tmp1, buffer_tmp2, buffer_tmp3, candidates, &count, (*input_string == 'I'));
		GetTimeString(input_string + 1, buffer_tmp4, buffer_tmp5, buffer_tmp6, candidates, &count, !(*input_string == 'I'));

		return count;
	}

	//取带单位的数字（如i123->一百二十三   或    壹佰贰拾叁）
	AppendComplexNumberString(input_string, buffer_units, buffer_special_units, candidates, &count, _SizeOf(buffer_units), (*input_string == 'I'));
	AppendComplexNumberString(input_string, buffer_units1, buffer_special_units1, candidates, &count, _SizeOf(buffer_units1), !(*input_string == 'I'));
	////取中文字符串（如i123->一二三   或    壹贰叁）
	AppendNumberStringWithUnit(input_string, buffer, candidates, &count, _SizeOf(buffer), (*input_string == 'I'));
	AppendNumberStringWithUnit(input_string, buffer1, candidates, &count, _SizeOf(buffer1), !(*input_string == 'I'));

	return count;
}

/*U+编码的输入法模式*/
int GetUPlusCandidates(const TCHAR *input_string, CANDIDATE *candidates, int array_length)
{
	static TCHAR buffer[0x10];
	int ret;

	//没有输入或者输入不为E则直接返回
	if (!array_length || !input_string)
		return 0;

	if (utf16_strncmp(input_string, TEXT("U+"), 2) || ((int)utf16_strlen(input_string) < 3) || ((int)utf16_strlen(input_string) > 7))
		return 0;

	_tcscpy_s(buffer, 10, TEXT("0x"));
	_tcscat_s(buffer, 10, input_string + 2);
	/*
	if (!StrToIntEx(buffer, STIF_SUPPORT_HEX, &ret))
		return 0;
	*/
	ret = htoi(buffer);
	UCS32ToUCS16(ret, buffer);

	candidates->type       = CAND_TYPE_SPW;
	candidates->spw.type   = SPW_STRING_SPECIAL;
	candidates->spw.length = (int)utf16_strlen(buffer);
	candidates->spw.string = buffer;
	candidates->spw.hint   = 0;

	return 1;
}

/**	获取以E特殊输入时的候选结果，执行程序
 */
int GetUnoHintCandidates(const TCHAR *input_string, CANDIDATE *candidates, int array_length)
{
	static TCHAR buffer[0x100];
	int prefix_len;

	if (pim_config->use_u_hint)
		return 0;
	//在系统登录的时候，禁止该功能
	/*
	if (window_logon)
		return 0;
	*/
	//没有输入或者输入不为E则直接返回
	if (!array_length || !input_string)
		return 0;
	if (pim_config->pinyin_mode == PINYIN_SHUANGPIN && !utf16_strncmp(input_string, TEXT("u"), 1))
		return 0;
	if (!utf16_strncmp(input_string, TEXT("E"), 1) || !utf16_strncmp(input_string, TEXT("u"), 1))
		prefix_len = 1;
	else
		return 0;
	if (!input_string[prefix_len])
		return 0;
	_tcscpy_s(buffer, _SizeOf(buffer), input_string + 1);
	candidates->type       = CAND_TYPE_SPW;
	candidates->spw.type   = SPW_STRING_EXEC;
	candidates->spw.length = (int)utf16_strlen(buffer);
	candidates->spw.string = buffer;
	candidates->spw.hint   = 0;
	return 1;
}

/**	获取以u特殊输入时的候选结果，执行程序
 */
int GetUCandidates(const TCHAR *input_string, CANDIDATE *candidates, int array_length)
{
    //static TCHAR buffer[0x100],hint[0x100];
	int prefix_len,i,count=0;

	//没有输入或者输入不为u则直接返回
	if (!array_length || !input_string ||
			(utf16_strncmp(input_string, TEXT("E"), 1) && utf16_strncmp(input_string, TEXT("u"), 1)))
		return 0;
	if (pim_config->pinyin_mode == PINYIN_SHUANGPIN && !utf16_strncmp(input_string, TEXT("u"), 1))
		return 0;
	if (!utf16_strncmp(input_string, TEXT("u"), 1) || !utf16_strncmp(input_string, TEXT("E"), 1))
		prefix_len = 1;
	else
		return 0;
	for (i = 0; i < RESERVED_WORD_COUNT; i += 3)	//找到
	{
		//if(_tcsnccmp(u_reserved_word[i], input_string + 1, utf16_strlen(input_string + 1)))
		if(utf16_strncmp(u_reserved_word[i], input_string + 1, utf16_strlen(input_string + 1)))
			continue;
        if(!utf16_strcmp(TEXT("hs"), u_reserved_word[i]))
			continue;
		candidates[count].type		 = CAND_TYPE_SPW;
		candidates[count].spw.type	 = SPW_STRING_EXEC;
		candidates[count].spw.length = (int)utf16_strlen(u_reserved_word[i]);
		candidates[count].spw.string = u_reserved_word[i];
		candidates[count].spw.hint	 = u_reserved_word[i + 1];
		count++;
	}

	return count;
}

/**	检索短语，获得短语候选，放入候选数组中。
 *	参数：
 *		name				短语名称
 *		candidate_array		短语候选缓冲区
 *		array_length		候选数组长度
 *	返回：
 *		检索到的短语候选数目
 */
int GetSpwCandidates(PIMCONTEXT *context, const TCHAR *name, CANDIDATE *candidate_array, int array_length)
{
	int i, j, found;
	int count = 0;
	int name_length;
	TCHAR *spw_hint, *spw_str;
	
	assert(name && candidate_array);
	
	if (!share_segment->spw_loaded){
		LoadSpwResource();
	}

	if (!spw_buffer)
	{
		spw_buffer = GetSharedMemory(spw_share_name);
		//可能存在其他进程已经装载了，但是退出后共享内存被释放的问题
		if (!spw_buffer && share_segment->spw_loaded)
		{
			share_segment->spw_loaded = 0;
			LoadSpwResource();
		}
	}

	if (spw_buffer && pim_config->use_special_word)
	{
		
		name_length = (int)utf16_strlen(name);
		for (i = 0, count = 0; count < array_length && i < share_segment->spw_count; i++)
		{

			//检查名称是否相符
			if (utf16_strcmp(name, spw_buffer + share_segment->spw_index[i]))
				continue;
			
			//spw_str中存放短语value，错开短语提示
			spw_str  = spw_buffer + share_segment->spw_index[i] + name_length + 1;
			spw_hint = 0;

			if (*spw_str == SPW_HINT_LEFT_CHAR)
			{
				spw_hint = spw_str;
				while (++spw_str)
				{
					if (*spw_str == SPW_HINT_RIGHT_CHAR)
					{
						spw_str++;
						break;
					}
				}
				spw_str++;
			}
			if (spw_hint && !utf16_strcmp(spw_hint, SPW_HINT_NULL_STR))
				spw_hint = 0;
			//不允许多行的情况下，过长的候选将不出现，为PhotoShop解决问题。
			if (no_multi_line && (int)utf16_strlen(spw_str) > 128)
				continue;
			//判断是否重复
			found = 0;
			for (j = 0; j < count; j++)
			{
				if (!utf16_strcmp(candidate_array[j].spw.string, spw_str))
				{
					found = 1;
					break;
				}
			}
			if (found)
				continue;
			//如果候选为''则continue
			if (!utf16_strcmp(TEXT(""), spw_str))
				continue;
			//将内容加入到候选数组中
			candidate_array[count].type		  = CAND_TYPE_SPW;
			candidate_array[count].spw.type	  = SPW_STRING_NORMAL;
			candidate_array[count].spw.string = spw_str;
			candidate_array[count].spw.hint	  = spw_hint;
			candidate_array[count].spw.length = (int)utf16_strlen(candidate_array[count].spw.string);
			count++;
		}
	}
	if (IsDatePrefix(name))
		count += GenerateDateCandidate(candidate_array + count, array_length - count);
	if (IsTimePrefix(name))
		count += GenerateTimeCandidate(candidate_array + count, array_length - count);
	if (count)
		return count;
	count = GetUPlusCandidates(name, candidate_array, array_length);
	if (count)
		return count;
	if (pim_config->i_mode_enabled || pim_config->I_mode_enabled)
	{

		count = GetICandidates(name, candidate_array, array_length);
		if (count)
			return count;
	}

	if (pim_config->B_mode_enabled)
    {
		count = GetBHCandidates(name, candidate_array, array_length);
		if (count)
			return count;
	}
	if (utf16_strlen(name) && pim_config->u_mode_enabled && (name[0] == 'u' || name[0] == 'E'))
	{
		//if (!IsFullScreen())
		//{
		if (pim_config->u_mode_enabled && pim_config->use_u_hint)
			count = GetUCandidates(name, candidate_array, array_length);
		else
			count = GetUnoHintCandidates(name, candidate_array, array_length);
		//}
	}
	return count;
}

/**	解析短语名称。
 *	短语名称为英文字母打头可以混合下划线并且紧接着=的一串字符。
 *	短语名称的长度有一定的限制，超过限制的字符将被忽略掉。
 *	参数：
 *		str				输入串
 *		name			短语名字缓冲区
 *		parsed_length	处理的字符数目（用于后续的处理使用）
 *	返回（无）
 */
void ParseSpwName(const TCHAR *str, TCHAR *name, TCHAR *hint, int *parsed_length)
{
	TCHAR tmp_name[SPW_NAME_LENGTH], tmp_hint[SPW_HINT_LENGTH];
	int  count, i, is_hint = 0, hint_count = 0;
	int  has_underline;			//是否已经有下划线了

	assert(str && name && parsed_length);
	//检查是否为合法的短语名字
	*parsed_length = 0;
	if (!((str[0] >= 'a' && str[0] <= 'z') || (str[0] >= 'A' && str[0] <= 'Z')))			//非法
		return;
	for (has_underline = 0, count = 0, i = 0; str[i] && str[i] != SPW_ASSIGN_CHAR; i++)
	{
		if (str[i] == SPW_HINT_LEFT_CHAR)
			is_hint = 1;
		if (is_hint)
		{
			if (count < SPW_HINT_LENGTH - 1)
				tmp_hint[hint_count++] = str[i];
			if (str[i] == SPW_HINT_RIGHT_CHAR)
				is_hint = 0;
			continue;
		}
		if (!((str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z') ||
		      (str[i] == '_' && i != 0) || (str[i] == ':' && i != 0) ||
		      (str[i] >= '0' && str[i] <= '9' && has_underline) || str[i]==';'))			//非法
			return;
		if (str[i] == '_')
			has_underline = 1;
		if (count < SPW_NAME_LENGTH - 1)		//超过的字母将被忽略掉
			tmp_name[count++] = str[i];
	}
	if (str[i] == SPW_ASSIGN_CHAR)		//合法的短语名称
	{
		tmp_name[count] = 0;
		tmp_hint[hint_count] = 0;
		utf16_strcpy(name, tmp_name);
		utf16_strcpy(hint, tmp_hint);
		*parsed_length = i;
	}
}

/**	解析短语内容（简单复制）
 *	参数：
 *		str				输入串
 *		content			短语内容缓冲区
 *		parsed_length	处理的字符数目（用于后续的处理使用）
 *	返回：无
 */
void ParseSpwContent(const TCHAR *str, TCHAR *content, int *parsed_length)
{
	int index;

	assert(str && content && parsed_length);
	index = 0;
	*parsed_length = 0;
	//清除开始的回车
	while(str[index] && *parsed_length < SPW_CONTENT_LENGTH - 1)
	{
		if (str[index] != '\r' && str[index] != '\n')
		{
			content[*parsed_length] = str[index];
			(*parsed_length)++;
		}
		index++;
	}
	content[*parsed_length] = 0;
}

/**	解析短语的数据行
 *	参数：
 *		str					行内容
 *		name				短语名称缓冲区
 *		content				短语内容缓冲区
 *	返回：
 *		TYPE_NAME			短语名称
 *		TYPE_CONTEXT		短语内容
 *		TYPE_COMMENT		注释
 *		TYPE_NONE			非法输入
 */
static int ParseSpwLine(const TCHAR *str, TCHAR *name, TCHAR *hint, TCHAR *content)
{
	int  name_parsed_length;
	int  content_parsed_length;
	int  ret = 0;
	if (!str || !*str)
		return SPW_TYPE_NONE;
	if (*str == SPW_COMMENT_CHAR)
		return SPW_TYPE_COMMENT;
	//检查是否为短语名称
	ParseSpwName(str, name, hint, &name_parsed_length);
	if (name_parsed_length)
		str += name_parsed_length + 1;			//跳过名字以及等号
	ParseSpwContent(str, content, &content_parsed_length);
	
	//如果定义名称行只是回车，则去掉这个回车（本行也删除）
	if (content_parsed_length && (content[0] == '\n' || content[0] == '\r'))
		content[0] = 0;
	if (name_parsed_length)
		ret |= SPW_TYPE_NAME;
	if (content_parsed_length)
		ret |= SPW_TYPE_CONTENT;
	
	return ret;
}

/**	处理短语数据，找出条目，添加到缓冲区
 *	参数：
 *		buffer			数据指针
 *		length			数据长度
 *	返回：
 *		失败：0
 *		成功：1
 */
int ProcessSpwData(const TCHAR *buffer, int length)
{
	int  index;
	int  last_content_length;
	TCHAR line[SPW_CONTENT_LENGTH + SPW_NAME_LENGTH + 1] = {0};
	TCHAR name[SPW_NAME_LENGTH], last_name[SPW_NAME_LENGTH];
	TCHAR hint[SPW_HINT_LENGTH], last_hint[SPW_HINT_LENGTH];
	TCHAR content[SPW_CONTENT_LENGTH + 0x100], last_content[SPW_CONTENT_LENGTH + 0x100];
	TCHAR temp_content[SPW_CONTENT_LENGTH + 0x100];

	assert(buffer);
	//初值
	line[0] = name[0] = content[0] = 0;
	last_name[0] = last_content_length = last_content[0] = 0;
	hint[0] = 0;
	index = 1;
	//遍历短语数据
	while(index < length)
	{
		int char_count;		//行字符串字符数目
		int type;			//spw的类型

		//获得一行数据
		char_count = 0;
		while(char_count < _SizeOf(line) - 1 && index < length)
		{
			line[char_count++] = buffer[index++];
			
			if (buffer[index - 1] == '\n')
				break;				//遇到回车结束
		}

		line[char_count] = 0;		//得到一行数据
		//处理数据
		type = ParseSpwLine(line, name, hint, content);
		
		if ((type & SPW_TYPE_COMMENT) || (type & SPW_TYPE_NONE))	//注释行、错误行跳过
		{
			//注释行以及错误行做为短语定义结束标志
			if (last_name[0] && last_content[0])
				if (!InsertSpw(last_name, last_hint, last_content))
					return 0;
			last_name[0] = last_content[0] = 0;
			continue;
		}
		//判断是不是单行组合短语
		if ((type & SPW_TYPE_NAME) && (type & SPW_TYPE_CONTENT) && utf16_strchr(content, SPW_COMMENT_CHAR))
		{
			int m = 0, n = 0;
			_tcscpy_s(temp_content, _SizeOf(content), content);
			while (temp_content[m])
			{
				//碰到分号，是分隔符，切分短语内容
				if ((SPW_COMMENT_CHAR == temp_content[m]) && (m > 0) && ('\\' != temp_content[m - 1]))
				{
					content[n] = 0;
					n = 0;
					if (!InsertSpw(name, hint, content))
						return 0;
				}
				else
				{
					//碰到分号转义，后退一个字符
					if ((SPW_COMMENT_CHAR == temp_content[m]) && (m > 0) && ('\\' == temp_content[m - 1]))
						n--;
					content[n++] = temp_content[m];
				}
				m++;
			}
			content[n] = 0;
		}
		//新的自定义条目
		if (type & SPW_TYPE_NAME)
		{
			if (last_name[0] && last_content[0])
				if (!InsertSpw(last_name, last_hint, last_content))		//加入到短语缓冲区
					return 0;			//出错，无法继续，返回。
			last_content_length = last_content[0] = 0;
			_tcsncpy_s(last_name, _SizeOf(last_name), name, SPW_NAME_LENGTH);
			_tcsncpy_s(last_hint, _SizeOf(last_hint), hint, SPW_HINT_LENGTH);

		}
		//内容需要与前一个进行合并，注意在存在短语名称定义时才
		//进行这项操作（可能有在定义之前有内容的情况）。
		if ((type & SPW_TYPE_CONTENT) && last_name[0])
		{
			int count = 0;				//内容计数器
			int multi_line = 0;			//是否为多行

			//在上一行最后加入回车
			if (last_content_length)
			{
				multi_line = 1;
				if (last_content[last_content_length - 1] != 0xa)
				{
					last_content[last_content_length++] = 0xd;
					last_content[last_content_length++] = 0xa;
				}
			}
			//如果前面没有回车，则插入一个回车
			while(content[count] && last_content_length < SPW_CONTENT_LENGTH - 1)
				last_content[last_content_length++] = content[count++];
			if (multi_line)
			{
				last_content[last_content_length++] = 0xd;
				last_content[last_content_length++] = 0xa;
			}
			last_content[last_content_length] = 0;
		}
	};

	//将最后一个加入到短语缓冲区中
	if (last_name[0] && last_content[0])
		return InsertSpw(last_name, last_hint, last_content);
	return 1;
}

/**	加载用户自定义短语到内存。
 *	参数：
 *		spw_file_name			自定义短语文件全路径
 *	返回：
 *		成功：1
 *		失败：0
 */
static TCHAR file_buffer[0x200000];
int LoadSpwData(const char *spw_file_name)
{
	int spw_file_length;
	assert(spw_file_name);
	//先清除内存，避免重新装载时之前的数据依然保留
	memset(file_buffer, 0, sizeof(file_buffer));
	if ((spw_file_length = LoadFromFile(spw_file_name, file_buffer, sizeof(file_buffer))) == -1)
	{
		printf("spw.c : 自定义短语文件打开失败。name=%s\n", spw_file_name);
		return 0;
	}

	if (!spw_file_length)
		return 0;
	if (!share_segment->spw_length)
		spw_buffer = AllocateSharedMemory(spw_share_name, SPW_BUFFER_SIZE);
	else
		spw_buffer = GetSharedMemory(spw_share_name);
	if (!spw_buffer)
		return 0;

	spw_file_length = spw_file_length / sizeof(TCHAR);

	//遍历短语文件，找出短语条目，插入到缓冲区中
	return ProcessSpwData(file_buffer, spw_file_length);
}

int IsSysPhraseFile(char *filename)
{
	int i, count;

	count = sizeof(DEFAULT_SPW_FILENAME) / _SizeOf(DEFAULT_SPW_FILENAME[0]) / sizeof(TCHAR);
	for (i = 0; i < count; i ++)
	{
		if (!strcmp(DEFAULT_SPW_FILENAME[i], filename))
			return 1;
	}
	return 0;
}

int LoadAllSpwData()
{
	int i;
	char name[MAX_PATH] = {0};

	if (share_segment->spw_loaded)
		return 1;
	for (i = 0; i < MAX_SPW_COUNT; i++)
	{
		if(IsSysPhraseFile(pim_config->spw_name[i])){
            GetFileFullName(pim_config->spw_name[i], name);
		}
		else{
            GetFileFullName(pim_config->spw_name[i], name);
			
		}

		//文件存在，并且文件名后面两位为ni（.ini的后两个，这样做，是为了防止name只是个目录，而不是文件）
		if(FileExists(name) && name[strlen(name) - 1] == 'i' && name[strlen(name) - 2] == 'n')
			LoadSpwData(name);
		else//如果没找到，则在配置中，清空短语库文件名称
			strcpy(pim_config->spw_name[i], "");

	}

	share_segment->spw_loaded = 1;

	return 1;
}

/**	释放spw文件
 */
int FreeSpwData()
{

	share_segment->spw_loaded = share_segment->spw_count = share_segment->spw_length = 0;
	if (spw_buffer)
    {
		FreeSharedMemory(spw_share_name, spw_buffer);
		spw_buffer = 0;
	}

	return 1;
}
