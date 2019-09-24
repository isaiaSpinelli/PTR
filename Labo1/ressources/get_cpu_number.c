#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include <utmpx.h>

int sched_getcpu(void);

#define n_sec 12

int main(void)
{
	time_t start_time = time(NULL);

	int CPU_N = sched_getcpu();
	int CPU_N_2 = CPU_N; 
	// Permet d'avoir l'heure courrante
	time_t curtime;
	struct tm *loctime;
	
	printf("CPU = %d (Demarrage)\n", CPU_N);

	while(time(NULL) < (start_time + n_sec)) {
		CPU_N_2 = sched_getcpu();
		if (CPU_N != CPU_N_2) {
			CPU_N = CPU_N_2;
			
			curtime = time (NULL);
			loctime = localtime (&curtime);
			
			printf("CPU = %d (%d:%d:%d)\n", CPU_N, loctime->tm_hour,loctime->tm_min,loctime->tm_sec);
		}
	}
	
    return EXIT_SUCCESS;
}
