#include "MyThread.h"

static int number_threads = 0;
static List * queue;
static mythread_control_block thread_control_blocks_table[THREAD_LIMIT];
static ucontext_t context_main;
static int quantum_size = 0;

static int running_thread = 0;
static semaphore semaphore_table[SEM_LIMIT];
static int active_semaphores;
long long int switch_ctr;
int stop;

int mythread_init(void)
{
    queue = list_create(NULL);
    active_semaphores = 0;

    int i;
    for(i = 0; i < THREAD_LIMIT; i++)
    {
        thread_control_blocks_table[i].state = UNUSED;
    }

    // Check for errors in creating the list
    if (queue == 0)
    {
        return -1;
    }

    return 0;
}

int mythread_create(char *threadname, void (*threadfunc)(), int stacksize)
{
    if (number_threads == THREAD_LIMIT)				// Maximum threads running
    {
    	return -1;
    }

    // Add the new thread to the table and set the variables in the thead control block
    getcontext(&thread_control_blocks_table[number_threads].context);

    thread_control_blocks_table[number_threads].context.uc_link = &context_main;
    thread_control_blocks_table[number_threads].stack = malloc(stacksize);
    thread_control_blocks_table[number_threads].context.uc_stack.ss_sp = thread_control_blocks_table[number_threads].stack;
    thread_control_blocks_table[number_threads].context.uc_stack.ss_size = stacksize;
    thread_control_blocks_table[number_threads].context.uc_stack.ss_flags = 0;
    sigemptyset(&thread_control_blocks_table[number_threads].context.uc_sigmask);

    int thread_id = number_threads;

    thread_control_blocks_table[thread_id].thread_name = strdup(threadname);
    thread_control_blocks_table[thread_id].thread_id = thread_id;
    thread_control_blocks_table[thread_id].state = READY;
    thread_control_blocks_table[thread_id].CPU_time = 0;

    if (thread_control_blocks_table[number_threads].stack == 0)
    {
        perror("Error: Could not allocate the stack.");
        return -1;
    }

    makecontext( &thread_control_blocks_table[number_threads].context, threadfunc, 0 );
    ++number_threads;

    queue = list_append_int(queue, thread_id);

    return thread_id;
}

void mythread_exit(void)
{
    thread_control_blocks_table[running_thread].state = EXIT;
    add_time(running_thread);
    return;
}

void runthreads(void)
{
    // Block Signals
    sigset_t sig;
    sigemptyset(&sig);
    sigaddset(&sig, SIGALRM);
    sigprocmask(SIG_BLOCK, &sig, NULL);

    if (number_threads == 0)
    {
        puts("No Threads to run");
        return;
    }

    struct itimerval itimer;
    itimer.it_interval.tv_sec = 0;
    itimer.it_interval.tv_usec = quantum_size;
    itimer.it_value.tv_sec = 0;
    itimer.it_value.tv_usec = quantum_size;
    setitimer(ITIMER_REAL, &itimer, 0);

    sigset(SIGALRM, &mythread_switch);
    running_thread = list_shift_int(queue);

    thread_control_blocks_table[running_thread].state = RUNNING;

    start_timer(running_thread);

    // Unblock signal
    sigprocmask(SIG_UNBLOCK, &sig, NULL);

    if(swapcontext(&context_main, &thread_control_blocks_table[running_thread].context) == -1)
    {
        perror("Error: Could not swap context");
        exit(1);
    }

    while(!list_empty(queue))
    {
        running_thread = list_shift_int(queue);
        thread_control_blocks_table[running_thread].state = RUNNING;

        start_timer(running_thread);

        if(swapcontext(&context_main, &thread_control_blocks_table[running_thread].context) == -1)
        {
            perror("Error: Could not swap context");
            exit(1);
        }
    }

    sigprocmask(SIG_BLOCK, &sig, NULL);

    int i;
    for(i = 0; i < number_threads ; i++)
    {
    	thread_control_blocks_table[i].state = EXIT;
    }

    return;
}

void set_quantum_size(int quantum)
{
    quantum_size = quantum;
}


