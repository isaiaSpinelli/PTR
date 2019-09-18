#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <float.h>

#define n_sec 1

int main(void)
{
	time_t start_time = time(NULL);

	unsigned long long nb_iterations = 0;
	while(time(NULL) < (start_time + n_sec)) {
		++nb_iterations;
	}

	printf("Nombre d'iteration par seconde = %llu\n", (nb_iterations/n_sec));
	
	start_time = time(NULL);
	float floatDonne = FLT_MAX;
	nb_iterations = 0;
	while(time(NULL) < (start_time + n_sec)) {
		++nb_iterations;
		floatDonne *= 0.97323321321 ;
	}

	printf("Nombre d'iteration par seconde (mul float) = %llu\n%f\n", (nb_iterations/n_sec), floatDonne);


	start_time = time(NULL);
	floatDonne = FLT_MAX;
	nb_iterations = 0;
	while(time(NULL) < (start_time + n_sec)) {
		++nb_iterations;
		floatDonne /= 1.1347;
	}

	printf("Nombre d'iteration par seconde (div float) = %llu\n%f\n", (nb_iterations/n_sec), floatDonne);
    return EXIT_SUCCESS;
}
