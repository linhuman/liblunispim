/*
 *	用户输入：wo3'men2da'jia
 *	解析：
 *		音节		wo3		men2	da		jia
 *		有无'		y		n		y		n
 *		有无调		y		y		n		n
 *		汉字		-		-		-		-
 *		字符串		wo3		men2	da'		jia
 *		字符串长度	3		4		3		3
 *	Real光标：13；显示光标：13。
 *
 *	当用户选择了“我们”这个词的时候
 *		音节		wo3		men2	da		jia
 *		有无'		y		n		y		n
 *		有无调		y		y		n		n
 *		汉字		我		们		-		-
 *		字符串		我		们		da'		jia
 *		字符串长度	2		2		3		3
 *	当前光标：13；显示光标：11。
 *
 *
 */
#include <kernel.h>
#include <config.h>
#include <spw.h>
#include <zi.h>
//#include <url.h>
#include <ci.h>
#include <editor.h>
#include <symbol.h>
#include <context.h>
#include <utility.h>
#include <vc.h>
#include <assert.h>
#include <english.h>
#include <utf16char.h>
#include <string.h>
#include <ctype.h>
#define	NEW_CI_DELETE_TIME	2500			//可以删除时间长度
static int new_ci_create_time	= 0;
static int new_ci_length		= 0;
static int new_syllable_length	= 0;
static HZ  new_ci_string[MAX_SYLLABLE_PER_INPUT + 2]	 = {0};
static SYLLABLE new_syllable[MAX_SYLLABLE_PER_INPUT + 2] = {0};

/**	获得候选的音节
*/
int GetCandidateSyllable(CANDIDATE *candidate, SYLLABLE *syllables, int length)
{
    assert(length >= 1);

    switch(candidate->type)
    {
    case CAND_TYPE_ZI:
        if (length < 1)
            return 0;

        //判断是否为类似xian的词
        if (!candidate->hz.is_word)
        {
            syllables[0] = candidate->hz.item->syllable;
            return 1;
        }

        if (length < (int)candidate->hz.word_item->syllable_length)
            return 0;

        memcpy(syllables, candidate->hz.word_item->syllable, sizeof(SYLLABLE) * candidate->hz.word_item->syllable_length);
        return candidate->hz.word_item->syllable_length;

    case CAND_TYPE_ICW:
        if (length < candidate->icw.length)
            return 0;

        memcpy(syllables, candidate->icw.syllable, sizeof(SYLLABLE) * candidate->icw.length);
        return candidate->icw.length;

    case CAND_TYPE_CI:
        if (length < (int)candidate->word.item->syllable_length)
            return 0;

        memcpy(syllables, candidate->word.syllable, sizeof(SYLLABLE) * candidate->word.item->syllable_length);
        return candidate->word.item->syllable_length;

    case CAND_TYPE_SPW: //短语输入的串不作为拼音处理
        //case CAND_TYPE_URL:
        return 0;

    case CAND_TYPE_ZFW:
        if (length < 1)
            return 0;

        syllables[0] = candidate->zfw.syllable;
        return 1;
    }

    return 0;
}

/**	获得候选的汉字
*	返回：
*		候选字符串的长度（以字节为单位）
*/
int GetCandidateString(PIMCONTEXT *context, CANDIDATE *candidate, TCHAR *buffer, int length)
{

    assert(length >= 1);

    //wmemset(buffer, 0, length);
    memset(buffer, 0, length*sizeof(TCHAR));
    switch(candidate->type)
    {
    case CAND_TYPE_ZI:
        if (candidate->hz.is_word)
        {
            if (length < (int)candidate->hz.word_item->ci_length)
                return 0;

            memcpy(buffer, GetItemHZPtr(candidate->hz.word_item), candidate->hz.word_item->ci_length * sizeof(HZ));
            return candidate->hz.word_item->ci_length;
        }
        if (length < 1)
            return 0;

        if (candidate->hz.item->hz > 0xFFFF)
        {
            UCS32ToUCS16(candidate->hz.item->hz, buffer);
            return 2;
        }
        else
        {
            *(HZ*)buffer = candidate->hz.item->hz;
            return 1;
        }

    case CAND_TYPE_ICW:
        if (length < (int)(candidate->icw.length))
            return 0;

        memcpy(buffer, candidate->icw.hz, sizeof(HZ) * candidate->icw.length);
        buffer[candidate->icw.length] = 0;

        if (pim_config->hz_output_mode & HZ_OUTPUT_TRADITIONAL)
            WordJ2F(buffer);
        return candidate->icw.length;

    case CAND_TYPE_CI:

        if (length < (int)(candidate->word.item->ci_length))
            return 0;

        memcpy(buffer, candidate->word.hz, sizeof(HZ) * candidate->word.item->ci_length);
        buffer[candidate->word.item->ci_length] = 0;

        if (pim_config->hz_output_mode & HZ_OUTPUT_TRADITIONAL)
            WordJ2F(buffer);

        return candidate->word.item->ci_length;

    case CAND_TYPE_SPW:

        if (length < candidate->spw.length)
            return 0;
        if (candidate->spw.type == SPW_STIRNG_ENGLISH)
        {
            AnsiToUtf16(candidate->spw.string, buffer, candidate->spw.length);

            if (context->input_length && length)
            {
                if (context->input_string[0] != buffer[0] && tolower(context->input_string[0]) == tolower(buffer[0]))
                    buffer[0] = context->input_string[0];
            }
        }
        else if (candidate->spw.type == SPW_STRING_BH)
            UCS32ToUCS16(candidate->spw.hz, buffer);
        else
            memcpy(buffer, candidate->spw.string, candidate->spw.length * sizeof(TCHAR));

        buffer[candidate->spw.length] = 0;
        return candidate->spw.length;

        //case CAND_TYPE_URL:
        //	if(length<candidate->url.length)
        //		return 0;

        //	memcpy(buffer, candidate->url.string, sizeof(TCHAR) * candidate->url.length);
        //	buffer[candidate->url.length] = 0;

        //	return candidate->url.length;

    case CAND_TYPE_ZFW:
        if (length < 1)
            return 0;

        *(TCHAR*)buffer = candidate->zfw.hz;
        return 1;
    }

    return 0;
}

/**	获得候选的汉字的显示字符串（过长的要缩短）
*	返回：
*		候选字符串的长度（以字节为单位）
*/
int GetCandidateDisplayString(PIMCONTEXT *context, CANDIDATE *candidate, TCHAR *buffer, int length, int first_candidate)
{
    int   str_length;
    TCHAR str_buffer[MAX_SPW_LENGTH + 2] = {0};

    //执行程序串
    if (candidate->type == CAND_TYPE_SPW)
    {
        /*	u命令去掉
        if (!pim_config->use_u_hint && candidate->spw.type == SPW_STRING_EXEC)
        {
            GetUDisplayString(candidate, buffer, length);
            return (int)utf16_strlen(buffer);
        }
        */
        if (candidate->spw.type == SPW_STRING_BH)
        {
            GetBHDisplayString(candidate, buffer, length);
            return (int)utf16_strlen(buffer);
        }
    }

    str_length = GetCandidateString(context, candidate, str_buffer, MAX_SPW_LENGTH + 2);

    if (first_candidate && !context->selected_item_count &&
            context->compose_cursor_index && context->compose_cursor_index < context->compose_length)
    {
        TCHAR str[MAX_SPW_LENGTH + 2];
        int	syllable_index = GetSyllableIndexByComposeCursor(context, context->compose_cursor_index);
        _tcsncpy_s(str, _SizeOf(str), context->default_hz, syllable_index);
        _tcscat_s(str, _SizeOf(str), str_buffer);
        _tcscpy_s(str_buffer, _SizeOf(str_buffer), str);
    }

    return PackStringToBuffer(str_buffer, str_length, buffer, length);
}

/**	求pos之前完整的音节数
 */
int GetSyllableIndexByComposeCursor(PIMCONTEXT *context, int pos)
{
    int i, ret = 0;
    int legal_length = GetLegalPinYinLength(context->input_string, context->state, context->english_state);

    if (pos <= 0)
        ret = 0;
    else if (context->compose_length == pos || context->input_pos > legal_length)
        ret = context->syllable_count;
    else
    {
        for (i = 0; i < pos && i < context->compose_length; i++)
            if (SYLLABLE_SEPARATOR_CHAR == context->compose_string[i])
                ret++;

        if (i < context->compose_length && SYLLABLE_SEPARATOR_CHAR == context->compose_string[i])
            ret++;

        for (i = 0; i < context->selected_item_count; i++)
            ret += context->selected_items[i].syllable_length;
    }

    return ret;
}

/**	用户选择后续处理
 */
void PostResult(PIMCONTEXT *context)
{
    if (!context || !context->result_syllable_count)
        return;

    //是不是一次选择
    if (context->selected_item_count == 1)		//只有一个
    {
        //ICW
        if (context->selected_items[0].candidate.type == CAND_TYPE_ICW)
        {
            if (pim_config->save_icw && !(pim_config->hz_output_mode & HZ_OUTPUT_TRADITIONAL))
            {U16_DEBUG_ECHO("context->result_string:%s", context->result_string);
                //加入到词库以及Cache中
                AddCi(context->result_syllables, context->result_syllable_count, (HZ*)context->result_string, context->result_syllable_count);
                //准备删除词汇
                PrepareDeleteNewCi((HZ*)context->result_string, context->result_syllable_count, context->result_syllables, context->result_syllable_count);
                //加入到cache
                InsertCiToCache((HZ*)context->result_string, context->selected_items[0].candidate.icw.length, context->result_syllable_count, 0);
            }
            return;
        }

        //单字
        if (context->selected_items[0].candidate.type == CAND_TYPE_ZI)
        {
            //输入类似xian的词汇
            if (context->selected_items[0].candidate.hz.is_word){
                InsertCiToCache((HZ*)context->result_string,
                                context->selected_items[0].candidate.hz.word_item->ci_length,
                        context->selected_items[0].candidate.hz.word_item->syllable_length,
                        0);
                U16_DEBUG_ECHO("context->result_string:%s", context->result_string);
            }else{
                U16_DEBUG_ECHO("context->result_string:%s", context->result_string);
                ProcessZiSelected(context->selected_items[0].candidate.hz.item);
            }
        }

        //词
        if (context->selected_items[0].candidate.type == CAND_TYPE_CI)
        {
            //非以词定字
            if (context->selected_items[0].left_or_right == ZFW_NONE)
            {U16_DEBUG_ECHO("context->result_string:%s", context->result_string);
                ProcessCiSelected(context->result_syllables,
                                  context->result_syllable_count,
                                  (HZ*)context->result_string,
                                  context->selected_items[0].candidate.hz.word_item->ci_length);

                CheckQuoteInput(*(HZ*)context->result_string);
                return;
            }

            //以词定字
            ProcessZiSelectedByWord(*(HZ*)context->result_string, context->result_syllables[0]);
        }
        return;
    }

    context->result_length = (int)utf16_strlen(context->result_string);

    //检查是否为用户词库中的新词
    //CheckNewUserWord(context->result_syllables, context->result_syllable_count, (HZ*)context->result_string, context->result_length);
    //多次选择，组成新词
    AddCi(context->result_syllables, context->result_syllable_count, (HZ*)context->result_string, context->result_length);
    //加入到Cache中
    InsertCiToCache((HZ*)context->result_string, context->result_length, context->result_syllable_count, 0);
    //准备删除词汇
    PrepareDeleteNewCi((HZ*)context->result_string, context->result_length, context->result_syllables, context->result_syllable_count);
}

