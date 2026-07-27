#include <memalloc.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MEM_FAIL            ((void *)-1)

#if defined(__x86_64__)
    #define MIN_SPLIT_BYTES 32
    #define ALIGN_SIZE(X)   (((X) + 0xF) & ~0xF)
#else
    #define MIN_SPLIT_BYTES 16
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

static M_header *head, *tail;
static M_header *heap_start, *heap_end;

static void add_block_to_start_of_list(M_header *block) {
    block->prev = NULL;
    block->next = head;

    if (head) {
        head->prev = block;
    }

    head = block;
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

static M_header *find_free_block(size_t size) {
    M_header *curr = head;
    M_header *best = NULL;
    size_t best_diff = MMAP_ALLOC_THRESHOLD;

    while (curr) {
        // Best fit
        // First fit is easier to implement, but best fit provides less external fragmentation
        // best fit might make it less predictable? idk, but it works best imo... hehe... best... get it?
        ssize_t diff = GET_REAL_SIZE(curr) - size;
        if (diff == 0) {
            return curr;
        }
 
        // Depending on the architecture of the CPU, blocks need at least 32/16 bytes to be able to split
        // The important part of the header is 16/8 bytes, but the payload needs to be aligned to 16/8 bytes as well
        // -- it casually fits the 2 pointers of the free list too huh? really convenient       
        if (diff < MIN_SPLIT_BYTES || diff >= (ssize_t)best_diff) {
            curr = curr->next;
            continue;
        }

        best = curr;
        best_diff = diff;
    }

    if (best == NULL) {
        return NULL;
    }

    best->size = size | (best->size & SIZE_FLAGS);

    M_header *splitted_h = (M_header *)((char *)best + size + (sizeof(size_t) * 2));
    splitted_h->size = best_diff;
    splitted_h->prev_size = GET_REAL_SIZE(best);
    add_block_to_start_of_list(splitted_h);

    return best;
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
    } else {
        header->prev_size = 0;
        header->size |= PREV_IN_USE_FLAG;
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
        munmap(header, GET_REAL_SIZE(header) + (sizeof(size_t) * 2));
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
        next_h->size &= ~(PREV_IN_USE_FLAG);
    }

    add_block_to_start_of_list(header);
}

void *m_calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) {
        return NULL;
    }

    void *ptr = m_alloc(nmemb * size);
    if (ptr == NULL) {
        return NULL;
    }

    memset(ptr, 0, (nmemb * size));
    return ptr;
}

void *m_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return m_alloc(size);
    }

    if (size == 0) {
        m_free(ptr);
        return NULL;
    }

    void *new = m_alloc(size);
    if (new != NULL) {
        memcpy(new, ptr, size);
        m_free(ptr);
    }

    return new;
}

/*
 * Testing using the standard names
 * */

void *malloc(size_t size) {
    return m_alloc(size);
}

void free(void *ptr) {
    m_free(ptr);
}

void *calloc(size_t nmemb, size_t size) {
    return m_calloc(nmemb, size);
}

void *realloc(void *ptr, size_t size) {
    return m_realloc(ptr, size);
}
