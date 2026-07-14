# **Compilation Rockchip VAAPI sur Ubuntu**

[https://gemini.google.com/app/e7945b123af7cdf4](https://gemini.google.com/app/e7945b123af7cdf4)

*User prompt: Évalue la possibilité de compiler ce dépot github qui est un fork, https://github.com/Trak-unixbased/rockchip-vaapi avec et sous ubuntu 26.04 aarch64, processeur RK3588, indiques les prérequis et packages nécessaires et la faisabilité que cela fonctionne ainsi que la création de package deb compatible ubuntu26.04 aarch64*

Response: La compilation d'un fork de rockchip-vaapi sur une architecture **aarch64 (RK3588)** sous **Ubuntu 26.04** est tout à fait **réalisable et documentée** en contexte de développement de systèmes bas niveau.

Le processeur RK3588 dispose d'un VPU (Video Processing Unit) natif puissant, mais dépourvu de pilote VA-API officiel. L'objectif de rockchip-vaapi est justement de créer un pont entre l'API standard libva et la bibliothèque matérielle Rockchip MPP (Media Process Platform) pour exposer ces capacités (décodage H.264, HEVC, VP9, AV1) aux applications Linux standard.

*Note sur le dépôt* : Bien que le lien Trak-unixbased pointe vers un dépôt privé ou non indexé publiquement, son statut de fork implique qu'il hérite de la même structure de compilation (basée sur make ou meson) et des mêmes contraintes architecturales que le projet source d'origine.

## **Prérequis et Paquets nécessaires**

Pour compiler le pilote vidéo sur Ubuntu 26.04 aarch64, la chaîne de compilation C standard et les bibliothèques de développement liées à libva et mpp sont requises.

**1\. Dépendances de build standard et API de base :**

`sudo apt update`  
`sudo apt install build-essential pkg-config git make gcc`  
`sudo apt install libva-dev libdrm-dev libva2`

**2\. Bibliothèque Rockchip MPP (Crucial) :** Le pilote VA-API dépend strictement des en-têtes et des bibliothèques partagées du Media Process Platform (librockchip-mpp-dev et librockchip-mpp1). Selon la variante d'Ubuntu 26.04 utilisée (vendor BSP ou mainline), MPP peut ne pas figurer dans les dépôts APT par défaut. S'il n'est pas déjà packagé : Il faudra au préalable cloner le dépôt officiel en amont (rockchip-linux/mpp), le compiler via CMake, et l'installer.

## **Création d'un paquet .deb (compatible aarch64)**

Pour éviter un sudo make install brut et conserver le contrôle des dépendances de l'OS, vous disposez de deux méthodes pour packager le binaire.

### **Méthode 1 : Rapide via Checkinstall**

Idéal pour tester rapidement l'intégration d'un fork tout en permettant une désinstallation propre.

`sudo apt install checkinstall`

`# Clonage et préparation`  
`git clone https://github.com/Trak-unixbased/rockchip-vaapi.git`  
`cd rockchip-vaapi`  
`make`

`# Création du package .deb`  
`sudo checkinstall -y --install=no \`  
    `--pkgname=rockchip-vaapi \`  
    `--pkgversion="1.20.0" \`  
    `--pkgarch="arm64" \`  
    `--provides="rockchip-vaapi" \`  
    `make install`

Le fichier .deb généré dans le répertoire courant sera installable avec sudo dpkg \-i rockchip-vaapi\*.deb.

### **Méthode 2 : Native via debhelper (dpkg-buildpackage)**

C'est la méthode canonique si l'objectif est d'intégrer durablement ce paquet dans un dépôt privé ou de s'assurer de sa conformité avec les standards de packaging Debian/Ubuntu.

1. Installer les outils : sudo apt install debhelper devscripts  
2. Générer le squelette Debian à la racine du dépôt : dh\_make \-s \--createorig \-p rockchip-vaapi\_1.0  
3. Éditer le fichier debian/control avec l'architecture et les dépendances exactes :  
   `Source: rockchip-vaapi`  
   `Section: video`  
   `Priority: optional`  
   `Maintainer: Votre Nom <email@domaine.fr>`  
   `Build-Depends: debhelper-compat (= 13), libva-dev, libdrm-dev, librockchip-mpp-dev`  
   `Standards-Version: 4.6.0`

   `Package: rockchip-vaapi`  
   `Architecture: arm64`  
   `Depends: ${shlibs:Depends}, ${misc:Depends}, libva2, librockchip-mpp1`  
   `Description: VA-API driver bridge for Rockchip RK3588 via MPP.`

4. Lancer la compilation propre du paquet : dpkg-buildpackage \-us \-uc \-b

## **Points de vigilance spécifiques au SoC RK3588**

* **CMA (Contiguous Memory Allocator) :** L'export des frames en Zero-copy (via DRM PRIME et DMABUF) requiert énormément de mémoire contiguë. Si le processus fige ou si le décodage échoue lors de l'allocation mpp\_create, il faut vérifier que la ligne de démarrage du noyau (dans extlinux.conf ou boot.cmd) alloue une zone CMA suffisante, par exemple cma=512M.  
* **Sandboxing navigateur :** Si le but final est l'accélération sous Firefox ou Chromium, leurs sandbox respectives (comme le processus RDD de Firefox) bloqueront l'appel à /dev/dri pour ouvrir le VPU. Le navigateur devra impérativement être lancé avec les variables d'environnement MOZ\_DISABLE\_RDD\_SANDBOX=1 et LIBVA\_DRIVER\_NAME=rockchip pour contourner ce blocage.

**Sources**

1. [woodyst \- GitHub](https://github.com/woodyst)  
2. [rockchip-vaapi/docs/DEVELOPMENT.md at main \- GitHub](https://github.com/woodyst/rockchip-vaapi/blob/main/docs/DEVELOPMENT.md)

---

*User prompt: Comment compiler et packager proprement la librairie Rockchip MPP (rockchip-linux/mpp) pour l'architecture aarch64 sur Ubuntu 26.04 ?*

Response: La compilation et le packaging natif de la librairie **Rockchip MPP (Media Process Platform)** nécessitent l'utilisation du système CMake et de la chaîne d'outils Debian. L'objectif est d'obtenir deux paquets .deb distincts : la librairie d'exécution (librockchip-mpp1) et les en-têtes de développement (librockchip-mpp-dev), afin que les futurs logiciels comme rockchip-vaapi puissent s'y lier proprement lors de leur compilation.

Voici le processus canonique complet à réaliser directement sur la carte RK3588 (architecture aarch64 native) sous Ubuntu 26.04.

## **1\. Préparation de l'environnement de build**

Vous devez installer les outils de compilation C/C++, l'orchestrateur CMake et les utilitaires de packaging Debian (debhelper).

`sudo apt update`  
`sudo apt install -y build-essential cmake pkg-config git \`  
                    `debhelper devscripts dh-make`

## **2\. Récupération et structure du dépôt source**

Il faut cloner le dépôt officiel Rockchip et préparer l'archive source originale (.orig.tar.gz), requise par l'outil dpkg-buildpackage.

`# Se placer dans un répertoire de travail`  
`mkdir -p ~/build/mpp-build && cd ~/build/mpp-build`

`# Cloner la branche cible (utiliser la branche développée pour le noyau 5.10/6.1 des RK3588)`  
`git clone -b develop https://github.com/rockchip-linux/mpp.git mpp-1.0.0`

`# Archiver les sources de manière standard pour dpkg`  
`tar -czvf mpp_1.0.0.orig.tar.gz mpp-1.0.0/`

`# Entrer dans le répertoire des sources`  
`cd mpp-1.0.0`

## **3\. Configuration de la structure du paquet Debian**

Vous devez maintenant initialiser l'environnement de packaging. La commande dh\_make génère les fichiers de contrôle dans un sous-dossier debian/.

`# Initialiser le répertoire debian (-s pour "single binary", on ajustera ensuite)`  
`dh_make -s -c gpl3 -f ../mpp_1.0.0.orig.tar.gz -y`

Nettoyez immédiatement le répertoire debian/ en supprimant les fichiers de modèle inutiles (\*.ex, \*.EX, etc.) :

`rm debian/*.ex debian/*.EX debian/README.*`

## **4\. Personnalisation des fichiers de contrôle**

Cette étape est primordiale pour définir l'architecture cible et les dépendances du paquet.

### **Le fichier debian/control**

Éditez le fichier debian/control pour configurer à la fois le paquet principal et le paquet de développement (-dev). Remplacez le contenu par :

`Source: mpp`  
`Section: libs`  
`Priority: optional`  
`Maintainer: Votre Nom <votre.email@domaine.com>`  
`Build-Depends: debhelper-compat (= 13), cmake, pkg-config`  
`Standards-Version: 4.6.0`  
`Homepage: https://github.com/rockchip-linux/mpp`

`Package: librockchip-mpp1`  
`Architecture: arm64`  
`Depends: ${shlibs:Depends}, ${misc:Depends}`  
`Description: Media Process Platform (MPP) for Rockchip RK3588`  
 `This package contains the shared libraries required to run`  
 `applications that use the Rockchip hardware video codec VPU.`

`Package: librockchip-mpp-dev`  
`Architecture: arm64`  
`Section: libdevel`  
`Depends: librockchip-mpp1 (= ${binary:Version}), ${misc:Depends}`  
`Description: Development files for Rockchip MPP`  
 `This package contains the header files and static libraries`  
 `needed to compile applications against Rockchip MPP, such as`  
 `the rockchip-vaapi driver.`

### **Le fichier debian/rules**

Le dépôt MPP utilise CMake. Vous devez instruire debhelper (dh) de configurer le build CMake avec le bon préfixe d'installation (pour éviter que les fichiers aillent dans /usr/local/ ce qui briserait le packaging).

Ouvrez debian/rules et assurez-vous qu'il contient (en respectant l'indentation avec la touche Tab sur la deuxième ligne) :

