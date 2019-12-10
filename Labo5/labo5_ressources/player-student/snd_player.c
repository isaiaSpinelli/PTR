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

#include "audio_player_board.h"

#define PERIOD_SND_TASK       		2        /**< Période de la tâche snd_task en MS*/
#define PERIOD_DISPLAY_TASK       	10        /**< Période de la tâche display task en MS */
#define PERIOD_UI_TASK       		150        /**< Période de la tâche UI task en MS */

#define FORWARD_REWIND_VAL 		1652104		/**< Nombre de data à forward ou rewind du buffer*/

#define MS 1000000		/**< Permet la transformation des periodes(MS) en NS*/

static unsigned int mode = PLAYING;

static unsigned int stopped_snd = 0;
static unsigned int stopped_display = 0;

static unsigned int forwarded_snd = 0;
static unsigned int forwarded_display = 0;

static unsigned int rewind_snd = 0;
static unsigned int rewind_display = 0;

struct snd_task_args {
    struct wav_file wf;
};

static RT_TASK snd_rt_task;
static RT_TASK display_rt_task;
static RT_TASK ui_rt_task;


void snd_task(void *cookie)
{
    struct snd_task_args *args = (struct snd_task_args*)cookie;
    size_t to_write = args->wf.wh.data_size;
    ssize_t write_ret;
    void *audio_datas_p = args->wf.audio_datas;
    unsigned long allData = 0;
	
    /* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_SND_TASK)*((RTIME)MS);
	int err = rt_task_set_periodic(&snd_rt_task, rt_timer_read() + period, period);
	if (err != 0) {
		printf("snd_rt_task set periodic failed: %s\n", strerror(-err));
		return;
	}
	
  
    do {
        write_ret = write_samples(audio_datas_p, to_write);
        allData += write_ret;
       
        if (write_ret < 0) {
            rt_printf("Error writing to sound driver\n");
            break;
        }

        to_write -= write_ret;
        audio_datas_p += write_ret;
        
        rt_task_wait_period(NULL);
        
        /* Si on est plus dans le mode playing*/
		if (mode != PLAYING){
			rt_printf("NB = %d\t\t", allData);
			rt_task_suspend(NULL);
		}
		/* Si on veut stopper la lecture de la musique */ 
		if (stopped_snd) {
			to_write = args->wf.wh.data_size;
			audio_datas_p = args->wf.audio_datas;
			stopped_snd = 0;
		} 
		/* S'il faut forward la musique */
		if (forwarded_snd) {
			/* Si la musique ne vas se finir  */
			if (to_write >= FORWARD_REWIND_VAL) {
				to_write -= FORWARD_REWIND_VAL;
				audio_datas_p += FORWARD_REWIND_VAL;
			} else { 
				/* Met à la fin de la musique */
				to_write = 0;
			}
			forwarded_snd = 0;
		}
		/* S'il faut rewind la musique */
		if (rewind_snd) {
			/* Si la musique ne vas pas boucler */
			if ((args->wf.wh.data_size - to_write) >= FORWARD_REWIND_VAL ) { 
				to_write += FORWARD_REWIND_VAL;
				audio_datas_p -= FORWARD_REWIND_VAL;
			} else { /* Met au début de la musique */
				to_write = args->wf.wh.data_size;
				audio_datas_p = args->wf.audio_datas;
			}
			
			rewind_snd = 0;
		}

    } while (to_write);
}

void display_task(void *cookie)
{
	unsigned int millis, secondes, minutes = 0;
    /* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_DISPLAY_TASK)*((RTIME)MS);
	int err = rt_task_set_periodic(&display_rt_task, rt_timer_read() + period, period);
	if (err != 0) {
		printf("display_task set periodic failed: %s\n", strerror(-err));
		return;
	}
	
  
    do {
		
		set_display_time( minutes, secondes, millis++);
        
        rt_task_wait_period(NULL);
        
         /* Si on est plus dans le mode playing*/
		if (mode != PLAYING){
			rt_task_suspend(NULL);
		}
		/* Si on veut stopper la lecture de la musique */ 
		if (stopped_display) {
			millis = secondes = minutes = 0;
			stopped_display = 0;
		} 
		/* S'il faut forward la musique */
		if (forwarded_display) {
			secondes += 10;
			forwarded_display = 0;
		}
		/* S'il faut rewind la musique */
		if (rewind_display) {
			secondes -= 10;
			rewind_display = 0;
		}

        if (millis >= 100){
			millis = 0;
			secondes++;
			if (secondes >= 60) {
				secondes = 0;
				minutes++;
				if (minutes == 60){
					minutes=0;
				}
			}
		}
    } while (1);
}

void ui_task(void *cookie)
{
	uint32_t switch_val, key_val, old_key_val =0;
	unsigned int modeVol=0;
	static char Vol = VOL_MIDDLE;
	//static char mode = PLAYING;
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
			mode = STOPPED;
			stopped_snd = 1;
			stopped_display = 1;
		} 
		
		/* Forward ou Vol+ */
		//if (key_val & KEY2){
		if (is_pressed_now(old_key_val, key_val, KEY2) ){
			if (modeVol){
				Vol = Vol >= VOL_MAX ? VOL_MAX : Vol+1;
				set_display_volume(Vol);
			} else {
				forwarded_snd = 1;
				forwarded_display = 1;
			}
			
		} 
		
		/* Rewind ou Vol- */
		if (is_pressed_now(old_key_val, key_val, KEY3) ){
			if (modeVol){
				Vol = Vol <= VOL_MIN ? VOL_MIN : Vol-1;
				set_display_volume(Vol);
			} else {
				rewind_snd = 1;
				rewind_display = 1;
			}
		} 
         
        /* Met à jour l'ancienne valeur des keys */
        old_key_val = key_val;
        /* Attend la prochaine période */
        rt_task_wait_period(NULL);



    } while (1);
}

int 
main(int argc, char *argv[]) 
{
    struct snd_task_args args;

    int audio_fd;

    if (argc < 2) {
        fprintf(stderr, "Please provide an audio file\n");
        exit(EXIT_FAILURE);
    }


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

    printf("Playing...\n");
    rt_task_spawn(&snd_rt_task, "snd_task", 0, 99, T_JOINABLE, snd_task, &args);
    rt_task_spawn(&display_rt_task, "display_task", 0, 99, T_JOINABLE, display_task , NULL);
    rt_task_spawn(&ui_rt_task, "ui_task", 0, 99, T_JOINABLE, ui_task , NULL );
    rt_task_join(&ui_rt_task);
    rt_task_join(&display_rt_task);
    rt_task_join(&snd_rt_task);

    clean_board();


    free(args.wf.audio_datas);

    munlockall();

    return EXIT_SUCCESS;
}
