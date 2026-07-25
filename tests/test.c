#include <memalloc.h>
#include <stdio.h>

void use_ptr(size_t *ptr) {
    *ptr = 10;
}

int main() {    
    size_t *ptr = m_alloc(sizeof(size_t));
    *ptr = 240000;
    printf("%ld\n", *ptr);
    use_ptr(ptr);
    printf("%ld\n", *ptr);

    int *arr = m_alloc(10 * sizeof(int));
    for (int i = 0; i < 10; i++) {
        arr[i] = i + 1;
    }

    for (int i = 0; i < 10; i++) {
        printf("%d: %d\n", i + 1, arr[i]);
    }

    int *mmap_alloc = m_alloc(sizeof(int) * (128 * 1024));

    int *alloc1 = m_alloc(32 * sizeof(int));
    int *alloc2 = m_alloc(sizeof(int));
    int *alloc3 = m_alloc(16 * sizeof(int));
    int *alloc4 = m_alloc(60 * sizeof(int));
    int *alloc5 = m_alloc(32 * sizeof(int));
    int *alloc6 = m_alloc(128 * sizeof(int));
    int *alloc7 = m_alloc(40 * sizeof(int));
    int *alloc8 = m_alloc(50 * sizeof(int));
    int *alloc9 = m_alloc(10 * sizeof(int));
    int *alloc10 = m_alloc(5 * sizeof(int));
    int *alloc11 = m_alloc(3 * sizeof(int));
    int *alloc12 = m_alloc(200 * sizeof(int));

    m_free(alloc1);
    m_free(alloc2);
    m_free(alloc3);
    m_free(alloc4);
    m_free(alloc5);
    m_free(alloc6);
    m_free(alloc7);
    m_free(alloc8);
    m_free(alloc9);
    m_free(alloc10);
    m_free(alloc11);
    m_free(alloc12);
    m_free(mmap_alloc);
    m_free(arr);
    m_free(ptr);

    printf("Passed!\n");

    return 0;
}
