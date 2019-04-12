#ifndef _DEBUG_H_
#define _DEBUG_H_ 
#include <string.h>
#include <stdio.h>
#include <utf16char.h>
#ifdef __cplusplus
extern "C" {
#endif 
#define DEBUG_ECHO(fmt, arg...) {printf("=====[File: %s Func: %s Line: %d]=====\n", strrchr(__FILE__, '/'), __FUNCTION__, __LINE__);printf(fmt, ##arg);printf("\n\n");}
#define U16_DEBUG_ECHO(fmt, arg...) {printf("=====[File: %s Func: %s Line: %d]=====\n", strrchr(__FILE__, '/'), __FUNCTION__, __LINE__);utf16_minfprintf(stdout, u##fmt, ##arg);printf("\n\n");}
#ifdef __cplusplus
}
#endif 
#endif 
