## Architecture du programme

Pour commencer, remettons en contexte le fonctionnement de la station météo et de ses 4 modes de fonctionnement :

•	Mode “standard” : Le système est démarré normalement pour faire l’acquisition des données.

•	Mode “configuration” : Le système est démarré avec le bouton rouge pressé. Il permet de configurer les paramètres du système, l’acquisition des capteurs est désactivée et le système bascule en mode standard au bout de 30 minutes sans activité.

•	Mode “maintenance” : Accessible depuis le mode standard ou économique, il permet d’avoir accès aux données des capteurs directement depuis une interface série et permet de changer en toute sécurité la carte SD sans risque de corrompre les données. On y accède en appuyant pendant 5 secondes sur le bouton rouge. En appuyant sur le bouton rouge pendant 5 secondes, le système rebascule dans le mode précédent.

•	Mode “économique” : Accessible uniquement depuis le mode standard, il permet d’économiser de la batterie en désactivant certains capteurs et traitements. On y accède en appuyant pendant 5 secondes sur le bouton vert. En appuyant 5 secondes sur le bouton vert, le système rebascule en mode standard. 

Notre programme devra récupérer les données des différents capteurs pour les inscrire dans la carte SD afin de prendre moins d’espace sur la SRAM. Il faudra que ces mesures soient horodatées grâce à l’horloge qui nous est fournie.

Pour déclancher le changement de mode lors de l'apui sur les boutons, nous allons utiliser deux fonctions d'interruption ``` attachInterrupt() ``` (chacune déclanchée par l'un ou l'autre des boutons) branchées sur les pins 2 et 3. Cette fonction permet  une interuption physique du µ-contrôleur et ainsi pouvoir changer de mode même lorsque que le programme réalise une tâche.

Nous allons utiliser un ```switch / case``` pour changer de mode (Standard, économique, maintenance et configuration) une fois l'appui sur le bouton détecté.

La première partie de notre code servira à lire les données des capteurs. Pour cela, nous allons créer une fonction ``` lecture() ``` qui va lire les données des capteurs, mais surtout laisser un intervalle de 10 minutes entre deux prises de mesure. Il va donc falloir utiliser une boucle ``` while() ``` composé d’une fonction ``` millis() ```, une simple soustraction et un ``` delay() ```.Cette boucle nous permet de compter le temps écoulé depuis le lancement du programme et donc d’effectuer les mesures à un intervalle de temps voulus. 

Ensuite, les valeurs seront lues par le programme grâce à ``` analogread() ``` et ``` digitalread() ```.

Pour ce qui est du stockage sur la carte SD, nous utiliserons la bibliothèque ```File``` et notement la fonction ``` File dataFile = SD.open(…,…) ``` qui associe la carte SD au nom dataFile. Cette manipulation nous permet ensuite d’utiliser la fonction ``` dataFile.println() ``` pour écrire les données des capteurs dans la carte SD.
