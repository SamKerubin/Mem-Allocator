#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include <stddef.h>

struct M_header;
typedef struct M_header M_header;

void *m_alloc(size_t size);
void m_free(void *ptr);

#endif // MEM_ALLOC_H
