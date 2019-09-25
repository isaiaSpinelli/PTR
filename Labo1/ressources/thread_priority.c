#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

#define EXECUTION_TIME 10 /* In seconds */

/* Barrier variable */
pthread_barrier_t barr;

// Structure pour chaque thread
struct threadInfo {
   long int nb_iterations;
   int prio_value ;
};

// Nombre total d'iteration
unsigned long long total_iterations = 0;


void *f_thread(void *arg)
{
	//int err = 0;
	long int nb_iterations = 0;
	
	struct threadInfo* threadInfo1 = (struct threadInfo*)arg;  
	//pthread_t thId = pthread_self();
	
	pthread_barrier_wait(&barr);
	
	fprintf(stdout, "Priorite = %d \n", threadInfo1->prio_value  );
	
	// met la priorites du threads
	/*if ( (err = pthread_setschedprio(thId, 10threadInfo1->prio_value)) != 0) {
		fprintf(stderr, "Could not set priorities (%d)\n", err );
		return NULL;
	}*/
	
	time_t start_time = time(NULL);
	while(time(NULL) < (start_time + EXECUTION_TIME)) {
		++nb_iterations;
	}
	
	// met a jour les resultats
	threadInfo1->nb_iterations = nb_iterations;
	total_iterations += nb_iterations;
	// Libere la barriere
	pthread_barrier_wait(&barr);
	
	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	int nb_threads = 0;
	int min_prio,max_prio;
	int i,j;
	int err = 0;
	
	
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
	
	
	pthread_t thread_id[nb_threads];
	struct threadInfo threadInfoTab[nb_threads];
	
	/* Get minimal and maximal priority values */
	min_prio = sched_get_priority_min(SCHED_FIFO);
	max_prio = sched_get_priority_max(SCHED_FIFO); // SCHED_FIFO
	fprintf(stdout, "min = %d - max = %d\n",
		min_prio,
		max_prio);
		
	max_prio -= min_prio;
	
	/* Initialize barrier */
	if (pthread_barrier_init(&barr, NULL, nb_threads+1)) {
		fprintf(stderr, "Could not initialize barrier!\n");
		return EXIT_FAILURE;
	}
	
	/* Set priorities and create threads */
	for (j = 0; j < nb_threads; ++j) {
		// calcul les priorites
		// min_prio + (j * (max_prio/nb_threads));
		threadInfoTab[j].prio_value = min_prio + (j * (max_prio/(nb_threads-1)));
		
		// cree les threads
		if ( (err = pthread_create(&thread_id[j], NULL, f_thread, (void*)&threadInfoTab[j])) != 0) {
			fprintf(stderr, "Could not create threads (%d)\n", err );
			return EXIT_FAILURE;
		}
		
		// met les priorites des threads
		if ( (err = pthread_setschedprio(thread_id[j], 0)) != 0) {
			fprintf(stderr, "Could not set priorities (%d)\n", err );
			//return EXIT_FAILURE;
		}
	}
	
	pthread_barrier_wait(&barr);
	
	fprintf(stdout, "Tous ready\n" );
	
	pthread_barrier_init(&barr, NULL, nb_threads+1);
	
	
	/* Wait for the threads to complete and set the results */
	pthread_barrier_wait(&barr);
	
	// affiche les resultats
	for (i = 0; i < nb_threads; ++i) {
		fprintf(stdout, "[%02d] %ld (%2.0f%%)\n",
		threadInfoTab[i].prio_value, threadInfoTab[i].nb_iterations,
		100.0 * threadInfoTab[i].nb_iterations / total_iterations);
	}
	return EXIT_SUCCESS;
}
