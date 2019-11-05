#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>

#include <rtdk.h>
#include <native/task.h>
#include <native/timer.h>
#include <sys/mman.h>

// Référence : https://xenomai.org/documentation/xenomai-2.4/html/api/trivial-periodic_8c-example.html

// Descripteur de tâche
RT_TASK timer_task;
 
// Handler des signaux recus
void handler_signal (void *arg)
{
	int nb_mesure = 10;
	int count = 0;
	RTIME now, previous;
	
	/*
	 * Arguments: &task (NULL=self),
	 *            start time,
	 *            period (here: 500 us)
	 */
	 // cree la tache periodique de 500 us
	rt_task_set_periodic(NULL, TM_NOW, 100);
	// init le temps 
	previous = rt_timer_read();

	while (count++ <= nb_mesure) {
		// attend la fin du compteur
		rt_task_wait_period(NULL);
		// Met a jour les temps
		previous = now;
		now = rt_timer_read();

		/*
		 * NOTE: printf may have unexpected impact on the timing of
		 *       your program. It is used here in the critical loop
		 *       only for demonstration purposes.
		 */
		/*printf("%ld.%06ld ms\n",
			   (long)(now - previous) / 1000000,
			   (long)(now - previous) % 1000000);*/
		
		// si c'est la 1er occurence
		if (count != 1) {
			rt_printf("%ld\n", (long)(now - previous));
		}
			 
		
		
		// Affiche le temps
		//printf("%ld\n", (long)(now - previous));		   
		   
	}
		

	
}

int main(int argc, char **argv)
{
	
	// Verouille l'espace d'adressage du processus pour éviter qu'elles ne soient paginées
	// Evite les échanges de mémoire pour ce programme
	if ( mlockall(MCL_CURRENT|MCL_FUTURE) != 0 ){
		fprintf(stderr, "Erreur mlockall\n");
		return EXIT_FAILURE;
	}
	
	// Permet d'utiliser le rt_printf
	rt_print_auto_init(1);
	
	/*
	 * Arguments: &task,
	 *            name,
	 *            stack size (0=default),
	 *            priority,
	 *            mode (FPU, start suspended, ...)
	 */
	rt_task_create(&timer_task, "TimerXenomai", 0, 99, 0);
	
	// Start la tache
	rt_task_start(&timer_task, &handler_signal, NULL);
	// Attend la tache
	rt_task_join(&timer_task);
	// Supprime la tache
	rt_task_delete(&timer_task);
	
	return EXIT_SUCCESS;
}
