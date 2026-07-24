#include <memalloc.h>
#include <sys/mman.h>
#include <unistd.h>

#define MEM_FAIL            ((void *)-1)

#if defined(__x86_64__)
    #define ALIGN_SIZE(X)   (((X) + 0xF) & ~0xF)
#else
    #define ALIGN_SIZE(X)   (((X) + 0x7) & ~0x7)
#endif

#define CURR_IN_USE_FLAG    (1 << 0)
#define MMAP_ALLOC_FLAG     (1 << 1)
#define PREV_IN_USE_FLAG    (1 << 2)
#define SIZE_FLAGS          (CURR_IN_USE_FLAG | MMAP_ALLOC_FLAG | PREV_IN_USE_FLAG)

#define GET_REAL_SIZE(b)    ((b)->size & ~(SIZE_FLAGS))
#define IS_BLOCK_FREE(b)    (((b)->size & CURR_IN_USE_FLAG) == 0)
#define IS_PREV_FREE(b)     (((b)->size & PREV_IN_USE_FLAG) == 0)
#define IS_MMAP_ALLOC(b)    (((b)->size & MMAP_ALLOC_FLAG) != 0)

#define MMAP_ALLOC_THRESHOLD 131072 // 128kb max before switching to mmap allocations

struct M_header {
    size_t prev_size;
    size_t size;

    M_header *prev;
    M_header *next;
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
        curr = (M_header *)((char *)curr + GET_REAL_SIZE(curr) + (sizeof(size_t) * 2));
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
    size_t aligned_size = ALIGN_SIZE(size);

    if (size >= MMAP_ALLOC_THRESHOLD) {
        // Alloc using mmap
        return NULL;
    }

            // Size of the header that -
            // matters, ignore the pointers -
            // if the block is being used
    total_size = (sizeof(size_t) * 2) + aligned_size;

    header = find_free_block(aligned_size);
    if (header) {
        header->size |= CURR_IN_USE_FLAG;
        return (void *)((size_t *)header + 2);
    }

    block = sbrk(total_size);
    if (block == MEM_FAIL) {
        return NULL;
    }

    header = block;
    header->size = aligned_size | CURR_IN_USE_FLAG;

    if (!head) {
        head = header;
    }

    if (tail) {
        tail->next = header;
        header->prev_size = GET_REAL_SIZE(tail);
    } else {
        header->prev_size = 0;
    }
    tail = header;

    return (void *)((size_t *)header + 2);
}

void m_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    M_header *header = (M_header *)(ptr - 1);
    if (IS_MMAP_ALLOC(header)) {
        // Handle mmap
        return;
    }

    header->size &= ~(CURR_IN_USE_FLAG);
    header->prev = (M_header *)((char *)header - header->prev_size - (sizeof(size_t) * 2));

    void *program_brk = sbrk(0);
    size_t real_size = GET_REAL_SIZE(header);

    if ((char *)header + real_size == program_brk) {
        size_t shrink_size = (sizeof(size_t) * 2) + real_size;
        if (head == tail) {
            head = tail = NULL;
        } else {
            // TODO: Merge prev block if free (update shrink_size to match the size of the merged blocks)

            tail = header->prev;
            tail->next = NULL;
        }

        sbrk(-(intptr_t)shrink_size);
        return;
    }

    // Merge prev and/or next if they are free
    // header->next = (M_header *)((char *)header + GET_REAL_SIZE(header) + (sizeof(size_t) * 2));
}
