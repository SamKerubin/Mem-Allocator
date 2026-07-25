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
M_header *heap_start, *heap_end;

static M_header *find_free_block(size_t size) {
    M_header *curr = head;

    while (curr) {
        // First fit
        if (GET_REAL_SIZE(curr) >= size) {
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
        if (header == head) {
            head = head->next;
        } else if (header == tail) {
            tail = tail->prev;
        } else {
            header->prev->next = header->next;
            header->next->prev = header->prev;
        }

        if (header != heap_end) {
            M_header *next_h = (M_header *)((char *)header + GET_REAL_SIZE(header) + (sizeof(size_t) * 2));
            next_h->size |= PREV_IN_USE_FLAG;
        }

        header->size |= CURR_IN_USE_FLAG;
        return (void *)((size_t *)header + 2);
    }

    block = sbrk(total_size);
    if (block == MEM_FAIL) {
        return NULL;
    }

    header = block;
    header->size = aligned_size | CURR_IN_USE_FLAG;

    if (!heap_start) {
        heap_start = header;
        header->size |= PREV_IN_USE_FLAG;
    }

    if (heap_end) {
        if (!IS_BLOCK_FREE(heap_end)) {
            header->size |= PREV_IN_USE_FLAG;
            header->prev_size = GET_REAL_SIZE(heap_end);
        }
    }
    heap_end = header;

    return (void *)((size_t *)header + 2);
}

void m_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    M_header *header = ((M_header *)ptr - 1);
    if (IS_MMAP_ALLOC(header)) {
        // Handle mmap
        return;
    }

    header->size &= ~(CURR_IN_USE_FLAG);

    M_header *prev_h;

    if (header == heap_end) {
        size_t shrink_size = (sizeof(size_t) * 2) + GET_REAL_SIZE(header);

        prev_h = (M_header *)((char *)header - (header->prev_size + (sizeof(size_t) * 2)));

        // Merge prev
        if (IS_PREV_FREE(header)) {
            shrink_size += header->prev_size + (sizeof(size_t) * 2);

            if (prev_h != heap_start) {
                M_header *prev_prev_h = (M_header *)((char *)prev_h - (prev_h->prev_size + (sizeof(size_t) * 2)));
                heap_end = prev_prev_h;
            }

            prev_h->prev->next = prev_h->next;
            prev_h->next->prev = prev_h->prev;
        } else {
            heap_end = prev_h;
        }

        sbrk(-(intptr_t)shrink_size);
        return;
    }

    M_header *next_h = (M_header *)((char *)header + GET_REAL_SIZE(header) + (sizeof(size_t) * 2));
    next_h->size &= ~(PREV_IN_USE_FLAG);

    // Merge prev
    if (IS_PREV_FREE(header)) {
        prev_h = (M_header *)((char *)header - (header->prev_size + (sizeof(size_t) * 2)));
        size_t total_size = GET_REAL_SIZE(header) + header->prev_size + (sizeof(size_t) * 2);
        prev_h->size = total_size | (prev_h->size & SIZE_FLAGS);
        header = prev_h;
    }

    // Merge next
    if (IS_BLOCK_FREE(next_h)) {
        size_t total_size = GET_REAL_SIZE(header) + GET_REAL_SIZE(next_h) + (sizeof(size_t) * 2);
        header->size = total_size | (header->size & SIZE_FLAGS);

        M_header *next_next_h = (M_header *)((char *)next_h + GET_REAL_SIZE(next_h) + (sizeof(size_t) * 2));
        next_next_h->prev_size = GET_REAL_SIZE(header);

        header->prev = next_h->prev;
        header->next = next_h->next;
        next_h->prev->next = header;
        next_next_h->next->prev = header;
    } else {
        next_h->prev_size = GET_REAL_SIZE(header);
    }
}
