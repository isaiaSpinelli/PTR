#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>


time_t start_time ;

// calcul la difference entre les occurences
void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result);

// Fonction du thread
void *f_thread(void *arg)
{
	// Variable pour le nombre de mesure et le temps d'attente
	int nb_mesure = 140000;
	int count = 0;
	int temps_us = 500;
	
	// Structures pour la mesure des différence de temps
	static struct timespec old,new;
	struct timespec res;
	
	// Structure pour l'attente avec nanosleep
	struct timespec tim,tim2;
	tim.tv_sec = 0;
	tim.tv_nsec = temps_us * 1000;

	// Attribue utile pour modifier les param des threads
	int policy;
	int ret;
	struct sched_param param; 

	// Recupere les param du thread
	if ( (ret = pthread_getschedparam(pthread_self(), &policy, &param)) != 0) {
		fprintf(stderr, "Could not get schedparam1 (%d)\n", ret);
		return NULL;
	}
	
	// Modifier la priorite
	param.sched_priority = 99;
	//fprintf(stdout, "set priority = %d\n", param.sched_priority);
	if ( (ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param)) != 0) {
		fprintf(stderr, "Could not set schedparam (%d)\n", ret);
		return NULL;
	}

	
	
	// Attendre une période de temps à l’intérieurd’une boucle.
	
	while(count <= nb_mesure) {
		
		if(nanosleep(&tim , &tim2) < 0 )   
		{
		  printf("Nano sleep system call failed \n");
		  return NULL;
		}
		// Met a jour les temps	
		old = new;
		if( clock_gettime( CLOCK_REALTIME , &new) == -1 ){
			fprintf(stderr, "Clock_gettime\n");
			return NULL;
		}
		
		// Calcul la diff
		timespec_diff(&old,&new,&res);
		

		// si c'est la 1er occurence
		if (count == 0)
			++count;
		else {
			printf ("%ld \n", res.tv_nsec);
			count++;
		}
	
	}
	
	
	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	int err = 0;
	pthread_t threadHautePriorite;
	
	/* Affiche la priorite max du mode SCHED_FIFO*/
	//fprintf(stdout, "Priorite max = %d\n", sched_get_priority_max(SCHED_FIFO));
	
	// cree le thread à haute priorité
	if ( (err = pthread_create(&threadHautePriorite, NULL, f_thread, NULL)) != 0) {
		fprintf(stderr, "Could not create threads (%d)\n", err );
		return EXIT_FAILURE;
	}

	// Attend la fin du thread
	pthread_join( threadHautePriorite, NULL );
	
	
	return EXIT_SUCCESS;
}

// calcul la difference entre les occurences
// source : https://gist.github.com/diabloneo/9619917
void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