/**	项输入串中增加字符
*/
void AddChar(PIMCONTEXT *context, TCHAR ch, int is_numpad)
{

    int i, index, syllable_remains;
    CANDIDATE *candidate;
    const TCHAR *symbol_string;
    int handle_symbol;
    if (!ch || context->input_length >= MAX_INPUT_LENGTH ||	context->syllable_count >= MAX_SYLLABLE_PER_INPUT){
        return;
    }
    //判断是否当作符号来处理
    if (context->state == STATE_EDIT ||
            (context->state == STATE_ILLEGAL && context->candidate_count && context->candidate_array[0].type == CAND_TYPE_SPW))
    {
        handle_symbol = 1;

        if (pim_config->ci_option & CI_WILDCARD && (ch == '*')){
            handle_symbol = 0;
        }
        if (context->candidate_array[0].spw.type == SPW_STRING_BH && (ch == '*' || ch== '?'))
            handle_symbol = 0;

        if (context->english_state != ENGLISH_STATE_NONE && ch == '-')
            handle_symbol = 0;

        if (pim_config->use_hz_tone)
        {
            if (ch == TONE_CHAR_1 || ch == TONE_CHAR_2 || ch == TONE_CHAR_3 || ch == TONE_CHAR_4 || ch == TONE_CHAR_CHANGE)
                handle_symbol = 0;
        }
        else if (ch == '@' || ch == '#')
            handle_symbol = 0;

        if (ch == SYLLABLE_SEPARATOR_CHAR || ch == '_' || ch == ':')
            handle_symbol = 0;

        if (pim_config->pinyin_mode == PINYIN_SHUANGPIN && ch == ';')
            handle_symbol = 0;

        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))
            handle_symbol = 0;

        if (is_numpad)
            handle_symbol = 0;
    }
    else{
        handle_symbol = 0;
    }


    if (handle_symbol)
    {
        if (!context->candidate_count)		//没有候选，返回
            return;

        symbol_string = GetSymbol(context, ch);
        if (!symbol_string)		//不是符号
            return;

        index			 = context->candidate_index + context->candidate_selected_index;
        candidate		 = &context->candidate_array[index];
        syllable_remains = context->syllable_count - context->syllable_pos;

        if ((candidate->type == CAND_TYPE_ICW) || (candidate->type == CAND_TYPE_SPW) ||
                (candidate->type == CAND_TYPE_CI && candidate->word.item->syllable_length == syllable_remains) ||
                (candidate->type == CAND_TYPE_CI && candidate->word.type == CI_TYPE_WILDCARD) ||
                (candidate->type == CAND_TYPE_ZI && syllable_remains == 1))
        {
            context->last_symbol = ch;
            SelectCandidate(context, context->candidate_selected_index);
        }

        return;
    }

    if (context->last_dot && !context->next_to_last_dot && ch != '.')
    {
        //依次向右移动一个字符
        for (i = context->input_length + 1; i > context->cursor_pos; i--)
            context->input_string[i] = context->input_string[i - 1];

        //填入当前的字符
        context->input_string[context->cursor_pos] = '.';
        context->cursor_pos++;
        context->input_length++;
        context->candidate_index  = 0;
        context->selected_digital = 0;
    }

    //依次向右移动一个字符
    for (i = context->input_length + 1; i > context->cursor_pos; i--)
        context->input_string[i] = context->input_string[i - 1];

    //填入当前的字符
    context->input_string[context->cursor_pos] = ch;
    context->cursor_pos++;
    context->input_length++;
    context->candidate_index  = 0;
    context->selected_digital = 0;
    context->last_dot		  = 0;
    context->next_to_last_dot = 0;

    ProcessContext(context);




}
/*
 * 这函数和AddChar的差别只是，只把输入添加到上下文，不执行ProcessContext，目的是等整个输入串都记录了，再执行处理
 */
void inputchar(PIMCONTEXT *context, TCHAR ch, int is_numpad)
{
    int i, index, syllable_remains;
    CANDIDATE *candidate;
    const TCHAR *symbol_string;
    int handle_symbol;

    if (!ch || context->input_length >= MAX_INPUT_LENGTH ||	context->syllable_count >= MAX_SYLLABLE_PER_INPUT)
        return;
    //判断是否当作符号来处理
    if (context->state == STATE_EDIT ||
            (context->state == STATE_ILLEGAL && context->candidate_count && context->candidate_array[0].type == CAND_TYPE_SPW))
    {
        handle_symbol = 1;

        if (pim_config->ci_option & CI_WILDCARD && (ch == '*'))
            handle_symbol = 0;

        if (context->candidate_array[0].spw.type == SPW_STRING_BH && (ch == '*' || ch== '?'))
            handle_symbol = 0;

        if (context->english_state != ENGLISH_STATE_NONE && ch == '-')
            handle_symbol = 0;

        if (pim_config->use_hz_tone)
        {
            if (ch == TONE_CHAR_1 || ch == TONE_CHAR_2 || ch == TONE_CHAR_3 || ch == TONE_CHAR_4 || ch == TONE_CHAR_CHANGE)
                handle_symbol = 0;
        }
        else if (ch == '@' || ch == '#')
            handle_symbol = 0;

        if (ch == SYLLABLE_SEPARATOR_CHAR || ch == '_' || ch == ':')
            handle_symbol = 0;

        if (pim_config->pinyin_mode == PINYIN_SHUANGPIN && ch == ';')
            handle_symbol = 0;

        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))
            handle_symbol = 0;

        if (is_numpad)
            handle_symbol = 0;
    }
    else
        handle_symbol = 0;


    if (handle_symbol)
    {
        if (!context->candidate_count)		//没有候选，返回
            return;

        symbol_string = GetSymbol(context, ch);
        if (!symbol_string)		//不是符号
            return;

        index			 = context->candidate_index + context->candidate_selected_index;
        candidate		 = &context->candidate_array[index];
        syllable_remains = context->syllable_count - context->syllable_pos;

        if ((candidate->type == CAND_TYPE_ICW) || (candidate->type == CAND_TYPE_SPW) ||
                (candidate->type == CAND_TYPE_CI && candidate->word.item->syllable_length == syllable_remains) ||
                (candidate->type == CAND_TYPE_CI && candidate->word.type == CI_TYPE_WILDCARD) ||
                (candidate->type == CAND_TYPE_ZI && syllable_remains == 1))
        {
            context->last_symbol = ch;
            SelectCandidate(context, context->candidate_selected_index);
        }

        return;
    }

    if (context->last_dot && !context->next_to_last_dot && ch != '.')
    {
        //依次向右移动一个字符
        for (i = context->input_length + 1; i > context->cursor_pos; i--)
            context->input_string[i] = context->input_string[i - 1];

        //填入当前的字符
        context->input_string[context->cursor_pos] = '.';
        context->cursor_pos++;
        context->input_length++;
        context->candidate_index  = 0;
        context->selected_digital = 0;
    }

    //依次向右移动一个字符
    for (i = context->input_length + 1; i > context->cursor_pos; i--)
        context->input_string[i] = context->input_string[i - 1];

    //填入当前的字符
    context->input_string[context->cursor_pos] = ch;
    context->cursor_pos++;
    context->input_length++;
    context->candidate_index  = 0;
    context->selected_digital = 0;
    context->last_dot		  = 0;
    context->next_to_last_dot = 0;
}

/**	设置非法模式下的写作串
 */
void SetIllegalComposeString(PIMCONTEXT *context)
{
    TCHAR *pc = context->compose_string;
    TCHAR *pi = context->input_string;
    TCHAR at_count = 0;

    while(*pi)
    {
        if (*pi == '@')
        {
            at_count++;
            if (at_count == 2)
            {
                pi++;
                continue;
            }
        }
        *pc++ = *pi++;
    }
    *pc = 0;

    context->compose_length = (int)utf16_strlen(context->compose_string);
    context->compose_cursor_index = context->cursor_pos - (at_count >= 2 ? 1 : 0);
}

/**	当输入非法的时候
 */
void PostIllegalInput(PIMCONTEXT *context)
{
    context->compose_cursor_index = context->cursor_pos;
    context->candidate_count	  = context->candidate_index = context->candidate_page_count = 0;
    context->input_pos			  = context->syllable_count = context->syllable_pos = 0;
    context->selected_item_count  = 0;

    SetIllegalComposeString(context);

    context->modify_flag |= MODIFY_COMPOSE;
    context->state		  = STATE_ILLEGAL;
}

/**	检查用户输入的音节是V还是U，需要与用户输入保持一致
 */
void CheckSyllableStringVAndU(PIMCONTEXT *context, int index, TCHAR *pinyin)
{
    int pos = context->syllable_start_pos[index];
    int i;

    for (i = 0; pinyin[i]; i++)
        if (context->input_string[i + pos] == 'v' && pinyin[i] == 'u')
            pinyin[i] = 'v';
}
/**	获得已经选择项的音节数组
 */
