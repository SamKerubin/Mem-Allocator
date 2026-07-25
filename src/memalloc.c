#include <memalloc.h>
#include <sys/mman.h>
#include <unistd.h>

#define MEM_FAIL            ((void *)-1)

#if defined(__x86_64__)
    #define MIN_SPLIT_BYTES 32
    #define ALIGN_SIZE(X)   (((X) + 0xF) & ~0xF)
#else
    #define MINMIN_SPLIT_BYTES 16
    #define ALIGN_SIZE(X)   (((X) + 0x7) & ~0x7)
#endif // __x86_64__

#define CURR_IN_USE_FLAG    (1 << 0)
#define MMAP_ALLOC_FLAG     (1 << 1)
#define PREV_IN_USE_FLAG    (1 << 2)
#define SIZE_FLAGS          (CURR_IN_USE_FLAG | MMAP_ALLOC_FLAG | PREV_IN_USE_FLAG)

#define GET_REAL_SIZE(b)    ((b)->size & ~(SIZE_FLAGS))
#define IS_BLOCK_FREE(b)    (((b)->size & CURR_IN_USE_FLAG) == 0)
#define IS_PREV_FREE(b)     (((b)->size & PREV_IN_USE_FLAG) == 0)
#define IS_MMAP_ALLOC(b)    (((b)->size & MMAP_ALLOC_FLAG) != 0)

#define MMAP_ALLOC_THRESHOLD 131072 // 128kb max before switching to mmap allocations

typedef struct M_header M_header;

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
        ssize_t diff = GET_REAL_SIZE(curr) - size;
        if (diff < 0) {
            curr = curr->next;
            continue;
        }

        // Depending on the architecture of the CPU, blocks need at least 32/16 bytes to be able to split
        // The important part of the header is 16/8 bytes, but the payload needs to be aligned to 16/8 bytes as well
        // -- it casually fits the 2 pointers of the free list too huh? really convenient

        if (diff < MIN_SPLIT_BYTES) {
            return curr;
        }

        if (diff >= MIN_SPLIT_BYTES) {
            curr->size = size | (curr->size & SIZE_FLAGS);

            M_header *splitted_h = (M_header *)((char *)curr + size);
            splitted_h->size = (size_t)diff;

            splitted_h->next = head;
            splitted_h->prev = NULL;
            head->prev = splitted_h;
            head = splitted_h;

            return curr;
        }
    }
    return NULL;
}

static void remove_block_from_free_list(M_header *h) {
    if (h->prev) {
        h->prev->next = h->next;
    } else {
        head = h->next;
    }

    if (h->next) {
        h->next->prev = h->prev;
    } else {
        tail = h->prev;
    }
}

void *m_alloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    void *block;
    M_header *header;
    size_t aligned_size = ALIGN_SIZE(size);
 
            // Size of the header that -
            // matters, ignore the pointers -
            // if the block is being used
    size_t total_size = (sizeof(size_t) * 2) + aligned_size;

    if (size >= MMAP_ALLOC_THRESHOLD) {
        block = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (block == MAP_FAILED) {
            return NULL;
        }

        header = block;
        header->size = total_size | MMAP_ALLOC_FLAG;

        return (void *)((size_t *)header + 2);
    }

    header = find_free_block(aligned_size);
    if (header) {
        remove_block_from_free_list(header);

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

    M_header *header = (M_header *)((size_t *)ptr - 2);
    if (IS_MMAP_ALLOC(header)) {
        munmap(header, GET_REAL_SIZE(header));
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

            remove_block_from_free_list(prev_h);
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

        remove_block_from_free_list(next_h);
    } else {
        next_h->prev_size = GET_REAL_SIZE(header);
    }

    header->next = head;
    header->prev = NULL;
    head->prev = header;
    head = header;
}
