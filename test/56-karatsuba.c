#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "thread.h"

/**
 * Paramètres de la fonction karatsuba
 */
typedef struct arg_struct {
    long double x;
    long double y;
} arg_struct;

/**
 * Multiplie deux nombres avec l'algorithme de Karatsuba
 *
 * @return le produit x * y
 */
static void *karatsuba(void *funcargs) {
    thread_t th1, th2, th3;
    int err;
    long double *res1 = NULL, *res2 = NULL, *res3 = NULL;

    arg_struct *args = (arg_struct *)funcargs;

    unsigned long x_len = args->x > 10 ? 1 + log10l(args->x) : 1;
    unsigned long y_len = args->y > 10 ? 1 + log10l(args->y) : 1;

    // the largest length
    unsigned long N = (y_len < x_len) ? x_len : y_len;

    // if the length is small it's faster to directly multiply
    if (N < 10) {
        res1 = malloc(sizeof(long double));
        *(long double *)res1 = args->x * args->y;
        return res1;
    }

    // max length divided and rounded up
    N = (N / 2) + N % 2;

    long double multiplier = powl(10, N);

    long double b = floorl(args->x / multiplier);
    long double a = floorl(args->x - b * multiplier);
    long double d = floorl(args->y / multiplier);
    long double c = floorl(args->y - d * multiplier);

    arg_struct args1 = {.x = a, .y = c};
    arg_struct args2 = {.x = a + b, .y = c + d};
    arg_struct args3 = {.x = b, .y = d};

    err = thread_create(&th1, karatsuba, (void *)&args1);
    assert(!err);
    err = thread_create(&th2, karatsuba, (void *)&args2);
    assert(!err);
    err = thread_create(&th3, karatsuba, (void *)&args3);
    assert(!err);

    err = thread_join(th1, (void *)&res1);
    assert(!err);
    err = thread_join(th2, (void *)&res2);
    assert(!err);
    err = thread_join(th3, (void *)&res3);
    assert(!err);

    long double multiplier1 = powl(10, 2 * N);

    *res1 += ((*res2 - *res1 - *res3) * multiplier) + *res3 * multiplier1;

    free(res2);
    free(res3);
    return res1;
}

/**
 * Génère un nombre aléatoire
 *
 * @param size l'exposant en binaire
 * @return un nombre aléatoire random()**size
 */
long double rand_double_long(unsigned long size) {
    double value = floor(1 + (rand() / (double)RAND_MAX) / 10.0L);
    return ldexp(value, size);
}

/**
 * Vérifie la multiplication de x et y avec la présision esp
 *
 * @param res le résultat prétendu de x * y
 * @param x le nombre x
 * @param y le nombre y
 * @param esp la précision de vérification
 * @return 1 si la multiplication est correcte, 0 sinon
 */
int check_res(long double res, long double x, long double y, long double esp) {
    long double mult = floorl(x * y);

    if (res == mult || fabsl(res - mult) < esp) {
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    unsigned long size;
    long double *res = NULL;
    struct timeval tv1, tv2;
    double s;

    srand(time(NULL));

    if (argc < 2) {
        printf("paramètre manquant : exposant du nombre\n");
        return EXIT_FAILURE;
    }

    size = atol(argv[1]);

    if (size > __DBL_MAX_EXP__ / 2) {
        printf("erreur : exposant trop grand (devrait être <= %d)\n",
               __DBL_MAX_EXP__ / 2);
        return EXIT_FAILURE;
    }

    long double x = rand_double_long(size);
    long double y = rand_double_long(size);

    arg_struct funcargs = {.x = x, .y = y};

    gettimeofday(&tv1, NULL);
    res = (long double *)karatsuba((void *)&funcargs);
    gettimeofday(&tv2, NULL);
    s = (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) * 1e-6;

    if (!check_res(*res, x, y, powl(10, 2 * size - __DBL_DIG__))) {
        printf("(FAILED) la multiplication calculée est incorrecte\n");
        printf(
            "résultat trouvé : %Lf \n, résultat attendu : %Lf\n"
            "x = %Lf \n, y = %Lf\n",
            *res, x * y, x, y);
        free(res);
        return EXIT_FAILURE;
    } else {
        printf("(SUCCESS) en %e s\n", s);
        free(res);
        return EXIT_SUCCESS;
    }
}