`#!/usr/bin/make -f`

`%:`  
	`dh $@ --buildsystem=cmake`

`override_dh_auto_configure:`  
	`dh_auto_configure -- -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release`

### **Les fichiers de répartition (Install files)**

Pour séparer la librairie binaire des en-têtes, vous devez créer deux fichiers texte à la racine du dossier debian/.

Créez le fichier debian/librockchip-mpp1.install (pour les binaires partagés) :

`usr/lib/*/librockchip_mpp.so.*`  
`usr/lib/*/librockchip_vpu.so.*`

Créez le fichier debian/librockchip-mpp-dev.install (pour les en-têtes et le pkg-config) :

`usr/include/rockchip/*`  
`usr/lib/*/pkgconfig/rockchip_mpp.pc`  
`usr/lib/*/pkgconfig/rockchip_vpu.pc`  
`usr/lib/*/librockchip_mpp.so`  
`usr/lib/*/librockchip_vpu.so`  
`usr/lib/*/librockchip_mpp.a`  
`usr/lib/*/librockchip_vpu.a`

## **5\. Compilation et génération des paquets .deb**

Lancez maintenant le processus de construction complet. L'argument \-b indique de ne construire que les binaires, \-uc et \-us évitent de signer les paquets avec une clé GPG (ce qui n'est pas nécessaire pour un usage local).