int GetSelectedItemSyllable(SELECT_ITEM *item, SYLLABLE *syllables, int length)
{
    if (length < 1)
        return 0;

    if (item->left_or_right == ZFW_NONE)
        return GetCandidateSyllable(&item->candidate, syllables, length);

    if (item->candidate.type != CAND_TYPE_CI)
        return 0;

    if (item->left_or_right == ZFW_LEFT)
    {
        if (item->candidate.type == CAND_TYPE_CI)
        {
            syllables[0] = item->candidate.word.syllable[0];
            return 1;
        }

        if (item->candidate.type == CAND_TYPE_ICW)
        {
            syllables[0] = item->candidate.icw.syllable[0];
            return 1;
        }

        return 0;
    }

    if (item->left_or_right == ZFW_RIGHT)
    {
        if (item->candidate.type == CAND_TYPE_CI)
        {
            syllables[0] = item->candidate.word.syllable[item->candidate.word.item->syllable_length - 1];
            return 1;
        }

        if (item->candidate.type == CAND_TYPE_ICW)
        {
            syllables[0] = item->candidate.icw.syllable[item->candidate.icw.length - 1];
            return 1;
        }

        return 0;
    }

    return 0;
}

/**	获得当前已经选择的项的字符串
 */
int GetSelectedItemString(PIMCONTEXT *context, SELECT_ITEM *item, TCHAR *buffer, int length)
{
    DEBUG_ECHO("GetSelectedItemString");
    TCHAR ft_ci[0x100];

    if (length <= 1)
        return 0;

    //特殊处理以词定字
    if (item->candidate.type == CAND_TYPE_CI)
    {
        if (item->left_or_right == ZFW_LEFT)
        {
            if (!(pim_config->hz_output_mode & HZ_OUTPUT_TRADITIONAL))
            {
                *(HZ*)buffer = item->candidate.word.hz[0];
                U16_DEBUG_ECHO("item->candidate.word.hz[0]:%s", item->candidate.word.hz[0]);
                *(buffer + 1) = 0;
            }
            else
            {
                memcpy(ft_ci, item->candidate.word.hz, sizeof(HZ) * item->candidate.word.item->ci_length);
                ft_ci[item->candidate.word.item->ci_length] = 0;
                WordJ2F(ft_ci);
                *(HZ*)buffer = *(HZ*)ft_ci;
                *(buffer + 1) = 0;
            }

            return 1;
        }

        if (item->left_or_right == ZFW_RIGHT)
        {
            if (!(pim_config->hz_output_mode & HZ_OUTPUT_TRADITIONAL))
            {
                *(HZ*)buffer = item->candidate.word.hz[item->candidate.word.item->ci_length - 1];
                U16_DEBUG_ECHO("item->candidate.word.hz[0]:%s", item->candidate.word.hz[0]);
                *(buffer + 1) = 0;
            }
            else
            {
                memcpy(ft_ci, item->candidate.word.hz, sizeof(HZ) * item->candidate.word.item->ci_length);
                ft_ci[item->candidate.word.item->ci_length] = 0;
                WordJ2F(ft_ci);

                *(HZ*)buffer = *((HZ*)ft_ci + item->candidate.word.item->ci_length - 1);
                *(buffer + 1) = 0;
            }

            return 1;
        }
    }

    //特殊处理以词定字
    if (item->candidate.type == CAND_TYPE_ICW)
    {
        if (item->left_or_right == ZFW_LEFT)
        {
            if (!(pim_config->hz_output_mode & HZ_OUTPUT_TRADITIONAL))
            {
                *(HZ*)buffer = item->candidate.icw.hz[0];
                U16_DEBUG_ECHO("item->candidate.icw.hz[0]:%s", item->candidate.icw.hz[0]);
                *(buffer + 1) = 0;
            }
            else
            {
                memcpy(ft_ci, item->candidate.icw.hz, sizeof(HZ) * item->candidate.icw.length);
                ft_ci[item->candidate.icw.length] = 0;
                WordJ2F(ft_ci);
                *(HZ*)buffer = *((HZ*)ft_ci);
                *(buffer + 1) = 0;
            }

            return 1;
        }

        if (item->left_or_right == ZFW_RIGHT)
        {
            if (!(pim_config->hz_output_mode & HZ_OUTPUT_TRADITIONAL))
            {
                *(HZ*)buffer = item->candidate.icw.hz[item->candidate.icw.length - 1];
                U16_DEBUG_ECHO("item->candidate.icw.hz[item->candidate.icw.length - 1]:%s", item->candidate.icw.hz[item->candidate.icw.length - 1]);
                *(buffer + 1) = 0;
            }
            else
            {
                memcpy(ft_ci, item->candidate.icw.hz, sizeof(HZ) * item->candidate.icw.length);
                ft_ci[item->candidate.icw.length] = 0;
                WordJ2F(ft_ci);
                *(HZ*)buffer = *((HZ*)ft_ci + item->candidate.icw.length - 1);
                *(buffer + 1) = 0;
            }

            return 1;
        }
    }

    return GetCandidateString(context, &item->candidate, buffer, length);
}
/**	生成结果串以及结果音节数组
 */
void MakeResult(PIMCONTEXT *context)
{
    int			item_count = context->selected_item_count;
    TCHAR		*p_result = context->result_string;
    SYLLABLE	*p_syllable = context->result_syllables;
    int			syllable_count;
    TCHAR		cand_string[MAX_RESULT_LENGTH + 1];
    SYLLABLE	syllables[MAX_SYLLABLE_PER_INPUT];
    int			i, length;
    const TCHAR	*symbol_string;
    int hz_output_ft = 0;
    //非法输入串，以用户输入为结果串
    if (context->syllable_count == 0 && context->selected_item_count == 0)
    {
        utf16_strncpy(p_result, context->input_string, MAX_RESULT_LENGTH);
        context->result_syllable_count = 0;
        context->modify_flag |= MODIFY_RESULT;
        return;
    }
    if(pim_config->hz_output_mode & HZ_OUTPUT_TRADITIONAL){
        //假如是在繁体字输入，先设置为简体，因为PostResult在处理最后的结果，汉字增加频度、词Cache等，
        //会分开简繁体，两种字体分别处理。这样会导致简体输入和繁体输入两种模式下，候选词的排序不一致
        hz_output_ft = 1;
        pim_config->hz_output_mode = HZ_OUTPUT_SIMPLIFIED;
    }
    //遍历用户已经选择的词/字/
    for (i = 0; i < context->selected_item_count; i++)
    {
        length = GetSelectedItemString(context, &context->selected_items[i], cand_string, _SizeOf(cand_string));
        if (length + (p_result - context->result_string) >= MAX_RESULT_LENGTH)
            continue;		//跳过越界的候选

        syllable_count = GetSelectedItemSyllable(&context->selected_items[i], syllables, MAX_SYLLABLE_PER_INPUT);
        if (syllable_count + (p_syllable - context->result_syllables) > MAX_SYLLABLE_PER_INPUT)
            continue;

        if (pim_config->space_after_english_word &&
                context->selected_items[i].candidate.type == CAND_TYPE_SPW &&
                context->selected_items[i].candidate.spw.type == SPW_STIRNG_ENGLISH)
        {
            _tcscat_s(cand_string, MAX_RESULT_LENGTH, TEXT(" "));
            length++;
        }

        memcpy(p_result, cand_string, length * sizeof(TCHAR));
        p_result += length;

        memcpy(p_syllable, syllables, syllable_count * sizeof(SYLLABLE));
        p_syllable += syllable_count;
    }
    *p_result = 0;
    if (context->last_symbol && (symbol_string = GetSymbol(context, context->last_symbol)) != 0)
    {
        _tcscat_s(p_result, MAX_RESULT_LENGTH - utf16_strlen(context->result_string), symbol_string);
        CheckQuoteInput(*(HZ*)symbol_string);
    }
    context->result_length = (int)utf16_strlen(context->result_string);
    context->result_syllable_count = (int)(p_syllable - context->result_syllables);
    context->modify_flag |= MODIFY_RESULT;
    context->state = STATE_RESULT;
    //处理最后的结果，汉字增加频度、词Cache等
    PostResult(context);
    if(hz_output_ft){
        pim_config->hz_output_mode = HZ_OUTPUT_TRADITIONAL;
        WordJ2F(context->result_string);
    }
}
/**	空格或者选择候选上屏，包含以词定字
 *	参数：
 *		index				选择的候选索引
 *		left_or_right		以词定字标示
 */
