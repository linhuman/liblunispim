#ifndef	_FONTCHECK_H_
#define	_FONTCHECK_H_

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	FONTMAP_FILE_NAME			"/zi/cmap.dat"
extern int LoadFontMapData(const char *file_name);
extern int FreeFontMapData();
extern int FontCanSupport(UC zi);

#ifdef __cplusplus
}
#endif

#endif 