void mythread_switch(void)
{
    switch_ctr++;

    if (list_empty(queue) && thread_control_blocks_table[running_thread].state == EXIT)
    {
        add_time(running_thread);
        stop = 1;
        setcontext(&context_main);
    }

    if (!list_empty(queue))
    {
        int previous_thread = running_thread;
        running_thread = list_shift_int(queue);

        add_time(previous_thread);
        start_timer(running_thread);

        if (thread_control_blocks_table[previous_thread].state == READY ||
        		thread_control_blocks_table[previous_thread].state == RUNNING)
        {
            queue = list_append_int(queue, previous_thread);
        }

        if (swapcontext(&thread_control_blocks_table[previous_thread].context,
        		&thread_control_blocks_table[running_thread].context) == -1)
        {
        	perror("Error: Could not swap context");
            exit(1);
        }
    }
    else
    {
        if(swapcontext(&thread_control_blocks_table[running_thread].context, &context_main) == -1)
        {
        	perror("Error: Could not swap context");
            exit(1);
        }
    }
}

void mythread_state(void)
{
    printf("\nTHREADNAME\tTHREAD\tTHREAD STATE\tCPU TIME(ns)\n");
    int i;
    for(i = 0; (thread_control_blocks_table[i].state != UNUSED) && (i < THREAD_LIMIT); ++i)
    {
        char * state_i;
        switch(thread_control_blocks_table[i].state)
        {
            case 0:
                state_i = "EXIT";
                break;
            case 1:
                state_i = "RUNNING";
                break;
            case 2:
                state_i = "BLOCKED";
                break;
            case 3:
                state_i = "READY";
                break;
            case 4:
                state_i = "WAIT";
                break;
            default:
                state_i = "UNDEFINED";
                break;
        }
        printf("\n%s\t%d\t%s\t\t%ld\n", thread_control_blocks_table[i].thread_name,
        		i, state_i, thread_control_blocks_table[i].CPU_time);
    }
}

int create_semaphore(int value)
{
    if (active_semaphores == SEM_LIMIT) return -1;

    semaphore_table[active_semaphores].sem = value;
    semaphore_table[active_semaphores].initial_val = value;
    semaphore_table[active_semaphores].waitqueue = list_create(NULL);

    int semaphore_id = active_semaphores;

    ++active_semaphores;

    return semaphore_id;
}

void destroy_semaphore(int semaphore)
{
    if (!list_empty(semaphore_table[semaphore].waitqueue))
    {
        perror("Call made to destroy_semaphore while threads are waiting on it");
        return;
    }

    if (semaphore_table[semaphore].initial_val != semaphore_table[semaphore].sem)
    {
        fprintf(stderr, "Initial value is not equal to current value\n");
    }

    return;
}

void semaphore_wait(int semaphore)
{
    sigset_t x;
    sigemptyset(&x);
    sigaddset(&x, SIGALRM);
    sigprocmask(SIG_BLOCK, &x, NULL);

    long long int old_switch_ctr = switch_ctr;
    (semaphore_table[semaphore]).sem--;

    if((semaphore_table[semaphore]).sem<0)
    {
        (thread_control_blocks_table[running_thread]).state = BLOCKED;
        (semaphore_table[semaphore]).waitqueue = list_append_int((semaphore_table[semaphore]).waitqueue,running_thread);
    }

    sigprocmask(SIG_UNBLOCK,&x,NULL);
    while(old_switch_ctr == switch_ctr);
}

void semaphore_signal(int semaphore)
{
    // Block Signals
    sigset_t sig;
    sigemptyset (&sig);
    sigaddset(&sig, SIGALRM);
    sigprocmask(SIG_BLOCK, &sig, NULL);

    semaphore_table[semaphore].sem = semaphore_table[semaphore].sem + 1;

    if (semaphore_table[semaphore].sem < 1)
    {
        int should_run;
        should_run = list_shift_int(semaphore_table[semaphore].waitqueue);
        thread_control_blocks_table[should_run].state = READY;
        queue = list_append_int(queue, should_run);
    }

    // Unblock Signals
    sigprocmask(SIG_UNBLOCK, &sig, NULL);
    mythread_switch();
}

void add_time(int thread_id)
{
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    long end_time = current_time.tv_nsec;
    thread_control_blocks_table[thread_id].CPU_time += end_time - thread_control_blocks_table[thread_id].start_time;
}

void start_timer(int thread_id)
{
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    thread_control_blocks_table[thread_id].start_time = current_time.tv_nsec;
}
