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
	long int nb_iterations = 0;
	struct threadInfo* threadInfo1 = (struct threadInfo*)arg;  

	// attribue utile pour modifier les param des threads
	int policy;
	int ret;
	struct sched_param param; 

	// recupere et modifie les param des threads
	if ( (ret = pthread_getschedparam(pthread_self(), &policy, &param)) != 0) {
		fprintf(stderr, "Could not get schedparam (%d)\n", ret);
		return NULL;
	}
	// essaie de modifier la priorite
	param.sched_priority = threadInfo1->prio_value;
	fprintf(stdout, "set priority = %d\n", threadInfo1->prio_value );
	if ( (ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param)) != 0) {
		fprintf(stderr, "Could not set schedparam (%d)\n", ret);
		return NULL;
	}
	
	// met les priorites des threads
	if ( (ret = pthread_setschedprio(pthread_self(), threadInfo1->prio_value)) != 0) {
		fprintf(stderr, "Could not set priorities (%d) (P=%d)\n", ret, threadInfo1->prio_value);
		//return EXIT_FAILURE;
	}
	// attend les autres threads
	pthread_barrier_wait(&barr);
	// compte les iterations
	time_t start_time = time(NULL);
	while(time(NULL) < (start_time + EXECUTION_TIME)) {
		++nb_iterations;
	}
	
	// met a jour le resultat
	threadInfo1->nb_iterations = nb_iterations;
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

	// Test de modifier les param des threads crees
	struct sched_param param;
	pthread_attr_t attr;
	
	pthread_attr_init (&attr);
	
	if ( (err = pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED)) != 0) {
		fprintf(stderr, "Could not set detachstate (%d)\n", err );
		return EXIT_FAILURE;
	}
	
	if ( (err = pthread_attr_setinheritsched (&attr, PTHREAD_INHERIT_SCHED)) != 0) {
		fprintf(stderr, "Could not set inheritsched (%d)\n", err );
		return EXIT_FAILURE;
	}
	
	if ( (err = pthread_attr_setschedpolicy (&attr, SCHED_FIFO)) != 0) {
		fprintf(stderr, "Could not set schedpolicy (%d)\n", err );
		return EXIT_FAILURE;
	}
	
	
		
	
	
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
		int denominateur = (nb_threads-1) == 0 ? 1 : (nb_threads-1);
		threadInfoTab[j].prio_value = min_prio + (j * (max_prio/(denominateur)));
		
		param.sched_priority = threadInfoTab[j].prio_value;
		pthread_attr_setschedparam (&attr, &param);
		
		// cree les threads
		if ( (err = pthread_create(&thread_id[j], &attr, f_thread, (void*)&threadInfoTab[j])) != 0) {
			fprintf(stderr, "Could not create threads (%d)\n", err );
			return EXIT_FAILURE;
		}
		
		// met les priorites des threads
		/*if ( (err = pthread_setschedprio(thread_id[j], threadInfoTab[j].prio_value)) != 0) {
			fprintf(stderr, "Could not set priorities (%d) (P=%d)\n", err, threadInfoTab[j].prio_value);
			//return EXIT_FAILURE;
		}*/
	}
	
	pthread_barrier_wait(&barr);
	fprintf(stdout, "Tous ready\n" );
	
	pthread_barrier_init(&barr, NULL, nb_threads+1);
	
	/* Wait for the threads to complete and set the results */
	pthread_barrier_wait(&barr);
	fprintf(stdout, "Tous fini\n" );
	
	// compte les iteration totals
	for (i = 0; i < nb_threads; ++i) {
		total_iterations += threadInfoTab[i].nb_iterations;
	}
	
	// affiche les resultats
	for (i = 0; i < nb_threads; ++i) {
		fprintf(stdout, "[%02d] %ld (%2.0f%%)\n",
		threadInfoTab[i].prio_value, threadInfoTab[i].nb_iterations,
		100.0 * threadInfoTab[i].nb_iterations / total_iterations);
	}
	return EXIT_SUCCESS;
}
