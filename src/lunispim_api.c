#include <context.h>
#include <editor.h>
#include <vc.h>
#include <utf16char.h>
#include <lunispim_api.h>
#include <utility.h>
#include <editor.h>
#include <symbol.h>
#include <config.h>
#include <init.h>
static int soft_cursor = 0;
int search(TCHAR ch)
{
    if(ch != 0)
        AddChar(&demo_context, ch, 0);
    return demo_context.candidate_page_count;
}
void reset_context()
{
    ResetContext(&demo_context);
}
void backspace()
{
    if(!BackSelectedCandidate(&demo_context)){
        BackspaceChar(&demo_context);
    }

}
int select_candidate(int cand_id)
{
    SelectCandidate(&demo_context, cand_id);
    if(*demo_context.result_string){
        return 1;
    }else{
        return 0;
    }
}

void get_compose_string(char *buffer, int buffer_size)
{
    Utf16ToUtf8(demo_context.compose_string, buffer, buffer_size);
}
void get_input_string(char *buffer, int buffer_size)
{
    Utf16ToUtf8(demo_context.input_string, buffer, buffer_size);
}

void get_result(char *buffer, int buffer_size)
{
    Utf16ToUtf8(demo_context.result_string, buffer, buffer_size);
}
int has_result()
{
    if(demo_context.result_length){
        return 1;
    }else{
        return 0;
    }
}
int next_candidate_page()
{
    return NextCandidatePage(&demo_context);
}
int prev_candidate_page()
{
    return PrevCandidatePage(&demo_context);
}
void next_candidate_item()
{
    NextCandidateItem(&demo_context);
}
void prev_candidate_item()
{
    PrevCandidateItem(&demo_context);
}
void move_cursor_left()
{
    if(demo_context.state == STATE_IINPUT ||
            demo_context.state == STATE_BINPUT ||
            demo_context.state == STATE_VINPUT){
        MoveCursorLeft(&demo_context);
    }else{
        MoveCursorLeftBySyllable(&demo_context);
    }

}


void move_cursor_right()
{
    MoveCursorRight(&demo_context);
}
int get_candidate(int cand_id, char *cand_str, int len)
{
    if(cand_id < demo_context.candidate_page_count){
        //_tcsncpy_s(cand_str, len, demo_context.candidate_string[cand_id], 32);
        Utf16ToUtf8(demo_context.candidate_string[cand_id], cand_str, len);
        return len + 1;

    }else{
        cand_str[0] = 0;
        return 0;
    }
} 
int get_context(LunispimContext* context)
{
    if(!context) return 0;
    iconv_t cd;
    cd = IconvOpen("UTF-16", "UTF-8");
    context->compose_length = demo_context.compose_length;
    context->candidate_count = demo_context.candidate_count;
    context->cursor_pos = demo_context.cursor_pos;
    context->compose_cursor_index = demo_context.compose_cursor_index;
    context->input_length = demo_context.input_length;
    context->candidate_selected_index = demo_context.candidate_selected_index;
    CodeConvert(cd, (char*)demo_context.input_string, sizeof(demo_context.input_string), context->input_string, sizeof(context->input_string));
    //Utf16ToUtf8(demo_context.input_string, context->input_string, MAX_INPUT_LENGTH + 0x10);
    if(soft_cursor){
        char16_t temp[MAX_COMPOSE_LENGTH + 1] = {0};
        _tcsncat_s(temp, MAX_COMPOSE_LENGTH + 1, demo_context.compose_string, demo_context.compose_cursor_index);
        //_tcsncat_s(temp, MAX_COMPOSE_LENGTH + 1, u"›", 1);
        _tcsncat_s(temp, MAX_COMPOSE_LENGTH + 1, u"|", 1);
        _tcsncat_s(temp, MAX_COMPOSE_LENGTH + 1, &demo_context.compose_string[demo_context.compose_cursor_index], demo_context.compose_length);
        CodeConvert(cd, (char*)temp, sizeof(temp), context->compose_string, sizeof(context->compose_string));
        //Utf16ToUtf8(temp, context->compose_string, MAX_COMPOSE_LENGTH + 1);
    }else{
        CodeConvert(cd, (char*)demo_context.compose_string, sizeof(demo_context.compose_string), context->compose_string, sizeof(context->compose_string));
        //Utf16ToUtf8(demo_context.compose_string, context->compose_string, MAX_COMPOSE_LENGTH);
    }
    context->candidate_page_count = demo_context.candidate_page_count;
    context->candidate_index = demo_context.candidate_index;
    if((demo_context.candidate_count - demo_context.candidate_index) <= demo_context.candidate_page_count) {
        context->is_last_page = 1;
    }else{
        context->is_last_page = 0;
    }
    context->selected_item_count = demo_context.selected_item_count;
    CodeConvert(cd, (char*)demo_context.selected_compose_string, sizeof(demo_context.selected_compose_string),
                context->selected_compose_string, sizeof(context->selected_compose_string));
    CodeConvert(cd, (char*)demo_context.spw_hint_string, sizeof(demo_context.spw_hint_string),
                context->spw_hint_string, sizeof(context->spw_hint_string));
    //Utf16ToUtf8(demo_context.selected_compose_string, context->selected_compose_string, MAX_COMPOSE_LENGTH);
    //Utf16ToUtf8(demo_context.spw_hint_string, context->spw_hint_string, MAX_SPW_HINT_STRING);
    context->state = demo_context.state;
    context->english_state = demo_context.english_state;
    context->syllable_count = demo_context.syllable_count;
    context->syllable_index = GetSyllableIndexByComposeCursor(&demo_context, demo_context.compose_cursor_index);
    // Utf16ToUtf8(demo_context.selected_compose_string, context->selected_compose_string, MAX_COMPOSE_LENGTH);
    IconvClose(cd);
    return 1;
}

