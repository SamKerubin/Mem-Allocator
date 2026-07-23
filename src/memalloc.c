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

#define GET_REAL_SIZE(b)    ((b)->size & ~(SIZE_FLAGS))
#define IS_BLOCK_FREE(b)    ((b)->size & OCCUPANCY_FLAG)
#define IS_MMAP_ALLOC(b)    ((b)->size & MMAP_ALLOC_FLAG)

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

static M_header *find_free_block(size_t size) {
    M_header *curr = head;

    while (curr) {
        // First fit
        if (IS_BLOCK_FREE(curr) && GET_REAL_SIZE(curr) >= size) {
            // TODO: Handle block splitting
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void *m_alloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    void *block;
    M_header *header;
    size_t total_size;

    if (size >= MMAP_ALLOC_THRESHOLD) {
        // Alloc using mmap
        return NULL;
    }

    total_size = sizeof(size_t) + ALIGN_SIZE(size) + sizeof(M_footer);

    header = find_free_block(total_size);
    if (header) {
        header->size &= ~(SIZE_FLAGS);
        return (void *)(header + 1);
    }

    block = sbrk(total_size);
    if (block == MEM_FAIL) {
        return NULL;
    }

    header = block;
    header->size = total_size | OCCUPANCY_FLAG;

    if (!head) {
        head = header;
    }
    if (tail) {
        tail->next = header;
    }
    tail = header;

    return (void *)(header + 1);
}

void m_free(void *ptr) {
    (void)ptr;
}
