#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

#define EXECUTION_TIME 5 /* In seconds */

/* Barrier variable */
pthread_barrier_t barr;

time_t start_time ;




void *f_thread(void *arg)
{
	int nb_mesure = 1000;
	int count = 0;
	int temps_us = 500;
	
	struct timespec tim,tim2;
	tim.tv_sec = 0;
	tim.tv_nsec = temps_us * 1000;

	// attribue utile pour modifier les param des threads
	int policy;
	int ret;
	struct sched_param param; 
	
	printf("nice\n");

	// recupere et modifie les param des threads
	if ( (ret = pthread_getschedparam(pthread_self(), &policy, &param)) != 0) {
		fprintf(stderr, "Could not get schedparam1 (%d)\n", ret);
		return NULL;
	}
	
	printf("P1=%d\n", param.sched_priority);
	// essaie de modifier la priorite
	param.sched_priority = 2;
	
	if ( (ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param)) != 0) {
		fprintf(stderr, "Could not set schedparam2 (%d)\n", ret);
		//return NULL;
	}
	printf("P2=%d\n", param.sched_priority);
	
	// met les priorites des threads
	if ( (ret = pthread_setschedprio(pthread_self(), 19)) != 0) {
		fprintf(stderr, "Could not set priorities (%d) (P=%d)\n", ret, 19);
		//return EXIT_FAILURE;
	}

	printf("P3=%d\n", param.sched_priority);/*
	// attendre une période de temps à l’intérieurd’une boucle.
	
	while(count++ < nb_mesure) {
		
		if(nanosleep(&tim , &tim2) < 0 )   
		{
		  printf("Nano sleep system call failed \n");
		  return NULL;
		}

		printf("Nano sleep successfull %d\n",count);
		
	}*/
	
	
	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	int err = 0;
	
	pthread_t threadHautePriorite;

	// Modifie les param du thread crees
	pthread_attr_t attr;
	
	pthread_attr_init (&attr);
	
	printf("1\n");
	
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
	
	printf("2\n");
	
	// cree le thread à haute priorité
	if ( (err = pthread_create(&threadHautePriorite, NULL, f_thread, NULL)) != 0) {
		fprintf(stderr, "Could not create threads (%d)\n", err );
		return EXIT_FAILURE;
	}
		
	printf("3\n");
	
	start_time = time(NULL);


	
	pthread_join( threadHautePriorite, NULL );
	
	printf("4\n");
	
	
	return EXIT_SUCCESS;
}
