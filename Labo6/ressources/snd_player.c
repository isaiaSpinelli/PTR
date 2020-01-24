/*
 * 
 * Spinelli Isaia et Gerardi Tommy 
 * le 26 janvier 2020
 * 
 * But : Le but est de réaliser un enregistreur audio capable de 
 * 			rejouer ce qui vient d’être enregistré
 */

#include "snd_player.h"

/* Cette tâche est responsable de jouer les données de son en les envoyant au driver gérant l’audio. La sortie audio est toujours active, que le système soit en enregis-trement ou en lecture. */
void audio_output_task(void *cookie)
{	
	int16_t* audio_processed;
	int write_ret, err;
	
	rt_printf("audio_output_task attend\n");
	/* Barrière de synchronisation (Attend le lancement de toutes les tâches) */
    rt_sem_p(&task_barrier, TM_INFINITE);
     
    
    do {
		/* Attend la réception de data audio */
		err = rt_queue_receive(&audio_out, (void **) &audio_processed, TM_INFINITE);
		if (err < 0) {
			rt_printf("Error rt_queue_receive [%s]\n", __FUNCTION__ );
			exit(EXIT_FAILURE);
		}

		/* Envoie les data */
		write_ret = write_samples(audio_processed , WORD_PER_PERIOD);

		/* S'il y a une erreur */
        if (write_ret < 0) {
            rt_printf("Error writing to sound driver\n");
            break;
        }
        
        
		/* Supprime la data audio précédemment récupérée */
		err = rt_queue_free(&audio_out, audio_processed);
		if (err != 0) {
			printf("Error rt_queue_free [%s] [err:%d]\n", __FUNCTION__, err);
			return;
		}
		
	} while(1);
    
    
    
    rt_printf("[%s] stopping\n", __FUNCTION__);
}

