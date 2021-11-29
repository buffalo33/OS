#include "thread_private.h"
#include "utils.h"

/**
 * Une file des threads prêts à être exécutés
 * @note La tête de la file est le thread en cours d'exécution
 */
struct uthread_list run_queue;

thread_t main_thread;
thread_t last_thread_standing = NULL;

/** Initialiser la librairie de threads */
void __attribute__((constructor)) init() {
    LOG("[INIT] Initializing the uthread library\n");
    STAILQ_INIT(&run_queue);

    uthread_t *new_thread = (uthread_t *)calloc(1, sizeof(uthread_t));
    main_thread = new_thread;

    init_thread_wait_fields(new_thread);

    add_thread_to_run_queue(new_thread);

    set_handler_stack();
}

void __attribute__((destructor)) destroy() {
    free(main_thread);
    free(signal_stack.ss_sp);
}

thread_t thread_self(void) {
    LOG("[SELF] Giving its id to thread %p\n", STAILQ_FIRST(&run_queue));
    return STAILQ_FIRST(&run_queue);
}

int thread_create(thread_t *new_thread_ptr, void *(*func)(void *),
                  void *funcarg) {
    LOG("[CREATE] Launching a new thread %p\n", new_thread_ptr);
    *new_thread_ptr = malloc(sizeof(uthread_t));
    thread_t new_thread = *new_thread_ptr;

    make_new_context_allocate_stack(
        &new_thread->context, &new_thread->valgrind_stackid, func, funcarg);

    init_thread_wait_fields(new_thread);

    thread_t current = STAILQ_FIRST(&run_queue);
    replace_f_thread();

    add_thread_to_run_queue(new_thread);

    run_f_thread(current);

    return EXIT_SUCCESS;
}

int thread_yield(void) {
    uthread_t *current = STAILQ_FIRST(&run_queue);
    replace_f_thread();

    run_f_thread(current);

    return EXIT_SUCCESS;
}

int thread_join(thread_t thread, void **retval) {
    LOG("[JOIN] Thread %p asked to join thread %p\n", STAILQ_FIRST(&run_queue),
        thread);

    if (thread->state != DONE && thread->state != KILLED) {
        LOG("[JOIN] Thread %p is not done yet\n", thread);

        thread_t current = STAILQ_FIRST(&run_queue);

#ifdef DEADLOCK_DETECTION
        if (is_joined_already_error(thread, current))
            return -1;

        if (is_generating_join_cycle_error(thread, current))
            return -1;
#endif

        f_thread_wait(thread);

        run_f_thread(current);
    }

    bool killed = thread->state == KILLED;

    if (retval) {
        *retval = thread->retval;
    }

    if (thread != main_thread) {
        free_thread_ressources(thread);
    }

    if (killed) {
        LOG("[JOIN] Thread %p was killed\n", thread)
        return -1;
    }

    LOG("[JOIN] Successfully joined thread %p\n", thread)

    return 0;
}

__attribute__((__noreturn__)) void inner_thread_exit(void *retval,
                                                     ustate_e state) {
    uthread_t *current = STAILQ_FIRST(&run_queue);
    terminate_f_thread(retval, state);

    if (current->waited_by) {
        STAILQ_INSERT_HEAD(&run_queue, current->waited_by, next);
        current->waited_by->waiting = NULL;
    }
    if (STAILQ_EMPTY(&run_queue)) {
        LOG("[EXIT] Run queue is empty\n");
        if (current != main_thread) {
            last_thread_standing = current;
            setcontext(&main_thread->context);
        }
        exit(EXIT_SUCCESS);
    }

    if (current == main_thread) {
        run_f_thread(current);
        free_thread_ressources(last_thread_standing);
        exit(EXIT_SUCCESS);
    }
    run_f_thread(NULL);
    exit(EXIT_FAILURE);  // ne devrait jamais être atteint
}

void thread_exit(void *retval) { inner_thread_exit(retval, DONE); }

void thread_crash() { inner_thread_exit(NULL, KILLED); }

typedef struct uthread_mutex {
    thread_t owner;
    struct uthread_list waiting_queue;
} uthread_mutex_t;

int thread_mutex_init(thread_mutex_t *mutex) {
    (*mutex) = malloc(sizeof(uthread_mutex_t));
    (*mutex)->owner = NULL;
    STAILQ_INIT(&(*mutex)->waiting_queue);
    return 0;
}

int thread_mutex_destroy(thread_mutex_t *mutex) {
    free(*mutex);
    return 0;
}

int thread_mutex_lock(thread_mutex_t *mutex) {
    thread_t current = STAILQ_FIRST(&run_queue);
    if ((*mutex)->owner) {
        current->state = WAITING;
        STAILQ_REMOVE_HEAD(&run_queue, next);
        STAILQ_INSERT_TAIL(&(*mutex)->waiting_queue, current, next);
        run_f_thread(current);
    }
    (*mutex)->owner = STAILQ_FIRST(&run_queue);
    return 0;
}

int thread_mutex_unlock(thread_mutex_t *mutex) {
    if (STAILQ_EMPTY(&(*mutex)->waiting_queue)) {
        (*mutex)->owner = NULL;
    } else {
        thread_t waiting = STAILQ_FIRST(&(*mutex)->waiting_queue);
        STAILQ_REMOVE_HEAD(&(*mutex)->waiting_queue, next);
        STAILQ_INSERT_AFTER(&run_queue, STAILQ_FIRST(&run_queue), waiting,
                            next);
        (*mutex)->owner = waiting;
    }
    return 0;
}
