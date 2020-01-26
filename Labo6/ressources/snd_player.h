/*
*
* Spinelli Isaia et Gerardi Tommy
* le 26 janvier 2020
*
* But : Le but est de réaliser un enregistreur audio capable de
* 			rejouer ce qui vient d’être enregistré
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <event.h>
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

#define PERIOD_SOURCE_TASK       		  2        /**< Période de la tâche snd_task en MS (F = F_ech / Nb_mot_par_canal => Periode = 1/F = 1/ (48000 / 128) = 2.7ms => 2 ms)   */
#define PERIOD_TIME_DISPLAY_TASK      10        /**< Période de la tâche display task en MS (10 MS est la valeur minimum affichée sur les 7 seg) */
#define PERIOD_CONTROL_TASK       		100        /**< Période de la tâche UI task en MS (100ms est environ la limite minimum du temps de réaction d'un humain)*/

#define PRIO_TASKS_AUDIO       	  	99        /**< Priorité des taches audio*/
#define PRIO_TASKS_CONTROL      		50        /**< Priorité des taches de controle */
#define PRIO_TASKS_DISPLAY       		10        /**< Priorité des taches d'affichage*/

#define STACK_SIZE       		0        /**< Taille des piles des taches */

#define QUEUE_SIZE          65536    /**< Taille de la pool qui pourra accueillir des messages */

#define FORWARD_REWIND_VAL 	 1920000		/**< Nombre de data à forward ou rewind du buffer pour 10 secondes (F * Nb_canaux * Nb_byte = 48'000 * 2 * 2 = 1920000) */
#define WORD_PER_PERIOD 		 384			/**< Nombre de mots écrit à chaque période (Déterminé par tests) */
#define BYTE_PER_PERIOD 		 384*2			/**< Nombre de byte écrit à chaque période */


#define MS 1000000		/**< Permet la transformation des periodes(MS) en NS*/

#define PLAYING_MASK			       0x1		/**< Definit qu'on est en mode lecture*/
#define RECORDING_MASK			     0x2		/**< Definit qu'on est en train d'enregistrer*/
#define PAUSED_MASK				       0x4		/**< Definit qu'on est en pause*/
#define FORWARDING_SOURCE_MASK	 0x8		/**< Definit qu'on veut faire un forward de 10s*/
#define REWINDING_SOURCE_MASK	   0x10	/**< Definit qu'on veut faire un rewind de 10s*/
#define FORWARDING_DISPLAY_MASK	 0x20	/**< Definit qu'on veut faire un forward de 10s (pour le display)*/
#define REWINDING_DISPLAY_MASK	 0x40	/**< Definit qu'on veut faire un rewind de 10s (pour le display)*/
#define NEW_RECORDING_MASK		   0x80	/**< Definit qu'un nouvel enregistrement débute */
#define DISPLAY_RESET_MASK		   0x100	/**< Definit qu'un nouvel enregistrement (pour le display) */
#define END_RECORDING_MASK       0x200	/**< Definit la fin d'un enregistrement */
#define END_FILE_MASK      		   0x400	/**< Definit la fin d'un fichier */

/* Cette tâche est responsable de jouer les données de son en les envoyant au driver gérant l’audio. La sortie audio est toujours active, que le système soit en enregis-trement ou en lecture. */
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

/* Evenements pour la sychronisation des taches */
static RT_EVENT evenement;

static RT_QUEUE audio_data_in;	/**< boites aux lettres pour l'envoie des data audio lues depuis un fichier ou le micro */
static RT_QUEUE sd_out;			/**< boites aux lettres pour l'envoie des data audio pour l'écriture sur la carte SD */
static RT_QUEUE audio_out;		/**< boites aux lettres pour l'envoie des data audio pour la sortie audio */

static RT_SEM task_barrier; 	/**< Sémaphore pour attendre le lancement de toutes les tâches */

/* Structure qui contient le fichier wav et le file descriptor audio */
struct snd_task_args {
  struct wav_file wf;
  int audio_fd ;
};

static char volume = VOL_MIDDLE;   /**< Volume du lecteur */

/* Cette tâche est responsable de jouer les données de son en les envoyant au driver gérant l’audio. La sortie audio est toujours active, que le système soit en enregis-trement ou en lecture. */
void audio_output_task(void *cookie);
/* Récupération du flux audio d’entrée ou lecture du fichier précédemment enregis-tré. Cette tâche place ensuite les données dans un buffer lui permettant de com-muniquer avec la tâche processing */
void source_task(void *cookie);

/* Cette tâche est responsable d’écrire dans le fichier les données reçues de la tâcheprocessing */
void sdcard_writer_task(void *cookie);

/* Cette tâche implémente la modification du volume */
void processing_task(void *cookie);

/* Gestion de l’affichage du temps. */
void time_display_task(void *cookie);

/* Cette tâche contrôle l’ensemble du système, en scrutant les les boutons et en agis-sant en fonction */
void control_task(void *cookie);
