#include <memalloc.h>
#include <stdio.h>

void use_ptr(size_t *ptr) {
    *ptr = 10;
}

int main() {    
    size_t *ptr = malloc(sizeof(size_t));
    *ptr = 240000;
    printf("%ld\n", *ptr);
    use_ptr(ptr);
    printf("%ld\n", *ptr);

    int *arr = malloc(10 * sizeof(int));
    for (int i = 0; i < 10; i++) {
        arr[i] = i + 1;
    }

    for (int i = 0; i < 10; i++) {
        printf("%d: %d\n", i + 1, arr[i]);
    }

    int *mmap_alloc = malloc(sizeof(int) * (128 * 1024));

    int *alloc1 = malloc(32 * sizeof(int));
    int *alloc2 = malloc(sizeof(int));
    int *alloc3 = malloc(16 * sizeof(int));
    int *alloc4 = malloc(60 * sizeof(int));
    int *alloc5 = malloc(32 * sizeof(int));
    int *alloc6 = malloc(128 * sizeof(int));
    int *alloc7 = malloc(40 * sizeof(int));
    int *alloc8 = malloc(50 * sizeof(int));
    int *alloc9 = malloc(10 * sizeof(int));
    int *alloc10 = malloc(5 * sizeof(int));
    int *alloc11 = malloc(3 * sizeof(int));
    int *alloc12 = malloc(200 * sizeof(int));

    free(alloc1);
    free(alloc2);
    free(alloc3);
    free(alloc4);
    free(alloc5);
    free(alloc6);
    free(alloc7);
    free(alloc8);
    free(alloc9);
    free(alloc10);
    free(alloc11);
    free(alloc12);
    free(mmap_alloc);
    free(arr);
    free(ptr);

    printf("Passed!\n");

    return 0;
}
