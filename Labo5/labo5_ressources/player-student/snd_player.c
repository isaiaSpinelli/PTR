#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <alchemy/task.h>
#include <alchemy/timer.h>

#include "io_utils.h"

#define PERIOD_SND_TASK       		2        /**< Période de la tâche snd_task */
#define PERIOD_DISPLAY_TASK       	10        /**< Période de la tâche display task */
#define PERIOD_UI_TASK       		150        /**< Période de la tâche UI task */

struct snd_task_args {
    int snd_fd;
    struct wav_file wf;
};

RT_TASK snd_rt_task;
RT_TASK display_rt_task;
RT_TASK ui_rt_task;

void snd_task(void *cookie)
{
    struct snd_task_args *args = (struct snd_task_args*)cookie;
    size_t to_write = args->wf.wh.data_size;
    ssize_t write_ret;
    void *audio_datas_p = args->wf.audio_datas;
    
    /* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_SND_TASK)*((RTIME)MS);
	int err = rt_task_set_periodic(&snd_rt_task, rt_timer_read() + period, period);
	if (err != 0) {
		printf("snd_rt_task set periodic failed: %s\n", strerror(-err));
		return;
	}
	
  
    do {
        write_ret = write(args->snd_fd, audio_datas_p, to_write);
        if (write_ret < 0) {
            rt_printf("Error writing to sound driver\n");
            break;
        }

        to_write -= write_ret;
        audio_datas_p += write_ret;
        
        rt_task_wait_period(NULL);

    } while (to_write);
}

void display_task(void *cookie)
{
	void *ioctrls = cookie;
	unsigned int millis, secondes, minutes = 0;
    /* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_DISPLAY_TASK)*((RTIME)MS);
	int err = rt_task_set_periodic(&display_rt_task, rt_timer_read() + period, period);
	if (err != 0) {
		printf("display_task set periodic failed: %s\n", strerror(-err));
		return;
	}
	
  
    do {
		
		display_time(ioctrls, minutes, secondes, millis++);
        
        rt_task_wait_period(NULL);

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
        /*
        millis = millis == 99 ? 0 : millis+1;
        secondes = (millis == 0) ? (secondes == 59 ? 0 : secondes+1 ): secondes;
        minutes = secondes == 0 ? (minutes == 59 ? 0 : minutes+1 ): minutes;
        * */
    } while (1);
}

void ui_task(void *cookie)
{
	void *ioctrls = cookie;
	unsigned int switch_val, key_val=0;
	unsigned int modeVol=0;
	char Vol;
    /* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_UI_TASK)*((RTIME)MS);
	int err = rt_task_set_periodic(&ui_rt_task, rt_timer_read() + period, period);
	if (err != 0) {
		printf("ui task set periodic failed: %s\n", strerror(-err));
		return;
	}
	
  
    do {
		switch_val = switches(ioctrls);
		if (switch_val & SW0 ){
			modeVol = 1;
		} else {
			modeVol = 0;
		}
			
		key_val = ~keys(ioctrls);
		
		/* Play - Pause */
		if (key_val & KEY0){
		} 
		
		/* Stop (play -> relance depuis le début) */
		if (key_val & KEY1){

		} 
		
		/* Forward ou Vol+ */
		if (key_val & KEY2){
			if (modeVol){
				Vol = Vol >= VOL_MAX ? VOL_MAX : Vol++;
				set_volume_leds(ioctrls, Vol);
			} else {
				
			}
			
		} 
		
		/* Rewind ou Vol- */
		if (key_val & KEY3){
			if (modeVol){
				Vol = Vol <= VOL_MIN ? VOL_MIN : Vol--;
				set_volume_leds(ioctrls, Vol);
			} else {
				
			}
		} 
		
        printf("switch 0 value : 0x%x\n", switches(ioctrls) & SW0);
         
        rt_task_wait_period(NULL);



    } while (1);
}

int 
main(int argc, char *argv[]) 
{
    int rtsnd_fd;
    int audio_fd;
    struct snd_task_args args;
    void *ioctrls;

    if (argc < 2) {
        fprintf(stderr, "Please provide an audio file\n");
        exit(EXIT_FAILURE);
    }

    /* Ouverture du driver RTDM */
    rtsnd_fd = open("/dev/rtdm/snd", O_RDWR);
    if (rtsnd_fd < 0) {
        perror("Opening /dev/rtdm/snd");
        exit(EXIT_FAILURE);
    }

    args.snd_fd = rtsnd_fd;

    ioctrls = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, rtsnd_fd, 0);
    if (ioctrls == MAP_FAILED) {
        perror("Mapping real-time sound file descriptor");
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
    printf("switch 0 value : 0x%x\n", switches(ioctrls) & SW0);
    printf("keys value: 0x%x\n", keys(ioctrls));
    display_time(ioctrls, 1, 2, 3);
    set_volume_leds(ioctrls, VOL_MIDDLE);
    /* Fin de l'exemple */

    printf("Playing...\n");
    rt_task_spawn(&snd_rt_task, "snd_task", 0, 99, T_JOINABLE, snd_task, &args);
    rt_task_spawn(&display_rt_task, "display_task", 0, 99, T_JOINABLE, display_task , ioctrls);
    rt_task_spawn(&ui_rt_task, "ui_task", 0, 99, T_JOINABLE, ui_task , ioctrls);
    rt_task_join(&ui_rt_task);
    rt_task_join(&display_rt_task);
    rt_task_join(&snd_rt_task);

    close(rtsnd_fd);
    if (munmap(ioctrls, 4096) == -1) {
        perror("Unmapping");
        exit(EXIT_FAILURE);
    }
    free(args.wf.audio_datas);
    munlockall();

    return EXIT_SUCCESS;
}
