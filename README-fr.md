 [RELEASE] rockchip-vaapi v1.0.11 — Décodage matériel stable 4K@60fps VP9 dans Firefox

Salut tout le monde,

Je viens de sortir la version v1.0.11 de mon pilote VA-API pour le RK3588 (Orange Pi 5 Plus, Rock 5B, etc.), qui relie libva à Rockchip MPP pour le décodage vidéo matériel dans Firefox.

GitHub: https://github.com/woodyst/rockchip-vaapi

---
Ce que ça fait

Permet le décodage matériel de VP9 / H.264 / HEVC / VP8 dans Firefox via VA-API → MPP → RK3588 VPU.

Chemin d'affichage DMA-BUF sans copie. Fonctionne avec YouTube, des fichiers locaux via mpv, et d'autres clients VA-API.

---
Quoi de neuf dans v1.0.11

Les versions précédentes repassaient sur le logiciel après quelques secondes de contenu 4K. Cette version corrige cela :

- 4K@60fps est maintenant stable — testé >30 000 images dans Firefox avec des changements de qualité DASH propres

et pas de NS_ERROR_DOM_MEDIA_FATAL_ERR

- Fin d'image asynchrone — le pilote bloquait le thread de décodage de Firefox jusqu'à 1.6s sur les images clés de segment, gelant le pipeline. La fin d'image retourne maintenant immédiatement

- Correction de condition de course — Firefox obtenait un DMA-BUF obsolète avant que l'image décodée soit

prête, causant une barre verte en haut de l'écran

- Journalisation désormais désactivée par défaut — ~500 fprintf/s ralentissaient silencieusement le décodage à 60fps ; activez avec RK_VAAPI_LOG=/tmp/rk.log si nécessaire

- mpv --vo=dmabuf-wayland — écran vert corrigé (format d'export COMPOSED_LAYERS)

- mpv --hwdec=vaapi-copy — écran vert corrigé (GetImage était une implémentation vide)

- Exigence CMA documentée — 4K nécessite cma=512M ; comprend une correction fdtput pour les cartes où le DTB remplace la ligne de commande du noyau

---
Exigences

- CMA ≥ 512MB (critique pour 4K — voir INSTALL.md si cma=512M dans la ligne de commande n'est pas effectif)

- librockchip-mpp, libva

- Firefox avec media.ffmpeg.vaapi.enabled = true et MOZ_DISABLE_RDD_SANDBOX=1

---
Installation rapide

git clone https://github.com/woodyst/rockchip-vaapi

cd rockchip-vaapi

make

sudo make install

Puis lancez Firefox :

LIBVA_DRIVER_NAME=rockchip MOZ_DISABLE_RDD_SANDBOX=1 firefox

Instructions complètes dans INSTALL.md

(https://github.com/woodyst/rockchip-vaapi/blob/main/INSTALL.md).

---

Vos retours et rapports de bugs sont les bienvenus. Testé sur Orange Pi 5 Plus avec Armbian et Firefox 128.