static void SelectCandidateInternal(PIMCONTEXT *context, int index, int left_or_right)
{
    CANDIDATE *candidate;
    SELECT_ITEM	*items = context->selected_items;
    int count = context->selected_item_count;
    int cursor_pos;
    int legal_length;
    int fuzzy_mode = fuzzy_mode = pim_config->use_fuzzy ? pim_config->fuzzy_mode : 0;
    if (count >= MAX_SYLLABLE_PER_INPUT || index >= context->candidate_page_count)
        return;

    index	 += context->candidate_index;
    candidate = &context->candidate_array[index];


    //执行程序命令
    if (candidate->type == CAND_TYPE_SPW && candidate->spw.type == SPW_STRING_EXEC)
    {
        //此处需要测试，TCHAR的转换不知道是否正确
        RunCommand(context, (TCHAR*)candidate->spw.string);
        ResetContext(context);
        context->modify_flag |= MODIFY_ENDCOMPOSE;
        return;
    }

    items[count].left_or_right = ZFW_NONE;
    items[count].candidate	   = *candidate;

    switch(candidate->type)
    {
    case CAND_TYPE_SPW:		//短语
        //短语可能和非法输入串发生混淆，例如输入aB，又定义了短语aB=阿坝，那么此时按空格
        //应该是把"啊"字上屏，从而得到"啊B"呢，还是将"阿坝"上屏呢？显然这取决于当前焦点
        //所在的候选是"阿坝"还是"啊"，如果是短语，则将input_length设置为legal_length，
        //这样本函数最后会进入MakeResult(上屏)，而不会继续ProcessContext(部分上屏)
        context->input_length = GetLegalPinYinLength(context->input_string, context->state, context->english_state);
        items[count].syllable_start_pos = context->syllable_pos;
        items[count].syllable_length	= context->syllable_count - context->syllable_pos;
        break;

        //case CAND_TYPE_URL:		//url
        //	items[count].syllable_start_pos = context->syllable_pos;
        //	items[count].syllable_length	= context->syllable_count - context->syllable_pos;
        //	break;

    case CAND_TYPE_ICW:		//ICW
        items[count].syllable_start_pos = context->syllable_pos;
        items[count].syllable_length	= candidate->icw.length;
        items[count].left_or_right		= left_or_right;
        break;

    case CAND_TYPE_CI:		//word
        items[count].syllable_start_pos = context->syllable_pos;
        items[count].left_or_right		= left_or_right;

        if (candidate->word.type == CI_TYPE_OTHER)
            items[count].syllable_length = candidate->word.origin_syllable_length;
        else
            items[count].syllable_length = candidate->word.item->syllable_length;
        break;

    case CAND_TYPE_ZI:		//zi
        items[count].syllable_start_pos = context->syllable_pos;
        items[count].syllable_length	= 1;
        //处理声调问题：一些候选字待有声调，但改字在context->input_string中并无声调，例如liangang，可能查liang的
        //候选字得到"量"(CON_L，VOW_IANG，TONE_2，只是举例实际中不一定)，如果选择了该字，GetInputPos测得该字的拼
        //音长度为3(包括声调)，但实际上lianggang里的liang是没有声调的，这将造成context->input_pos计算错误
        if (items[count].candidate.hz.is_word)
        {
            //除了普通的逆向解析(非小音节字，小音节词)，其他情况(逆向解析小音节字、所有正向解析)都应该是没有声调的。
            //此处以前写为：
            //if (context->syllables[context->syllable_pos].con != items[count].candidate.hz.origin_syllable.con ||
            //    context->syllables[context->syllable_pos].vow != items[count].candidate.hz.origin_syllable .vow)
            //不妥，未考虑到模糊音的情况，如"圣修堂"，设置了模糊，输入senxiutang，选择"圣"，满足if条件，不合理
            if (!ContainSyllable(context->syllables[context->syllable_pos],  items[count].candidate.hz.origin_syllable, fuzzy_mode))
            {
                //替换掉音节，因为下面的GetInputPos函数正是利用context->syllables(而不是items[count]中的音节)来计算
                //context->input_pos的(这样做可以解决二者不一致的问题，如items[count]中的音节有声调(如字库中选出的候
                //选字通常都是有声调的)，而context->syllables中的音节没有声调，context->input_pos应该以context->syllables
                //为准)
                context->syllables[context->syllable_pos] = items[count].candidate.hz.origin_syllable;
                context->syllables[context->syllable_pos].tone = TONE_0;
            }
        }
        else
        {
            //同上
            if (!ContainSyllable(context->syllables[context->syllable_pos], items[count].candidate.hz.item->syllable, fuzzy_mode))
            {
                context->syllables[context->syllable_pos] = items[count].candidate.hz.item->syllable;
                context->syllables[context->syllable_pos].tone = TONE_0;
            }
        }

        //如果是小音节字，增加一个音节。注意小音节词仍然算一个音节，即输入xianren，如果
        //直接选择"西安"是一个音节，如果分别选择"西"和"安"是两个音节。按backspace键可以
        //明确地看到这一区别，当compose_string为"西安ren"时，按一次backspace前者变为"xianren"
        //，后者变为"西anren"
        if (!items[count].candidate.hz.is_word && items[count].candidate.hz.hz_type == ZI_TYPE_OTHER)
        {
            context->syllable_count++;
        }

        break;

    case CAND_TYPE_ZFW:		//以词定字
        items[count].syllable_start_pos = context->syllable_pos;
        items[count].syllable_length	= candidate->zfw.word->item->syllable_length;
        break;

    default:
        return;
    }

    context->selected_item_count++;

    cursor_pos = context->cursor_pos;

    //context->expand_candidate = pim_config->always_expand_candidates ? 1 : 0;
    context->candidate_index  = 0;
    context->cursor_pos		  = context->input_length;
    context->selected_digital = 0;
    context->input_pos		  = GetInputPos(context);
    context->syllable_pos	 += items[count].syllable_length;

    if (context->selected_item_count && cursor_pos != context->input_length)
    {
        int index = context->selected_item_count - 1;

        //虽然此时context->syllables尚未重新解析，但至少当前的音节已经更新(见上面CAND_TYPE_ZI)的代码，此时
        //先计算一下context->syllable_start_pos，以保证光标位置正确(例如|xianren，选择了"西"，正确的结果应
        //该为"西|anren"，但如果缺少下面这句，结果为"西an|ren")，在ProcessContext里重新解析音节后还会再计算
        //context->syllable_start_pos
        MakeSyllableStartPosition(context);

        index = context->selected_items[index].syllable_start_pos + context->selected_items[index].syllable_length;
        context->cursor_pos = context->syllable_start_pos[index];
    }

    legal_length = GetLegalPinYinLength(context->input_string, context->state, context->english_state);

    if (legal_length < context->input_length)
        ProcessContext(context);
    else if (context->state == STATE_ILLEGAL || context->syllable_pos >= context->syllable_count)	//结束！
        MakeResult(context);				//生成结果返回
    else
        ProcessContext(context);			//处理新的候选
}

/*	处理候选选择
 */
void SelectCandidate(PIMCONTEXT *context, int index)
{
    if (context->syllable_mode)
    {
        //光标之前的音节数
        int syllable_index = GetSyllableIndexByComposeCursor(context, context->compose_cursor_index);

        //光标既不在开头也不在结尾
        if (syllable_index > 0 && syllable_index < context->syllable_count)
        {
            int i, selected_len, selected_syllable_len, default_len, cand_len, new_syllable_index, count;
            TCHAR selected_string[MAX_RESULT_LENGTH + 1] = {0};
            TCHAR cand_index_str[MAX_RESULT_LENGTH + 1] = {0};

            _tcscpy_s(selected_string, MAX_RESULT_LENGTH, context->selected_compose_string);

            GetCandidateString(context, &context->candidate_array[context->candidate_index + index], cand_index_str, _SizeOf(cand_index_str));

            //要特别注意区分SELECT_ITEM的成员syllable_length和GetSelectedItemSyllable
            //的返回值，二者可能不是相同的！！！前者是指该选中项所占的context->syllables
            //中的音节数，例如选中项是小音节词，那么syllable_length==1；后者在内部调用
            //GetCandidateSyllable，返回的是实际的音节数，对于小音节词，通常是2。这里我
            //们意图为光标之前的尚未转化为汉字的音节从default_hz中为其指定默认值。但光
            //标之前到底有几个音节未转化为汉字？GetSyllableIndexByComposeCursor是根据context
            //->compose_string中的分隔符和选中项的syllable_length来计算光标之前的音节数
            //的。但是这样计算出的音节数可能和光标之前的default_hz中的汉字数不匹配，例如
            //输入xianshirenhenhao，移动光标，结果为"xianshi|renhenhao"，default_hz为"西
            //安市人很好"，此时syllable_index为2(xian'shi)，即光标之前有2个音节，而此时
            //没有已经选择的汉字，所以默认的字数是2-2=0，这显然错误。但实际上对于default_hz
            //来说，光标之前的字数应为3("西安市"))，这样3-0=3，才能获得正确的需要默认的
            //汉字数。即我们希望获得的默认汉字数是光标之前的default_hz的字数(或者说光标
            //之前的default_hz_syllables中的音节数)。

            //例： compse_string 你们zai'gan'|shen'me (|为光标位置)
            //候选为 1.在干什么 2.神 3.身 4.深 ...
            //selected_compose_string 你们
            //cand_string 你们
            //syllable_hz 在干什么
            //cand_index_string 身 假设选了3
            //syllable_index 4(指未修正前的，你们zai'gan一共4个音节)
            selected_len = (int)utf16_strlen(selected_string);
            default_len = (int)utf16_strlen((TCHAR*)context->default_hz);
            cand_len = (int)utf16_strlen(cand_index_str);

            //再次注意selected_syllable_len和selected_len不一定相等！！！
            selected_syllable_len = context->syllable_pos;

            new_syllable_index = GetSyllableIndexInDefaultString(context, syllable_index);

            //关于条件"default_len > cand_len"的意义:
            //第2个条件的意义：
            //default_len == cand_len
            //输入ziguanghao，移动光标使其为ziguang|hao，通常第1个候选是所有尚未
            //转化为汉字的拼音的一个默认值(见MakeCandidate的第二个if，且该默认值
            //通常就是default_hz)，这里第1个候选为"紫光号"，从第2个候选开始是拼音
            //为hao的单字，选择第1个候选，此时default_len为3，cand_len也为3，但此
            //时实际上是没有需要默认填充的汉字的，不应该进入下面的if。否则结果为
            //"紫光紫光号"
            //default_len > cand_len
            //输入ziguanghao，移动光标使其为ziguang|hao，第1个候选为"紫光号"，从
            //第2个候选开始是拼音为hao的单字，选择第"号"，此时default_len为3，cand_len
            //为1，需要默认填充"紫光"，应该进入下面的if
            //default_len < cand_len
            //设置超级简拼个数为3，并设置韵母首字母模糊，输入jincha，移动光标使其
            //为jin|cha，此时第1个候选是"金叉"，第2个候选是"重婚案"，选择第2个候选，
            //此时default_len为2，cand_len为3，需要默认填充"金"，应该进入下面的if
            //那么第2个条件是否可以为default_len != cand_len呢？不行！
            //例如输入qixianren，移动光标使其为qi|xianren，此时第1个候选为"七贤人"，
            //第2个候选为"西安人"，default_len == cand_len，且有一个默认填充的的汉
            //字"七"。从文字本身判断，即使不能100%准确，也比从长度判断好得多
            if (new_syllable_index > selected_len && utf16_strncmp((TCHAR*)context->default_hz, cand_index_str, default_len))
            {
                //最后一个参数表示光标之前的、尚未转化为汉字的音节数
                _tcsncat_s(selected_string, MAX_RESULT_LENGTH - selected_len,
                           (TCHAR*)context->default_hz, new_syllable_index - selected_len);

                //注意syllable_length不是new_syllable_index - selected_len，原因见上面注释。
                //只需记住candidate里是真实的音节，其余地方是解析出的音节！！！

                //将默认填充的汉字转化为一个智能组词候选
                count = context->selected_item_count;
                context->selected_items[count].left_or_right		   = ZFW_NONE;
                context->selected_items[count].syllable_length         = syllable_index - selected_syllable_len;
                context->selected_items[count].syllable_start_pos      = selected_syllable_len;

                context->selected_items[count].candidate.type		   = CAND_TYPE_ICW;
                context->selected_items[count].candidate.icw.length    = new_syllable_index - selected_len;
                _tcsncpy_s(context->selected_items[count].candidate.icw.hz,
                           MAX_ICW_LENGTH + 1, context->default_hz, new_syllable_index - selected_len);
                for (i = 0; i < new_syllable_index - selected_len; i++)
                    context->selected_items[count].candidate.icw.syllable[i] = context->default_hz_syllables[i];

                context->syllable_pos += syllable_index - selected_syllable_len;
                context->selected_item_count++;
            }
        }
    }

    SelectCandidateInternal(context, index, 0);
}

