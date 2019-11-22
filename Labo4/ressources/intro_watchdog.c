
/**
* \file intro_watchdog.c
* \brief Application pour l'introduction aux concept de tâche.
* \author Y. Thoma
* \version 0.3
* \date 08 novembre 2019
*
* Ce laboratoire permet de tester les mécanismes de base
* de l'exécutif Xenomai pour la création et la gestion
* de tâches temps-réel. Il met également en place un
* watchdog qui pourrait être exploité s'il n'existe pas de détection
* de surcharge au niveau du noyau.
*
* Code Xenomai 3.0
* 
* Modif : Spinelli Isaia :
* Modification de periodicTask2 afin d'augmenter la charge CPU (+20) de cette tâche 
* à chaque fois qu'elle se sera executée 20 fois. Pour faire simple ces valeurs sont 
* constante (#DEFINE).
*
*/

#include <sys/mman.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include <alchemy/task.h>
#include <alchemy/timer.h>
#include <alchemy/sem.h>

#include "general.h"
#include "busycpu.h"
#include "watchdog.h"


#define PERIOD_TASK1       1000        /**< Période de la tâche 1 */
#define PERIOD_TASK2       200         /**< Période de la tâche 2 */
#define PERIOD_TASK3       4000        /**< Période de la tâche 3 */

#define CPU_TASK1          50          /**< Temps de traitement de la tâche 1 */
#define CPU_TASK2          100         /**< Temps de traitement de la tâche 2 */
#define CPU_TASK3          100         /**< Temps de traitement de la tâche 3 */

#define PRIO_TASK1         10    /**< Priorité de la tâche 1=faible, 99=forte */
#define PRIO_TASK2         15    /**< Priorité de la tâche 1=faible, 99=forte */
#define PRIO_TASK3         20    /**< Priorité de la tâche 1=faible, 99=forte */

#define CHARGE_CPU_TASK2_ADD        20    /**< Augmentation de la charge CPU de la tâche 2 après N_FOIS_TASK2 execution de la tâche 2*/
#define N_FOIS_TASK2         		20    /**< Nombre de fois de l'execution de la tâche 2 avant l'ajout de la charge CPU */

RT_TASK myTask1;  /**< Descripteur de la tâche périodique 1 */
RT_TASK myTask2;  /**< Descripteur de la tâche périodique 2 */
RT_TASK myTask3;  /**< Descripteur de la tâche périodique 3 */

int installed_task1 = 0; /**< Indique si la tâche 1 est chargée dans le module */
int installed_task2 = 0; /**< Indique si la tâche 2 est chargée dans le module */
int installed_task3 = 0; /**< Indique si la tâche 3 est chargée dans le module */

RT_SEM barrier; //*< Sémaphore pour une barrière de synchro */

void cleanup_objects(void);

void suspend(void)
{
  rt_printf("Suspending the tasks\n");
  if (installed_task1) {
    rt_printf("Suspending task 1\n");
    rt_task_suspend(&myTask1);
  }
  if (installed_task2) {
    rt_printf("Suspending task 2\n");
    rt_task_suspend(&myTask2);
  }
  if (installed_task3) {
    rt_printf("Suspending task 3\n");
    rt_task_suspend(&myTask3);
  }
  return;
}



/** \brief Tâche périodique
*
* La fonction  busy_cpu(..) permet de consommer (inutilement) du temps CPU;
* utile pour simuler des charges!
* La fonction rt_printf() permet d'afficher quelque chose sur la console.
* Elle s'utilise comme la fonction rt_printf()
* \param cookie Non utilisé
*/
void periodicTask1(void *cookie) {

  rt_sem_p(&barrier, TM_INFINITE);

  /* Configuration de la tâche périodique */
  RTIME period = ((RTIME)PERIOD_TASK1)*((RTIME)MS);
  int err = rt_task_set_periodic(&myTask1, rt_timer_read() + period, period);
  if (err != 0) {
    printf("task 1 set periodic failed: %s\n", strerror(-err));
    return;
  }
  else {
    rt_printf("task 1 set periodic OK\n");
  }

  rt_printf("Starting periodic task 1\n", TM_NOW);

  while (1) {

    /* simulation traitement */
    rt_printf("Periodic task 1: starts execution\n");
    busy_cpu(CPU_TASK1);
    rt_printf("Periodic task 1: end of execution\n");

    rt_task_wait_period(NULL);
  }

}



