#ifndef	_PIM_STATE_H_
#define	_PIM_STATE_H_

#include <context.h>

#ifdef __cplusplus
extern "C" {
#endif

//extern int IsNeedKey(PIMCONTEXT *context, int virtual_key, uintptr_t scan_code, byte key_state);
extern void ProcessKey(PIMCONTEXT *context, TCHAR ch);

#ifdef __cplusplus
}
#endif

#endif
