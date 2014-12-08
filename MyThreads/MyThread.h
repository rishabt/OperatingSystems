#ifndef _MYTHREADSH_
#define _MYTHREADSH_

#include <stdio.h>
#include <stdlib.h>
#include <slack/std.h>
#include <slack/list.h>
#include <slack/sig.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/time.h>
#include <time.h>

#define TRUE = 1
#define FALSE = 0
#define SEM_LIMIT 32
#define THREAD_LIMIT 32
#define RUNNING 1
#define EXIT 0
#define BLOCKED 2
#define READY 3
#define WAIT 4
#define UNUSED 10

typedef struct mythread_control_block{
    ucontext_t context;
    char * thread_name;
    void * stack;
    int thread_id;
    int state;
    long CPU_time;
    long start_time;
} mythread_control_block;


typedef struct semaphore{
    int sem;
    int initial_val;
    List * waitqueue;
} semaphore;

/*
 * This function initializes all the global data structures for the thread system.
 */
int mythread_init(void);

/*
 * This function creates a new thread. It returns an integer that points to the thread control block
 * that is allocated to the newly created thread in the thread control block table. If the library is
 * unable to create the new thread, it returns -1 and prints out an error message. This function is
 * responsible for allocating the stack and setting up the user context appropriately. The newly
 * created thread starts running the threadfunc function when it starts. The threadname is stored
 * in the thread control block and is printed for information purposes. A newly created thread is in
 * the RUNNABLE state when it is inserted into the system.
 */
int mythread_create(char *threadname, void (*threadfunc)(), int stacksize);

/*
 * This function is called at the end of the function that was invoked by the thread. This function
 * will remove the thread from the runqueue (i.e., the thread does not need to run any more).
 * However, the entry in the thread control block table could be still left in there
 */
void mythread_exit(void);

/*
 * In MyThreads, threads are created by the mythread_create() function. The
 * mythread_create() function needs to be executed by the main thread (the default thread of
 * process – the one running before MyThreads created any threads). Even after all threads are
 * created, the main thread will still keep running. To actually run the threads, you need to run the
 * runthreads(). The runthreads() switches control from the main thread to one of the threads
 * in the runqueue.
 * In addition to switching over to the threads in the runqueue, the runthreads() function
 * activates the thread switcher. The thread switcher is an interval timer that triggers context
 * switches every quantum nanoseconds.
 */
void runthreads(void);

/*
 * Sets the quantum size of the round robin scheduler. The round robin scheduler is pretty simple it
 * just picks the next thread from the runqueue and appends the current to the end of the runqueue.
 * Then it switches over to the new thread.
 */
void set_quantum_size(int quantum);

/*
 * This function creates a semaphore and sets its initial value to the given parameter. The
 * mythread_init() function would have initialized the semaphores table and set the total number
 * of active semaphore count to zero
 */
int create_semaphore(int value);

/*
 * When a thread calls this function, the value of the semaphore is decremented. If the value goes
 * below 0, the thread is put into a WAIT state. That means calling thread is taken out of the
 * runqueue if the value of the semaphore goes below 0.
 */
void semaphore_wait(int semaphore);

/*
 * When a thread calls this function, the value of the semaphore is incremented. If value is not
 * greater than 0, then we should have at least one thread waiting on it. The thread at the top of the
 * wait queue associated with the semaphore is dequeued from the wait queue and enqueued in the
 * runqueue. The state of the thread is changed to RUNNABLE.
 */
void semaphore_signal(int semaphore);

/*
 * This function removes a semaphore from the system. A call to this function while threads are
 * waiting on the semaphore should fail. That is the removal process should fail with an appropriate
 * error message. If there are no threads waiting, this function will proceed with the removal after
 * checking whether the current value is the same as the initial value of the semaphore. If the values
 * are different, then a warning message is printed before the semaphore is destroyed.
 */
void destroy_semaphore(int semaphore);

/*
 * This function prints the state of all threads that are maintained by the library at any given time.
 * For each thread, it prints the following information in a tabular form: thread name, thread state
 * (print as a string RUNNING, BLOCKED, EXIT, etc), and amount of time run on CPU.
 */
void mythread_state(void);

/*
 * This function switches the thread by swapping the context of the previously running thread with the
 * next thread.
 */
void mythread_switch(void);

/*
 * This function gets the current time, subtracts the begin time obtained from the thread control block from it
 * and stores it as the CPU time in the control block, since CPU_time = end_time - begin_time
 */
void add_time(int thread_id);

/*
 * This function ets the beginning time of a thread and stores it in the thread control block, so that
 * we can later calculate the total CPU time.
 */
void start_timer(int thread_id);

#endif // _MYTHREADSH_
