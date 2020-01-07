#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <cobalt/stdio.h>

#include <alchemy/task.h>
#include <alchemy/timer.h>
#include <alchemy/sem.h>

#include <queue.h>

#include "audio_player_board.h"
#include "audio_utils.h"

#define AUDIO_OUTPUT_TASK_NAME  "Audio output task"
#define SOURCE_TASK_NAME        "Source task"
#define SDCARD_WRITER_TASK_NAME "Sdcard writer task"
#define TIME_DISPLAY_TASK_NAME  "Time display task"
#define CONTROL_TASK_NAME       "Control task"
#define PROCESSING_TASK_NAME    "Processing task"

#define PERIOD_SOURCE_TASK       		2        /**< Période de la tâche snd_task en MS (F = F_ech / Nb_mot_par_canal => Periode = 1/F = 1/ (48000 / 128) = 2.7ms => 2 ms)   */
#define PERIOD_TIME_DISPLAY_TASK       	10        /**< Période de la tâche display task en MS (10 MS est la valeur minimum affichée sur les 7 seg) */
#define PERIOD_CONTROL_TASK       		100        /**< Période de la tâche UI task en MS (100ms est environ la limite minimum du temps de réaction d'un humain)*/

#define PRIO_TASKS       		99        /**< Priorité des taches */
#define STACK_SIZE       		0        /**< Taille des piles des taches */

#define FORWARD_REWIND_VAL 		 1920000		/**< Nombre de data à forward ou rewind du buffer pour 10 secondes (F * Nb_canaux * Nb_byte = 48'000 * 2 * 2 = 1920000) */
#define WORD_PER_PERIOD 		 384			/**< Nombre de mots écrit à chaque période (Déterminé par tests) */

#define MS 1000000		/**< Permet la transformation des periodes(MS) en NS*/

/* Cette tâche est responsable de jouer les données de son en les envoyant au drivergérant l’audio. La sortie audio est toujours active, que le système soit en enregis-trement ou en lecture. */
static RT_TASK audio_output_rt_task;
/* Récupération du flux audio d’entrée ou lecture du fichier précédemment enregis-tré. Cette tâche place ensuite les données dans un buffer lui permettant de com-muniquer avec la tâche processing */
static RT_TASK source_rt_task;
/* Cette tâche est responsable d’écrire dans le fichier les données reçues de la tâcheprocessing */
static RT_TASK sdcard_writer_rt_task;
/* Gestion de l’affichage du temps. */
static RT_TASK time_display_rt_task;
/* Cette tâche contrôle l’ensemble du système, en scrutant les les boutons et en agis-sant en fonction */
static RT_TASK control_rt_task;
/* Cette tâche implémente la modification du volume */
static RT_TASK processing_rt_task;

static RT_QUEUE audio_data_in;	/**< boites aux lettres pour l'envoie des data audio lues depuis un fichier ou le micro */
static RT_QUEUE sd_out;			/**< boites aux lettres pour l'envoie des data audio pour l'écriture sur la carte SD */
static RT_QUEUE audio_out;		/**< boites aux lettres pour l'envoie des data audio pour la sortie audio */

static RT_SEM task_barrier; 	/**< Sémaphore pour attendre le lancement de toutes les tâches */

/* Cette tâche est responsable de jouer les données de son en les envoyant au drivergérant l’audio. La sortie audio est toujours active, que le système soit en enregis-trement ou en lecture. */
void audio_output_task(void *cookie)
{
    rt_sem_p(&task_barrier, TM_INFINITE);
    rt_printf("[%s] stopping\n", __FUNCTION__);
}