/** \brief Tâche périodique
*
* La fonction  busy_cpu(..) permet de consommer (inutilement) du temps CPU;
* utile pour simuler des charges!
* La fonction rt_printf() permet d'afficher quelque chose sur la console.
* Elle s'utilise comme la fonction rt_printf()
* \param cookie Non utilisé
*/
void periodicTask2(void *cookie) {
	/* Compteur afin d'augmenter la surcharge après un certain temps*/
	static unsigned long compteurSurcharge = 0;
	static unsigned long cpu_Charge = CPU_TASK2;

  rt_sem_p(&barrier, TM_INFINITE);

  /* Configuration de la tâche périodique */
  RTIME period = ((RTIME)PERIOD_TASK2)*((RTIME)MS);
  int err = rt_task_set_periodic(&myTask2, rt_timer_read() + period, period);
  if (err != 0) {
    printf("task 2 set periodic failed: %s\n", strerror(-err));
    return;
  }
  else {
    rt_printf("task 2 set periodic OK\n");
  }

  rt_printf("Starting periodic task 2\n", TM_NOW);

  while (1) {

    /* simulation traitement */
    rt_printf("Periodic task 2: starts execution\n");
    /* Après l'execution de N_FOIS_TASK2 fois de la tâche 2, augmente sa charge CPU de CHARGE_CPU_TASK2_ADD */
    if ( ++compteurSurcharge > N_FOIS_TASK2) {
		rt_printf("SURCHARGE task 2 ! (+%d)\n",CHARGE_CPU_TASK2_ADD);
		cpu_Charge += CHARGE_CPU_TASK2_ADD;
		compteurSurcharge = 0;
	}
    busy_cpu(cpu_Charge);
    rt_printf("Periodic task 2: end of execution\n");

    rt_task_wait_period(NULL);
  }

}

/** \brief Tâche périodique
*
* La fonction  busy_cpu(..) permet de consommer (inutilement) du temps CPU;
* utile pour simuler des charges!
* La fonction rt_printf() permet d'afficher quelque chose sur la console.
* Elle s'utilise comme la fonction rt_printf()
* \param cookie Non utilisé
*/
void periodicTask3(void *cookie) {

  rt_sem_p(&barrier, TM_INFINITE);

  /* Configuration de la tâche périodique */
  RTIME period = ((RTIME)PERIOD_TASK3)*((RTIME)MS);
  int err = rt_task_set_periodic(&myTask3, rt_timer_read() + period, period);
  if (err != 0) {
    printf("task 3 set periodic failed: %s\n", strerror(-err));
    return;
  }
  else {
    rt_printf("task 3 set periodic OK\n");
  }


  rt_printf("Starting periodic task 3\n", TM_NOW);

  while (1) {

    /* simulation traitement */
    rt_printf("Periodic task 3: starts execution\n");
    busy_cpu(CPU_TASK3);
    rt_printf("Periodic task 3: end of execution\n");

    rt_task_wait_period(NULL);
  }

}

/** \brief Routine appelée au chargement du module
*
* Cette routine est appelé lors du chargement du module dans l'espace noyau.
* C'est le point d'entrée de notre application temps-réel.
*/
int main(int argc, char* argv[]) {

  int err; /* stockage du code d'erreur */

  printf("Initialisation\n");

  /* Avoids memory swapping for this program */
  mlockall(MCL_CURRENT|MCL_FUTURE);

  if (!start_watchdog(suspend))
    goto fail;

  rt_sem_create(&barrier, "sem", 0, S_PULSE);

  /* Création de la tâche périodique 1 */
  err =  rt_task_spawn (&myTask1, "myTask1", STACK_SIZE, PRIO_TASK1, 0, periodicTask1, 0);
  if (err != 0) {
    printf("task 1 creation failed: %s\n", strerror(-err));
    goto fail;
  }
  else {
    installed_task1=1;
  }

  /* Création de la tâche périodique 2 */
  err =  rt_task_spawn (&myTask2, "myTask2", STACK_SIZE, PRIO_TASK2, 0, periodicTask2, 0);
  if (err != 0) {
    printf("task 2 creation failed: %s\n", strerror(-err));
    goto fail;
  }
  else {
    installed_task2=1;
  }

  /* Création de la tâche périodique 3 */
  err =  rt_task_spawn (&myTask3, "myTask3", STACK_SIZE, PRIO_TASK3, 0, periodicTask3, 0);
  if (err != 0) {
    printf("task 3 creation failed: %s\n", strerror(-err));
    goto fail;
  }
  else {
    installed_task3=1;
  }

  /* Relâchement des tâches. Le broadcast fonctionne car les tâches sont plus prioritaires
     que le main(). Si elles ne le sont pas le broadcast n'est pas sûr. */
  rt_sem_broadcast(&barrier);

  pause();

  return 0;
  
  fail:
    cleanup_objects();
    return -1;
}

/** \brief Routine appelée lors d'un échec de l'initialisation
*
* Routine de nettoyage des objets alloués.
* Cette routine est appelée lorsque l'initialisation a échoué.
*/
void cleanup_objects (void) {

  printf("intro_watchdog: objects destruction\n");

  /* Destruction du watchdog */
  end_watchdog();

  /* Destruction des objets dans l'ordre inverse de leur création
  * afin d'éviter tout problème de dépendance entre les objets */
  if (installed_task3)
    rt_task_delete(&myTask3);
  if (installed_task2)
    rt_task_delete(&myTask2);
  if (installed_task1)
    rt_task_delete(&myTask1);

  printf("intro_watchdog : bye bye!\n");
}
