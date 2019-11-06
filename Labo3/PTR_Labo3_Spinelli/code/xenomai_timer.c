/*
 * 
 * Spinelli Isaia 06 Novembre 2019
 * Exercice 3 Labo 3
 * 
*/
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <native/task.h>
#include <native/timer.h>


// Référence : https://xenomai.org/documentation/xenomai-2.4/html/api/trivial-periodic_8c-example.html

// Descripteur de tâche
RT_TASK demo_task;

/* NOTE: error handling omitted. */
void demo(void *arg)
{		// 140'000 * 500 us = 70 secondes
        int count = 140000;
        int err=-1;
        RTIME now, previous;
        
        // cree la tache periodique de 500 us
		if ( (err = rt_task_set_periodic(NULL, TM_NOW, 500000L)) != 0 ) {
			fprintf(stderr, "Erreur rt_task_set_periodic (err=%d)\n", err);
			return;
		}
        
        // init le temps de départ
        previous = rt_timer_read();
        // Tant qu'on a pas fait toutes les mesures
        while (count--) {
			// attend la fin de la période
			if ( (err = rt_task_wait_period(NULL)) != 0 ) {
				fprintf(stderr, "Erreur rt_task_wait_period (err=%d)\n", err);
				return;
			}
			
			// Met a jour le new temps
			now = rt_timer_read();
			
			
			// Affiche le temps	   
			rt_printf("%ld\n", (now - previous));
			
			// Met a jour l'ancien temps
			previous = now;
        }
        
        exit(0);
}

int main(int argc, char* argv[])
{	
		// Permet d'utiliser le rt_printf
        rt_print_auto_init(1);
        // Verouille l'espace d'adressage du processus pour éviter qu'elles ne soient paginées (swapper)
		if ( mlockall(MCL_CURRENT|MCL_FUTURE) != 0 ){
			
		}
	
        /*
         * Arguments: &task,
         *            name,
         *            stack size (0=default),
         *            priority,
         *            mode (FPU, start suspended, ...)
         */
         // Crée la tache
        if ( rt_task_create(&demo_task, "TimerXenomai", 0, 99, 0) != 0 ){
			fprintf(stderr, "Erreur rt_task_create\n");
			return -1;
		}
			
        /*
         * Arguments: &task,
         *            task function,
         *            function argument
         */
         // Start la tache
        if ( rt_task_start(&demo_task, &demo, NULL) != 0 ){
			fprintf(stderr, "Erreur rt_task_start\n");
			return -1;
		}
        
        pause();
        
        rt_task_join(&demo_task);
        
        // Supprime la tache
        if ( rt_task_delete(&demo_task) != 0 ){
			fprintf(stderr, "Erreur rt_task_delete\n");
			return -1;
		}
        return 0;
}
