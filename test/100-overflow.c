#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread.h"

/* Test de débordement de la pile d'un thread */

static void* recInfinite(void* arg) {
    intptr_t res = (intptr_t)&arg + (intptr_t)recInfinite(NULL);
    return (void*)res;
}

int main() {
    thread_t th1;
    thread_create(&th1, recInfinite, NULL);

    thread_join(th1, NULL);
    return EXIT_SUCCESS;
}
