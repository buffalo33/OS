#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "thread.h"

/**
 * Identifiants pour les axes
 */
typedef enum { ABSCISSA, ORDINATE, UNKNOWN } coord;

/** Paramètres pour la fonction merge */
typedef struct {
    double *x;          // la liste des abcisses
    double *y;          // la liste des ordonnées
    long int size;      // la taille des listes
    coord axis_sorted;  // l'axe selon lequel la liste doit être triée
} point_list;

/** Paramètres pour la fonction ppr_rec */
typedef struct {
    point_list *abs;  // liste des points triés selon l'abscisse
    point_list *ord;  // liste des points triés selon l'ordonnée
} double_point_list;

typedef struct {
    point_list *abs_g;
    point_list *abs_d;
    point_list *ord_g;
    point_list *ord_d;
    double x;
} arg_struct_ppr_div;

typedef struct {
    point_list *ord;
    double dist;
    double x;
} arg_struct_ppr_extract;

typedef struct {
    point_list *ord;
    double dist;
} arg_struct_ppr_bande;

/**
 * Calcule la distance au carré entre deux points
 *
 * @param x1 abcisse du premier point
 * @param x2 abcisse du second point
 * @param y1 ordonnée du premier point
 * @param y2 ordonnée du second point
 */
double dist(double x1, double x2, double y1, double y2) {
    return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}

/**
 * Fusionne les différentes parties de la liste
 *
 * @param sub_list un pointeur vers le premier élément de la liste
 * @param size la taille de la sous-liste considérée pour la fusion.
 * @param middle l'indice du milieu de la liste
 */
void merge(point_list *args, double middle) {
    long int i, j, k;

    /* On prépare une liste qui contiendra le tri croissant de 'sub_list' */
    double *tmp_x = malloc(args->size * sizeof(double));
    double *tmp_y = malloc(args->size * sizeof(double));

    double *x, *y;
    if (args->axis_sorted == ABSCISSA) {
        x = args->x;
        y = args->y;
    } else {
        x = args->y;
        y = args->x;
    }

    for (i = 0, j = middle, k = 0; k < args->size; k++) {
        if (j == args->size) {
            tmp_x[k] = x[i];
            tmp_y[k] = y[i++];
        } else if (i == middle || x[j] < x[i]) {
            tmp_x[k] = x[j];
            tmp_y[k] = y[j++];
        } else {
            tmp_x[k] = x[i];
            tmp_y[k] = y[i++];
        }
    }

    for (i = 0; i < args->size; i++) {
        x[i] = tmp_x[i];
        y[i] = tmp_y[i];
    }

    free(tmp_x);
    free(tmp_y);
}

/**
 * Trie une point_list selon l'axe spécifié dans la structure.
 *
 * @param funcargs une point_list ) à trier
 */
static void *merge_sort(void *funcargs) {
    thread_t th, th2;
    int err;
    void *res = NULL, *res2 = NULL;
    point_list *args = (point_list *)funcargs;

    /* On passe un peu la main aux autres pour eviter de faire uniquement la
     * partie gauche de l'arbre */
    thread_yield();

    if (args->size < 2) {
        return NULL;
    }

    long int middle = args->size / 2;

    point_list funcargs1 = {.x = args->x,
                            .y = args->y,
                            .size = middle,
                            .axis_sorted = args->axis_sorted};

    point_list funcargs2 = {.x = args->x + middle,
                            .y = args->y + middle,
                            .size = args->size - middle,
                            .axis_sorted = args->axis_sorted};

    err = thread_create(&th, merge_sort, (void *)(&funcargs1));
    assert(!err);
    err = thread_create(&th2, merge_sort, (void *)(&funcargs2));
    assert(!err);

    err = thread_join(th, &res);
    assert(!err);
    err = thread_join(th2, &res2);
    assert(!err);

    merge(args, middle);
    return NULL;
}

/**
 * Construit un nouveau point_list
 *
 * @param size la taille des listes x & y
 * @param c l'axee selon lequel la liste est triée
 * @return pointeur sur la structure crée
 */

