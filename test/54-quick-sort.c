#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "thread.h"

/**
 * Paramètres de la fonction quick_sort
 */
typedef struct list {
    long long *list;
    int first;
    int last;
} list_t;

/**
 * Choisit aléatoirement un pivot
 *
 * @param first le premier indice
 * @param last le dernier indice
 * @return l'indice du pivot
 */

int choose_pivot(int first, int last) {
    assert(last - first + 1 > 0);
    return first + (rand() % (last - first + 1));
}

/**
 * Permute deux valeurs
 *
 * @param t un pointeur vers le premier élément de la liste considérée.
 * @param first indice de la première valeur à permuter
 * @param second indice de la seconde valeur à permuter
 */
void swap(long long *t, int first, int second) {
    long long tmp = t[first];
    t[first] = t[second];
    t[second] = tmp;
}

/**
 * Partitionne la liste en deux avec :
 * de t[0] ... t[j-1] les éléments inférieur au pivot
 * de t[j] ... t[j+1] les éléments supérieurs ou égaux au pivot
 *
 * @param list un pointeur vers le premier élément de la liste considérée.
 * @param first indice de la première valeur
 * @param last indice de la dernière valeur
 * @param p indice du pivot
 * @return l'indice du pivot dans la liste
 */
int partition(long long *list, int first, int last, int p) {
    swap(list, p, last);

    int j = first;
    int i;
    for (i = first; i < last; i++) {
        if (list[i] <= list[last]) {
            swap(list, i, j);
            j++;
        }
    }

    swap(list, j, last);
    return j;
}

/**
 * Effectue l'algorithme de tri rapide
 *
 * @param funcargs la list_t à trier
 */
static void *quick_sort(void *funcargs) {
    list_t *args = (list_t *)funcargs;

    if (args->first < args->last) {
        int q = choose_pivot(args->first, args->last);
        q = partition(args->list, args->first, args->last, q);

        list_t funcargs1 = {
            .list = args->list, .first = args->first, .last = q - 1};

        list_t funcargs2 = {
            .list = args->list, .last = args->last, .first = q + 1};

        thread_t th1, th2;
        int err = thread_create(&th1, quick_sort, (void *)(&funcargs1));
        assert(!err);
        err = thread_create(&th2, quick_sort, (void *)(&funcargs2));
        assert(!err);

        void *res1 = NULL, *res2 = NULL;
        err = thread_join(th1, &res1);
        assert(!err);
        err = thread_join(th2, &res2);
        assert(!err);
    }

    return NULL;
}

/**
 * Remplit la liste avec des valeurs aléatoires.
 *
 * @param list un pointeur vers la liste triée
 * @param size la taille de la liste
 */
void rand_fill_list(long long *list, int size) {
    int i;
    for (i = 0; i < size; ++i) {
        list[i] = rand() % (size + 1);
    }
}

/**
 * Vérifie que le tableau est trié dans l'ordre croissant.
 *
 * @param list un pointeur vers la liste triée
 * @param size la taille de la liste
 * @return 1 en cas de succès, 0 en cas d'erreur
 */
int quick_sort_checker(long long *list, int size) {
    int i;
    for (i = 1; i < size; ++i) {
        if (list[i] < list[i - 1]) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    struct timeval tv1, tv2;
    double s;

    srand(time(NULL));

    if (argc < 2) {
        printf("argument manquant : taille de la liste à trier\n");
        return EXIT_FAILURE;
    }

    int size = atoi(argv[1]);
    long long list[size];
    rand_fill_list(list, size);

    list_t funcargs = {.list = list, .last = size - 1, .first = 0};

    gettimeofday(&tv1, NULL);
    quick_sort((void *)&funcargs);
    gettimeofday(&tv2, NULL);
    s = (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) * 1e-6;

    if (!quick_sort_checker(list, size)) {
        printf("(FAILED) : la liste n'est pas triée dans l'ordre croissant\n");
        return EXIT_FAILURE;
    } else {
        printf("(SUCCESS) : en %e s\n", s);
        return EXIT_SUCCESS;
    }
}