/* Récupération du flux audio d’entrée ou lecture du fichier précédemment enregis-tré. Cette tâche place ensuite les données dans un buffer lui permettant de com-muniquer avec la tâche processing */
void source_task(void *cookie)
{
	/* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_SOURCE_TASK)*((RTIME)MS);
	int err = rt_task_set_periodic(&source_rt_task, rt_timer_read() + period, period);
	if (err != 0) {
		printf("source_task set periodic failed: %s\n", strerror(-err));
		return;
	}
	
    rt_sem_p(&task_barrier, TM_INFINITE);
    rt_printf("[%s] stopping\n", __FUNCTION__);
}

/* Cette tâche est responsable d’écrire dans le fichier les données reçues de la tâcheprocessing */
void sdcard_writer_task(void *cookie)
{

    rt_sem_p(&task_barrier, TM_INFINITE);

    /* === Exemple d'écriture d'un fichier wav ===
     * NOTE : n'hésitez pas à adapter ce code en fonction de vos choix et besoins
     * 
    int audio_fd;
    void *audio_buf;
    ssize_t audio_buf_size;
    struct wav_header wh;
    int ret;
    
    wh.file_blkID   = WAV_FILE_BLKID;
    wh.fmt_blkID    = WAV_FMT_BLKID;
    wh.data_blkID   = WAV_DATA_BLKID;
    wh.data_size    = 0;

    write_wav_header(audio_fd, &wh);

    audio_fd = ...
    audio_buf = ...
    audio_buf_size = ...

    ret = append_wav_data(audio_fd, audio_buf, audio_buf_size);
    if (ret) {
        rt_printf("[%s] Couldn't append data\n", __FUNCTION__);
        ...
    }

    === Fin de l'exemple === */

    rt_printf("[%s] stopping\n", __FUNCTION__);
}

/* Cette tâche implémente la modification du volume */
void processing_task(void *cookie)
{
	char* msg;
	int err;
	
    rt_sem_p(&task_barrier, TM_INFINITE);
    
    
    do {
		/* Attend la réception de data audio */
		rt_queue_receive(&audio_data_in, (void **) &msg, TM_INFINITE);
		if (err < 0) {
			perror("rt_queue_receive [s]\n", __FUNCTION__ );
			exit(EXIT_FAILURE);
		}
				
		/* Supprime la data audio précédemment récupérée */
		rt_queue_free(&audio_data_in, msg);
		if (err != 0) {
			printf("rt_queue_free [%s]\n", __FUNCTION__);
			return;
		}
		
		rt_printf("Message recu : [%s]\n", msg);
		
	} while(1);
    
    
    rt_printf("[%s] stopping\n", __FUNCTION__);
}

/* Gestion de l’affichage du temps. */
void time_display_task(void *cookie)
{
	
	/* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_TIME_DISPLAY_TASK)*((RTIME)MS);
	int err = rt_task_set_periodic(&time_display_rt_task, rt_timer_read() + period, period);
	if (err != 0) {
		printf("display_task set periodic failed: %s\n", strerror(-err));
		return;
	}
	
    rt_sem_p(&task_barrier, TM_INFINITE);
    rt_printf("[%s] stopping\n", __FUNCTION__);
}

/* Cette tâche contrôle l’ensemble du système, en scrutant les les boutons et en agis-sant en fonction */
void control_task(void *cookie)
{
	uint32_t switch_val, key_val, old_key_val =0;
	
	char* msg;
	int err;
	
	/* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_CONTROL_TASK)*((RTIME)MS);
	int err = rt_task_set_periodic(&control_rt_task, rt_timer_read() + period, period);
	if (err != 0) {
		printf("display_task set periodic failed: %s\n", strerror(-err));
		return;
	}
	
    rt_sem_p(&task_barrier, TM_INFINITE);
    
    do {
		
		/* Récupère la valeur des boutons */
		key_val = get_buttons();
		
		if (is_pressed_now(old_key_val, key_val, KEY0) ){
				msg = rt_queue_alloc(&audio_data_in, 20   );
				if (msg == NULL) {
					perror("rt_queue_alloc [s]\n", __FUNCTION__ );
					exit(EXIT_FAILURE);
				}
				strcpy(msg, "Hello world !");
				err = rt_queue_send(&audio_data_in, msg, strlen(msg)+1, Q_BROADCAST);
				if (err < 0) {
					perror("rt_queue_send [s]\n", __FUNCTION__ );
					exit(EXIT_FAILURE);
				}
		}
		
		
		/* Met à jour l'ancienne valeur des keys */
        old_key_val = key_val;
        
        /* Attend la prochaine période */
        rt_task_wait_period(NULL);
        
        
	 } while (1);
    
    rt_printf("[%s] stopping\n", __FUNCTION__);
}

int 
main(int argc, char *argv[]) 
{
    int audio_fd, err ;

    if (init_board() != 0) {
        perror("Error at board initialization.");
        exit(EXIT_FAILURE);
    }
	
    if (argc != 2) {
        printf("Not enough arguments!\n");
        exit(EXIT_FAILURE);
    }

    /* Ouverture du fichier audio */
    audio_fd = open(argv[1], O_RDWR|O_CREAT);
    if (audio_fd < 0) {
        perror("Opening audio file");
        exit(EXIT_FAILURE);
    }

    /* Setup task barrier */
    if (rt_sem_create(&task_barrier, "Task barrier", 0, S_FIFO)) {
        perror("Creating task barrier");
        exit(EXIT_FAILURE);
    }

    mlockall(MCL_CURRENT | MCL_FUTURE);
    
    /* Création des 3 boites aux lettres permettant l’échange des données sonores.*/
    err = rt_queue_create(&audio_data_in, "audio_data_in", 1024, Q_UNLIMITED, Q_FIFO); // 1024 / 16 olieu de Q_UNLI...
    if (err != 0) {
		perror("rt_queue_create error [s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
    err = rt_queue_create(&sd_out, "sd_out", 1024, Q_UNLIMITED, Q_FIFO);
    if (err != 0) {
		perror("rt_queue_create error [s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
    err = rt_queue_create(&audio_out, "audio_out", 1024, Q_UNLIMITED, Q_FIFO);
    if (err != 0) {
		perror("rt_queue_create error [s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
    
    /* Création des différentes tâches */
    rt_task_spawn(&source_rt_task, SOURCE_TASK_NAME, STACK_SIZE, PRIO_TASKS, T_JOINABLE, source_task, NULL);
    rt_task_spawn(&time_display_rt_task, TIME_DISPLAY_TASK_NAME, STACK_SIZE, PRIO_TASKS, T_JOINABLE, time_display_task, NULL);
    rt_task_spawn(&control_rt_task, CONTROL_TASK_NAME, STACK_SIZE, PRIO_TASKS, T_JOINABLE, control_task, NULL);
    rt_task_spawn(&audio_output_rt_task, AUDIO_OUTPUT_TASK_NAME, STACK_SIZE, PRIO_TASKS, T_JOINABLE, audio_output_task, NULL);
    rt_task_spawn(&processing_rt_task, PROCESSING_TASK_NAME, STACK_SIZE, PRIO_TASKS, T_JOINABLE, processing_task, NULL);
    rt_task_spawn(&sdcard_writer_rt_task, SDCARD_WRITER_TASK_NAME, STACK_SIZE, PRIO_TASKS, T_JOINABLE, sdcard_writer_task, NULL);
	
	/* Reveil toutes les tâches après les avoir toutes crées */
    rt_sem_broadcast(&task_barrier);

    rt_task_join(&time_display_rt_task);
    rt_task_join(&control_rt_task);
    rt_task_join(&source_rt_task);
    rt_task_join(&audio_output_rt_task);
    rt_task_join(&processing_rt_task);
    rt_task_join(&sdcard_writer_rt_task);
    
    
    /* Suppression des 3 boites aux lettres */
    err = rt_queue_delete(&audio_data_in);
    if (err != 0) {
		perror("rt_queue_delete error [s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
    err = rt_queue_delete(&sd_out);
    if (err != 0) {
		perror("rt_queue_delete error [s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
    err = rt_queue_delete(&audio_out);
    if (err != 0) {
		perror("rt_queue_delete error [s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}

    clean_board();
    close(audio_fd);

    munlockall();

    return EXIT_SUCCESS;
}
