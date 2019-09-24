#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

#define EXECUTION_TIME 10 /* In seconds */

/* Barrier variable */
pthread_barrier_t barr;


void *f_thread(void *arg)
{
	time_t start_time = time(NULL);

	unsigned long long nb_iterations = 0;
	while(time(NULL) < (start_time + EXECUTION_TIME)) {
		++nb_iterations;
	}
	
	*((long int*)arg) = nb_iterations;
	
	pthread_barrier_wait(&barr);
	
	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	int nb_threads = 0;
	
	unsigned long long total_iterations = 0;
	int min_prio,max_prio;
	int i,j;
	
	
	/* Parse input */
	if (argc != 2) {
		fprintf(stderr, "Usage: %s NB_THREADS\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	nb_threads = strtoimax(argv[1], (char **)NULL, 10);
	
	if (nb_threads <= 0) {
		fprintf(stderr, "NB_THREADS must be > 0 (actual: %d)\n", nb_threads);
		return EXIT_FAILURE;
	}
	
	long int nb_iterations[nb_threads];
	unsigned int prio_value[nb_threads];
	pthread_t thread_id[nb_threads];
	
	/* Get minimal and maximal priority values */
	min_prio = sched_get_priority_min(SCHED_FIFO);
	max_prio = sched_get_priority_max(SCHED_FIFO);
	printf("min = %d - max = %d", min_prio, max_prio );
	max_prio -= min_prio;
	printf("min = %d - max = %d", min_prio, max_prio );
	
	/* Initialize barrier */
	if (pthread_barrier_init(&barr, NULL, nb_threads)) {
		fprintf(stderr, "Could not initialize barrier!\n");
		return EXIT_FAILURE;
	}
	
	/* Set priorities and create threads */
	for (j = 0; j < nb_threads; ++j) {
		// calcul la 1er priorite
		prio_value[j] = min_prio + (j * (max_prio/nb_threads));
		
		// cree les threads
		if ( pthread_create(&thread_id[j], NULL, f_thread, (void*)nb_iterations[j]) == 0) {
			fprintf(stderr, "Could not set priorities\n");
			return EXIT_FAILURE;
		}
		// met les priorites des threads
		if ( pthread_setschedprio(thread_id[j], prio_value[j]) == 0) {
			fprintf(stderr, "Could not set priorities\n");
			return EXIT_FAILURE;
		}
	}
	
	/* Wait for the threads to complete and set the results */
	// int pthread_barrier_wait(pthread_barrier_t *barrier);
	
	
	for (i = 0; i < nb_threads; ++i) {
		fprintf(stdout, "[%02d] %ld (%2.0f%%)\n",
		prio_value[i], nb_iterations[i],
		100.0 * nb_iterations[i] / total_iterations);
	}
	return EXIT_SUCCESS;
}
