#include "MyThread.h"

int mutex;
double answer = 10;

void threadfunc();

int main()
{
	int number_threads = 12;
	char* thread_names[] = {"Thread A", "Thread B", "Thread C", "Thread D", "Thread E", "Thread F",
			"Thread G", "Thread H", "Thread I", "Thread J", "Thread K", "Thread K"
	};

	int success_init = mythread_init();

	if(success_init == 0)
	{
		set_quantum_size(300);

		mutex = create_semaphore(1);

		int i;
		for(i = 0; i < number_threads; i++)
		{
			mythread_create(thread_names[i], (void *) &threadfunc, 4096);
		}

		mythread_state();

		runthreads();

		destroy_semaphore(mutex);

		mythread_state();
	}

	printf("Answer is %f\n", answer);

	return 0;

}

void threadfunc()
{
	int i;
	for(i = 0; i < 10; i++)
	{
		semaphore_wait(mutex);

		int j;
		for(j = 0; j < 10; j++)
		{
			answer = answer * 0.99;
		}

		semaphore_signal(mutex);
	}

	mythread_exit();
}
