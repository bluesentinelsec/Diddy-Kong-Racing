#ifndef _THREAD30_H_
#define _THREAD30_H_

#include "types.h"

void bgload_init(void);
s32 bgload_active(void);
void bgload_tick(void);
s32 bgload_timer(void);
s32 bgload_start(s32 levelId, s32 cutsceneId);

#endif
