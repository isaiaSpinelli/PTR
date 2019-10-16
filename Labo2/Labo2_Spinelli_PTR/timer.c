#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>

// macro afin d'afficher une erreur
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)

// variable global
int count = 0;

// calcul la difference entre les occurences
void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result);
 
// Handler des signaux recus
void handler_signal (int signum)
{
	static struct timespec old,new;
	struct timespec res;
	
	// met a jour les temps	
	old = new;
	if( clock_gettime( CLOCK_REALTIME , &new) == -1 )
		errExit("clock_gettime");
	// calcul la diff
	timespec_diff(&old,&new,&res);
	
	// si c'est la 1er occurence
	if (count == 0)
		++count;
	else {
		printf ("%ld \n", res.tv_nsec);
		count++;
		//printf ("%d : %ld.%09ld \n", count++, res.tv_sec, res.tv_nsec);
	}
		
		
	
}

int main(int argc, char **argv)
{
	
	unsigned long long nsec = 0;
	
	struct itimerspec spec;
	struct sigevent event;
	timer_t timer;
	int nb_mesure = 0;
	long temps_us = 0;
	
	
	/* Parse input */
	if (argc != 3) {
		fprintf(stderr, "Usage: %s NB_MESURE TEMPS_US\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	nb_mesure = strtoimax(argv[1], (char **)NULL, 10);
	temps_us  = strtoimax(argv[2], (char **)NULL, 10);
	
	if (nb_mesure <= 0) {
		fprintf(stderr, "NB_MESURE must be > 0 (actual: %d)\n", nb_mesure);
		return EXIT_FAILURE;
	}
	
	if (temps_us <= 0) {
		fprintf(stderr, "TEMPS_US must be > 0 (actual: %ld)\n", temps_us);
		return EXIT_FAILURE;
	}
	
	// Configure le timer
	if ( signal(SIGRTMIN, handler_signal) == SIG_ERR ) 
		errExit("signal");
		
		
	event.sigev_notify = SIGEV_SIGNAL;
	event.sigev_signo = SIGRTMIN;
	nsec = temps_us * 1000; // en nanosec
	spec.it_interval.tv_sec = nsec / 1000000000;
	spec.it_interval.tv_nsec = nsec % 1000000000;
	spec.it_value = spec.it_interval;

	// Alloue le timer
	if ( timer_create(CLOCK_REALTIME, &event, &timer) != 0)
		errExit("timer_create");
	
	// Programme le timer
	if ( timer_settime(timer, 0, &spec, NULL) != 0 ) 
		errExit("timer_settime");
	

	/* Do busy work. */
	while (count < nb_mesure+1);
	
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
