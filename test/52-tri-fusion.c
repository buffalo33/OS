#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "thread.h"

/**
 * Paramètres de la fonction merge_sort
 */
typedef struct arg_struct {
    unsigned long *list;
    unsigned long size;
} arg_struct_t;

/**
 * Fusionne les différentes parties de la liste
 *
 * @param sub_list un pointeur vers le premier élément de la liste considérée.
 * @param size la taille de la sous-liste considérée pour la fusion.
 * @param middle
 */
void merge(unsigned long *sub_list, unsigned long size, unsigned long middle) {
    unsigned long i, j, k;

    /* On prépare une liste qui contiendra le tri croissant de 'sub_list' */
    unsigned long *x = malloc(size * sizeof(*x));

    for (i = 0, j = middle, k = 0; k < size; k++) {
        if (j == size) {
            x[k] = sub_list[i++];
        } else if (i == middle || sub_list[j] < sub_list[i]) {
            x[k] = sub_list[j++];
        } else {
            x[k] = sub_list[i++];
        }
    }

    for (i = 0; i < size; i++) {
        sub_list[i] = x[i];
    }
    free(x);
}

/**
 * Remplit la liste avec des valeurs aléatoires.
 *
 * @param list un pointeur vers la liste triée
 * @param size la taille de la liste
 */
static void *merge_sort(void *funcargs) {
    thread_t th, th2;
    int err;
    void *res = NULL, *res2 = NULL;
    arg_struct_t *args = (arg_struct_t *)funcargs;

    /* On passe un peu la main aux autres pour eviter de faire uniquement la
     * partie gauche de l'arbre */
    thread_yield();

    if (args->size < 2) {
        return NULL;
    }

    int middle = args->size / 2;

    arg_struct_t funcargs1, funcargs2;
    funcargs1.list = args->list;
    funcargs1.size = middle;
    funcargs2.list = args->list + middle;
    funcargs2.size = args->size - middle;

    err = thread_create(&th, merge_sort, (void *)(&funcargs1));
    assert(!err);
    err = thread_create(&th2, merge_sort, (void *)(&funcargs2));
    assert(!err);

    err = thread_join(th, &res);
    assert(!err);
    err = thread_join(th2, &res2);
    assert(!err);

    merge(args->list, args->size, middle);
    return NULL;
}

/**
 * Remplit la liste avec des valeurs aléatoires.
 *
 * @param list un pointeur vers la liste triée
 * @param size la taille de la liste
 */
void rand_fill_list(unsigned long *list, unsigned long size) {
    unsigned long i;
    for (i = 0; i < size; ++i) {
        list[i] = (rand() % (size + 1));
    }
}

/**
 * Vérifie que le tableau est trié dans l'ordre croissant.
 *
 * @param list un pointeur vers la liste triée
 * @param size la taille de la liste
 * @return 1 en cas de succès, 0 en cas d'erreur
 */
int merge_sort_checker(unsigned long *list, unsigned long size) {
    unsigned long i;
    for (i = 1; i < size; ++i) {
        if (list[i] < list[i - 1]) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    unsigned long size;
    struct timeval tv1, tv2;
    double s;

    srand(time(NULL));

    if (argc < 2) {
        printf(
            "argument manquant: entier x pour lequel déterminer la taille de "
            "la liste à trier\n");
        return EXIT_FAILURE;
    }

    size = atol(argv[1]);
    unsigned long *list = (unsigned long *)malloc(size * (sizeof(*list)));
    rand_fill_list(list, size);

    arg_struct_t funcargs;
    funcargs.list = list;
    funcargs.size = size;

    gettimeofday(&tv1, NULL);
    (void)merge_sort((void *)&funcargs);
    gettimeofday(&tv2, NULL);
    s = (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) * 1e-6;

    if (!merge_sort_checker(list, size)) {
        printf("(FAILED) : la liste n'est pas triée dans l'ordre croissant\n");
        free(list);
        return EXIT_FAILURE;
    } else {
        printf("(SUCCESS) : en %e s\n", s);
        free(list);
        return EXIT_SUCCESS;
    }
}
