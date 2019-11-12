
/** \brief Fonction permettant de simuler de l'utilisation de temps CPU
*
* Cette fonction peut être appelée par plusieurs tâches de manière
* concurrente. Chaque tâche consommera ensuite le temps CPU demandé.
*
* \param ms Nombre de millisecondes à consommer
*/
void busy_cpu(int ms);