point_list *construct_arg(long int size, coord c) {
    point_list *new_arg = (point_list *)malloc(sizeof(point_list));

    assert(new_arg != NULL);

    new_arg->axis_sorted = c;
    new_arg->size = size;

    new_arg->y = (double *)malloc(size * sizeof(double));
    new_arg->x = (double *)malloc(size * sizeof(double));
    assert(new_arg->x != NULL);
    assert(new_arg->y != NULL);
    for (long int i = 0; i < size; i++) {
        new_arg->y[i] = -INFINITY;
        new_arg->x[i] = -INFINITY;
    }
    return new_arg;
}

/** Libère une liste de points */
void free_point_list(point_list *arg) {
    free(arg->x);
    free(arg->y);
    free(arg);
}

/**
 * fonction permettant de libérer la mémoire
 *
 * @param arg pointeur sur la mémoire à liberer
 */
void arg_ppri_div_free(arg_struct_ppr_div *arg) {
    free_point_list(arg->abs_d);
    free_point_list(arg->abs_g);
    free_point_list(arg->ord_d);
    free_point_list(arg->ord_g);
    free(arg);
}

/**
 * fonction permettant de libérer la mémoire
 *
 * @param arg pointeur sur la mémoire à liberer
 */
void arg_ppr_bande_free(arg_struct_ppr_bande *arg) {
    free_point_list(arg->ord);
    free(arg);
}

/**
 * Diviser les deux tableaux X (trié par abscisses croissantes) et
 * Y (trié par ordonnées croissantes) en quatre sous tableau XG, XD, YG, YD
 *
 * @param args pointeur contenant X et Y
 * @return pointeur contenant XG,XD,YG,YD
 */
static void *ppr_div(void *funcargs) {
    double_point_list *args = funcargs;

    arg_struct_ppr_div *arg_ppr_div = malloc(sizeof(arg_struct_ppr_div));

    long int mid = args->abs->size / 2;

    arg_ppr_div->abs_g = construct_arg(mid, ABSCISSA);
    arg_ppr_div->ord_g = construct_arg(mid, ORDINATE);

    arg_ppr_div->abs_d = construct_arg(args->abs->size - mid, ABSCISSA);
    arg_ppr_div->ord_d = construct_arg(args->abs->size - mid, ORDINATE);

    arg_ppr_div->x = args->abs->x[mid];

    long int yd = 0, yg = 0, xd = 0, xg = 0;

    for (long int i = 0; i < args->abs->size; i++) {
        if ((args->ord->x[i] <= arg_ppr_div->x && yg < mid) ||
            yd >= (args->abs->size - mid)) {
            arg_ppr_div->ord_g->y[yg] = args->ord->y[i];
            arg_ppr_div->ord_g->x[yg] = args->ord->x[i];
            yg++;
        } else {
            arg_ppr_div->ord_d->y[yd] = args->ord->y[i];
            arg_ppr_div->ord_d->x[yd] = args->ord->x[i];
            yd++;
        }

        if (i < mid) {
            arg_ppr_div->abs_g->x[xg] = args->abs->x[i];
            arg_ppr_div->abs_g->y[xg] = args->abs->y[i];
            xg++;
        } else {
            arg_ppr_div->abs_d->x[xd] = args->abs->x[i];
            arg_ppr_div->abs_d->y[xd] = args->abs->y[i];
            xd++;
        }
    }

    return (void *)arg_ppr_div;
}

/**
 * Contruit Y' par extraction de tous les points de Y
 * dont la distance à la droite x = args->x est inférieure à args->dist
 *
 * @param funcargs structure contenant l'équation de la droite (eq tq x = a),
 *             une distance et les points triés selon les ordonnées croissantes
 *
 * @return pointeur comptant Y' et la distance
 */
