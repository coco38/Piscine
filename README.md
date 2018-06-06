# Piscine

Pour ceux que ça intéresse, j'ai fait un automatisme largement inspiré du blog :
http://clement.storck.me/blog/2014/08/controle-et-supervision-de-la-piscine/

Pour les fonctions/informations, il intègre:
- remontée de la température du local (pour la fonction hors-gel)
- remontée de la température de l'eau
- remontée du Ph
- remontée du Redox (pour régulation du chlore)
- pilotage de l'éclairage à distance
- pilotage de la pompe à distance
- automatisme de la pompe 

Pour la gestion horaire, j'ai gardé la minuterie en façade de mon coffret d'origine.

Pour le matériel, je suis parti sur:
- un Z-Uno
- un afficheur LCD 4x20
- une carte 4 relais (pilotage du 220V)
- 2 cartes Phidgets Adaptateur pH/ORP 1130

La principale différence par rapport au projet du blog citée précédemment, est le passage sur un Z-Uno. J'ai déjà un réseau Zwave et la gestion des commandes et des remontées d'informations à Jeedom est nettement plus facile.

Jérôme
