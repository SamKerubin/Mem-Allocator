#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include <stddef.h>

void *m_alloc(size_t size);
void m_free(void *ptr);

void *malloc(size_t size);
void free(void *ptr);

#endif // MEM_ALLOC_H