int get_return_string(char* buffer, int buffer_size)
{
    TCHAR py_string[6];
    int max_input_length = MAX_INPUT_LENGTH + 0x10;
    TCHAR result_str[MAX_INPUT_LENGTH + 0x10] = {0};
    int pos = 0;
    if(demo_context.selected_item_count > 0){
        for(int i=0; i<demo_context.selected_item_count; i++){
            for(int index=0; index<demo_context.selected_items[i].syllable_length; index++){
                int syllables_index = demo_context.selected_items[i].syllable_start_pos + index;
                int py_len = GetSyllableString(demo_context.syllables[syllables_index], py_string, 6, 0);
                pos += py_len;
                if(demo_context.input_string[pos] == SYLLABLE_SEPARATOR_CHAR){
                    pos++;
                }
                _tcscpy_s(result_str, MAX_INPUT_LENGTH + 0x10, demo_context.selected_compose_string);
                size_t selected_len = utf16_strlen(demo_context.selected_compose_string);
                _tcscpy_s(&result_str[selected_len], max_input_length - selected_len, &demo_context.input_string[pos]);
            }
        }
    }else{
        if(demo_context.state == STATE_VINPUT){
            return Utf16ToUtf8(demo_context.compose_string + 1, buffer, buffer_size);
        }else{
            return Utf16ToUtf8(demo_context.input_string, buffer, buffer_size);
        }

    }
    return Utf16ToUtf8(result_str, buffer, buffer_size);
}
//ch为char16_t，不用char，防止溢出
int get_symbol(TCHAR ch, char* buffer, int buffer_size)
{
    const TCHAR* symbol_string = 0;
    symbol_string = GetSymbol(&demo_context, ch);
    if(symbol_string == 0) return 0;
    CheckQuoteInput(*symbol_string);
    Utf16ToUtf8(symbol_string, buffer, buffer_size);
    return 1;

}

void toggle_english_candidate()
{
     ToggleEnglishCandidate(&demo_context);
     ProcessContext(&demo_context);
}
void switch_chiness_input()
{
    if(demo_context.english_state != ENGLISH_STATE_NONE){
        demo_context.english_state = ENGLISH_STATE_NONE;
    }
    ProcessContext(&demo_context);
}
void switch_english_input()
{
    demo_context.english_state = ENGLISH_STATE_INPUT;
}

