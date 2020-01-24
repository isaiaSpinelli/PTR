# PTR Laboratoire 6
## Isaïa Spinelli et Tommy Gerardi, le 07.01.2020

Améliorer : Incrémentation du temps (une variable) + Magic number
            Tricks pour quitter l'app


Justification des priorités
Justification Q fifo priorité
Justification valeur queue create
explication inverssion led
Check valeur de retour !! (rt_queues_recevie)



### Introduction et explications
Le but de ce laboratoire est de réaliser un lecteur / enregistreur audio capable de jouer des fichiers wav ainsi que de rejouer ce qui vient d’être enregistré, et ce sur une carte DE1-SoC.

La DE1-SoC possède une interface audio accessible depuis la partie logique programmable. Le système qui nous est fourni contient la partie logique ainsi qu’un driver spécialement développé pour ce laboratoire. Ce driver nous permet d’envoyer des données sur les deux canaux audios. En interne, il dispose d’un buffer de 128 mots par canal. De plus, il permet de piloter et scruter les différentes E/S de la carte (afficheurs 7seg, boutons poussoirs, leds et switchs). L’enregistrement se fera sur un fichier unique, et ce fichier sera relu pour le playback. Il n’est pas demandé de pouvoir travailler avec d’autres fichiers.


### Choix architecturaux
Afin de réaliser notre lecteur / enregistreur audio, nous avons décidé de le séparer en 6 tâches distinctes comme proposé dans la donnée :



![](.\archtecture.PNG)

audio_data_i, sd_out et audio_out sont 3 buffers (boites aux lettres) permettant l'echange des données audios.

Voici la déscription des différentes tâches :

![](./explication_taches.PNG)

### La synchronisation

Afin de synchroniser les 6 tâches, plusieurs méthodes ont été utilisées :

Une sémaphore pour faire un barrière de synchronisation pour attendre que toutes les tâches soient prêtes.

Une variable globale "volume" (0-10) modifiée seulement dans la tâches "control_task" lors d'un changement de position des switches. Cette variable globale n'est pas protégée contre les accès concurrents. Il aurait été possible de mettre un mutex en place afin d'y remédier, mais nous n'avons pas jugé cette mesure nécessaire dans ce cas.  

Finalement, un drapeau événement (RT_EVENT) permet de communiquer le mode dans lequel nous sommes (playing, recording, paused) ainsi que des actions à traiter (forwarding, rewinding, new recording,...).

### Les périodes

Parmi ces 6 tâches, 3 d'entre elles exécutent de manière périodique :

- source_task : Période de 2ms, faute de quoi l'audio n'est pas lu assez vite ce qui mène à des problèmes à l'écoute.
- time_display_task: Période de 10 ms car c'est la plus petite résolution de l'affichage.
- control_task : Période de 150ms car cela nous semblait être une bonne valeur entre 2 appuis consécutifs sur des boutons, elle aurait pu être plus courte.

Remarque : Pour plus d'informations sur les périodes, veuillez-vous référer aux commentaire du code.

Pour les 3 autres tâches (audio_output_task, sdcard_writer_task, processing_task) elles attendent constamment la réception de données audio via leur boite aux lettre respectives.



### Les priorités





Le code présent dans **snd_player.c** détaille lui-aussi quelques-uns de nos choix, n'hésitez pas à la consulter pour plus d'informations.


### Utilisation

![](.\exemple_utilisation.PNG)

### Conclusion
Ce laboratoire nous a permis de mettre en pratique ce que nous avons vu en cours. L'avantage de ce laboratoire est qu'il est ludique et que les fichiers fournis permettent de rapidement se mettre dedans en comprenant ce qu'on doit faire et ce qui est déjà fait. C'était particulièrement sympa de pouvoir nous enregistrer et nous écouter, cela nous a permis de plaisanter en travaillant.

Idée :

Nous avons pensé qu'il serait intéressant de faire un laboratoire afin de prendre conscient de l'importance du temps reel. Par exemple, de nous fournir un programme compilable et fonctionnelle à moitié. l'idée serait que nous devrions nous ajouté de la synchronisation + priorité afin que le programme soit fonctionel à 100%.



Gerardi Tommy et Spinelli Isaia le 26 janvier 2020.
