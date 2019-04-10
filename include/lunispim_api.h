#ifndef LUNISPIM_API_H_
#define LUNISPIM_API_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <uchar.h>
#define True 1
#define False 0
#define Bool unsigned short
#define API_STRUCT_INIT(Type, var) ((var).data_size = sizeof(Type) - sizeof((var).data_size))
#define	MAX_INPUT_LENGTH		64								//最大用户输入字符长度
#define	MAX_COMPOSE_LENGTH		128								//最大写作串长度
#define	MAX_CANDIDATE_STRING_LENGTH	32						//最大候选字符串的长度（用于显示,utf16编码）
#define MAX_RESULT_LENGTH 2048
#define	MAX_SPW_HINT_STRING		64			//SPW提示字符串长度
#define	STATE_VINPUT		6				//V输入状态
#define	STATE_IINPUT		8				//I输入状态
#define	STATE_BINPUT		13				//笔画输入状态
#define	STATE_ILLEGAL		5				//非法输入状态
#define	STATE_EDIT			1				//编辑状态
#define	ENGLISH_STATE_NONE	0				//非英文模式
#define	ENGLISH_STATE_CAND	2				//英文候选模式
#define	ENGLISH_STATE_INPUT	1				//英文输入模式
#define HZ_OUTPUT_SIMPLIFIED		(1 << 0)		//输出简体字（默认）
#define HZ_OUTPUT_TRADITIONAL		(1 << 1)		//输出繁体字
#define	HZ_SYMBOL_CHINESE			(1 << 5)		//中文符号
#define	HZ_SYMBOL_HALFSHAPE			(1 << 6)		//半角符号
#define MAX_PATH 390
typedef char16_t TCHAR;
typedef struct lunispim_context_t {
    int candidate_count;
    int compose_length;
    int input_length;
    int candidate_selected_index;
    int cursor_pos;
    char input_string[MAX_INPUT_LENGTH + 0x10];
    char compose_string[MAX_COMPOSE_LENGTH];
    int candidate_page_count;
    int candidate_index;
    int is_last_page;
    int selected_item_count;
    char selected_compose_string[MAX_COMPOSE_LENGTH];
    char spw_hint_string[MAX_SPW_HINT_STRING];
    int compose_cursor_index;   //写作串光标位置
    int english_state;
    int state;
    int syllable_count;
    int syllable_index;
} LunispimContext;
typedef struct lunispim_config_t {
    int hz_output_mode;
    int english_state;
    int symbol_type;
    char* resources_data_dir;
    char* user_data_dir;
} LunispimConfig;
typedef struct lunispim_api_t {
    int data_size;
    int (*search)(TCHAR ch);
    void (*reset_context)();
    void (*backspace)();
    int (*select_candidate)(int cand_id);
    void (*get_compose_string)(char *buffer, int buffer_size);
    void (*get_input_string)(char *buffer, int buffer_size);
    void (*get_result)(char *buffer, int buffer_size);
    int (*next_candidate_page)();
    int (*prev_candidate_page)();
    int (*get_candidate)(int cand_id, char *cand_str, int len);
    int (*get_context)(LunispimContext* context);
    int (*has_result)();
    int (*get_return_string)(char *buffer, int buffer_size);
    void (*next_candidate_item)();
    void (*prev_candidate_item)();
    void (*move_cursor_left)();
    void (*move_cursor_right)();
    int (*get_symbol)(TCHAR ch, char *buffer, int buffer_size);
    void (*toggle_english_candidate)();
    void (*switch_chiness_input)();
    void (*switch_english_cand)();
    void (*set_soft_cursor)();
    int (*get_syllable_index)();
    void (*get_config)(LunispimConfig* config);
    void (*update_config)(LunispimConfig* config);
    char* (*initialize)();
} LunispimApi;
LunispimApi* get_unispim_api();
#ifdef __cplusplus
}
#endif

#endif  //LUNISPIM_API_H_
