Pour valider que la chaîne de compilation est opérationnelle et que les trames vidéo restent strictement allouées dans la mémoire du VPU (Zero-Copy) sans saturer le processeur, voici la séquence de validation standard.

## **1\. Validation de l'exposition du VPU avec vainfo**

L'outil vainfo interroge la bibliothèque libva pour lister les profils matériels reconnus. Il faut forcer l'utilisation du pilote Rockchip, car sur les environnements ARM, les variables par défaut pointent souvent vers des pilotes génériques ou MESA.

Bash  
env LIBVA\_DRIVER\_NAME=rockchip vainfo \--display drm \--drmsite /dev/dri/renderD128

**Comportement attendu :**  
La commande doit retourner la liste complète des codecs supportés par le RK3588 (H.264, HEVC, VP9, AV1) avec les points d'entrée VAEntrypointVLD (Video Length Decoding \- qui signifie décodage matériel). Si vous obtenez une erreur vaInitialize failed, vérifiez les permissions de /dev/dri/renderD128 et de /dev/mpp\_service.

## **2\. Test de décodage pur (sans affichage) avec FFmpeg**

Pour prouver que le Zero-Copy fonctionne, on demande à FFmpeg de décoder un fichier vidéo, de conserver les trames au format matériel VA-API, et de les jeter vers null. Si le CPU intervient (copie RAM), la charge montera en flèche. Si le Zero-Copy fonctionne, la charge CPU restera minimale même à plusieurs centaines de FPS.

Bash  
ffmpeg \-hwaccel vaapi \\  
       \-hwaccel\_device /dev/dri/renderD128 \\  
       \-hwaccel\_output\_format vaapi \\  
       \-i fichier\_test\_4k.mp4 \\  
       \-f null \-

**Ce qu'il faut observer :**

* L'argument \-hwaccel\_output\_format vaapi est la clé : il interdit à FFmpeg de redescendre les trames décodées vers la RAM système.  
* Dans la sortie console, la vitesse (speed=...) doit être largement supérieure à 1x (souvent 5x à 10x sur le RK3588 pour du 4K).  
* La charge CPU (visible via htop dans un autre terminal) ne doit pas dépasser 5 à 10% sur un seul cœur.

## **3\. Test de lecture Zero-Copy avec mpv**

Le lecteur mpv gère nativement l'export DMABUF/DRM PRIME s'il est correctement invoqué. Selon que vous testez depuis une interface graphique (Wayland/X11) ou directement en console TTY (DRM pur), la commande diffère légèrement.  
**Sous un environnement graphique (Wayland) :**

Bash  
mpv \--hwdec=vaapi \--gpu-context=wayland \--vo=gpu fichier\_test\_4k.mp4

**Directement en console (sans serveur d'affichage) :**

Bash  
mpv \--hwdec=vaapi \--vo=drm fichier\_test\_4k.mp4

**La nuance critique :**  
Assurez-vous de **ne pas utiliser** \--hwdec=vaapi-copy. L'option \-copy force le rapatriement des frames décompressées vers la RAM du CPU pour appliquer des filtres logiciels, brisant ainsi la chaîne Zero-Copy et saturant la bande passante mémoire.  
Lorsque mpv se lance, la ligne suivante doit apparaître dans le terminal :  
Using hardware decoding (vaapi).

## **4\. Monitoring bas niveau (Kernel)**

Pendant que ffmpeg ou mpv tourne, vous pouvez interroger directement le noyau pour vérifier quel composant matériel est sollicité et combien de mémoire CMA est consommée par les tampons vidéo :

Bash  
\# Surveiller les sessions actives du Media Process Platform  
cat /sys/kernel/debug/mpp\_service/session

\# Surveiller l'utilisation du décodeur VDPU  
cat /sys/kernel/debug/mpp\_service/vdpu/session

Ces commandes retourneront un tableau listant le PID du processus (votre FFmpeg ou mpv) et le nombre de requêtes matérielles traitées, validant de manière irréfutable que le hardware est bien à la manœuvre.
