#ifndef	_SPW_H_
#define	_SPW_H_

#include <kernel.h>
#include <context.h>

#ifdef __cplusplus
extern "C" {
#endif

#define		SYS_SPW_FILE_NAME			"/phrase/系统短语库.ini"	//短语文件名称
#define		SPW_COMMENT_CHAR			';'							//短语注释符号
#define		SPW_ASSIGN_CHAR				'='							//短语赋值符号
#define		SPW_HINT_LEFT_CHAR			'['							//短语提示符号_左
#define		SPW_HINT_RIGHT_CHAR			']'							//短语提示符号_右
#define		SPW_HINT_NULL_STR			TEXT("[]")					//短语提示空串
#define		SPW_NAME_LENGTH				16							//短语名字最大长度
#define		SPW_HINT_LENGTH				64							//短语提示最大长度
#define		SPW_CONTENT_LENGTH			MAX_SPW_LENGTH				//短语内容最大长度
#define		SPW_BUFFER_SIZE				0x500000					//短语存储区大小
#define		SPW_MAX_ITEMS				400000						//短语最多数目

#define		SPW_TYPE_NAME				1							//短语名字
#define		SPW_TYPE_CONTENT			2							//短语内容
#define		SPW_TYPE_COMMENT			4							//短语注释
#define		SPW_TYPE_NONE				8							//出错！非短语相关

#define		SPW_STRING_NORMAL			1							//普通的短语
#define		SPW_STRING_EXEC				2							//执行程序类型
#define		SPW_STRING_SPECIAL			3							//特殊类型，如I,D,H等
#define		SPW_STRING_BH				4							//笔划候选
#define		SPW_STIRNG_ENGLISH			5							//英文单词

#define		MAX_SPW_COUNT				1 //32							//短语文件最多数目

//u命令的保留关键字
static const TCHAR u_reserved_word[][MAX_SPW_HINT_STRING] =
{
	TEXT("qj"),			TEXT("启动全集输入*"),         		TEXT(""),
	TEXT("cstar"),		TEXT("启动CStar输入方式*"),			TEXT(""),
	TEXT("dy"),			TEXT("打开符号(短语)输入*"),		TEXT(""),
	TEXT("ft"),			TEXT("启动繁体输入*"),				TEXT(""),
	TEXT("hs"),			TEXT("切换横竖排显示*"),			TEXT(""),
	TEXT("jt"),			TEXT("启动简体输入*"),				TEXT(""),
	TEXT("qjsr"),		TEXT("打开/关闭全角输入中文功能*"),	TEXT(""),
	TEXT("qp"),			TEXT("启动全拼输入*"),				TEXT(""),
	TEXT("sp"),			TEXT("启动双拼输入*"),				TEXT(""),

};

#define RESERVED_WORD_COUNT	 (sizeof(u_reserved_word) / _SizeOf(u_reserved_word[0]) / sizeof(TCHAR))

//获得短语候选
int GetSpwCandidates(PIMCONTEXT *context, const TCHAR *name, CANDIDATE *candidate_array, int array_length);

//获得特殊输入的提示信息
const TCHAR *GetSPWHintString(const TCHAR *input_string);

//加载用户自定义短语到内存
//extern int LoadSpwData(const TCHAR *spw_file_name);
extern int LoadAllSpwData();
extern int FreeSpwData();

#ifdef __cplusplus
}
#endif

#endif