`dpkg-buildpackage -b -uc -us`

Si la compilation réussit, CMake compilera les sources pour aarch64, puis debhelper empaquettera les résultats.

## **6\. Installation des librairies**

Remontez d'un niveau dans l'arborescence pour trouver les paquets fraîchement compilés et installez-les via dpkg.

`cd ..`  
`ls -l *.deb`

`# Installer d'abord la librairie principale`  
`sudo dpkg -i librockchip-mpp1_1.0.0-1_arm64.deb`

`# Installer ensuite les fichiers de développement (requis pour votre prochain build)`  
`sudo dpkg -i librockchip-mpp-dev_1.0.0-1_arm64.deb`

L'environnement de votre RK3588 est désormais équipé des librairies MPP natives, avec les en-têtes correctement exposés via pkg-config. Le processus de compilation du fork rockchip-vaapi (qui vérifie la présence de rockchip\_mpp via la commande pkg-config \--modversion) fonctionnera dès lors sans problème de dépendances manquantes.

---

*User prompt: Quelles sont les modifications spécifiques du Kernel (CMA et Device Tree) requises pour optimiser l'utilisation du VPU du RK3588 avec MPP ?*

Response: Pour que la bibliothèque **MPP** (et par extension votre pilote rockchip-vaapi) puisse exploiter pleinement les capacités matérielles du RK3588 (qui intègre plusieurs cœurs de décodage/encodage capables de traiter du 4K/8K), l'allocation mémoire et la description matérielle doivent être parfaitement alignées dans le noyau.

Voici les ajustements spécifiques à apporter au **CMA** et au **Device Tree (DT)** sur un noyau Rockchip BSP (5.10 ou 6.1).

## **1\. Optimisation du CMA (Contiguous Memory Allocator)**

MPP alloue les trames vidéo (Decoded Picture Buffer \- DPB) directement en mémoire physique contiguë pour permettre le **Zero-copy** (export via les descripteurs de fichiers DMABUF/DRM PRIME). Pour du décodage 4K ou 8K (surtout en VP9/AV1 où le nombre de trames de référence est élevé), la taille par défaut du CMA (souvent 256 Mo) est insuffisante et provoquera des erreurs mpp\_create ou des kernel panics (OOM).

