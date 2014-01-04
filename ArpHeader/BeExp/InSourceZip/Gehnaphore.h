#ifndef	_GEHNAPHORE_H
#define	_GEHNAPHORE_H

#include <BeBuild.h>
#include <OS.h>
#include <SupportDefs.h>

void init_gehnaphore(int32* value);
void lock_gehnaphore(int32* value);
void unlock_gehnaphore(int32* value);

#endif /* _GEHNAPHORE_H */
