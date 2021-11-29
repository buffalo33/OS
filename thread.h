/**
 * @file thread.h
 * @brief Bibliothèque de threads utilisateurs
 */

#ifndef __THREAD_H__
#define __THREAD_H__

#include <stdio.h>

#ifndef USE_PTHREAD

/**
 * Etats possibles d'un thread
 */
typedef enum ustate {
    RUNNING, /**< en cours d'exécution */
    READY,   /**< en attente pour être exécuté */
    DONE,    /**< terminé */
    WAITING, /**< en attente d'un autre thread (joining) */
    KILLED,  /**< tué à cause d'un débordement de pile */
} ustate_e;

/** Identifiant d'un thread */
typedef struct uthread *thread_t;

/** Récupérer l'identifiant d'un thread */
extern thread_t thread_self(void);

/**
 * Créer un nouveau thread qui va exécuter la fonction `func`
 * avec l'argument `funcarg`
 *
 * @param newthread un pointeur vers un identifiant de thread
 * @param func la fonction à exécuter au sein du thread
 * @param funcarg l'argument pour `func`
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
extern int thread_create(thread_t *newthread, void *(*func)(void *),
                         void *funcarg);

/** Passer la main à un autre thread */
extern int thread_yield(void);

/**
 * Attendre la fin d'exécution d'un thread.
 * La valeur renvoyée par celui-ci est placée dans `*retval`.
 *
 * @param thread l'identifiant du thread à attendre
 * @param retval un pointeur vers un emplacement pour la valeur de retour
 * @note si `retval` est `NULL`, la valeur de retour est ignorée
 */
extern int thread_join(thread_t thread, void **retval);

/**
 * Terminer le thread courant en renvoyant la valeur de retour `retval`
 *
 * @note Cette fonction ne retourne jamais
 * @note L'attribut `noreturn` aide le compilateur à optimiser le code de
 * l'application (élimination de code mort). Attention à ne pas mettre
 * cet attribut dans votre interface tant que votre thread_exit()
 * n'est pas correctement implémenté (il ne doit jamais retourner).
 */
extern void thread_exit(void *retval) __attribute__((__noreturn__));

/* Interface possible pour les mutex */
typedef struct uthread_mutex *thread_mutex_t;
int thread_mutex_init(thread_mutex_t *mutex);
int thread_mutex_destroy(thread_mutex_t *mutex);
int thread_mutex_lock(thread_mutex_t *mutex);
int thread_mutex_unlock(thread_mutex_t *mutex);

#else /* USE_PTHREAD */

/* Si on compile avec -DUSE_PTHREAD, ce sont les pthreads qui sont utilisés */
#include <pthread.h>
#include <sched.h>
#define thread_t pthread_t
#define thread_self pthread_self
#define thread_create(th, func, arg) pthread_create(th, NULL, func, arg)
#define thread_yield sched_yield
#define thread_join pthread_join
#define thread_exit pthread_exit

/* Interface possible pour les mutex */
#define thread_mutex_t pthread_mutex_t
#define thread_mutex_init(_mutex) pthread_mutex_init(_mutex, NULL)
#define thread_mutex_destroy pthread_mutex_destroy
#define thread_mutex_lock pthread_mutex_lock
#define thread_mutex_unlock pthread_mutex_unlock

#endif /* USE_PTHREAD */

#endif /* __THREAD_H__ */