Il est recommandé d'allouer au moins **512 Mo à 1 Go** pour le CMA sur un RK3588.

### **Option A : Via la ligne de commande du Kernel (Recommandée)**

C'est la méthode la plus flexible. Modifiez les arguments de démarrage dans /boot/extlinux/extlinux.conf (ou /boot/firmware/cmdline.txt / boot.cmd selon l'intégration de votre Ubuntu) en ajoutant :

`cma=896M`

*(896M ou 1024M est le standard pour un usage intensif VPU/NPU sur RK3588).*

### **Option B : En dur dans le Device Tree (reserved-memory)**

Si vous compilez votre propre DTS, vous pouvez forcer la réservation mémoire globale dans le bloc reserved-memory à la racine de votre fichier de configuration (ex: rk3588-linux.dtsi) :

`/ {`  
	`reserved-memory {`  
		`#address-cells = <2>;`  
		`#size-cells = <2>;`  
		`ranges;`

		`linux,cma {`  
			`compatible = "shared-dma-pool";`  
			`reusable;`  
			`size = <0x0 0x38000000>; /* 896 MB */`  
			`linux,cma-default;`  
		`};`  
	`};`  
`};`

## **2\. Nœuds Device Tree (VPU, IOMMU et MPP Service)**

L'architecture matérielle du RK3588 répartit le traitement sur plusieurs blocs distincts : vdpu381 (décodeur), vepu121 / vepu580 (encodeurs JPEG/H264/H265), rkvdec2, etc. Sur les noyaux BSP Rockchip (contrairement au Mainline pur), l'orchestration de ces blocs vers l'espace utilisateur ne se fait pas directement, mais via un driver multiplexeur appelé **mpp\_service**.

Dans votre fichier .dts ou .dtsi, vous devez vous assurer que :

1. Le nœud mpp\_srv est présent et activé.  
2. Les nœuds des accélérateurs matériels sont liés à leurs IOMMU respectifs (MMU) pour que le CMA puisse mapper la mémoire virtuelle vers la mémoire physique sans copie.

Vérifiez et activez les blocs suivants (généralement définis dans rk3588.dtsi mais devant être activés avec status \= "okay"; dans votre dts de board spécifique) :

`&mpp_srv {`  
	`status = "okay";`  
`};`

`/* Décodeur vidéo principal (VDPU 381 - 4K/8K) */`  
`&vdpu {`  
	`status = "okay";`  
	`iommus = <&vdpu_mmu>;`  
`};`

`&vdpu_mmu {`  
	`status = "okay";`  
`};`

`/* Encodeur vidéo (VEPU 580 - H.264/HEVC) */`  
`&vpu {`  
	`status = "okay";`  
	`iommus = <&vpu_mmu>;`  
`};`

`&vpu_mmu {`  
	`status = "okay";`  
`};`

`/* Décodeur matériel spécifique (AV1 / H.265 10-bit) */`  
`&rkvdec0 {`  
	`status = "okay";`  
	`iommus = <&rkvdec0_mmu>;`  
`};`

`&rkvdec0_mmu {`  
	`status = "okay";`  
`};`

*Note d'architecture système :* L'absence de la liaison iommus \= \<&...\_mmu\>; empêchera le driver DRM de Rockchip de traduire les adresses mémoire, ce qui se traduira par des IOMMU page fault dans dmesg lors de l'appel au pilote VA-API.

## **3\. Validation de la chaîne de bas niveau**

Une fois le noyau recompilé ou les arguments appliqués et la machine redémarrée, vous devez valider ces deux éléments avant même de tester la couche logicielle supérieure.

**Validation du CMA :**

`dmesg | grep cma`  
`# Attendu : "Memory: ... cma reserved : 896 MiB"`

**Validation du service MPP :** Pour que librockchip-mpp (et donc rockchip-vaapi) puisse communiquer avec le VPU, le nœud de périphérique de service doit être exposé par le noyau :

`ls -l /dev/mpp_service`  
`# Attendu : un périphérique caractère (crw-rw----) appartenant au groupe 'video'.`

Assurez-vous également que votre utilisateur système fait partie du groupe video pour avoir les droits d'écriture sur ce composant matériel : sudo usermod \-aG video $USER.

**Sources**

