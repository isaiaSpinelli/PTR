# PTR Laboratoire 3
## Linux et Xenomai

Spinelli Isaia

le 05 Novembre 2019

## Introduction et objectif

Lors du dernier labo nous avons testé les capacités d’une application à servir des tâches périodiques
sous différentes conditions. Nous allons maintenant effectuer des tests et récolter des données dans
un environnement plus chargé en termes d’utilisation du temps processeur. Afin de créer un tel
environnement, nous allons utiliser les commandes suivantes :

![image](./img/Commande_Dohell.png)

Les options du script dohell sont les suivantes :
1. -b hackbench : chemin de l’application ou du script à stresser
2. -s server [-p port] : destination à laquelle des paquets réseau seront envoyés via nc (ou netcat) ;
le port par défaut est 9
3. -m folder : répertoire (ou point de montage) dans lequel un fichier de 100MB sera créé et rempli
de 0 grâce à une boucle infinie
4. duration : temps d’exécution du script, en secondes



## Etape 1 Priorité des threads et Linux

Pour tous le laboratoire j'ai choisi une période de **500 us**. Afin de l'executer longtemps (70 secondes) il faut faire 140'000 mesures.


Il est important de lancer le timer sur le même coeur que le programme dohell afin de le perturber au maximum. Voici la commande executée:
![image](./img/E1_commande2.png)



Voici ci dessous 2 résultats différents:
 - 1 Timer sans perturbation
 - 2 Timer avec perturabtion

![image](./img/E1_court.png)

On peut clairement voir que sans perturbation le timer est correcte. Par contre, avec de la perturbation, la moyenne double donc il y a une erreur de 100%. De plus, la variance prend une valeur extreme. On ne peut clairement pas faire de temps réel avec ce genre de timer.

Afin d'avoir une mesure sur plus d'une minute avec perturbation, voici la commande et le résultat :

![image](./img/E1_long.png)

On peut voir que la moyenne est toujours completement fausse et la variance toujours très eleve.


## Étape 2 Priorités

Maintenant que nous avons un environnement pour lancer les expériences, nous allons l’utiliser sur
différentes solutions. Dans cette etape, nous avons écrit un programme qui exécute une tâche périodique dans une tâche haute priorité et qui utilise la fonction nanosleep() pour attendre une période de temps à l’intérieur d’une boucle.

Remarque : La commande pour la perturbation est toujours la même décrite dans l'introduction.

Voici la commande à taper pour lancer les mesures :

![image](./img/E2_commande2.png)

Remarque : la commande sudo est importante afin de pouvoir changer la priorité du thread.

Pour commencer, voici les résultats sans perturbation :
![image](./img/E2_sansperturabation.png)

On peut voir que la variance est enorme et que la moyenne n'est pas précise. Donc de base on peut constater que le nanosleep est moins précis qu'un timer ce qui semble correcte.

Ensuite, voici les resultats avec perturbations :

![image](./img/E2_long.png)

On peut voir que les résultats sont meilleurs. La moyenne est plus précise et la variance est moins extreme. Je ne serais pas expliquer pourquoi.


## Étape 3 Xenomai

Maintenant le but est de faire la même chose mais via l'interface native de Xenomai. Etant donné que celui-ci prend en charge le temps réel, on espère que les résultats seront bien plus précise que precedemment.

Remarque : La commande pour la perturbation est toujours la même décrite dans l'introduction.

Voici la commande à taper pour lancer les mesures :

![image](./img/E3_commande2.png)

On peut voir ici les résultats avec perturbations :

![image](./img/E3_long.png)

On peut voir ici que la moyenne est extremement précise ! On voir que le temps réel est un autre monde que le non temps reel.
De plus, la variance reste relativement faible pour un temps de 500us.