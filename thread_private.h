#ifndef __THREAD_PRIVATE_H
#define __THREAD_PRIVATE_H
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/queue.h>
#include <ucontext.h>
#include <unistd.h>
#include <valgrind/valgrind.h>

#include "thread.h"

#define STACK_SIZE 128 * 1024
#ifdef VERBOSE
#define VERBOSE_TEST 1
#else
#define VERBOSE_TEST 0
#endif

#define LOG(...)                          \
    do {                                  \
        if (VERBOSE_TEST)                 \
            fprintf(stderr, __VA_ARGS__); \
    } while (0);

/** Définition du type uthread_list de queue pour stocker les threads */
STAILQ_HEAD(uthread_list, uthread);

/** Représentation interne d'un thread */
typedef struct uthread {
    ucontext_t context;   /**< le contexte d'exécution */
    void *retval;         /**< la valeur de retour */
    ustate_e state;       /**< l'état du thread */
    thread_t waited_by;   /**< le thread souhaitant join ce uthread ou
                                 NULL sinon */
    thread_t waiting;     /**< le thread que ce uthread souhaite joindre ou
                                    NULL sinon */
    int valgrind_stackid; /**< identifiant de la pile pour valgrind */
    STAILQ_ENTRY(uthread) next; /**< le thread suivant dans la file */
} uthread_t;

/** Identifiant d'un thread */
typedef uthread_t *thread_t;

/** Exit a thread with a specific state */
void thread_crash();

#endif
