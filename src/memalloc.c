#include <memalloc.h>

void *m_alloc(size_t size) {
    (void)size;
    return NULL;
}

void m_free(void *ptr) {
    (void)ptr;
}
