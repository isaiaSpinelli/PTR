/*
 * Auteurs : Gerardi Tommy et Spinelli Isaia
 * But: Lecteur audio capable de jouer des fichiers wav.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cobalt/stdio.h>
#include <alchemy/task.h>
#include <alchemy/timer.h>
#include <event.h>

#include "audio_player_board.h"


#define PERIOD_SND_TASK       		2        /**< Période de la tâche snd_task en MS (F = F_ech / Nb_mot_par_canal => Periode = 1/F = 1/ (48000 / 128) = 2.7ms => 2 ms)   */
#define PERIOD_DISPLAY_TASK       	10        /**< Période de la tâche display task en MS (10 MS est la valeur minimum affichée sur les 7 seg) */
#define PERIOD_UI_TASK       		150        /**< Période de la tâche UI task en MS (150ms est environ la limite minimum du temps de réaction d'un humain)*/

#define PRIO_TASKS       		99        /**< Priorité des taches */
#define STACK_SIZE       		0        /**< Taille des piles des taches */

#define FORWARD_REWIND_VAL 		 1920000		/**< Nombre de data à forward ou rewind du buffer pour 10 secondes (F * Nb_canaux * Nb_byte = 48'000 * 2 * 2 = 1920000) */
#define WORD_PER_PERIOD 		 384			/**< Nombre de mots écrit à chaque période (Déterminé par tests) */

#define MS 1000000		/**< Permet la transformation des periodes(MS) en NS*/


#define SND_STOP_MASK       			0x1        /**< Masque pour les evenements (STOP)   */
#define SND_REWIND_MASK       			0x2        /**< Masque pour les evenements (REWIND)  */
#define SND_FORWARD_MASK       			0x4        /**< Masque pour les evenements (FORWARD) */

#define DISPLAY_STOP_MASK       		0x10        /**< Masque pour les evenements (STOP)   */
#define DISPLAY_REWIND_MASK       		0x20        /**< Masque pour les evenements (REWIND)  */
#define DISPLAY_FORWARD_MASK       		0x40        /**< Masque pour les evenements (FORWARD) */


static unsigned int mode = PLAYING; /**< Permet de savoir dans quel mode le lecteur se trouve */
static char Vol = VOL_MIDDLE;   /**< Volume du lecteur */

struct snd_task_args {
    struct wav_file wf;
};
/* Tache pour la lecture du son */
static RT_TASK snd_rt_task;
/* Tache pour l'affichage du temps du son */
static RT_TASK display_rt_task;
/* Tache pour la gestion des boutons */
static RT_TASK ui_rt_task;

/* Evenements pour la sychronisation des taches */
static RT_EVENT evenement;

/**
 * @brief Gestion de l'envoie du son
 * @param poiteur sur les arguments
 * @return void
 */
void snd_task(void *cookie)
{
    struct snd_task_args *args = (struct snd_task_args*)cookie;
    size_t to_write = args->wf.wh.data_size;
    
    ssize_t write_ret;
    void *audio_datas_p = args->wf.audio_datas;

    unsigned int mask_r = 0;
    /* Permet de modifier le volume des samples */
    int i=0;
    int16_t sample_volume[WORD_PER_PERIOD];
	
    /* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_SND_TASK)*((RTIME)MS);
	int err = rt_task_set_periodic(&snd_rt_task, rt_timer_read() + period, period);
	if (err != 0) {
		printf("snd_rt_task set periodic failed: %s\n", strerror(-err));
		return;
	}
	
  
    do {
		
		/* Prepare les datas avec le bon volume */
		for (i=0; i < WORD_PER_PERIOD; ++i){ 
			sample_volume[i] = map_volume(* ((int16_t*)(audio_datas_p)+i) , Vol);
		}
		
		/* Envoie les data */
       write_ret = write_samples(sample_volume , to_write);
  

		/* S'il y a une erreur */
        if (write_ret < 0) {
            rt_printf("Error writing to sound driver\n");
            break;
        }
		/* Met à jour les datas */
        to_write -= write_ret;
        audio_datas_p += write_ret;
        /* Attend une période */
        rt_task_wait_period(NULL);
        
   
   
		/* Si on veut stopper la lecture de la musique */ 
		rt_event_wait(&evenement, SND_STOP_MASK, &mask_r, EV_ALL, TM_NONBLOCK);
		if ( (mask_r & SND_STOP_MASK) == SND_STOP_MASK ){
			/* Clear */
			mask_r &= ~SND_STOP_MASK;
			rt_event_clear(&evenement, SND_STOP_MASK, NULL);
			/* Remet à 0 les datas */
			to_write = args->wf.wh.data_size;
			audio_datas_p = args->wf.audio_datas;
		}

		/* S'il faut rewind la musique */
		rt_event_wait(&evenement, SND_REWIND_MASK, &mask_r, EV_ANY, TM_NONBLOCK);
		if ( (mask_r & SND_REWIND_MASK) == SND_REWIND_MASK ){
			/* Clear */
			mask_r &= ~SND_REWIND_MASK;
			rt_event_clear(&evenement, SND_REWIND_MASK, NULL);
			/* Si la musique ne vas pas boucler */
			if ((args->wf.wh.data_size - to_write) >= FORWARD_REWIND_VAL ) { 
				to_write += FORWARD_REWIND_VAL;
				audio_datas_p -= FORWARD_REWIND_VAL;
			} else { /* Met au début de la musique */
				to_write = args->wf.wh.data_size;
				audio_datas_p = args->wf.audio_datas;
			}
		}

		/* S'il faut forward la musique */
		rt_event_wait(&evenement, SND_FORWARD_MASK, &mask_r, EV_ANY, TM_NONBLOCK);
		if ( (mask_r & SND_FORWARD_MASK) == SND_FORWARD_MASK ){
			/* Clear */
			mask_r &= ~SND_FORWARD_MASK;
			rt_event_clear(&evenement, SND_FORWARD_MASK, NULL);
			/* Si la musique ne vas se finir  */
			if (to_write >= FORWARD_REWIND_VAL) {
				to_write -= FORWARD_REWIND_VAL;
				audio_datas_p += FORWARD_REWIND_VAL;
			} else { 
				/* Met à la fin de la musique */
				to_write = 0;
			}
		}

		 /* Si on est plus dans le mode playing*/
		if (mode != PLAYING){
			rt_task_suspend(NULL);
		}
		
    } while (to_write);
    
    rt_printf("FIN DU FICHIER !!\n\n");
    mode = END;
    
}