/* Récupération du flux audio d’entrée ou lecture du fichier précédemment enregis-tré. Cette tâche place ensuite les données dans un buffer lui permettant de com-muniquer avec la tâche processing */
void source_task(void *cookie)
{
	unsigned int mask , ret;
	struct snd_task_args *args = (struct snd_task_args*)cookie;
    long long to_write ;
    void *audio_datas_p ;
    
    void* data_to_process;

	/* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_SOURCE_TASK)*((RTIME)MS);
	int err = rt_task_set_periodic(&source_rt_task, rt_timer_read() + period, period);
	if (err != 0) {
		printf("source_task set periodic failed: %s\n", strerror(-err));
		return;
	}
	
	//audio_datas_p  = malloc(WORD_PER_PERIOD);
	rt_printf("source_task attend\n");
	/* Barrière de synchronisation (Attend le lancement de toutes les tâches) */
    rt_sem_p(&task_barrier, TM_INFINITE);
    
    
    do {
		/* Bloque si on ne doit pas play ou record */
		rt_event_wait(&evenement, PLAYING_MASK | RECORDING_MASK, &mask, EV_ANY, TM_INFINITE);
		
		/* Alloue pour un sample de donnée audio */
		data_to_process = rt_queue_alloc(&audio_data_in, WORD_PER_PERIOD);
		if (data_to_process == NULL) {
			rt_printf("rt_queue_alloc [%s]\n", __FUNCTION__ );
			exit(EXIT_FAILURE);
		}
		/* Si on est en mode playing*/
		if(mask & PLAYING_MASK){
			rt_event_wait(&evenement, FORWARDING_SOURCE_MASK | REWINDING_SOURCE_MASK | END_RECORDING_MASK, &mask, EV_ANY, TM_NONBLOCK);
			
			
			// Si fin d un record -> préparer le fichier 
			if(mask & END_RECORDING_MASK){
				
				err = rt_event_clear(&evenement, END_RECORDING_MASK, NULL);
				if (err != 0) {
					printf("rt_event_clear failed: %s\n", strerror(-err));
					return;
				}
				
				/* Lecture de l'entête du fichier wav. Valide au passage que le
				 * fichier est bien au format wav */
				if (parse_wav_header(args->audio_fd, &(args->wf.wh))) {
					exit(EXIT_FAILURE);
				}

				printf("Successfully parsed wav file...\n");
				
				args->wf.audio_datas = malloc(args->wf.wh.data_size);
				if (args->wf.audio_datas == NULL) {
					perror("Allocating for audio data");
					exit(EXIT_FAILURE);
				}

				/* Copie des données audio du fichier wav */
			   if (copy_wav_data(args->audio_fd, &(args->wf))) {
					fprintf(stderr, "Error copying audio data\n");
					exit(EXIT_FAILURE);
				}
				
				to_write = args->wf.wh.data_size;
				audio_datas_p = args->wf.audio_datas;

				printf("Successfully copied audio data from wav file...\n");
    
			}
			/* Avance de 10 secondes */
			if(mask & FORWARDING_SOURCE_MASK){
				err = rt_event_clear(&evenement, FORWARDING_SOURCE_MASK, NULL);
				if (err != 0) {
					printf("rt_event_clear failed: %s\n", strerror(-err));
					return;
				}
				
				/* Si la musique ne va pas se finir  */
				if (to_write >= FORWARD_REWIND_VAL) {
					
					/* Signal l'affichage qu'il peut aussi forward */
					err = rt_event_signal(&evenement, FORWARDING_DISPLAY_MASK);
					if (err != 0) {
						printf("rt_event_signal failed: %s\n", strerror(-err));
						return;
					}
					to_write -= FORWARD_REWIND_VAL;
					audio_datas_p += FORWARD_REWIND_VAL;
					
				} else { 
					/* Met à la fin de la musique */
					to_write = 0;
				}
				
			}
			/* Recule de 10 secondes */
			if(mask & REWINDING_SOURCE_MASK){
				err = rt_event_clear(&evenement, REWINDING_SOURCE_MASK, NULL);
				if (err != 0) {
					printf("rt_event_clear failed: %s\n", strerror(-err));
					return;
				}
				
				/* Si la musique ne vas pas boucler */
				if ((args->wf.wh.data_size - to_write) >= FORWARD_REWIND_VAL ) { 
					/* Signal l'affichage qu'il peut aussi rewind */
					err = rt_event_signal(&evenement, REWINDING_DISPLAY_MASK);
					to_write += FORWARD_REWIND_VAL;
					audio_datas_p -= FORWARD_REWIND_VAL;
				} else { /* Met au début de la musique */
					to_write = args->wf.wh.data_size;
					audio_datas_p = args->wf.audio_datas;
				}
				
			}
	
			/* copie les data à envoyer */
			memcpy(data_to_process, audio_datas_p, WORD_PER_PERIOD);
			
			/* Met à jour les datas */
			to_write -= WORD_PER_PERIOD;
			audio_datas_p += WORD_PER_PERIOD;
			
			/* Si c'est la fin du fichier */
			if(to_write <= 0){
				rt_printf("End of file \n");				
				/* Enleve le bit de playing */
				err = rt_event_clear(&evenement, PLAYING_MASK, NULL );
				if (err < 0) {
					rt_printf("rt_event_clear [%s]\n", __FUNCTION__ );
					exit(EXIT_FAILURE);
				}
				rt_event_signal(&evenement, END_FILE_MASK | DISPLAY_RESET_MASK); 
				/* Remet les datas au début */
				to_write = args->wf.wh.data_size;
				audio_datas_p = args->wf.audio_datas;
			}
		}else{
			/* Recuperation du flux audio */
			ret = read_samples(data_to_process, WORD_PER_PERIOD);
			if (ret == 0) {
				perror("Allocating for audio data foward");
				rt_queue_free(&audio_data_in, data_to_process);
				exit(EXIT_FAILURE);
			}
		}

		/* Envoie les datas */
		err = rt_queue_send(&audio_data_in, data_to_process, WORD_PER_PERIOD, Q_BROADCAST);
		if (err < 0) {
			rt_printf("rt_queue_send [%s] [err=%d]\n", __FUNCTION__, err );
			exit(EXIT_FAILURE);
		}
		
		 /* Attend une période */
		rt_task_wait_period(NULL);
		
	} while(1);
    
    
    
    rt_printf("[%s] stopping\n", __FUNCTION__);
}