/**	准备删词的工作
 */
void PrepareDeleteNewCi(HZ *new_ci, int ci_length, SYLLABLE *syllable, int syllable_length)
{
    //UCS4的词不存在于新词表
    if (ci_length != syllable_length)
        return;

    new_ci_create_time	= GetCurrentTicks();
    new_ci_length		= ci_length;
    new_syllable_length = syllable_length;

    memcpy(new_ci_string, new_ci, sizeof(HZ) * ci_length);
    memcpy(new_syllable, syllable, sizeof(SYLLABLE) * syllable_length);
}

/*	处理上下文。
 *	上下文处理的基础：
 *	0. state
 *	1. input_string
 *	2. input_pos
 *	3. syllables
 *	4. syllable_pos
 *	5. selected_items
 *	6. selected_item_count
*/
void ProcessContext(PIMCONTEXT *context)
{
    unsigned long long start,end;
    SYLLABLE new_syllables[MAX_SYLLABLE_PER_INPUT];
    //int new_syllable_flags[MAX_SYLLABLE_PER_INPUT] = {0};
    //int new_separator_flags[MAX_SYLLABLE_PER_INPUT] = {0};
    TCHAR cand_string[MAX_RESULT_LENGTH + 1];
    int	new_syllable_count;
    int selected_length;
    int cursor_pos, input_pos, new_cursor_pos;
    int legal_length;
    int i, j;
    TCHAR *p;

    //智能编辑状态
    if (context->state == STATE_IEDIT && context->english_state == ENGLISH_STATE_NONE)
    {
        MakeCandidate(context);
        //CalcCurrentURLStr(context);
        return;
    }

    //V输入状态
    if (context->state == STATE_VINPUT && context->english_state == ENGLISH_STATE_NONE)
    {
        TCHAR fullshape_string[MAX_RESULT_LENGTH] = {0};
        if (pim_config->v_mode_enabled)
        {
            utf16_strcpy(context->compose_string, TEXT(">"));
            utf16_strcat(context->compose_string, context->input_string + 1);
        }
        else
        {
            utf16_strcpy(context->compose_string, context->input_string);
        }
        if(!(pim_config->hz_option & HZ_SYMBOL_HALFSHAPE)){
            GetFullShapes(context->compose_string, fullshape_string, _SizeOf(fullshape_string));
            _tcscpy_s(context->compose_string, MAX_RESULT_LENGTH, fullshape_string);
        }
        context->compose_length = (int)utf16_strlen(context->compose_string);
        context->compose_cursor_index = context->cursor_pos;
        MakeCandidate(context);		//寻找SPW
        //CalcCurrentURLStr(context);
        return;
    }

    if (STATE_ILLEGAL == context->state || STATE_IINPUT == context->state || STATE_UINPUT == context->state)
    {
        new_syllable_count =
                ParsePinYinStringReverse(context->input_string,
                                         new_syllables,
                                         MAX_SYLLABLE_PER_INPUT,
                                         pim_config->use_fuzzy ? pim_config->fuzzy_mode : 0,
                                         pim_config->pinyin_mode//,
                                         //new_syllable_flags,
                                         /*new_separator_flags*/);

        if (!new_syllable_count)	//还是非法的
        {
            //设置Compose字符串，主要过滤两个@@
            SetIllegalComposeString(context);
            context->selected_compose_string[0] = 0;
            context->syllable_count = 0;
            MakeCandidate(context);		//寻找SPW
            //CalcCurrentURLStr(context);
            return;
        }

        //一旦变为合法的，继续处理
        context->state = STATE_EDIT;	//继续进行处理
    }

    //英文输入、候选状态
    if (context->english_state != ENGLISH_STATE_NONE)
    {
        utf16_strcpy(context->compose_string, context->input_string);
        context->compose_length = (int)utf16_strlen(context->compose_string);
        context->compose_cursor_index = context->cursor_pos;
        context->modify_flag |= MODIFY_COMPOSE;
        if (pim_config->input_style == STYLE_CSTAR || context->english_state == ENGLISH_STATE_INPUT){
            MakeCandidate(context);

        }

        //CalcCurrentURLStr(context);
        return;
    }
    //1. 重新计算音节分割，已选择的部分不变
    new_syllable_count =
            ParsePinYinStringReverse(context->input_string + context->input_pos,
                                     new_syllables + context->syllable_pos,
                                     MAX_SYLLABLE_PER_INPUT - context->syllable_pos,
                                     pim_config->use_fuzzy ? pim_config->fuzzy_mode : 0,
                                     pim_config->pinyin_mode);

    if(context->input_string[0] == 'I'){
        legal_length = GetLegalPinYinLength(context->input_string, STATE_IINPUT, context->english_state);
    }else if(context->input_string[0] == 'B'){
        legal_length = GetLegalPinYinLength(context->input_string, STATE_BINPUT, context->english_state);
    }else{
        legal_length = GetLegalPinYinLength(context->input_string, context->state, context->english_state);
    }

    //非法拼音串，第二个条件的作用在于用户可能希望对于非法串有不同的处理
    //方式，例如输入aB，虽然B不合法，但由于a是合法的，候选结果中有"啊"，
    //此时用户按空格，上屏内容应为"啊B"，即选择的汉字"啊"应该上屏；又如输
    //入wangfuzong(见下面几行的处的注释)，此时又希望已经选择的汉字"王"被
    //消去。我们用if的第二个条件来区分这两种情况，大体上，legal_length是
    //输入串中首个大写字母之前的长度，因此条件二的意思是如果未输入大写字母
    //就应该考虑重新拆分音节而不是上屏。此外，双拼不受此影响
    if (!new_syllable_count && (pim_config->pinyin_mode == PINYIN_SHUANGPIN || legal_length == context->input_length))
    {
        int state = context->state;
        PostIllegalInput(context);  //每次执行这函数都会把context->state设置为STATE_ILLEGAL
        //再试一次，例如输入wangfuzhong，先选择"王"，结果为"王fuzhong"，
        //移动光标，使其为"王|fuzhong"，按Delete删除f，此时剩余串uzhong
        //为非法，但wanguzhong却合法(wan'gu'zhong)，因此此处应该再次尝试
        //从头解析拼音串
        new_syllable_count =
                ParsePinYinStringReverse(context->input_string,
                                         new_syllables,
                                         MAX_SYLLABLE_PER_INPUT,
                                         pim_config->use_fuzzy ? pim_config->fuzzy_mode : 0,
                                         pim_config->pinyin_mode);

        if (!new_syllable_count)
        {
            //还是要寻找spw的候选
            MakeCandidate(context);

            if (state == STATE_EDIT)
            {
                if (context->input_length >= 1 && context->input_string[0] == 'v')
                {
                    context->state = STATE_VINPUT;
                    ProcessContext(context);
                }else if(context->input_length >= 1 && context->input_string[0] == 'B'){
                    context->state = STATE_BINPUT;
                    ProcessContext(context);

                }else if(context->input_length >= 1 && (context->input_string[0] == 'i' || context->input_string[0] == 'I')){
                    context->state = STATE_IINPUT;
                    ProcessContext(context);
                }
                else
                    context->state = STATE_EDIT;

            }
            else if (state == STATE_VINPUT){
                context->state = STATE_VINPUT;
            }else if(state == STATE_BINPUT){
                context->state = STATE_BINPUT;
            }else if (pim_config->pinyin_mode == PINYIN_SHUANGPIN && context->candidate_count > 0 &&
                      context->candidate_array[0].type != CAND_TYPE_SPW)
                context->state = STATE_EDIT;

            return;
        }
    }

    for (i = 0; i < context->syllable_pos; i++)
    {
        new_syllables[i] = context->syllables[i];
    }

    new_syllable_count += context->syllable_pos;

    //2. 判断已经选择的音节是否与现在的音节相符合
    for (i = 0; i < new_syllable_count && i < context->syllable_pos; i++)
    {
        if (!SameSyllable(new_syllables[i], context->syllables[i])){
            break;
        }
    }

    if (i != context->syllable_pos)			//前面的已经发生的改变，全部展开
    {
        context->selected_item_count = 0;
        context->syllable_pos = 0;
        context->input_pos = 0;
    }
    else	//不需要全部展开，将新的音节补充上
    {
        for (i = context->syllable_pos; i < new_syllable_count; i++)
        {
            context->syllables[i] = new_syllables[i];
        }
        context->syllable_count = new_syllable_count;
    }
    //3. 生成Compose串
    //3.1 生成已经选择的汉字串
    p = context->compose_string;
    *p = 0;

    for (i = 0; i < context->selected_item_count; i++)
    {
        GetSelectedItemString(context, &context->selected_items[i], cand_string, _SizeOf(cand_string));
        //链接选择的汉字
        for (j = 0; cand_string[j]; j++)
            *p++ = cand_string[j];
    }

    //现在写作串前面为已经选择的汉字
    selected_length = (int)(p - context->compose_string);
    _tcsncpy_s(context->selected_compose_string, _SizeOf(context->selected_compose_string),	context->compose_string, selected_length);
    context->selected_compose_string[selected_length] = 0;

    //3.2 生成尚未选择的拼音串
    input_pos	   = context->input_pos;
    cursor_pos	   = context->cursor_pos;
    new_cursor_pos = cursor_pos;
    context->sp_hint_string[0] = 0;

    for (i = context->syllable_pos; i < context->syllable_count; i++)
    {
        TCHAR pinyin[0x10];
        TCHAR hint[0x10];
        int  py_len;

        if (pim_config->pinyin_mode == PINYIN_QUANPIN)
        {
            py_len = GetSyllableString(context->syllables[i], pinyin, _SizeOf(pinyin),/* context->syllable_correct_flag[i],*/ 0);
            CheckSyllableStringVAndU(context, i, pinyin);

        }
        else{
            py_len = GetSyllableStringSP(context->syllables[i], pinyin, _SizeOf(pinyin));
        }

        GetSyllableString(context->syllables[i], hint, _SizeOf(hint),/* context->syllable_correct_flag[i],*/ 0);
        if (utf16_strlen(context->sp_hint_string) + utf16_strlen(hint) < _SizeOf(context->sp_hint_string) - 1)
            _tcscat_s(context->sp_hint_string, _SizeOf(context->sp_hint_string), hint);

        for (j = 0; pinyin[j]; j++)
            *p++ = pinyin[j];

        input_pos += py_len;

        //现在用户输入了分隔符
        if (context->input_string[input_pos] == SYLLABLE_SEPARATOR_CHAR)
        {
            *p++ = SYLLABLE_SEPARATOR_CHAR;
            input_pos++;
            if (utf16_strlen(context->sp_hint_string) + 1 < _SizeOf(context->sp_hint_string) - 1)
                _tcscat_s(context->sp_hint_string, _SizeOf(context->sp_hint_string), TEXT("'"));

            continue;
        }

        //没有音调的，需要加上音节分隔符
        if (i != context->syllable_count - 1)
        {
            *p++ = SYLLABLE_SEPARATOR_CHAR;
            if (utf16_strlen(context->sp_hint_string) + 1 < _SizeOf(context->sp_hint_string) - 1)
                _tcscat_s(context->sp_hint_string, _SizeOf(context->sp_hint_string), TEXT("'"));

            if (input_pos < cursor_pos)
                new_cursor_pos++;		//光标要跳过一个
        }
    }

    if (legal_length < context->input_length)
    {
        for (i = legal_length; i < context->input_length; i++)
            *p++ = context->input_string[i];
    }

    *p = 0;

    //制作每一个音节在输入串中位置数据
    MakeSyllableStartPosition(context);

    //3.3 设定新的光标位置
    context->compose_cursor_index = new_cursor_pos - context->input_pos + selected_length;
    context->compose_length		  = (int)utf16_strlen(context->compose_string);
    context->modify_flag		 |= MODIFY_COMPOSE;
    context->force_vertical		  = 0;
    //4. 确定候选信息

    if (pim_config->input_style == STYLE_CSTAR){
        MakeCandidate(context);
    }
}

