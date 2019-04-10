#ifndef	_SYMBOL_H_
#define	_SYMBOL_H_

#ifdef	__cplusplus
extern "C"
{
#endif

#define	SYMBOL_NUMBER	32
#define	SYMBOL_LENGTH	5

#define	SYMBOL_INI_FILE_NAME	"/ini/中文符号.ini"

typedef struct tagSYMBOLITEM
{
	TCHAR	english_ch;								//英文符号字符
	TCHAR	english_symbol[2];						//英文符号串
	TCHAR	chinese_symbol[SYMBOL_LENGTH];			//中文符号串
	TCHAR	default_chinese_symbol[SYMBOL_LENGTH];	//默认中文符号串
}SYMBOLITEM;

struct tagPIMCONTEXT;

extern const TCHAR *GetSymbol(struct tagPIMCONTEXT *context, TCHAR ch);
extern int IsSymbolChar(TCHAR ch);
extern int LoadSymbolData(const char*);
extern int FreeSymbolData();
extern void CheckQuoteInput(HZ symbol);
extern void GetFullShapes(const TCHAR *src, TCHAR *dest, int dest_len); 

#ifdef	__cplusplus
}
#endif

#endif
