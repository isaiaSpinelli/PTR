#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
// new include necessaire 
#include <time.h>

#define NB_MESURES 30

int main (int argc, char** argv) {
	
	// tableau de structure qui contient des secondes et des nano secondes
	struct timespec ts[NB_MESURES];
	int i;
	
	for(i = 0; i < NB_MESURES; ++i) {
		
		// recupère le temps en fonction de la clock passe en paramètre :
		// CLOCK_REALTIME, CLOCK_MONOTONIC, CLOCK_PROCESS_CPUTIME_ID, CLOCK_THREAD_CPUTIME_ID
		if( clock_gettime( CLOCK_MONOTONIC_HR , &ts[i]) == -1 ) {
			perror( "clock gettime" );
			exit( EXIT_FAILURE );
		}	
		// affiche les resultats (seconde.nanoseconde)
		printf("%2d : %ld.%06ld\n", i, ts[i].tv_sec, ts[i].tv_nsec);
	}
		
	return EXIT_SUCCESS;
		
}
