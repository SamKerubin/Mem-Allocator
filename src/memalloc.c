#include <memalloc.h>
#include <sys/mman.h>
#include <unistd.h>

#define MEM_FAIL            ((void *)-1)

#if defined(__x86_64__)
    #define ALIGN_SIZE(X)   (((X) + 0xF) & ~0xF)
#else
    #define ALIGN_SIZE(X)   (((X) + 0x7) & ~0x7)
#endif

#define OCCUPANCY_FLAG      (1 << 0)
#define MMAP_ALLOC_FLAG     (1 << 1)
#define SIZE_FLAGS          OCCUPANCY_FLAG | MMAP_ALLOC_FLAG

#define GET_REAL_SIZE(sz)   ((sz) & ~(SIZE_FLAGS))
#define IS_BLOCK_FREE(b)    ((b)->size & OCCUPANCY_FLAG)
#define IS_MMAP_ALLOC(sz)   ((sz) & MMAP_ALLOC_FLAG)

#define MMAP_ALLOC_THRESHOLD 131072 // 128kb max before switching to mmap allocations

struct M_header {
    size_t size;

    M_header *prev;
    M_header *next;
};

struct M_footer {
    size_t size_copy;
};

M_header *head, *tail;

void *m_alloc(size_t size) {
    (void)size;
    return NULL;
}

void m_free(void *ptr) {
    (void)ptr;
}