static void *ppr_extract(void *funcargs) {
    arg_struct_ppr_extract *args = (arg_struct_ppr_extract *)funcargs;

    arg_struct_ppr_bande *res = malloc(sizeof(arg_struct_ppr_bande));
    res->ord = construct_arg(args->ord->size, ORDINATE);
    res->dist = args->dist;

    long int id_tmp = 0;

    for (long int i = 0; i < args->ord->size; i++) {
        double abs = (args->ord->x[i] < args->x) ? args->ord->x[i] - args->x
                                                 : args->ord->x[i] - args->x;

        if (abs < args->dist) {
            res->ord->x[id_tmp] = args->ord->x[i];
            res->ord->y[id_tmp++] = args->ord->y[i];
        }
    }

    res->ord->size = id_tmp + 1;

    return res;
}

/**
 * Calcule la plus petite distance entre 2 points
 *
 * @param funcargs pointeur contenant Y' et une distance
 * @return la plus petite distance entre 2 points
 */
static void *ppr_bande(void *funcargs) {
    arg_struct_ppr_extract *args = (arg_struct_ppr_extract *)funcargs;
    double *res = malloc(sizeof(double));
    *res = args->dist;

    for (long int i = 0; i < args->ord->size - 1; i++) {
        long int m = (i + 7 < args->ord->size) ? i + 7 : args->ord->size - 1;
        for (long int j = i + 1; j < m; j++) {
            double tmp_d = dist(args->ord->x[i], args->ord->x[j],
                                args->ord->y[i], args->ord->y[j]);
            *res = (tmp_d < *res) ? tmp_d : *res;
        }
    }

    return (void *)res;
}

/**
 * Calcule naïvement la plus petite distance entre 2 points
 *
 * @param args une liste de points
 * @return la plus petite distance
 */
void *ppr_naif(point_list *args) {
    double *res = (double *)malloc(sizeof(double));
    *res = INFINITY;

    for (long int i = 0; i < args->size - 1; i++) {
        for (long int j = i + 1; j < args->size; j++) {
            double tmp_d = dist(args->x[i], args->x[j], args->y[i], args->y[j]);
            *res = (tmp_d < *res) ? tmp_d : *res;
        }
    }

    return (void *)res;
}

/**
 * Initie le calcul récursif de la plus petite distance entre 2 points
 *
 * @param funcargs listes X et Y de points triées respectivement selon les
 * abcisses et ordonnées
 * @return la plus petite distance entre 2 points
 */
static void *ppr_rec(void *funcargs) {
    thread_t th1, th2, th3, th4, th5;
    int err;
    void *res1 = NULL, *res2 = NULL, *res3 = NULL, *res4 = NULL;
    void *res5 = NULL;

    double_point_list *args = (double_point_list *)funcargs;

    /* On passe un peu la main aux autres pour eviter de faire uniquement la
     * partie gauche de l'arbre */
    thread_yield();

    if (args->abs->size < 3) {
        return ppr_naif(args->abs);
    }

    err = thread_create(&th1, ppr_div, (void *)args);
    assert(!err);

    err = thread_join(th1, &res1);
    assert(!err);

    arg_struct_ppr_div *arg_ppr_div = (arg_struct_ppr_div *)res1;

    double_point_list ppr_rec_1 = {
        .abs = arg_ppr_div->abs_d,
        .ord = arg_ppr_div->ord_d,
    };

    double_point_list ppr_rec_2 = {
        .abs = arg_ppr_div->abs_g,
        .ord = arg_ppr_div->ord_g,
    };

    err = thread_create(&th2, ppr_rec, (void *)&ppr_rec_1);
    assert(!err);
    err = thread_create(&th3, ppr_rec, (void *)&ppr_rec_2);
    assert(!err);

    err = thread_join(th2, &res2);
    assert(!err);
    err = thread_join(th3, &res3);
    assert(!err);

    arg_struct_ppr_extract arg_extract = {
        .dist = (*(double *)res2 < *(double *)res3) ? *(double *)res2
                                                    : *(double *)res3,
        .ord = args->ord,
        .x = arg_ppr_div->x,
    };

    err = thread_create(&th4, ppr_extract, (void *)&arg_extract);
    assert(!err);

    err = thread_join(th4, &res4);
    assert(!err);

    arg_struct_ppr_bande *new_ord = (arg_struct_ppr_bande *)res4;

    err = thread_create(&th5, ppr_bande, (void *)new_ord);
    assert(!err);

    err = thread_join(th5, &res5);
    assert(!err);

    arg_ppri_div_free((arg_struct_ppr_div *)res1);
    arg_ppr_bande_free(new_ord);
    free(res2);
    free(res3);

    return res5;
}

