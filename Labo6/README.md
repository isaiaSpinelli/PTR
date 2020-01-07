# PTR Laboratoire 6
## Isaïa Spinelli et Tommy Gerardi, le 07.01.2020


### Introduction
Le but de ce laboratoire est de réaliser un enregistreur audio capable de rejouer ce qui vient d’êtreenregistré, et ce sur une carte DE1-SoC.


### Choix architecturaux
Afin de réaliser notre lecteur audio, nous avons décidé de le séparer en 3 tâches disctinctes. La première s'occupe de la lecture du fichier audio, ainsi que de la gestion du volume de ce dernier. La deuxième tâche a pour but de gérer l'affichage du temps sur les afficheurs 7 segments. Finalement la dernière tâche gère l'interface utilisateur, c'est-à-dire tout ce qui est boutons.  


Afin de synchroniser les 3 tâches, plusieurs méthodes ont été utilisées.  
Premièrement, deux variables globales permettent aux tâches de connaître le mode actuel de lecture (play/pause/stop/end), ainsi que le volume de la musique (0-10). Ces variables globales ne sont, dans notre cas, pas protégées contre les accès concurrents. Il aurait été possible de mettre des mutex en place afin d'y remédier, mais nous n'avons pas jugé cette mesure nécessaire dans ce cas.  
Deuxièmement, nous avons aussi mis en place des drapeaux événements permettant de communiquer les appuis sur les boutons aux autres tâches.  


Nous avons séparé les tâches de la sorte car cela permettait une meilleure gestion des périodes d'exécution pour chacunes de celles-ci.  
Par exemple, la tâche qui s'occupe de l'affichage doit s'exécuter toutes les 10 millisecondes, car c'est la plus petite période qu'on peut représenter avec 2 afficheurs 7 segments. La tâche qui s'occupe de lire le fichier à quant à elle une période de 2ms, faute de quoi l'audio n'est pas lu assez vite ce qui mène à des problèmes à l'écoute. Si ces deux tâches n'avaient pas été séparées, la gestion de l'affichage aurait été plus compliqué. La période de lecture des boutons est de 150ms, cela nous semblait être une bonne valeur entre 2 appuis consécutifs sur des boutons, elle aurait pû être plus courte.  

Le code présent dans **snd_player.c** détaille lui-aussi quelques-uns de nos choix, n'hésitez pas à la consulter pour plus d'informations.

### Conclusion
Ce laboratoire nous a permis de mettre en pratique ce que nous avons vu en cours. L'avantage de ce laboratoire est qu'il est ludique et que les fichiers fournis permettent de rapidement se mettre dedans en comprennant ce qu'on doit faire et ce qui est déjà fait.