/**
 * @brief Gestion de l'affichage du lecteur
 * @param poiteur sur les arguments (non utilisé => NULL)
 * @return void
 */
void display_task(void *cookie)
{
	unsigned int millis, secondes, minutes = 0;
	unsigned int mask_r = 0;
	
    /* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_DISPLAY_TASK)*((RTIME)MS);
	int err = rt_task_set_periodic(&display_rt_task, rt_timer_read() + period, period);
	if (err != 0) {
		printf("display_task set periodic failed: %s\n", strerror(-err));
		return;
	}
	
  
    do {
		/* Met à jour l'affichage */
		set_display_time( minutes, secondes, millis++);
        /* Attend une période */
        rt_task_wait_period(NULL);
        
        /* Si on est plus dans le mode playing*/
		if (mode == END){
			set_display_time( 0, 0, 0);
			return ;
		}
		else if (mode != PLAYING){
			rt_task_suspend(NULL);
		}
      
		
		/* Si on veut stopper l'affichage du temps */ 
		rt_event_wait(&evenement, DISPLAY_STOP_MASK, &mask_r, EV_ANY, TM_NONBLOCK);
		if ( (mask_r & DISPLAY_STOP_MASK) == DISPLAY_STOP_MASK ){
			/* Clear */
			mask_r &= ~DISPLAY_STOP_MASK;
			rt_event_clear(&evenement, DISPLAY_STOP_MASK, NULL);
			/* Remet à 0 l'affichage */
			millis = secondes = minutes = 0;
		}
		
		/* S'il faut rewind l'affichage */
		rt_event_wait(&evenement, DISPLAY_REWIND_MASK, &mask_r, EV_ANY, TM_NONBLOCK);
		if ( (mask_r & DISPLAY_REWIND_MASK) == DISPLAY_REWIND_MASK ){
			/* Clear */
			mask_r &= ~DISPLAY_REWIND_MASK;
			rt_event_clear(&evenement, DISPLAY_REWIND_MASK, NULL);
			
			/* Recule de 10 secondes */
			if (secondes < 10 && minutes == 0) {
				secondes = 0;
				
			} else {
				if (secondes < 10) {
					minutes--;
					secondes = 60 - (10-secondes);
				} else {
					secondes -= 10;
				}
				
			}
		}
			
		/* S'il faut forward l'affichage */
		rt_event_wait(&evenement, DISPLAY_FORWARD_MASK, &mask_r, EV_ANY, TM_NONBLOCK);
		if ( (mask_r & DISPLAY_FORWARD_MASK) == DISPLAY_FORWARD_MASK ){
			/* Clear */
			mask_r &= ~DISPLAY_FORWARD_MASK;
			rt_event_clear(&evenement, DISPLAY_FORWARD_MASK, NULL);
			
			/* Avance de 10 secondes */
			secondes += 10;
			if (secondes >= 60) {
				secondes = secondes % 60;
				minutes++;
				if (minutes == 60){
					minutes = 0;
				}
			}
		}
		

		/* Gestion des bouclements du temps */
        if (millis >= 100){
			millis = 0;
			secondes++;
			if (secondes >= 60) {
				secondes = 0;
				minutes++;
				if (minutes == 60){
					minutes = 0;
				}
			}
		}
    } while (1);
}

/**
 * @brief Gestion des boutons
 * @param poiteur sur les arguments (non utilisé => NULL)
 * @return void
 */
