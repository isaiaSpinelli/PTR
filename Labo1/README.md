# PTR Laboratoire 1

## 1ères mesures
1. programme (cpu_loop.c) qui compte le nombre d’itérations d’une boucle exécutée
durant n_sec secondes:

![image](/img/cpu_loop1.png)
2. le nombre d’itérations effectuées par seconde :

![image](/img/nbIteration1.png)

5.1. nombre d’itérations en fonction des opérations:

![image](/img/nbIterationOp.png)
On peut voir que la multiplication est un peu plus rapide que la division.

6. En variant les optimisations (-O2 et -O3), personnellement, j'ai pas l'impression qu'il ait beaucoup de différence.

## Multi-tâche : Commandes utiles

Commande permettant d'avoir des informations sur le ou les processeurs physiques :
**cat /proc/cpuinfo**

**man taskset** vous apprendra comment forcer l’exécution d’un processus sur un coeur spécifique.

Vérifier le statut d’un processus (**ps PID**).

Permet d’observer le statut des processus en cours d’exécution **ps maux**. Description de la colonne STAT :
![image](/img/STATE_PS.png)

## Ordonnancement en temps partagé
taskset vous permettra de forcer l’utilisation d’un seul coeur. (**taskset -pc 0 $$**)

En lancant l'application cpu_loop en parralèle (&) on peut voir que le nombre d'itération ce divise par nombre de processus lancé. Donc, il partage bien le meme processeur.

En ouvrant 2 shells et en utilisant la même commande taskset, on peut voir qu'ils se partagent tous le même precesseur. De plus, le shell ou on sleep fait encore moin d'itération surement car il est plus préempté.
![image](/img/Ordonnancement.png)

## Migration de tâches
Écrivez un programme (get_cpu_number.c) qui affiche à l’écran le numéro du CPU sur lequel il
tourne au démarrage et chaque déplacement qu’il subit (avec une indication de l’heure à laquelle le
déplacement se produit).
???

Exécution de cpu_loop sur le même CPU de get_cpu_number. Qu’est-ce qu’il se passe ?

Le nombre d'itération de cpu_loop chute car le precessus "get_cpu_number" prend des ressources.

Delogement de get_cpu_number :

![image](/img/delogement_h.png)

On peut voir que le processus "get_cpu_number" à changé plusieurs fois de CPU.

(htop permet de voir interactivement les processus et l'occupation des CPUs).

## Les priorités et niceness

nice -n 5 ./cpu_loop & ./cpu_loop

les priorités vont de -20 a 19 sachant que -20 est la plus prioritaire. Donc si on lance plusieurs processus en même temps, celui avec la priorité la plus haute a plus d'itération. On peut aussi remarquer que si on utilise nice et mettant une haute priorité au processus permetttant de deloger get_cpu_number, il se deloge immédiatement.

## Codage