/* Cette tâche est responsable d’écrire dans le fichier les données reçues de la tâcheprocessing */
void sdcard_writer_task(void *cookie)
{
	unsigned int mask = 0;
	void* audio_processed;
    int ret,err;
	struct snd_task_args *args = (struct snd_task_args*)cookie;
	/* Prépare l'en-tête du fichier wav */
	struct wav_header wh;
	wh.file_blkID   = WAV_FILE_BLKID;
	wh.fmt_blkID    = WAV_FMT_BLKID;
	wh.data_blkID   = WAV_DATA_BLKID;
	wh.data_size    = 0;
	
	rt_printf("sdcard_writer_task attend\n");
	/* Barrière de synchronisation (Attend le lancement de toutes les tâches) */
    rt_sem_p(&task_barrier, TM_INFINITE);
    
    do {
		/* Attend la réception de data audio */
		err = rt_queue_receive(&sd_out, (void **) &audio_processed, TM_INFINITE);
		
		if (err < 0) {
			rt_printf("Error rt_queue_receive [%s]\n", __FUNCTION__ );
			exit(EXIT_FAILURE);
		}
		
		/* Test si on est en mode playing ou recording */
		rt_event_wait(&evenement, NEW_RECORDING_MASK, &mask, EV_ANY, TM_NONBLOCK);

		if (mask & NEW_RECORDING_MASK) {
			/* Ecrase l'ancien fichier */
			mask = mask & ~NEW_RECORDING_MASK;
			err = rt_event_clear(&evenement, NEW_RECORDING_MASK, NULL );
			if (err < 0) {
				rt_printf("rt_event_clear [%s]\n", __FUNCTION__ );
				exit(EXIT_FAILURE);
			}
			write_wav_header(args->audio_fd, &wh);
			
		}
		
		/* Ajoute les données audio au fichier */
		ret = append_wav_data(args->audio_fd, audio_processed, WORD_PER_PERIOD);
		if (ret) {
			rt_printf("[%s] Couldn't append data\n", __FUNCTION__);
		}
		
		
		/* Supprime la data audio précédemment récupérée */
		err = rt_queue_free(&sd_out, audio_processed);
		if (err != 0) {
			printf("Error rt_queue_free [%s] [err:%d]\n", __FUNCTION__, err);
			return;
		}
	} while(1);


    rt_printf("[%s] stopping\n", __FUNCTION__);
}

/* Cette tâche implémente la modification du volume */
void processing_task(void *cookie)
{
	void* audio_in;
	int16_t* sample_volume;
	int16_t* audio_processed;
	int16_t* audio_processed_for_sd;
	int err,i;
	unsigned int mask;
	/* Stock les datas audio avec la modification du volume */
	sample_volume = (int16_t*)malloc(WORD_PER_PERIOD);
	
	rt_printf("processing_task attend\n");
	/* Barrière de synchronisation (Attend le lancement de toutes les tâches) */
    rt_sem_p(&task_barrier, TM_INFINITE);
    
    
    do {
		/* Attend la réception de data audio */
		err = rt_queue_receive(&audio_data_in, (void **) &audio_in, TM_INFINITE);
		if (err < 0) {
			rt_printf("Error rt_queue_receive [%s]\n", __FUNCTION__ );
			exit(EXIT_FAILURE);
		}
		/* Prepare les datas avec le bon volume */
		for (i=0; i < WORD_PER_PERIOD/2; ++i){ 
			sample_volume[i] = map_volume(* ((int16_t*)(audio_in)+i) , volume);
		}
		
		/* Test si on est en mode playing ou recording */
		rt_event_wait(&evenement, RECORDING_MASK | PLAYING_MASK, &mask, EV_ANY, TM_NONBLOCK);
		if (mask & PLAYING_MASK) {
			
			/* Alloue le ptr pour l'envoi*/
			audio_processed = rt_queue_alloc(&audio_out, WORD_PER_PERIOD );
			if (audio_processed == NULL) {
				rt_printf("rt_queue_alloc [%s]\n", __FUNCTION__ );
				exit(EXIT_FAILURE);
			}
			
			memcpy(audio_processed, sample_volume, WORD_PER_PERIOD);
		
			/* Envoie les datas traités à audio out*/
			err = rt_queue_send(&audio_out, audio_processed, WORD_PER_PERIOD, Q_BROADCAST);
			if (err < 0) {
				rt_printf("rt_queue_send [%s] [err=%d]\n", __FUNCTION__, err );
				exit(EXIT_FAILURE);
			}
		} else { 
			
			/* Alloue le ptr pour l'envoi*/
			audio_processed_for_sd = rt_queue_alloc(&sd_out, WORD_PER_PERIOD );
			if (audio_processed == NULL) {
				rt_printf("rt_queue_alloc [%s]\n", __FUNCTION__ );
				exit(EXIT_FAILURE);
			}
			
			memcpy(audio_processed_for_sd, sample_volume, WORD_PER_PERIOD);
			
			/* Envoie les datas traités à sd out*/
			err = rt_queue_send(&sd_out, audio_processed_for_sd, WORD_PER_PERIOD, Q_BROADCAST);
			if (err < 0) {
				rt_printf("rt_queue_send [%s] [err=%d]\n", __FUNCTION__, err );
				exit(EXIT_FAILURE);
			}
		}

		/* Supprime la data audio précédemment récupérée */
		err = rt_queue_free(&audio_data_in, audio_in);
		if (err != 0) {
			printf("Error rt_queue_free [%s] [err:%d]\n", __FUNCTION__, err);
			return;
		}
		
	} while(1);
    
    free(sample_volume);
    rt_printf("[%s] stopping\n", __FUNCTION__);
}

