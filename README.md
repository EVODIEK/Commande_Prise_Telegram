# Multiprise commandée par Telegram (ESP32 + 2 relais)

Ce projet permet de commander à distance **deux prises** depuis un smartphone, au moyen d'un **bot Telegram**. Un ESP32, connecté au WiFi, interroge l'API Telegram, exécute les commandes reçues en actionnant deux relais, puis renvoie l'état des prises. Aucun serveur intermédiaire ni application dédiée n'est nécessaire.

## Sommaire

1. [Fonctionnalités](#fonctionnalités)
2. [Principe de fonctionnement](#principe-de-fonctionnement)
3. [Matériel requis](#matériel-requis)
4. [Câblage](#câblage)
5. [Création du bot Telegram](#création-du-bot-telegram)
6. [Installation logicielle](#installation-logicielle)
7. [Configuration des identifiants](#configuration-des-identifiants)
8. [Utilisation](#utilisation)
9. [Fonctionnement détaillé du programme](#fonctionnement-détaillé-du-programme)
10. [Sécurité](#sécurité)
11. [Dépannage](#dépannage)
12. [Limites et améliorations possibles](#limites-et-améliorations-possibles)
13. [Structure du dépôt](#structure-du-dépôt)

## Fonctionnalités

- Commande indépendante de deux prises (`ON1`, `OFF1`, `ON2`, `OFF2`) et commande groupée (`ALLON`, `ALLOFF`).
- Consultation de l'état des prises (`STATUS`) et aide intégrée (`/help`).
- Commandes insensibles à la casse (`on1`, `ON1`, `On1` sont équivalents).
- Accès réservé à une conversation Telegram autorisée ; toute autre tentative est refusée et **signalée au propriétaire**.
- Notification au démarrage de la carte, avec l'état initial des prises.
- Relais désactivés au démarrage (état initial sûr).
- Reconnexion WiFi automatique, avec redémarrage de la carte en cas d'échec prolongé.
- Identifiants isolés dans un fichier `secrets.h` exclu du dépôt.

## Principe de fonctionnement

```
 Smartphone                 Serveurs Telegram                  ESP32
┌────────────┐   ON1      ┌──────────────────┐   HTTPS      ┌────────────────┐
│ Application │──────────►│ API Bot Telegram │◄─────────────│ Interrogation  │
│ Telegram    │◄──────────│                  │  (toutes les │ toutes les     │
└────────────┘ confirmation└─────────────────┘   1,2 s)     │ 1,2 s          │
                                                            └───┬────────┬───┘
                                                         GPIO 23│        │GPIO 22
                                                         ┌──────▼───┐ ┌──▼───────┐
                                                         │ Relais 1 │ │ Relais 2 │
                                                         └────┬─────┘ └────┬─────┘
                                                           Prise 1      Prise 2
```

L'ESP32 fonctionne par **interrogation périodique** (*polling*) : toutes les 1,2 seconde, il demande à Telegram les nouveaux messages, vérifie l'identité de l'expéditeur, exécute la commande, puis répond. Le délai de réaction est donc de l'ordre de 1 à 3 secondes.

## Matériel requis

| Élément | Remarque |
|---|---|
| Carte ESP32 (ex. ESP32 DevKit) | Connexion WiFi 2,4 GHz |
| Module relais 2 voies | Logique **inversée** : niveau bas = relais actif ; de préférence à optocoupleurs |
| Prises ou charges à commander | Dans la limite du calibre des relais |
| Fils de liaison, alimentation USB 5 V | |
| Boîtier isolant, fusible adapté | Indispensables pour une charge sur le secteur |
| Smartphone avec Telegram | Pour créer le bot et envoyer les commandes |

## Câblage

| Module relais | ESP32 |
|---|---|
| `VCC` | `5V` (ou `3V3` selon le module) |
| `GND` | `GND` |
| `IN1` | `GPIO 23` (prise 1) |
| `IN2` | `GPIO 22` (prise 2) |

Pour chaque voie, la charge se branche sur les bornes `COM` et `NO` (normalement ouvert) du relais, en série avec le conducteur de phase.

> **Avertissement de sécurité.** La commande d'appareils sur le secteur (220–230 V) présente un risque d'électrocution et d'incendie. Mettre hors tension avant toute intervention, séparer physiquement la partie basse tension (ESP32) de la partie secteur, utiliser un boîtier isolant, un fusible dimensionné et des conducteurs de section adaptée, et faire valider le montage par une personne qualifiée. Pour les essais, utiliser une charge basse tension (LED, lampe 12 V).

## Création du bot Telegram

1. Dans Telegram, ouvrir la conversation avec **@BotFather**.
2. Envoyer `/newbot`, puis choisir un nom et un identifiant (se terminant par `bot`).
3. Conserver le **jeton** (*token*) fourni : il donne un contrôle total sur le bot.
4. Récupérer votre **identifiant de conversation** (*chat ID*) : envoyer un message à **@userinfobot**, qui renvoie votre identifiant numérique.
5. Ouvrir la conversation avec le bot créé et appuyer sur **Démarrer**, afin qu'il puisse vous écrire.

## Installation logicielle

### 1. Environnement

- Installer l'[IDE Arduino](https://www.arduino.cc/en/software).
- Ajouter le support ESP32 (gestionnaire de cartes, paquet « esp32 » d'Espressif).
- Sélectionner la carte correspondant à votre modèle (par exemple « ESP32 Dev Module »).

### 2. Bibliothèques

À installer depuis le gestionnaire de bibliothèques :

- `UniversalTelegramBot`
- `ArduinoJson` (version 6.x recommandée pour la compatibilité avec `UniversalTelegramBot`)

Les bibliothèques `WiFi` et `WiFiClientSecure` sont fournies avec le paquet ESP32.

## Configuration des identifiants

Les identifiants ne figurent pas dans le dépôt. Pour les définir :

1. Dans le dossier `Commande_Prise_Telegram/`, copier `secrets.h.example` en `secrets.h`.
2. Renseigner les quatre valeurs :

| Constante | Contenu |
|---|---|
| `WIFI_SSID` | Nom du réseau WiFi (2,4 GHz) |
| `WIFI_PASSWORD` | Mot de passe du réseau |
| `BOT_TOKEN` | Jeton fourni par @BotFather |
| `OWNER_CHAT_ID` | Identifiant numérique de la conversation autorisée |

3. Ouvrir `Commande_Prise_Telegram/Commande_Prise_Telegram.ino` dans l'IDE Arduino, sélectionner le port, puis cliquer sur **Téléverser**.
4. Ouvrir le moniteur série à 115200 bauds : l'adresse IP s'affiche une fois le WiFi connecté. Le bot envoie alors un message de démarrage dans Telegram.

## Utilisation

Dans la conversation avec le bot :

| Commande | Action |
|---|---|
| `ON1` / `OFF1` | Active / désactive la prise 1 |
| `ON2` / `OFF2` | Active / désactive la prise 2 |
| `ALLON` | Active les deux prises |
| `ALLOFF` | Désactive les deux prises |
| `STATUS` | Affiche l'état des prises |
| `/start` | Message d'accueil et état courant |
| `/help` | Liste des commandes |

Toute autre saisie renvoie « Commande inconnue ».

## Fonctionnement détaillé du programme

- **`setRelay(pin, on)`** : applique l'état souhaité en tenant compte de la logique inversée (`on` → niveau bas, `off` → niveau haut).
- **`applyCommand()`** : normalise le texte reçu (suppression des espaces, majuscules), exécute la commande correspondante et met à jour l'état mémorisé (`relay1On`, `relay2On`).
- **`handleNewMessages()`** : compare l'identifiant de l'expéditeur à `OWNER_CHAT_ID`. En cas de différence, le message est refusé et le propriétaire est averti (identifiant et nom de l'expéditeur).
- **`connectWiFi()`** : tente la connexion pendant 20 secondes, puis redémarre la carte en cas d'échec.
- **`loop()`** : vérifie la connexion WiFi, puis interroge Telegram toutes les 1,2 seconde.

## Sécurité

- **Ne jamais publier** le jeton du bot ni le fichier `secrets.h`. Le `.gitignore` du dépôt l'exclut.
- Si le jeton a été exposé (dépôt public, capture d'écran, message), le **révoquer** immédiatement : @BotFather → `/revoke` → sélection du bot, puis mettre à jour `secrets.h`.
- Le filtrage par `OWNER_CHAT_ID` empêche toute autre personne de commander les prises, même si elle trouve le bot.
- Le certificat du serveur Telegram n'est pas vérifié (`setInsecure()`) : acceptable pour une démonstration, déconseillé sur un réseau non fiable.
- Les commandes passent par un service externe : en cas de panne d'Internet ou de Telegram, les prises ne sont plus pilotables à distance.

## Dépannage

| Symptôme | Cause probable | Solution |
|---|---|---|
| Points affichés indéfiniment, redémarrages répétés | Échec de connexion WiFi | Vérifier le SSID, le mot de passe et la bande 2,4 GHz |
| Aucun message de démarrage | Jeton ou identifiant erroné, bot non démarré | Vérifier `BOT_TOKEN`, `OWNER_CHAT_ID` et appuyer sur **Démarrer** |
| « Accès refusé » reçu | `OWNER_CHAT_ID` incorrect | Corriger l'identifiant dans `secrets.h` |
| Prise active alors que l'état affiche OFF | Module relais à logique directe | Inverser les niveaux dans `setRelay()` |
| Relais qui claque sans commutation | Alimentation insuffisante | Alimenter le module en 5 V, vérifier la masse commune |
| Erreur de compilation `secrets.h` | Fichier absent | Copier `secrets.h.example` en `secrets.h` |
| Erreur de compilation liée à ArduinoJson | Version incompatible | Installer ArduinoJson 6.x |

## Limites et améliorations possibles

- L'état des prises n'est pas mémorisé : après une coupure de courant ou un redémarrage, elles sont désactivées.
- Le mode interrogation introduit un délai de quelques secondes.
- Pas de retour d'état physique : l'état affiché est l'état commandé, non mesuré (un capteur de courant permettrait de le vérifier).
- Extensions envisageables : programmation horaire, minuterie, clavier de boutons Telegram, autorisation de plusieurs utilisateurs, vérification du certificat TLS, mesure de consommation.

## Structure du dépôt

```
Commande_Prise_Telegram/
├── Commande_Prise_Telegram/
│   ├── Commande_Prise_Telegram.ino   # Firmware
│   └── secrets.h.example             # Modèle d'identifiants (à copier en secrets.h)
├── .gitignore                        # Exclut secrets.h
└── README.md
```
