#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "thread.h"

/**
 * Paramètres de la fonction `sum`
 */
typedef struct list {
    unsigned long *list;  // pointeur vers une liste
    unsigned long size;   // taille de la liste
} list_t;

/**
 * Effectue la somme d'une liste en la découpant
 * en 2 sous-listes sur lesquelles on applique une récursivité.
 *
 * @param funcargs la list_t à sommer
 * @return la somme de la liste
 */
static void *merge_sum(void *funcargs) {
    list_t *args = (list_t *)funcargs;
    thread_t th1, th2;
    int err;
    unsigned long *res1 = NULL, *res2 = NULL;

    /* On passe un peu la main aux autres pour éviter de faire uniquement la
     * partie gauche de l'arbre */
    thread_yield();

    if (args->size < 2) {
        res1 = malloc(sizeof(*res1));
        *res1 = args->list[0];
        return res1;
    }

    long int middle = args->size / 2;
    list_t funcargs1 = {.list = args->list, .size = middle};
    list_t funcargs2 = {
        .list = args->list + middle,
        .size = args->size - middle,
    };

    err = thread_create(&th1, merge_sum, &funcargs1);
    assert(!err);
    err = thread_create(&th2, merge_sum, &funcargs2);
    assert(!err);

    err = thread_join(th1, (void *)&res1);
    assert(!err);
    err = thread_join(th2, (void *)&res2);
    assert(!err);

    *res1 += *res2;
    free(res2);

    return res1;
}

/**
 * Remplit la liste avec des valeurs aléatoires.
 *
 * @param list un pointeur vers la liste à remplir
 * @param size la taille de la liste
 * @return le somme de la liste
 */
unsigned long prepare_sum_test(unsigned long *list, unsigned long size) {
    unsigned long i;
    unsigned long sum = 0;

    for (i = 0; i < size; ++i) {
        list[i] = rand() % (size + 1);
        sum += list[i];
    }

    return sum;
}

int main(int argc, char *argv[]) {
    struct timeval tv1, tv2;
    double s;

    srand(time(NULL));

    if (argc < 2) {
        printf("argument manquant : taille de la liste à sommer\n");
        return EXIT_FAILURE;
    }

    unsigned long size = atol(argv[1]);
    unsigned long list[size];
    unsigned long sum = prepare_sum_test(list, size);

    list_t funcargs = {.list = list, .size = size};

    gettimeofday(&tv1, NULL);
    unsigned long *res_ptr = merge_sum((void *)&funcargs);
    gettimeofday(&tv2, NULL);
    s = (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) * 1e-6;

    unsigned long res = *res_ptr;
    free(res_ptr);

    if (res != sum) {
        printf("(FAILED) la somme calculée est incorrecte\n");
        printf("somme retournée : %lu, somme attendue : %lu", res, sum);
        return EXIT_FAILURE;
    }

    printf("(SUCCESS) en %e s\n", s);
    return EXIT_SUCCESS;
}
