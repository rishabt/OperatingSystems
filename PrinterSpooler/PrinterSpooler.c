#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>
#include <semaphore.h>

#define TRUE 1

/* ====================================================================================================
 *											QUEUE
 * ==================================================================================================== */

// Defines the struct queue, which follows the First In First Out (FIFO) principle
typedef struct Queue{
  int size;
  int start;
  int end;
  int counter;
  int *elements;
} queue;

// Initializes the queue struct and it's variables
queue* queue_initialize(int size)
{
	queue* queue;
	queue = malloc(sizeof(int)*4 + sizeof(int*));  		// Allocates the memory
	queue->counter = 0;
	queue->start = 0;
	queue->size = size;
	queue->elements = (int*)malloc(sizeof(int*)*size);  	// Allocates the memory of elements dynamic array

	return queue;
}

// Function to enqueue an item in the queue -> returns the position of the element in the queue
int enqueue(queue* queue, int job)
{
	int position;

	if(queue->counter < queue->size)
	{
		position = queue->end;
		queue->elements[queue->end] = job;
		queue->end = (queue->end + 1) % queue->size;
		queue->counter++;
	}

	return position;
}

// Function to dequeue an an item from the queue -> returns the position of the last element of the queue
int dequeue(queue* queue, int* item)
{
	int position;

	if(queue->counter > 0)
	{
		position = queue->start;
		*item = queue->elements[queue->start];
		queue->start = (queue->start + 1) % queue->size;
		queue->counter--;
	}

	return position;
}

// Function to check is the queue is full
int isFull(queue* queue)
{
	int result = (queue->counter == queue->size);
	return result;
}

// Function to check if the queue is empty
int isEmpty(queue *queue)
{
	int result = (queue->counter == 0);
	return result;
}

/*
 * ==================================================================================================== */


/* ====================================================================================================
 *											Buffer
 * ==================================================================================================== */

// Defines struct buffer which stores the requests received from the client
typedef struct Buffer
{
	queue *q;
	pthread_mutex_t lock;
	pthread_cond_t cond;
} buffer;

// Initializes the buffer and it's variables
buffer* buffer_initialize(int size)
{
	buffer* buf = (buffer*)malloc(sizeof(buffer));

	buf->q = queue_initialize(size);
	pthread_mutex_init(&(buf->lock), NULL);
	pthread_cond_init(&(buf->cond), NULL);

	return buf;
}

/*
 * ==================================================================================================== */


/* ====================================================================================================
 * 										CONSTANTS
 * ==================================================================================================== */

sem_t full, empty;

int number_clients;
int number_printers;
int buffer_size;

pthread_attr_t attr;
pthread_mutex_t mutex;
int client_sleep_time = 5;				// Given in the assignment
buffer* buf;

/*
 * ==================================================================================================== */


/*
 * ====================================================================================================
 * 										MAIN SPOOLER
 * ==================================================================================================== */

// Function to initialize the variables
void init()
{
	buf = buffer_initialize(buffer_size);
	sem_init(&full, 0, 0);
	sem_init(&empty, 0, buffer_size);
	pthread_attr_init(&attr);
}

// Client thread function
void* client(void* param)
{
	int client_id = (int) param;
	printf("Starting client %d ...\n", client_id);

	while(TRUE)
	{
		int page_request = rand() % 10 + 1;			// Generates random request
		int wait;									// Variable to check wait

		pthread_mutex_lock(&(buf->lock));			// Acquires the lock to buffer

		if(isFull(buf->q)) {						// If buffer is full client sleeps
		    wait = 1;
		    printf("Client %d has %d pages to print, buffer full, sleeps\n", client_id, page_request);
		}

		while(isFull(buf->q)) {						// Waits for the client to be empty
		    pthread_cond_wait(&(buf->cond), &(buf->lock));
		}

		sem_wait(&empty);
		int position = enqueue(buf->q, page_request);
		sem_post(&full);

		pthread_cond_signal(&(buf->cond));			// Signals buffer condition variable to waiting threads
		pthread_mutex_unlock(&(buf->lock));			// Unlocks the buffer

		if (wait)
		{
		  printf("Client %i wakes up, puts request in Buffer[%i]\n", client_id, position);
		}
		else
		{
		  printf("Client %i has %i pages to print, puts request in Buffer[%i]\n", client_id, page_request, position);
		}

		sleep(client_sleep_time);					// Sleeps for the desired time before generating next request
	}

	return NULL;
}

// Printer thread function
void* printer(void* param)
{
	int printer_id = (int) param;
	printf("Starting printer %d ...\n", printer_id);

	while(TRUE)
	{
		pthread_mutex_lock(&(buf->lock));			// Acquires buffer lock

		if (isEmpty(buf->q)) {						// Checks to see if buffer is empty
		    printf("No request in buffer, Printer %d sleeps\n", printer_id);
		}

		while(isEmpty(buf->q)) {					// Waits for buffer to have something
		    pthread_cond_wait(&(buf->cond), &(buf->lock));
		}

		int page_request;
		sem_wait(&full);
		int position = dequeue(buf->q, &page_request);
		sem_post(&empty);

		pthread_cond_signal(&(buf->cond));		// Signals waiting threads
		pthread_mutex_unlock(&(buf->lock));		// Unlocks the buffer

		printf("Printer %d starts printing %d pages from Buffer[%d]\n", printer_id, page_request, position);
		sleep(page_request);
		printf("Printer %i finishes printing %d pages from Buffer[%d]\n", printer_id, page_request, position);
	}

	return NULL;
}

// Main Function
int main()
{
	printf("Please input number of clients: ");		// Takes number of clients as input
	scanf("%d", &number_clients);

	printf("Please input number of printers: ");	// Takes number of printers as input
	scanf("%d", &number_printers);

	printf("Please input buffer size: ");			// Takes buffer size as input
	scanf("%d", &buffer_size);

	init();											// Calls the initialization function

	long i;
	for(i = 0; i < number_clients; i++)				// Creates the threads for the clients
	{
		pthread_t client_thread;
		pthread_create(&client_thread, &attr, client, (void*) i);
	}

	for(i = 0; i < number_printers; i++)			// Creates the threads for printers
	{
		pthread_t printer_thread;
		pthread_create(&printer_thread, &attr, printer, (void*) i);
	}

	while(1)										// Goes into an infinite loop of sleep
	{
		sleep(1);
	}

	return 0;
}