void switch_english_cand()
{
    if(demo_context.english_state != ENGLISH_STATE_CAND){
        ToggleEnglishCandidate(&demo_context);
    }
    ProcessContext(&demo_context);
}
void set_soft_cursor(int bool)
{
    soft_cursor = bool;
}
void set_hz_output_mode(int mode)
{
    pim_config->hz_output_mode = mode;
}
void get_config(LunispimConfig* config)
{
    if(pim_config == 0) LoadDefaultConfig();
    config->hz_output_mode = pim_config->hz_output_mode;
    if(pim_config->hz_option & HZ_SYMBOL_CHINESE){
        config->symbol_type |= HZ_SYMBOL_CHINESE;
    }else{
        if(config->symbol_type & HZ_SYMBOL_CHINESE){
            config->symbol_type ^= HZ_SYMBOL_CHINESE;
        }
    }
    if(pim_config->hz_option & HZ_SYMBOL_HALFSHAPE){
        config->symbol_type |= HZ_SYMBOL_HALFSHAPE;
    }else{
        if(config->symbol_type & HZ_SYMBOL_HALFSHAPE){
            config->symbol_type ^= HZ_SYMBOL_HALFSHAPE;
        }
    }
    config->user_data_dir = pim_config->user_data_dir;
    config->resources_data_dir = pim_config->resources_data_dir;
    config->pinyin_mode = pim_config->pinyin_mode;
    config->use_english_input = pim_config->use_english_input;
}
void update_config(LunispimConfig* config)
{
    if(pim_config == 0) LoadDefaultConfig();
    pim_config->hz_output_mode = config->hz_output_mode;
    if(config->symbol_type & HZ_SYMBOL_CHINESE){
        pim_config->hz_option |= HZ_SYMBOL_CHINESE;
    }else{
        if(pim_config->hz_option & HZ_SYMBOL_CHINESE){
            pim_config->hz_option ^= HZ_SYMBOL_CHINESE;
        }
    }
    if(config->symbol_type & HZ_SYMBOL_HALFSHAPE){
        pim_config->hz_option |= HZ_SYMBOL_HALFSHAPE;
    }else{
        if(pim_config->hz_option & HZ_SYMBOL_HALFSHAPE){
            pim_config->hz_option ^= HZ_SYMBOL_HALFSHAPE;
        }
    }
    strcpy_s(pim_config->user_data_dir, MAX_PATH, config->user_data_dir);
    strcpy_s(pim_config->resources_data_dir, MAX_PATH, config->resources_data_dir);
    pim_config->pinyin_mode = config->pinyin_mode;
    pim_config->use_english_input = config->use_english_input;

}
char* initialize()
{
    Initialize();
    return GetInitializeErrorInfo();
}

LunispimApi* get_unispim_api()
{
    static LunispimApi u_api = {0};
    if(!u_api.data_size){
        API_STRUCT_INIT(LunispimApi, u_api);
        u_api.backspace = &backspace;
        u_api.select_candidate = &select_candidate;
        u_api.get_candidate = &get_candidate;
        u_api.get_compose_string = &get_compose_string;
        u_api.get_input_string = &get_input_string;
        u_api.get_result = &get_result;
        u_api.next_candidate_page = &next_candidate_page;
        u_api.prev_candidate_page = &prev_candidate_page;
        u_api.reset_context = &reset_context;
        u_api.search = &search;
        u_api.get_context = &get_context;
        u_api.has_result = &has_result;
        u_api.get_return_string = &get_return_string;
        u_api.next_candidate_item = &next_candidate_item;
        u_api.prev_candidate_item = &prev_candidate_item;
        u_api.move_cursor_left = &move_cursor_left;
        u_api.move_cursor_right = &move_cursor_right;
        u_api.get_symbol = &get_symbol;
        u_api.toggle_english_candidate = &toggle_english_candidate;
        u_api.switch_english_cand = &switch_english_cand;
        u_api.switch_chiness_input = &switch_chiness_input;
        u_api.switch_english_input = &switch_english_input;
        u_api.set_soft_cursor = &set_soft_cursor;
        u_api.get_config = &get_config;
        u_api.update_config = &update_config;
        u_api.initialize = &initialize;
    }
    return &u_api;
}