/**	运行u、E命令
 */
void RunCommand(PIMCONTEXT *context, const TCHAR *cmd_string)
{
    if (!utf16_strcmp(cmd_string, TEXT("ft")))
        SetToFanti(context);
    else if (!utf16_strcmp(cmd_string, TEXT("jt")))
        SetToJianti(context);
    else if (!utf16_strcmp(cmd_string, TEXT("sp")))
        SetToShuangPin(context);
    else if (!utf16_strcmp(cmd_string, TEXT("qp")))
        SetToQuanPin(context);
    else if (!utf16_strcmp(cmd_string, TEXT("abc")))
        SetInputStyle(context, STYLE_ABC);
    else if(!utf16_strcmp(cmd_string, TEXT("qjsr")))
        pim_config->hz_option ^= HZ_SYMBOL_HALFSHAPE;
    else if (!utf16_strcmp(cmd_string, TEXT("cstar")))
        SetInputStyle(context, STYLE_CSTAR);
    else if (!utf16_strcmp(cmd_string, TEXT("qj")))
        SetToLargeSet(context);

}

int GetInputPos(PIMCONTEXT *context)
{
    int i, j;
    int pos = 0;

    //遍历当前的音节，找到需要开始的输入串位置
    for (i = 0; i < context->selected_item_count; i++)
    {
        for (j = context->selected_items[i].syllable_start_pos;
             j < context->selected_items[i].syllable_start_pos + context->selected_items[i].syllable_length;
             j++)
        {
            TCHAR pinyin[0x10];
            int  py_len;

            if (pim_config->pinyin_mode == PINYIN_QUANPIN)
                py_len = GetSyllableString(context->syllables[j], pinyin, _SizeOf(pinyin),/* context->syllable_correct_flag[j],*/ 0);
            else
                py_len = GetSyllableStringSP(context->syllables[j], pinyin, _SizeOf(pinyin));

            pos += py_len;
            if (context->input_string[pos] == SYLLABLE_SEPARATOR_CHAR)
                pos++;
        }
    }

    return pos;
}

/**	将GetSyllableIndexByComposeCursor的结果转化为default_hz_syllables中的结果
 */
int GetSyllableIndexInDefaultString(PIMCONTEXT *context, int syllable_index)
{
    int i, j, pos, has_star, fuzzy_mode, new_syllable_index;
    int syllable_diff, remain_syllable_len, default_syllable_len, checked_syllable_len;
    TCHAR checked_syllable_string[MAX_SYLLABLE_PER_INPUT * MAX_PINYIN_LENGTH + 1]  = {0};
    SYLLABLE parsed_syllables[MAX_SYLLABLE_PER_INPUT + 0x10] = {0};
    SYLLABLE reparsed_syllables[MAX_SYLLABLE_PER_INPUT + 0x10] = {0};
    SYLLABLE current_syllable;

    default_syllable_len = (int)utf16_strlen((TCHAR*)context->default_hz);
    remain_syllable_len = syllable_index - context->syllable_pos;
    syllable_diff = default_syllable_len - (context->syllable_count - context->syllable_pos);

    //此if还有待进一步思考，目前这样写的依据是syllable_diff < 0时下面的for
    //不会执行，不满足循环条件
    if (syllable_diff < 0)
        return syllable_index;

    //注意，此种情况不能返回syllable_index，由remain_syllable_len的计算公式
    //可知syllable_index == remain_syllable_len + context->syllable_pos，但
    //后者显然不一定等于_tcslen(context->selected_compose_string)，(如选择了
    //"西安")
    if (!remain_syllable_len || !syllable_diff)
        return remain_syllable_len + utf16_strlen(context->selected_compose_string);

    //用户输入里有通配符的情况
    has_star = 0;
    for (i = context->syllable_pos; i < syllable_index; i++)
    {
        if (context->syllables[i].con == CON_ANY && context->syllables[i].vow == VOW_ANY)
        {
            has_star = 1;
            break;
        }
    }

    //避免因声调引起不匹配，因为音节解析结果与声调无关
    for (i = 0; i < remain_syllable_len; i++)
    {
        parsed_syllables[i] = context->syllables[i + context->syllable_pos];
        parsed_syllables[i].tone = TONE_0;
    }

    //对context->default_hz_syllables的至少前remain_syllable_len个音节进行重解析(逆向解析)，
    //如果解析结果包含在context->syllables中，则当前位置就是光标在default_hz_syllables中的
    //对应位置
    new_syllable_index = -1;
    for (i = remain_syllable_len; i <= remain_syllable_len + syllable_diff; i++)
    {
        //获取i以前的音节对应的拼音
        pos = 0;
        for (j = 0; j < i; j++)
        {
            //排除声调可能引起的干扰
            current_syllable = context->default_hz_syllables[j];
            current_syllable.tone = TONE_0;

            pos += GetSyllableString(
                        current_syllable,
                        checked_syllable_string + pos,
                        MAX_SYLLABLE_PER_INPUT * MAX_PINYIN_LENGTH + 1 - pos,
                        0);
        }

        //重新解析这些拼音
        fuzzy_mode = pim_config->use_fuzzy ? pim_config->fuzzy_mode : 0;
        if (pim_config->ci_option & CI_AUTO_FUZZY)
            fuzzy_mode |= FUZZY_ZCS_IN_CI;

        //由于智能组词结果中不含韵母首字母模糊(见SaveCiOption、CI_AUTO_VOW_FUZZY、
        //FUZZY_SUPER，这里为了避免潜在的问题，暂时不考虑韵母首字母模糊)
        //if (pim_config->ci_option & CI_AUTO_VOW_FUZZY)
        //	fuzzy_mode |= FUZZY_SUPER;

        checked_syllable_len =
                ParsePinYinStringReverse(
                    checked_syllable_string,
                    reparsed_syllables,
                    MAX_SYLLABLE_PER_INPUT,
                    fuzzy_mode,
                    PINYIN_QUANPIN);

        if (!has_star)
        {
            if (checked_syllable_len == remain_syllable_len)
            {
                if (CompareSyllables(parsed_syllables, reparsed_syllables, checked_syllable_len, fuzzy_mode))
                {
                    new_syllable_index = i + utf16_strlen(context->selected_compose_string);
                    return new_syllable_index;
                }
            }
        }
        else
        {
            //对于有通配符的，要找最大匹配的情况，例如:
            //先造一个词"西安市人民银行"，输入xian*|yinhang，此时default_hz为"西安市人民银行",
            //即"西安市""西安市人""西安市人民"的音节均可匹配xian*，必须选最长的"西安市人民"
            if (WildCompareSyllables(parsed_syllables, remain_syllable_len, reparsed_syllables, checked_syllable_len, fuzzy_mode))
                new_syllable_index = i + utf16_strlen(context->selected_compose_string);
        }
    }

    if (has_star && new_syllable_index >= 0)
        return new_syllable_index;

    //若未转化成功，仍然返回原来的值
    return syllable_index;
}

/**	项输入串中删除字符
*/
int BackspaceChar(PIMCONTEXT *context)
{
    int i;

    if (!context->cursor_pos)		//打头的时候，不需要后退
        return 0;

    if (context->state == STATE_VINPUT && context->input_length > 1 && context->cursor_pos == 1)
        return 0;

    //依次向右移动一个字符
    for (i = context->cursor_pos - 1; i < context->input_length; i++)
        context->input_string[i] = context->input_string[i + 1];

    context->cursor_pos--;
    context->input_length--;
    context->candidate_index  = 0;
    context->last_dot		  = 0;
    context->next_to_last_dot = 0;

    if (context->cursor_pos == context->input_pos)		//与前面已经选择的重合
    {
        if (context->selected_item_count)
        {
            context->syllable_pos = context->selected_items[context->selected_item_count - 1].syllable_start_pos;
            context->selected_item_count--;
            context->input_pos = GetInputPos(context);
        }
    }
    if (context->input_length){
        ProcessContext(context);
        return 1;
    }else{
        ResetContext(context);	//输入串为空时，重置上下文
        return 0;
    }
}
/**	回退一个候选选择ABC模式
 *	如果已经没有选择过的候选，返回0，否则返回1
 */
