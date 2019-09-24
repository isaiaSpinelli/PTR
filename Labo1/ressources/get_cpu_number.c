#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include <utmpx.h>

int sched_getcpu(void);

#define sec_interval 1
#define n_sec 12

int main(void)
{
	time_t start_time = time(NULL);
	int i = 1;
	
	printf("CPU = %d (Demarrage)\n", sched_getcpu());

	while(time(NULL) < (start_time + n_sec)) {
		if (time(NULL) > start_time + (i*sec_interval)) {
			i++;
			printf("CPU = %d\n", sched_getcpu());
		}
	}
	
    return EXIT_SUCCESS;
}
