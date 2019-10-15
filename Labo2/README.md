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

Remarque : Pour que la programme compile il a fallu utiliser "-lrt" afin qu'il connaise les fonctions "timer_create" et "timer_settime"

## Mesures

résultat pour un intervalle de 500 us :

![image](/img/res_500.png)
![image](/img/octave_500.png)

résultat pour un intervalle de 1000 us :
![image](/img/res_1000.png)
![image](/img/octave_1000.png)

résultat pour un intervalle de 250 us :
![image](/img/res_250.png)
![image](/img/octave_250.png)

Test pour 1000 mesures avec un interval de 1ms 2 fois de suite:
![image](/img/res_1000_2foi.png)


Remarque :

On peut constater que la moyenne est généralement assez précise. Par contre, la variance et le standard deviation n'est pas du tout régulier. Il peut vite y avoir des pics (entre 200 - 1200 environ).

## Perturbations

Pour cette étape, j'ai surtout regardé le standard deviation afin de voir facilement la perturbation qui pourrait être causer.

### nice

nice plusieurs coeurs :
![image](/img/res_1000_nice.png)
nice un coeur :
![image](/img/res_1000_nice_1c.png)

Nous pouvons voir les résultats une fois avec une niceness de -18 et une autre avec 18 ou 19. On peut remarquer qu'il n'y a pas de différence sur un coeur ou plusieurs.

nice + grep (1: avec grep - 2: sans grep) :
![image](/img/res_1000_nice_Grep.png)

Parcontre, en faisant une grosse opération, on peut voir la différence. La variance est plus élevé (1600) quand le grep tourne en même temps.

Afin de confirmer la perturbation avec grep, j'ai fait deux mesures en laissant grep tourner derrière. Ici on peut clairement voir que le standard deviation monte jusqu'a 4000.
(1 et 2: avec grep)
![image](/img/res_1000_nice_Grep_confirmatio.png)

Je pense que le processus grep tourne sur tous les coeurs mais que parfois, il utilise le coeur utilisé par le timer. Donc, il arrive à perturbe un peu le timer.

### ./cpu_loop

Plusieurs coeurs :
![image](/img/res_1000_cpu_loop.png)

Biensûr, on ne voit pas de différence car on a plusieurs coeurs. Par contre, voici deux mesures avec un coeur

Un coeur :
![image](/img/res_1000_cpu_loop_1c.png)

On peut voir que les résultats sont complétement à la masse. Ce qui est normal car le processus cpu_loop prend beaucoup de temps processeur. En effet, comme ils travaillent sur le même coeur, il peut arriver que le timer se fasse pré-empter à des moments critiques.

### Ping

Voici 2 mesures en recevant des pings:
![image](/img/res_1000_ping.png)
On peut voir que la variance est comme d'habitude, elle peut varier mais rien de très spécial.

## Conclusion

Afin de perturber notre timer, il faut prendre beaucoup de temps CPU sur le même coeur où le timer tourne. 
