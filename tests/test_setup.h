#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void failed(int signo) {
    (void)signo;
    printf("Yes! It failed!\n");
    exit(1);
}

void setup() {
    struct sigaction sa;

    sa.sa_handler = failed;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGSEGV, &sa, NULL);
}

