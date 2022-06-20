#ifndef OS_MEMORY_H
#define OS_MEMORY_H

#include "config.h"

void init_memory(void);
void *malloc(int size);
int free(void *point);

#endif //OS_MEMORY_H