/**
 * Vérifie que le tableau est trié dans l'ordre croissant.
 *
 * @param list un pointeur vers la liste triée
 * @param size la taille de la liste
 * @return 1 en cas de succès, 0 en cas d'erreur
 */
int merge_sort_checker(double *list, double size) {
    long int i;
    for (i = 1; i < size; ++i) {
        if (list[i] < list[i - 1]) {
            return 0;
        }
    }
    return 1;
}

/**
 * Calcule la plus petite distance entre 2 points d'un nuage
 *
 * @param funcargs pointeur sur les liste X,Y
 * @return la plus petite distance trouvée
 */
static void *ppr(void *funcargs) {
    thread_t th;
    int err;
    void *res = NULL;
    double_point_list *args = (double_point_list *)funcargs;

    (void)merge_sort((void *)args->abs);
    (void)merge_sort((void *)args->ord);

    if (!merge_sort_checker(args->abs->x, args->abs->size) &&
        !merge_sort_checker(args->ord->y, args->ord->size)) {
        printf("(FAILED) : la liste n'est pas triée dans l'ordre croissant\n");

        exit(EXIT_FAILURE);
    }

    err = thread_create(&th, ppr_rec, (void *)args);
    assert(!err);
    err = thread_join(th, &res);
    assert(!err);

    return res;
}

/**
 * Génére un double aléatoire compris appartenant à [-size, size]
 * @param size borne supérieure
 * @return un nombre appartenant à [-size, size]
 */
double rand_double(double size) {
    double div = RAND_MAX / (2 * size);
    return -1.0 * size + (rand() / div);
}

/**
 * Remplit x1,y1 avec des valeurs aléatoires et le recopie dans x2,y2
 *
 * @param x1 un pointeur vers la liste abcisses
 * @param y1 un pointeur vers la liste coordonnées
 * @param x2 un pointeur vers la liste abscisses
 * @param y2 un pointeur vers la liste coordonnées
 * @param size la taille de la liste
 */
void rand_fill_point_copy(double *x1, double *y1, double *x2, double *y2,
                          int size) {
    int i;
    for (i = 0; i < size; ++i) {
        x1[i] = rand_double((double)size);
        y1[i] = rand_double((double)size);

        x2[i] = x1[i];
        y2[i] = y1[i];
    }
}

int main(int argc, char *argv[]) {
    long int size;
    struct timeval tv1, tv2;
    double s;

    srand(time(NULL));

    if (argc < 2) {
        printf("argument manquant: taille de la liste de points\n");
        return EXIT_FAILURE;
    }

    size = atol(argv[1]);
    double x1[size], y1[size], x2[size], y2[size];
    rand_fill_point_copy(x1, y1, x2, y2, size);

    point_list list_abs = {
        .x = x1, .y = y1, .size = size, .axis_sorted = ABSCISSA};
    point_list list_ord = {
        .x = x2, .y = y2, .size = size, .axis_sorted = ORDINATE};

    double *res_ppr_naif_ptr = (double *)ppr_naif((void *)&list_abs);
    double res_ppr_naif = *res_ppr_naif_ptr;
    free(res_ppr_naif_ptr);

    double_point_list funcargs = {.abs = &list_abs, .ord = &list_ord};

    gettimeofday(&tv1, NULL);
    double *res_ptr = (double *)ppr((void *)&funcargs);
    gettimeofday(&tv2, NULL);
    s = (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) * 1e-6;

    double res = *res_ptr;
    free(res_ptr);

    if (res != res_ppr_naif) {
        printf("(FAILED) : la liste n'est pas triée dans l'ordre croissant\n");
        printf("res = %lf != %lf \n", res, res_ppr_naif);
        return EXIT_FAILURE;
    } else {
        printf("(SUCCESS) : en %e s\n", s);
        return EXIT_SUCCESS;
    }
}
