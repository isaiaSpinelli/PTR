
/**
* \file watchdog.c
* \brief Fonctions permettant la mise en place d'un watchdog.
* \author Y. Thoma & D. Molla
* \version 0.1
* \date 08 novembre 2019
*
* Le watchdog proposé est implémenté à l'aide de 2 tâches responsables
* de détecter une surcharge du processeur. Si tel est le cas, une fonction
* fournie par le développeur est appelée. Elle devrait suspendre ou détruire
* les tâches courantes, afin de redonner la main à l'OS.
*
* Code Xenomai 3.1
*
*/

#include <alchemy/task.h>
#include <alchemy/alarm.h>
#include <alchemy/timer.h>

#include "watchdog.h"
#include "general.h"

/**< Période des différentes taches en MS */
#define PERIOD_TASK_CANARI 			1000        		/**< Période de la tâche du canari */
#define PERIOD_TASK_WATCHDOG    	1000         	/**< Période de la tâche du watchdog */

/**< Priorité des tâches :  1=faible, 99=forte */
#define PRIO_TASK_CANARI         	1    		/**< Priorité de la tâche du canari */
#define PRIO_TASK_WATCHDOG       	99    		/**< Priorité de la tâche du watchdog */

RT_TASK canariTask;  	/**< Descripteur de la tâche canari */
RT_TASK watchdogTask;  	/**< Descripteur de la tâche watchdog */

unsigned long countCanari = 0;

void(*watchdog_suspend_function)(void) = NULL; /**< Fonction appelée lors d'une surcharge */


/** \brief Tâche périodique de très basse priorité et de période inférieur au watchdog signale son activité à chaque période.
*
* 
* \param cookie Non utilisé
*/
void canari(void *cookie) {

  /* Configuration de la tâche périodique */
  RTIME period = ((RTIME)PERIOD_TASK_CANARI)*((RTIME)MS);
  int err = rt_task_set_periodic(&canariTask, rt_timer_read() + period, period);
  if (err != 0) {
    printf("task canari set periodic failed: %s\n", strerror(-err));
    return;
  }

  rt_printf("Starting periodic task canari\n", TM_NOW);

  
  while (1) {

    /* Indique que le canari est actif à chaque période */
    rt_printf(" ************************ CANARI EXECUTION %lu ************************\n", countCanari++);

    rt_task_wait_period(NULL);
  }

}

/** \brief Tâche périodique de très haute priorité et de période supérieur au canari vérifie à chaque période si le canari est encore en vie.
*
* 
* \param cookie Non utilisé
*/
void watchdog(void *cookie) {

	static unsigned long check = -1;
	/* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_TASK_WATCHDOG)*((RTIME)MS);
	int err = rt_task_set_periodic(&watchdogTask, rt_timer_read() + period, period);
	if (err != 0) {
		printf("task watchdog set periodic failed: %s\n", strerror(-err));
		return;
	}

	rt_printf("Starting periodic task watchdog\n", TM_NOW);


	while (1) {
		
		if (check == countCanari) {
			rt_printf("************************ ERROR ************************\n");
			watchdog_suspend_function();
		}
		check = countCanari;
		/* Vérifie à chaque période si le canari est encore en vie */
		rt_printf("************************ CANARI ALIVE ************************\n");

		rt_task_wait_period(NULL);
	}

}


/** \brief Démarre le watchdog
*
* Le watchdog lance deux tâches. Une tâche de faible priorité qui incrémente
* un compteur, et une de priorité maximale mais de période moindre qui
* vérifie que le compteur s'est incrémenté. Si tel n'est pas le cas,
* la fonction passée en paramètre est appelée. Elle devrait contenir du code
* responsable de suspendre ou supprimer les tâches courantes.
* \param suspend_func Fonction appelée par le watchdog en cas de surcharge
*/
int start_watchdog(void(* suspend_func)(void))
{
	int err; /* stockage du code d'erreur */
	
	watchdog_suspend_function = suspend_func;
  
	/* Création du canari */
	err =  rt_task_spawn (&canariTask, "canari", STACK_SIZE, PRIO_TASK_CANARI, 0, canari, 0);
	if (err != 0) {
		printf("task canari failed: %s\n", strerror(-err));
		return -err;
	}
  
  
	/* Création du watchdog */
	err =  rt_task_spawn (&watchdogTask, "watchdog", STACK_SIZE, PRIO_TASK_WATCHDOG, 0, watchdog, 0);
	if (err != 0) {
		printf("task watchdog failed: %s\n", strerror(-err));
		return -err;
	}

  return 1;
}


/** \brief Termine les deux tâches du watchdog
*
* Cette fonction doit être appelée par la fonction de cleanup du module
* afin de libérer les ressources allouées par le watchdog.
* En l'occurence, les deux tâches du watchdog sont supprimées.
* Elle peut aussi être appelée à n'importe quel instant pour supprimer
* la surveillance du watchdog.
*/
int end_watchdog(void)
{

  return 1;
}