/* Gestion de l’affichage du temps. */
void time_display_task(void *cookie)
{
	unsigned int millis = 0, secondes = 0, minutes = 0, mask = 0;
	
	/* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_TIME_DISPLAY_TASK)*((RTIME)MS);
	int err = rt_task_set_periodic(&time_display_rt_task, rt_timer_read() + period, period);
	if (err != 0) {
		printf("display_task set periodic failed: %s\n", strerror(-err));
		return;
	}
	
	rt_printf("time_display_task attend\n");

		
	/* Barrière de synchronisation (Attend le lancement de toutes les tâches) */
    rt_sem_p(&task_barrier, TM_INFINITE);
    
    
    do {
		/* Met à jour l'affichage */  
		set_display_time( minutes, secondes, millis++);
        /* Attend une période */
        rt_task_wait_period(NULL);
        
        /* Bloque si il ne faut pas play ou record */
        rt_event_wait(&evenement, PLAYING_MASK | RECORDING_MASK, &mask, EV_ANY, TM_INFINITE);
        
			
		/* Si on doit play */
        if(mask & PLAYING_MASK){
			/* Récupère la valeur des flags (forwarding et rewinding)*/
			rt_event_wait(&evenement, REWINDING_DISPLAY_MASK | FORWARDING_DISPLAY_MASK, &mask, EV_ANY, TM_NONBLOCK);

			/* Si on doit forwad */
			if(mask & FORWARDING_DISPLAY_MASK){
				/* clear le flag */
				err = rt_event_clear(&evenement, FORWARDING_DISPLAY_MASK, NULL);
				if (err != 0) {
					printf("rt_event_clear failed: %s\n", strerror(-err));
					return;
				}

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
			/* Si on doit rewind */
			if(mask & REWINDING_DISPLAY_MASK){
				/* clear le flag */
				err = rt_event_clear(&evenement, REWINDING_DISPLAY_MASK, NULL);
				if (err != 0) {
					printf("rt_event_clear failed: %s\n", strerror(-err));
					return;
				}
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
		} 
		
		/* Récupère la valeur des flags (forwarding et rewinding)*/
		rt_event_wait(&evenement, DISPLAY_RESET_MASK, &mask, EV_ANY, TM_NONBLOCK);

		
		/* Si un enregistrement est terminé */
		if(mask & DISPLAY_RESET_MASK){
			/* clear le flag */
			err = rt_event_clear(&evenement, DISPLAY_RESET_MASK, NULL);
			if (err != 0) {
				printf("rt_event_clear failed: %s\n", strerror(-err));
				return;
			}
			/* redemarre à 0 */
			secondes = minutes = millis = 0;
		}
		
		
		/* Gestion des bouclements du temps */
        if (millis >= 100){
			millis = 0;
			secondes++;
			if (secondes >= 60) {
				secondes = 0;
				minutes++;
				if (minutes >= 60){
					minutes = 0;
				}
			}
		}
    } while (1);
    
    
    rt_printf("[%s] stopping\n", __FUNCTION__);
}

/* Cette tâche contrôle l’ensemble du système, en scrutant les les boutons et en agis-sant en fonction */
void control_task(void *cookie)
{
	uint32_t switch_val, key_val, old_key_val = 0;
	int i;
	int err;
	unsigned int mode = PLAYING;
	unsigned int mask;
	
	/* Configuration de la tâche périodique */
	RTIME period = ((RTIME)PERIOD_CONTROL_TASK)*((RTIME)MS);
	err = rt_task_set_periodic(&control_rt_task, rt_timer_read() + period, period);
	if (err != 0) {
		printf("display_task set periodic failed: %s\n", strerror(-err));
		return;
	}
	rt_printf("control_task attend\n");
	/* Barrière de synchronisation (Attend le lancement de toutes les tâches) */
    rt_sem_p(&task_barrier, TM_INFINITE);
    
    do {
		
		/* Récupère la valeur des boutons */
		key_val = get_buttons();
		
		/* KEY0 ->  Lance l’enregistrement sur la carte, et stoppe l’enregistrement s’il est en cours */
		if (is_pressed_now(old_key_val, key_val, KEY0) ){
			if(mode == RECORDING){
				rt_printf("End of recording\n");
				mode = PAUSED;
				rt_event_clear(&evenement, RECORDING_MASK, NULL);
				rt_event_signal(&evenement, PAUSED_MASK | END_RECORDING_MASK | DISPLAY_RESET_MASK);
			}else{
				rt_printf("Recording...\n");
				rt_event_clear(&evenement, PAUSED_MASK  | PLAYING_MASK, NULL);
				rt_event_signal(&evenement, RECORDING_MASK | NEW_RECORDING_MASK | DISPLAY_RESET_MASK);
				mode = RECORDING;
			}
		}
		
		/* KEY1 ->  Lance la lecture du fichier précédemment enregistré. Si la lecture est cours,
				celle-ci se met en pause, et si elle est en pause, elle doit reprendre.*/
		if (is_pressed_now(old_key_val, key_val, KEY1) ){
			if(mode != RECORDING){
				
				/* Si le fichier est fini, relance le fichier */
				rt_event_wait(&evenement, END_FILE_MASK, &mask, EV_ANY, TM_NONBLOCK);
				if(mask & END_FILE_MASK){
					rt_printf("Play \n");
					/* Clear nous même le flag (l'event clear ne fonctionne pas) */
					mask = mask & ~END_FILE_MASK;
					err = rt_event_clear(&evenement, END_FILE_MASK, NULL);
					if (err != 0) {
						printf("rt_event_clear failed: %s\n", strerror(-err));
						return;
					}
					mode = PLAYING;
					rt_event_clear(&evenement, PAUSED_MASK, NULL);
					rt_event_signal(&evenement, PLAYING_MASK);
				} else if(mode == PLAYING ){
					rt_printf("Pause \n");
					mode = PAUSED;
					rt_event_clear(&evenement, PLAYING_MASK, NULL);
					rt_event_signal(&evenement, PAUSED_MASK);
				}else{
					rt_printf("Play \n");
					mode = PLAYING;
					rt_event_clear(&evenement, PAUSED_MASK, NULL);
					rt_event_signal(&evenement, PLAYING_MASK);
				}	
			}
		}
		
		/* KEY2 -> Avance la lecture de 10 secondes. */
		if (is_pressed_now(old_key_val, key_val, KEY2) ){
			if(mode == PLAYING){
				rt_printf("Forward \n");
				err = rt_event_signal(&evenement, FORWARDING_SOURCE_MASK );
				if (err != 0) {
					printf("rt_event_signal failed: %s\n", strerror(-err));
					return;
				}
			}
		}
		
		/* KEY3 -> Recule de 10 secondes dans le fichier. */
		if (is_pressed_now(old_key_val, key_val, KEY3) ){
			if(mode == PLAYING){
				rt_printf("Rewind \n");
				err = rt_event_signal(&evenement, REWINDING_SOURCE_MASK | REWINDING_DISPLAY_MASK);
				if (err != 0) {
					printf("rt_event_signal failed: %s\n", strerror(-err));
					return;
				}
			}
		}
		
		/* Met à jour le volume */
		switch_val = get_switches();
		
		i = 0;
		while(switch_val){
			i++;
			if(switch_val & 1){
				break;
			}
			switch_val = switch_val >> 1;
		}
		volume = i;
		set_display_volume(volume);
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
    int err ;
    struct snd_task_args args;

    if (init_board() != 0) {
        perror("Error at board initialization.");
        exit(EXIT_FAILURE);
    }
	
    if (argc != 2) {
        printf("Not enough arguments!\n");
        exit(EXIT_FAILURE);
    }
    
    /* Ouverture du fichier audio */
    args.audio_fd = open(argv[1], O_RDWR|O_CREAT);
    if (args.audio_fd < 0) {
        perror("Opening audio file");
        exit(EXIT_FAILURE);
    }

    /* Setup task barrier */
    if (rt_sem_create(&task_barrier, "Task barrier", 0, S_FIFO)) {
        perror("Creating task barrier");
        exit(EXIT_FAILURE);
    }
    mlockall(MCL_CURRENT | MCL_FUTURE);
    
    /* Crée l'evenement pour la sychronisation */
    err = rt_event_create(&evenement, "evenement_snd_player", PLAYING_MASK | END_RECORDING_MASK, EV_PRIO);
	if (err != 0) {
		printf("rt_event_create error [%s]\n", __FUNCTION__ );
		exit(EXIT_FAILURE);
	}
	
     
    /* Création des 3 boites aux lettres permettant l’échange des données sonores.*/
    err = rt_queue_create(&audio_data_in, "audio_data_in", 65536, Q_UNLIMITED, Q_FIFO); // 1024 / 16 olieu de Q_UNLI...
    if (err != 0) {
		printf("rt_queue_create error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
    err = rt_queue_create(&sd_out, "sd_out", 65536, Q_UNLIMITED, Q_FIFO);
    if (err != 0) {
		printf("rt_queue_create error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
    err = rt_queue_create(&audio_out, "audio_out", 65536, Q_UNLIMITED, Q_FIFO);
    if (err != 0) {
		printf("rt_queue_create error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
    

    /* Création des différentes tâches */
    
    err = rt_task_spawn(&source_rt_task, SOURCE_TASK_NAME, STACK_SIZE, PRIO_TASKS, T_JOINABLE, source_task, &args);
    if (err != 0) {
		printf("rt_task_spawn error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}

	err = rt_task_spawn(&time_display_rt_task, TIME_DISPLAY_TASK_NAME, STACK_SIZE, PRIO_TASKS, T_JOINABLE, time_display_task, NULL);
    if (err != 0) {
		printf("rt_task_spawn error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}

	err = rt_task_spawn(&control_rt_task, CONTROL_TASK_NAME, STACK_SIZE, PRIO_TASKS, T_JOINABLE, control_task, NULL);
    if (err != 0) {
		printf("rt_task_spawn error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}

	err = rt_task_spawn(&audio_output_rt_task, AUDIO_OUTPUT_TASK_NAME, STACK_SIZE, PRIO_TASKS, T_JOINABLE, audio_output_task, NULL);
    if (err != 0) {
		printf("rt_task_spawn error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
	
	err = rt_task_spawn(&processing_rt_task, PROCESSING_TASK_NAME, STACK_SIZE, PRIO_TASKS, T_JOINABLE, processing_task, NULL);
    if (err != 0) {
		printf("rt_task_spawn error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}

	err = rt_task_spawn(&sdcard_writer_rt_task, SDCARD_WRITER_TASK_NAME, STACK_SIZE, PRIO_TASKS, T_JOINABLE, sdcard_writer_task, &args);
    if (err != 0) {
		printf("rt_task_spawn error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}

	
	/* Reveil toutes les tâches après les avoir toutes crées */
	err = rt_sem_broadcast(&task_barrier);
    if (err != 0) {
		printf("rt_sem_broadcast error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
	
    
    
    /* Attend l'execution de toutes les tâches */
    err = rt_task_join(&time_display_rt_task);
    if (err != 0) {
		printf("rt_task_join error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
	err = rt_task_join(&control_rt_task);
    if (err != 0) {
		printf("rt_task_join error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
	err = rt_task_join(&source_rt_task);
    if (err != 0) {
		printf("rt_task_join error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
	err = rt_task_join(&audio_output_rt_task);
    if (err != 0) {
		printf("rt_task_join error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
	err = rt_task_join(&processing_rt_task);
    if (err != 0) {
		printf("rt_task_join error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
	err = rt_task_join(&sdcard_writer_rt_task);
    if (err != 0) {
		printf("rt_task_join error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}


    
    
    /* Suppression des 3 boites aux lettres */
    err = rt_queue_delete(&audio_data_in);
    if (err != 0) {
		printf("rt_queue_delete error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
    err = rt_queue_delete(&sd_out);
    if (err != 0) {
		printf("rt_queue_delete error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
    err = rt_queue_delete(&audio_out);
    if (err != 0) {
		printf("rt_queue_delete error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
	
	/* Supprime l'evenement */
    err = rt_event_delete(&evenement);
	if (err != 0) {
		printf("rt_event_delete error [%s]\n", __FUNCTION__ );
        exit(EXIT_FAILURE);
	}
	
    clean_board();
    close(args.audio_fd);

    munlockall();

    return EXIT_SUCCESS;
}