int BackSelectedCandidate(PIMCONTEXT *context)
{
    if (!context->selected_item_count)
        return 0;

    context->selected_item_count--;
    context->syllable_pos -= context->selected_items[context->selected_item_count].syllable_length;
    context->input_pos = GetInputPos(context);

    ProcessContext(context);
    return 1;
}
void MoveCursorRight(PIMCONTEXT *context)
{
    if (context->cursor_pos < context->input_length)
    {

            context->cursor_pos++;


    }else{
        if(context->state == STATE_VINPUT){
            context->cursor_pos = 1;
        }else{
            context->cursor_pos = 0;
        }

        if (context->selected_item_count)
        {
            context->syllable_pos = 0;
            context->selected_item_count = 0;
            context->input_pos = GetInputPos(context);
        }
    }
    ProcessContext(context);
}
/**	向左移动光标
 *	这函数是字符为单位移动，不是以音节为单位移动的，有分割符的，连上分割符当做一个字符
 */
void MoveCursorLeft(PIMCONTEXT *context)
{
    if (context->cursor_pos)
    {
        if (context->state == STATE_VINPUT && context->input_length > 0 && 1 == context->cursor_pos)
        {
            context->cursor_pos = context->input_length;
        }
        else
            context->cursor_pos--;
    }
    else
        context->cursor_pos = context->input_length;
    if (context->cursor_pos == context->input_pos)		//与前面已经选择的重合，input_pos是开始输入的位置，不是输入结束的位置
    {
        if (context->selected_item_count)
        {
            context->syllable_pos = context->selected_items[context->selected_item_count - 1].syllable_start_pos;
            context->selected_item_count--;
            context->input_pos = GetInputPos(context);
            context->modify_flag |= MODIFY_COMPOSE;
        }
    }

    ProcessContext(context);
}
void MoveCursorLeftBySyllable(PIMCONTEXT *context)
{
    if (context->cursor_pos && (context->cursor_pos == context->input_pos)){    //与前面已经选择的重合，input_pos是开始输入的位置，不是输入结束的位置
        if (context->selected_item_count)
        {
            context->syllable_pos = context->selected_items[context->selected_item_count - 1].syllable_start_pos;
            context->selected_item_count--;
            context->input_pos = GetInputPos(context);
            context->modify_flag |= MODIFY_COMPOSE;
        }
    }else if (context->cursor_pos){

        int pos = 0;
        for(int i=0; i<context->syllable_count; i++){
            TCHAR pinyin[0x10];
            int last_pos;
            int py_len;
            if(pim_config->pinyin_mode == PINYIN_SHUANGPIN)
                py_len = GetSyllableStringSP(context->syllables[i], pinyin, _SizeOf(pinyin));
            else
                py_len = GetSyllableString(context->syllables[i], pinyin, _SizeOf(pinyin), 0);
            last_pos = pos;
            pos += py_len;
            if (context->input_string[pos] == SYLLABLE_SEPARATOR_CHAR)
                pos++;
            if(pos >= context->cursor_pos){
                context->cursor_pos = last_pos;
                break;
            }
        }

    }
    else
        context->cursor_pos = context->input_length;
    ProcessContext(context);
}
/**	获得当前上下文的候选信息
 */
void MakeCandidate(PIMCONTEXT *context)
{
    int i, compose_cursor_index, cursor_pos, candidate_count = 0;

    if (context->state == STATE_IEDIT)
    {
        int syllable_count  = context->iedit_syllable_index == context->syllable_count ?
                    context->syllable_count : context->syllable_count - context->iedit_syllable_index;
        SYLLABLE *syllables = context->iedit_syllable_index == context->syllable_count ?
                    context->syllables : context->syllables + context->iedit_syllable_index;

        context->candidate_count =
                GetCandidates(context, 0, syllables, syllable_count, context->candidate_array, MAX_CANDIDATES, 0);

        ProcessCandidateStuff(context);
        return;
    }

    //***当光标位于拼音串中间(而非首位)时，第一个候选项应该是所有尚未转化为汉字的音节得出的候选项，从第二个候选项
    //开始才是光标之后的音节得出的候选项，第一个候选项的汉字通常被存储在context->syllable_hz中，来处理光标位于拼音
    //串中间时的选择问题
    if (context->syllable_mode && context->compose_cursor_index && context->compose_cursor_index < context->compose_length)
    {
        //希望能给所有尚未转化为汉字的拼音找一个汉字串作为第一个候选
        //(默认的候选结果)，但context->compose_cursor_index和context->cursor_pos
        //会影响这一逻辑(见GetCandidates代码，例如光标(context->compose_cursor_index
        //)位于拼音串中间时，候选的结果通常应该是光标之后的音节对应的
        //词(而不是所有未转化为汉字的音节对应的词))，所以这里先将二者
        //置0，获得候选后再恢复
        //例：不将context->cursor_pos置0
        //输入zi'guang'h'w'h'q，本来默认的候选为"紫光海外华侨"，按left
        //键4次后变为"海外华侨"

        //保存之前的context->compose_cursor_index
        compose_cursor_index		  = context->compose_cursor_index;
        context->compose_cursor_index = 0;

        //保存之前的context->cursor_pos
        cursor_pos          = context->cursor_pos;
        context->cursor_pos = 0;

        context->candidate_count =
                GetCandidates(context,
                              context->input_string + context->input_pos,
                              context->syllables + context->syllable_pos,
                              context->syllable_count - context->syllable_pos,
                              context->candidate_array,
                              MAX_CANDIDATES,
                              !context->syllable_pos);

        if (context->candidate_count)
            candidate_count = context->candidate_count = 1;

        //恢复之前的context->compose_cursor_index
        context->compose_cursor_index = compose_cursor_index;

        //恢复之前的context->cursor_pos
        context->cursor_pos = cursor_pos;
    }

    //获得候选
    context->candidate_count = candidate_count +
            GetCandidates(context,
                          //如果已经有输入，则不应该读取首字母输入
                          context->input_string + context->input_pos,
                          context->syllables + context->syllable_pos,
                          context->syllable_count - context->syllable_pos,
                          context->candidate_array + candidate_count,
                          MAX_CANDIDATES - candidate_count,
                          !context->syllable_pos);
    //更新默认汉字串
    //if (context->candidate_count &&
    //(0 == context->compose_cursor_index || context->compose_length == context->compose_cursor_index ||
    //(context->syllable_count != (int)_tcslen(context->syllable_hz))))
    //    GetCandidateString(context, &context->candidate_array[0], context->syllable_hz, _SizeOf(context->syllable_hz));
    //上面代码废弃的原因：1.默认汉字串的意义应该是为所有尚未转化为汉字的音节指定一个默认的候选结果，由于移动光标
    //并不会造成任何汉字转化行为，因此context->syllable_hz应该与光标无关；2.context->syllable_hz应该尽量与尚未转
    //化为汉字的音节数保持一致，且是最新的
    if (context->candidate_count)
    {
        TCHAR candidate_hz[MAX_SYLLABLE_PER_INPUT + 0x10] = {0};
        GetCandidateString(context, &context->candidate_array[0], candidate_hz, _SizeOf(candidate_hz));

        if ((int)utf16_strlen(candidate_hz) >= context->syllable_count - context->syllable_pos)
        {
            _tcscpy_s((TCHAR*)context->default_hz, _SizeOf(context->default_hz), candidate_hz);
            GetCandidateSyllable(&context->candidate_array[0], context->default_hz_syllables, MAX_SYLLABLE_PER_INPUT + 0x10);
        }
    }

    //处理第一条候选和第二条候选相等的情况，应该是为了避免和上面的***处存在重复
    if (candidate_count && context->candidate_count > 1)
    {
        TCHAR cand_str1[MAX_SPW_LENGTH + 2], cand_str2[MAX_SPW_LENGTH + 2];
        int is_same;

        GetCandidateString(context, &context->candidate_array[0], cand_str1, MAX_SPW_LENGTH);
        GetCandidateString(context, &context->candidate_array[1], cand_str2, MAX_SPW_LENGTH);

        is_same = !utf16_strcmp(cand_str1, cand_str2);
        if (!is_same)
        {
            int len1 = (int)utf16_strlen(cand_str1);
            int len2 = (int)utf16_strlen(cand_str2);

            if (len1 > len2)
                is_same = !utf16_strcmp(cand_str1 + len1 - len2, cand_str2);
        }

        if (is_same)
        {
            for (i = 1; i < context->candidate_count - 1; i++)
                context->candidate_array[i] = context->candidate_array[i + 1];

            context->candidate_count--;
        }
    }

    //获取SPW的提示信息
    context->spw_hint_string[0] = 0;
    if (!context->syllable_pos && context->english_state == ENGLISH_STATE_NONE)
    {
        const TCHAR *spw_hint;
        if (spw_hint = GetSPWHintString(context->input_string)){

            _tcscpy_s(context->spw_hint_string, _SizeOf(context->spw_hint_string), spw_hint);
        }
    }

    if (context->candidate_index >= context->candidate_count)
        context->candidate_index = 0;

    context->candidate_selected_index = 0;

    //处理候选准备事务
    ProcessCandidateStuff(context);
}

/**	处理候选字符串，当前候选数目等事务
 */
void ProcessCandidateStuff(PIMCONTEXT *context)
{
    int i;

    //SetCandidatesViewMode(context);

    if (VIEW_MODE_EXPAND == context->candidates_view_mode)
        context->candidate_page_count = pim_config->candidates_per_line * GetExpandCandidateLine();
    else
        context->candidate_page_count = pim_config->candidates_per_line;

    //如果超过边界
    if (context->candidate_index + context->candidate_page_count > context->candidate_count)
        context->candidate_page_count = context->candidate_count - context->candidate_index;

    //获得候选字符串
    for (i = 0; i < context->candidate_page_count; i++)
    {
        GetCandidateDisplayString(context,
                                  &context->candidate_array[context->candidate_index + i],
                context->candidate_string[i],
                MAX_CANDIDATE_STRING_LENGTH,
                i == 0 && context->candidate_index);

        context->candidate_trans_string[i][0] = 0;

        if (context->english_state != ENGLISH_STATE_NONE && pim_config->use_english_input && pim_config->use_english_translate &&
                context->candidate_array[context->candidate_index + i].spw.type == SPW_STIRNG_ENGLISH)
        {
            TCHAR* trans_str = GetEnglishTranslation(context->candidate_string[i]);

            if (trans_str)
                _tcscpy_s(context->candidate_trans_string[i], MAX_TRANSLATE_STRING_LENGTH, trans_str);
        }
    }

    context->modify_flag |= MODIFY_COMPOSE;
}
/**	处理候选下翻页
 *	如果已经在最后一页，再执行返回0，否则返回1
 */