void ui_task(void *cookie)
{
	uint32_t switch_val, key_val, old_key_val =0;
	unsigned int modeVol=0;

    /* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_UI_TASK)*((RTIME)MS);
	int err = rt_task_set_periodic(&ui_rt_task, rt_timer_read() + period, period);
	if (err != 0) {
		printf("ui task set periodic failed: %s\n", strerror(-err));
		return;
	}
	
  
    do {
		/* Gère le mode (Volume ou Forward/Rewind  )*/
		switch_val = get_switches();
		if (switch_val & SW0 ){
			modeVol = 1;
		} else {
			modeVol = 0;
		}
		/* Récupère la valeur des boutons */
		key_val = get_buttons();
		
		/* Play - Pause */
		if (is_pressed_now(old_key_val, key_val, KEY0) ){
			if (mode == PLAYING) {
				mode = PAUSED;
			} else {
				mode = PLAYING;
				rt_task_resume(&snd_rt_task);
				rt_task_resume(&display_rt_task);	
				
			}
		} 
		
		/* Stop (play -> relance depuis le début) */
		if (is_pressed_now(old_key_val, key_val, KEY1) ){
			/* Signal les taches */
			rt_event_signal(&evenement, 0x1);
			rt_event_signal(&evenement, 0x10);
			
			mode = STOPPED;

		} 
		
		/* Forward ou Vol+ */
		if (is_pressed_now(old_key_val, key_val, KEY2) ){
			if (modeVol){
				Vol = Vol >= VOL_MAX ? VOL_MAX : Vol+1;
				set_display_volume(Vol);
			} else {
				/* Signal les taches */
				rt_event_signal(&evenement, 0x4);
				rt_event_signal(&evenement, 0x40);
			}
			
		} 
		
		/* Rewind ou Vol- */
		if (is_pressed_now(old_key_val, key_val, KEY3) ){
			if (modeVol){
				Vol = Vol <= VOL_MIN ? VOL_MIN : Vol-1;
				set_display_volume(Vol);
			} else {
				/* Signal les taches */
				rt_event_signal(&evenement, 0x2);
				rt_event_signal(&evenement, 0x20);
			}
		} 
         
        /* Met à jour l'ancienne valeur des keys */
        old_key_val = key_val;
        /* Attend la prochaine période */
        rt_task_wait_period(NULL);
        
        /* Si on est plus dans le mode playing*/
		if (mode == END){
			return ;
		}

    } while (1);
}

/**
 * @brief Fonction principale qui prépare et lance les tâches 
 * @param Nombre d'arguments
 * @param Tableau de string des arguments
 * @return EXIT_SUCCESS en cas de succès
 */
int main(int argc, char *argv[]) 
{
    struct snd_task_args args;

    int audio_fd;
	/* Test le nombre d'arg */
    if (argc < 2) {
        fprintf(stderr, "Please provide an audio file\n");
        exit(EXIT_FAILURE);
    }

	/* Initialise la board */
    if (init_board() != 0) {
        perror("Error at board initialization.");
        exit(EXIT_FAILURE);
    }

    /* Ouverture du fichier audio */
    audio_fd = open(argv[1], O_RDONLY);
    if (audio_fd < 0) {
        perror("Opening audio file");
        exit(EXIT_FAILURE);
    }

    /* Lecture de l'entête du fichier wav. Valide au passage que le
     * fichier est bien au format wav */
    if (parse_wav_header(audio_fd, &(args.wf.wh))) {
        exit(EXIT_FAILURE);
    }

    printf("Successfully parsed wav file...\n");
    
    args.wf.audio_datas = malloc(args.wf.wh.data_size);
    if (args.wf.audio_datas == NULL) {
        perror("Allocating for audio data");
        exit(EXIT_FAILURE);
    }

    /* Copie des données audio du fichier wav */
    if (copy_wav_data(audio_fd, &(args.wf))) {
        fprintf(stderr, "Error copying audio data\n");
        exit(EXIT_FAILURE);
    }

    printf("Successfully copied audio data from wav file...\n");

    mlockall(MCL_CURRENT | MCL_FUTURE);

    /* EXEMPLE d'utilisation des différentes fonctions en lien avec les
     * entrées/sorties */
    printf("switch 0 value : 0x%x\n", get_switches() & SW0);
    printf("keys value: 0x%x\n", get_buttons());
    set_display_time(1, 2, 3);
    set_display_volume(VOL_MIDDLE);
    /* Fin de l'exemple */
    
    
    /* Crée l'evenement pour la sychronisation */
    rt_event_create(&evenement, "evenement_snd_player", 0x0, EV_FIFO);
    
    
	/* Crée et attend les 3 différentes taches */
    printf("Playing...\n");
    rt_task_spawn(&snd_rt_task, "snd_task", STACK_SIZE, PRIO_TASKS, T_JOINABLE, snd_task, &args);
    rt_task_spawn(&display_rt_task, "display_task", STACK_SIZE, PRIO_TASKS, T_JOINABLE, display_task , NULL);
    rt_task_spawn(&ui_rt_task, "ui_task", STACK_SIZE, PRIO_TASKS, T_JOINABLE, ui_task , NULL );
    rt_task_join(&ui_rt_task);
    rt_task_join(&display_rt_task);
    rt_task_join(&snd_rt_task);
    
    /* Supprime l'evenement */
    rt_event_delete(&evenement);
	
    clean_board();
    free(args.wf.audio_datas);
    munlockall();

    return EXIT_SUCCESS;
}
