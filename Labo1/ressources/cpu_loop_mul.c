#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define n_sec 1

int main(void)
{
	time_t start_time = time(NULL);
	float floatDonne = 1.1812;

	unsigned long long nb_iterations = 0;
	while(time(NULL) < (start_time + n_sec)) {
		++nb_iterations;
		floatDonne *= 1.2912 ;
	}

	printf("Nombre d'iteration par seconde (mul float) = %llu\n%f\n", (nb_iterations/n_sec), floatDonne);

    return EXIT_SUCCESS;
}