int NextCandidatePage(PIMCONTEXT *context)
{
    int old_index = context->candidate_index;
    int is_end = 0;
    if (VIEW_MODE_EXPAND == context->candidates_view_mode)
        context->candidate_index += pim_config->candidates_per_line * GetExpandCandidateLine();
    else
        context->candidate_index += pim_config->candidates_per_line;

    if (context->candidate_index >= context->candidate_count){
        context->candidate_index = old_index;
        is_end = 1;
    }

    if (old_index != context->candidate_index)
    {
        context->selected_digital = 0;
        context->candidate_selected_index = 0;
        ProcessCandidateStuff(context);
    }
    return !is_end;
}

/**	处理候选上翻页
 *	如果已经在第一页，再执行会返回0，否则返回1
 */
int PrevCandidatePage(PIMCONTEXT *context)
{
    int old_index = context->candidate_index;
    int is_start = 0;
    if (VIEW_MODE_EXPAND == context->candidates_view_mode)
        context->candidate_index -= pim_config->candidates_per_line * GetExpandCandidateLine();
    else
        context->candidate_index -= pim_config->candidates_per_line;

    if (context->candidate_index < 0){
        is_start = 1;
        context->candidate_index = 0;
    }

    if (old_index != context->candidate_index)
    {
        context->selected_digital = 0;
        context->candidate_selected_index = 0;
        ProcessCandidateStuff(context);
    }
    return !is_start;
}

/**	向下一行候选
 */
void NextCandidateLine(PIMCONTEXT *context)
{
    int index = context->candidate_selected_index + pim_config->candidates_per_line;

    //在本页中
    if (index < context->candidate_page_count)
    {
        context->selected_digital = 0;
        context->candidate_selected_index = index;
        context->modify_flag |= MODIFY_COMPOSE;
        return;
    }

    //如果为最后一页，则不处理
    if (context->candidate_index + context->candidate_page_count == context->candidate_count)
        return;

    //翻到下一页，并且设置为0
    NextCandidatePage(context);

    index = index % pim_config->candidates_per_line;
    if (index < context->candidate_page_count)
        context->candidate_selected_index = index;
    else
        context->candidate_selected_index = 0;
}

/**	向上滚动一行
 */
void PrevCandidateLine(PIMCONTEXT *context)
{
    int index = context->candidate_selected_index;

    //在本页中
    if (index >= pim_config->candidates_per_line)
    {
        context->selected_digital = 0;
        context->candidate_selected_index = index - pim_config->candidates_per_line;
        context->modify_flag |= MODIFY_COMPOSE;
        return;
    }

    //如果为第一页，则不处理
    if (!context->candidate_index)
        return;

    //翻到上一页，并且设置为最后
    PrevCandidatePage(context);
    context->candidate_selected_index = index;
}

/**	处理候选下移一个
 */
void NextCandidateItem(PIMCONTEXT *context)
{
    //最后一个需要进行翻页
    if (context->candidate_selected_index >= context->candidate_page_count - 1)
    {
        //不是最后一页的话进行翻页
        if (context->candidate_index + context->candidate_page_count >= context->candidate_count)
            return;

        NextCandidatePage(context);
        context->candidate_selected_index = 0;

        return;
    }

    context->selected_digital = 0;
    context->candidate_selected_index++;
    context->modify_flag |= MODIFY_COMPOSE;
}

/**	处理候选最后一个
 */
void EndCandidateItem(PIMCONTEXT *context)
{
    int page_index;

    if (!context->candidate_count)
        return;

    if (VIEW_MODE_EXPAND == context->candidates_view_mode)
        page_index = context->candidate_count - pim_config->candidates_per_line * GetExpandCandidateLine();
    else
        page_index = context->candidate_count - pim_config->candidates_per_line;

    if (page_index < 0)
        page_index = 0;

    context->candidate_page_count		= context->candidate_count - page_index;
    context->candidate_selected_index	= context->candidate_page_count - 1;
    context->candidate_index			= page_index;
    context->selected_digital			= 0;

    ProcessCandidateStuff(context);
    context->modify_flag |= MODIFY_COMPOSE;
}

/**	处理候选上移一个
 */
void PrevCandidateItem(PIMCONTEXT *context)
{
    int cand_pos = context->candidate_index + context->candidate_selected_index;

    if (!context->candidate_selected_index)
    {
        if (!context->candidate_index)		//第一页不进行翻页
            return;

        PrevCandidatePage(context);
        context->candidate_selected_index = context->candidate_page_count - 1;
        return;
    }

    context->selected_digital = 0;
    context->candidate_selected_index--;
    context->modify_flag |= MODIFY_COMPOSE;
}

/**	处理候选到第一个
 */
void HomeCandidateItem(PIMCONTEXT *context)
{
    int cand_pos = context->candidate_index + context->candidate_selected_index;

    context->candidate_index		  = 0;
    context->candidate_selected_index = 0;
    context->selected_digital		  = 0;

    ProcessCandidateStuff(context);
    context->modify_flag |= MODIFY_COMPOSE;
}

/**	回车上屏
 *	参数：
 *		is_space		是否为按下空格
 */
void SelectInputString(PIMCONTEXT *context, int is_space)
{
    TCHAR fullshape_string[MAX_RESULT_LENGTH] = {0};

    //英文输入模式
    if (context->english_state != ENGLISH_STATE_NONE)
    {
        utf16_strncpy(context->result_string, context->input_string, MAX_RESULT_LENGTH);
        if (pim_config->space_after_english_word)
            _tcscat_s(context->result_string, MAX_RESULT_LENGTH, TEXT(" "));
    }
    //在VINPUT模式下，没有输入则返回一个v
    else if (context->state == STATE_VINPUT)
    {
        if (is_space || pim_config->input_space_after_v)
        {
            TCHAR *v_input_string;

            if (pim_config->v_mode_enabled && (context->input_length > 1))
                v_input_string = context->input_string + 1;
            else
                v_input_string = context->input_string;

            //全角模式
            if (!(pim_config->hz_option & HZ_SYMBOL_HALFSHAPE)){
                GetFullShapes(v_input_string, fullshape_string, _SizeOf(fullshape_string));
            }

            if (!(pim_config->hz_option & HZ_SYMBOL_HALFSHAPE) && fullshape_string[0])
                _tcscpy_s(context->result_string, _SizeOf(context->result_string), fullshape_string);
            else if (context->input_length > 1)
                _tcscpy_s(context->result_string, _SizeOf(context->result_string), v_input_string);
            else
                _tcscpy_s(context->result_string, _SizeOf(context->result_string), v_input_string);
        }
        else
        {
            //全角模式
            if (!(pim_config->hz_option & HZ_SYMBOL_HALFSHAPE))
                GetFullShapes(context->input_string, fullshape_string, _SizeOf(fullshape_string));

            if (!(pim_config->hz_option & HZ_SYMBOL_HALFSHAPE) && fullshape_string[0])
                _tcscat_s(context->result_string, _SizeOf(context->result_string), fullshape_string);
            else
                _tcscat_s(context->result_string, _SizeOf(context->result_string), context->input_string);
        }
    }
    else if (context->state == STATE_ILLEGAL || context->state == STATE_IINPUT || context->state == STATE_UINPUT)
    {
        //全角模式
        if (!(pim_config->hz_option & HZ_SYMBOL_HALFSHAPE))
            GetFullShapes(context->compose_string, fullshape_string, _SizeOf(fullshape_string));

        if (!(pim_config->hz_option & HZ_SYMBOL_HALFSHAPE) && fullshape_string[0])
            utf16_strncpy(context->result_string, fullshape_string, MAX_RESULT_LENGTH);
        else
            utf16_strncpy(context->result_string, context->compose_string, MAX_RESULT_LENGTH);
    }
    else
    {
        //全角模式
        if (!(pim_config->hz_option & HZ_SYMBOL_HALFSHAPE))
            GetFullShapes(context->input_string, fullshape_string, _SizeOf(fullshape_string));

        if (!(pim_config->hz_option & HZ_SYMBOL_HALFSHAPE) && fullshape_string[0])
            utf16_strncpy(context->result_string, fullshape_string, MAX_RESULT_LENGTH);
        else
            utf16_strncpy(context->result_string, context->input_string, MAX_RESULT_LENGTH);
    }

    context->result_length		   = (int)utf16_strlen(context->result_string);
    context->result_syllable_count = 0;
    context->modify_flag		  |= MODIFY_RESULT;
    context->state				   = STATE_RESULT;
    context->selected_digital	   = 0;

    return;
}

/**	以当前光标位置，确定所处于的音节
 */
int GetSyllableIndexByPosition(PIMCONTEXT *context, int pos)
{
    int i;

    for (i = 1; i < context->syllable_count; i++)
        if (context->syllable_start_pos[i] >= pos)
            break;

    return i - 1;
}
/**	向后前进一个音节的长度
 */
void NextSyllable(PIMCONTEXT *context)
{
    //英文输入、候选状态
    if (context->english_state != ENGLISH_STATE_NONE)
    {
        if (context->cursor_pos < context->input_length)
            context->cursor_pos++;
        else
            context->cursor_pos = context->input_length;
    }
    else
    {
        int index = GetSyllableIndexByPosition(context, context->cursor_pos + 1);
        if (index == context->syllable_count - 1)
        {
            if (context->cursor_pos != context->input_length)
                context->cursor_pos = context->input_length;
            else
            {	//清空全部的选择
                context->cursor_pos			 = 0;
                context->selected_item_count = 0;
                context->input_pos			 = 0;
                context->syllable_pos		 = 0;
            }
        }
        else
            context->cursor_pos = context->syllable_start_pos[index + 1];
    }

    ProcessContext(context);
}
