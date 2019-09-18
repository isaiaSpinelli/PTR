#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <float.h>

#define n_sec 1

int main(void)
{
	time_t start_time = time(NULL);
	float floatDonne = FLT_MAX;

	unsigned long long nb_iterations = 0;
	while(time(NULL) < (start_time + n_sec)) {
		++nb_iterations;
		floatDonne /= 1.17291;
	}

	printf("Nombre d'iteration par seconde (div float) = %llu\n%f\n", (nb_iterations/n_sec), floatDonne);

    return EXIT_SUCCESS;
}
