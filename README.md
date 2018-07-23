# Piscine

Pour ceux que ça intéresse, j'ai fait un automatisme largement inspiré du blog :
http://clement.storck.me/blog/2014/08/controle-et-supervision-de-la-piscine/

Pour les fonctions/informations, il intègre:
- remontée de la température du local
- remontée de la température de l'eau
- remontée du Ph
- remontée du Redox
- pilotage de l'éclairage à distance
- pilotage de la pompe à distance
- automatisme de la pompe
- régulation du chlore et mise en sécurité si la température de l'eau est inférieure à 15°C
- fonction Hors-Gel

Pour la gestion horaire, j'ai gardé la minuterie en façade de mon coffret d'origine.

Les 2 potentiomètres permettent de recaler les valeurs mesurées (ORP et Ph) pour corriger la dérive des sondes.

La principale différence par rapport au projet du blog citée précédemment, est le passage sur un Z-Uno. J'ai déjà un réseau Zwave et la gestion des commandes et des remontées d'informations à Jeedom est nettement plus facile.

Jérôme
