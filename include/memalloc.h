#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include <stddef.h>

void *m_alloc(size_t size);
void m_free(void *ptr);
void *m_calloc(size_t nmemb, size_t size);
void *m_realloc(void *ptr, size_t size);

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);

#endif // MEM_ALLOC_H