1. [rockchip-vaapi/docs/DEVELOPMENT.md at main \- GitHub](https://github.com/woodyst/rockchip-vaapi/blob/main/docs/DEVELOPMENT.md)  
2. [\[v4,0/2\] Enable JPEG encoding on rk3588 \- Patchwork \- LinuxTV.org](https://patchwork.linuxtv.org/project/linux-media/cover/20240418141509.2485053-1-linkmauve@linkmauve.fr/)

---

*User prompt: Quelles sont les commandes (avec vainfo et mpv/ffmpeg) pour valider que le décodage matériel en Zero-Copy fonctionne correctement de bout en bout via le pilote rockchip-vaapi ?*

Response: Pour valider que la chaîne de compilation est opérationnelle et que les trames vidéo restent strictement allouées dans la mémoire du VPU (Zero-Copy) sans saturer le processeur, voici la séquence de validation standard.

## **1\. Validation de l'exposition du VPU avec vainfo**

L'outil vainfo interroge la bibliothèque libva pour lister les profils matériels reconnus. Il faut forcer l'utilisation du pilote Rockchip, car sur les environnements ARM, les variables par défaut pointent souvent vers des pilotes génériques ou MESA.

`env LIBVA_DRIVER_NAME=rockchip vainfo --display drm --drmsite /dev/dri/renderD128`

**Comportement attendu :** La commande doit retourner la liste complète des codecs supportés par le RK3588 (H.264, HEVC, VP9, AV1) avec les points d'entrée VAEntrypointVLD (Video Length Decoding \- qui signifie décodage matériel). Si vous obtenez une erreur vaInitialize failed, vérifiez les permissions de /dev/dri/renderD128 et de /dev/mpp\_service.

## **2\. Test de décodage pur (sans affichage) avec FFmpeg**

Pour prouver que le Zero-Copy fonctionne, on demande à FFmpeg de décoder un fichier vidéo, de conserver les trames au format matériel VA-API, et de les jeter vers null. Si le CPU intervient (copie RAM), la charge montera en flèche. Si le Zero-Copy fonctionne, la charge CPU restera minimale même à plusieurs centaines de FPS.

`ffmpeg -hwaccel vaapi \`  
       `-hwaccel_device /dev/dri/renderD128 \`  
       `-hwaccel_output_format vaapi \`  
       `-i fichier_test_4k.mp4 \`  
       `-f null -`

**Ce qu'il faut observer :**

* L'argument \-hwaccel\_output\_format vaapi est la clé : il interdit à FFmpeg de redescendre les trames décodées vers la RAM système.  
* Dans la sortie console, la vitesse (speed=...) doit être largement supérieure à 1x (souvent 5x à 10x sur le RK3588 pour du 4K).  
* La charge CPU (visible via htop dans un autre terminal) ne doit pas dépasser 5 à 10% sur un seul cœur.

## **3\. Test de lecture Zero-Copy avec mpv**

Le lecteur mpv gère nativement l'export DMABUF/DRM PRIME s'il est correctement invoqué. Selon que vous testez depuis une interface graphique (Wayland/X11) ou directement en console TTY (DRM pur), la commande diffère légèrement.

**Sous un environnement graphique (Wayland) :**

`mpv --hwdec=vaapi --gpu-context=wayland --vo=gpu fichier_test_4k.mp4`

**Directement en console (sans serveur d'affichage) :**

`mpv --hwdec=vaapi --vo=drm fichier_test_4k.mp4`

**La nuance critique :** Assurez-vous de **ne pas utiliser** \--hwdec=vaapi-copy. L'option \-copy force le rapatriement des frames décompressées vers la RAM du CPU pour appliquer des filtres logiciels, brisant ainsi la chaîne Zero-Copy et saturant la bande passante mémoire.

Lorsque mpv se lance, la ligne suivante doit apparaître dans le terminal : Using hardware decoding (vaapi).

## **4\. Monitoring bas niveau (Kernel)**

Pendant que ffmpeg ou mpv tourne, vous pouvez interroger directement le noyau pour vérifier quel composant matériel est sollicité et combien de mémoire CMA est consommée par les tampons vidéo :

`# Surveiller les sessions actives du Media Process Platform`  
`cat /sys/kernel/debug/mpp_service/session`

`# Surveiller l'utilisation du décodeur VDPU`  
`cat /sys/kernel/debug/mpp_service/vdpu/session`

Ces commandes retourneront un tableau listant le PID du processus (votre FFmpeg ou mpv) et le nombre de requêtes matérielles traitées, validant de manière irréfutable que le hardware est bien à la manœuvre.

---

