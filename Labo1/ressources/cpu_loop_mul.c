#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define n_sec 10

int main(void)
{
	time_t start_time = time(NULL);
	float floatDonne = 43.34;

	unsigned int nb_iterations = 0;
	while(time(NULL) < (start_time + n_sec)) {
		++nb_iterations;
		floatDonne /= 1 ;
	}

	printf("Nombre d'iteration par seconde (mul float) = %d\n", nb_iterations/n_sec);

    return EXIT_SUCCESS;
}
