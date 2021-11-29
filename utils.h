#ifndef __UTILS_H
#define __UTILS_H
#include "thread_private.h"

extern struct uthread_list run_queue;

stack_t signal_stack;

/**
 * A function to wrap the call to `func` and capture its return value
 */
void wrapper(void *(*func)(void *), void *funcarg) {
    thread_exit(func(funcarg));
}

static inline void init_thread_wait_fields(thread_t thread) {
    thread->waited_by = NULL;
    thread->waiting = NULL;
}

static inline void add_thread_to_run_queue(thread_t thread) {
    STAILQ_INSERT_HEAD(&run_queue, thread, next);
}

void limit_stack(void *beg) {
    LOG("[OVERFLOW] Protecting page starting at %p\n", beg);

    long pagesize = sysconf(_SC_PAGE_SIZE);
    int ret = mprotect(beg, pagesize, PROT_NONE);

    if (ret == -1) {
        perror("[OVERFLOW] Error while protecting stack");
        exit(EXIT_FAILURE);
    }
}

static inline void make_new_context_allocate_stack(ucontext_t *context,
                                                   int *valgrind_stackid,
                                                   void *(*func)(void *),
                                                   void *funcarg) {
    getcontext(context);

    context->uc_stack.ss_size = STACK_SIZE;
    long pagesize = sysconf(_SC_PAGE_SIZE);
    int ret = posix_memalign(&context->uc_stack.ss_sp, pagesize, STACK_SIZE);
    if (ret != 0) {
        LOG("[STACK] Error while allocating aligned stack\n");
    }
    limit_stack(context->uc_stack.ss_sp);
    *valgrind_stackid = VALGRIND_STACK_REGISTER(
        context->uc_stack.ss_sp, context->uc_stack.ss_sp + STACK_SIZE);
    makecontext(context, (void (*)(void))wrapper, 2, func, funcarg);
}

static inline void replace_f_thread() {
    thread_t current = STAILQ_FIRST(&run_queue);
    current->state = READY;
    STAILQ_REMOVE_HEAD(&run_queue, next);
    STAILQ_INSERT_TAIL(&run_queue, current, next);
}

static inline void run_f_thread(thread_t current) {
    thread_t next_thread = STAILQ_FIRST(&run_queue);
    next_thread->state = RUNNING;
    if (current) {
        swapcontext(&current->context, &next_thread->context);
    } else {
        setcontext(&next_thread->context);
    }
}

static inline bool is_joined_already_error(thread_t thread, thread_t current) {
    if (thread->waited_by) {
        // ERROR Joined twice on the same thread
        LOG("ERROR: %p is being joined by %p and %p wishes to join as well\n",
            thread, thread->waited_by, current);
        return true;
    }
    return false;
}

static inline bool is_generating_join_cycle_error(thread_t thread,
                                                  thread_t current) {
    thread_t tmp_thread = thread;
    while (tmp_thread) {
        if (tmp_thread == current) {
            LOG("ERROR %p is trying to join %p, this would create deadlock\n",
                current, thread);
            return true;
        }
        tmp_thread = tmp_thread->waiting;
    }
    return false;
}

static inline void f_thread_wait(thread_t waited_thread) {
    thread_t current = STAILQ_FIRST(&run_queue);
    current->waiting = waited_thread;
    waited_thread->waited_by = current;
    current->state = WAITING;
    STAILQ_REMOVE_HEAD(&run_queue, next);
}

static inline void free_thread_ressources(thread_t thread) {
    LOG("[EXIT] Freeing stack %p\n", thread->context.uc_stack.ss_sp);
    VALGRIND_STACK_DEREGISTER(thread->valgrind_stackid);
    free(thread->context.uc_stack.ss_sp);
    free(thread);
}

static inline void terminate_f_thread(void *retval, ustate_e state) {
    thread_t current = STAILQ_FIRST(&run_queue);
    current->retval = retval;
    current->state = state;
    STAILQ_REMOVE_HEAD(&run_queue, next);
}

void segv_handler() {
    LOG("[OVERFLOW] Lauching signal handler\n");
    thread_crash(NULL);
}

void set_handler_stack() {
    LOG("[OVERFLOW] Setting up signal handler\n");
    signal_stack.ss_size = SIGSTKSZ;
    signal_stack.ss_sp = malloc(signal_stack.ss_size);
    sigaltstack(&signal_stack, NULL);

    struct sigaction sa = {.sa_handler = segv_handler, .sa_flags = SA_ONSTACK};
    sigaction(SIGSEGV, &sa, NULL);
}

#endif
