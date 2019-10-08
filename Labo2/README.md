# PTR Laboratoire 2

## Gettimeofday et Gettimeofday2

3-4 On comparant les resultats de gettimeofday et gettimeofday2, On peut voir que le 2eme récupère des différence de temps plus faible. Ce qui est normal car il "perd" pas de temps à afficher entre chaque capture les resultats.

5. Etant donné que le fonction gettimeofday() manipule des structures avec des secondes et des microsecondes, on peut facilement imaginer que la précision tourne autour de la microsecondes et pas moins.

En annexe, le code dans "gettimeofday2.c"  

## Horloges Posix

Après avoir modifié le fichier gettimeofday.c afin d'utiliser la fonction clock_gettime() proposée par Posix, j'ai pu les comparer.
Donc, la comparaison entre la fonction proposée par Posix ("clock_gettime") et gettimeofday() est claire, Posix propose une methode avec une précision de **1ns** ce qui est 1000x meilleures que la précision de gettimeofday().

Finalement, la différence entre les horloges de Posix (CLOCK_REALTIME, CLOCK_MONOTONIC, CLOCK_PROCESS_CPUTIME_ID, CLOCK_THREAD_CPUTIME_ID) sont surtout à l'intervalle de quand la clock commence. Par exemple, la clock CLOCK_THREAD_CPUTIME_ID, va commencer lors du démarrage du thread. Donc, nous affichera plutot des secondes aux alentours de 0. Contrairement à la CLOCK_REALTIME qui représente les secondes et les nanosecondes depuis l'époque.


Remarque : CLOCK_REALTIME_HR et CLOCK_MONOTONIC_HR ne compile plus.


## Développement : timer
sigaction (SIGALRM, &sa, NULL); =>
Premièrement, il va indiquer que lorsque le signal SIGALRM sera reçu, il faudra executer le fonction donnée (timer_handler).

Ensuite, il initialise la structure timer de type struct itimerval qui permet de définir un temps de début et un temps d'interval.

setitimer (ITIMER_REAL, &timer, NULL); =>
Finalement, il va mettre en place un "minuteur" en fonction de la structure initialisée plutot. Ce minuteur enverra en fonction du type de la clock choisis un signal qui sera le même utilisé plus tot (SIGALRM) qui déclenchera donc la fonction prédéfinie.

En bref, après 250ms, ce logiciel appel la fonction souhaitée toutes les 250 ms.

En annexe, il y a le code avec quelques modifications. (timer_Example.c)

### Modifications

Il nous est demandé d'écrire un programme qui prend en entrée le nombre de mesures à faire et un temps en microse-condes.  De programmer  un  timer  périodique CLOCK_REALTIME qui  affiche  sur  la  sortie  standard  le temps écoulé entre deux différentes occurences.

le code est en annexe. (timer.c)

## Mesures

## Perturbations
