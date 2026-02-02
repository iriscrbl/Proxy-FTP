# 🌐 Proxy FTP

Un serveur proxy FTP développé en C qui permet de relayer les connexions FTP entre des clients et des serveurs FTP distants, avec gestion du mode actif et passif.

## 👥 Équipe

- **OUMERRETANE Emmy**
- **NGUYEN Phuong**
- **CORBILLE Iris**

**Groupe B - Projet R3.05**

---

## 📋 Table des matières

- [Description](#-description)
- [Fonctionnalités](#-fonctionnalités)
- [Prérequis](#-prérequis)
- [Installation](#-installation)
- [Utilisation](#-utilisation)
- [Architecture technique](#-architecture-technique)
- [Exemples d'utilisation](#-exemples-dutilisation)
- [Dépannage](#-dépannage)
- [Licence](#-licence)

---

## 📖 Description

Ce proxy FTP agit comme intermédiaire entre un client FTP et un serveur FTP distant. Il intercepte les commandes du client, les traite et les transmet au serveur approprié. Le proxy gère automatiquement la conversion des connexions de données du mode **actif (PORT)** vers le mode **passif (PASV)**.

### Pourquoi un proxy FTP ?

- **Sécurité** : Contrôle centralisé des connexions FTP
- **Compatibilité** : Conversion automatique entre mode actif et passif
- **Multi-clients** : Gestion simultanée de plusieurs clients grâce aux processus fork
- **Transparence** : Le client utilise une syntaxe simplifiée `USER login@serveur`

---

## ✨ Fonctionnalités

- ✅ **Connexion de contrôle** : Établissement de la connexion client ↔ proxy ↔ serveur
- ✅ **Authentification** : Parsing de la commande `USER login@serveur.ftp.com`
- ✅ **Mode actif vers passif** : Conversion automatique `PORT` → `PASV`
- ✅ **Transfert de données** : Relais transparent des données entre client et serveur
- ✅ **Commandes supportées** :
  - `USER` (avec syntaxe `login@serveur`)
  - `PASS`
  - `LIST`
  - `RETR`
  - `STOR`
  - `QUIT`
  - Et toutes les autres commandes FTP standards
- ✅ **Multi-clients** : Gestion de plusieurs connexions simultanées via `fork()`

---

## 🔧 Prérequis

- **Système d'exploitation** : Linux (Ubuntu, Debian, etc.)
- **Compilateur** : GCC
- **Bibliothèques** : 
  - `stdio.h`
  - `stdlib.h`
  - `sys/socket.h`
  - `netdb.h`
  - `string.h`
  - `unistd.h`
- **Permissions** : Droits d'exécution et de création de sockets
- **Fichier requis** : `simpleSocketAPI.h` (bibliothèque de gestion des sockets)

---

## 📥 Installation

### 1. Cloner le dépôt

```bash
git clone https://github.com/votre-username/proxy-ftp.git
cd proxy-ftp
```

### 2. Vérifier la présence de `simpleSocketAPI.h`

Assurez-vous que le fichier `simpleSocketAPI.h` est présent dans le même répertoire que `proxy.c`.

### 3. Compiler le programme

```bash
gcc -o proxy proxy.c -Wall
```

Si vous avez des avertissements, vous pouvez les ignorer ou les corriger selon vos besoins.

---

## 🚀 Utilisation

### Lancer le proxy

```bash
./proxy <port_serveur_FTP>
```

**Paramètres :**
- `<port_serveur_FTP>` : Le port sur lequel le serveur FTP distant écoute (généralement `21`)

**Exemple :**

```bash
./proxy 21
```

Le proxy affichera alors :

```
L'adresse d'ecoute est: 127.0.0.1
Le port d'ecoute est: 45678
```

> ⚠️ **Note** : Le port d'écoute du proxy est attribué automatiquement (défini à `0` dans `SERVPORT`)

### Se connecter avec un client FTP

Une fois le proxy lancé, connectez-vous avec votre client FTP favori :

#### Avec `ftp` + ses options en ligne de commande :

```bash
ftp-ssl -z nossl -d 127.0.0.1 55415
```

Remplacez `45678` par le port affiché par le proxy.

#### Commande de connexion :

Lorsque le client demande un nom d'utilisateur, utilisez le format :

```
USER login@serveur.ftp.com
```

**Exemple avec un serveur FTP anonyme :**

```
Name: anonymous@ftp.fr.debian.org
Password: [Votre email ou laissez vide]
```

Le proxy va :
1. Extraire le login (`anonymous`) et le serveur (`ftp.fr.debian.org`)
2. Se connecter au serveur FTP distant
3. Relayer toutes les commandes et données entre vous et le serveur

---

## 🏗️ Architecture technique

### Schéma de fonctionnement

```
┌─────────┐         ┌─────────┐         ┌─────────────┐
│ Client  │ ◄─────► │  PROXY  │ ◄─────► │ Serveur FTP │
│   FTP   │         │   FTP   │         │   distant   │
└─────────┘         └─────────┘         └─────────────┘
     │                   │                      │
     │   Connexion       │   Connexion          │
     │   de contrôle     │   de contrôle        │
     │                   │                      │
     └─── Canal DATA ────┴──── Canal DATA ──────┘
       (mode actif)           (mode passif)
```

### Flux de connexion

1. **Initialisation du proxy** :
   - Création de la socket serveur
   - Liaison (bind) sur `127.0.0.1:0` (port automatique)
   - Mise en écoute avec `listen()`

2. **Connexion client** :
   - Acceptation de la connexion (`accept()`)
   - Création d'un processus fils (`fork()`) pour gérer le client
   - Envoi du message de bienvenue `220`

3. **Authentification** :
   - Réception de `USER login@serveur`
   - Extraction du login et du serveur
   - Connexion au serveur FTP distant
   - Transmission de `USER login` au serveur

4. **Gestion des commandes** :
   - **PORT** : Conversion automatique en `PASV`
   - **LIST, RETR, etc.** : Transfert des données via les canaux établis
   - **Autres commandes** : Relais transparent

5. **Déconnexion** :
   - Fermeture des sockets de données
   - Fermeture des sockets de contrôle
   - Terminaison du processus fils

### Conversion PORT → PASV

Le proxy transforme automatiquement les connexions actives en passives :

- **Client** : Envoie `PORT 192,168,1,100,195,80`
- **Proxy** : 
  - Parse l'IP et le port du client (`192.168.1.100:50000`)
  - Se connecte au client en mode actif
  - Envoie `PASV` au serveur
  - Parse la réponse `227 Entering Passive Mode (...)` du serveur
  - Se connecte au serveur en mode passif
  - Confirme au client : `200 PORT command successful`

---

## 💡 Exemples d'utilisation

### Exemple 1 : Connexion au serveur Debian

```bash
# Terminal 1 : Lancer le proxy
./proxy 21

# Sortie :
# L'adresse d'ecoute est: 127.0.0.1
# Le port d'ecoute est: 34567

# Terminal 2 : Se connecter avec ftp
ftp-ssl -z nossl -d 127.0.0.1 55415

# Connexion :
Name: anonymous@ftp.fr.debian.org
Password: [Entrée]

# Commandes FTP disponibles :
ftp> ls
ftp> cd debian
ftp> get README
ftp> quit
```

### Exemple 2 : Connexion à un serveur privé

```bash
# Terminal 1 : Proxy
./proxy 21

# Terminal 2 : Client
ftp 127.0.0.1 [port_affiché]

Name: monlogin@ftp.monserveur.com
Password: monmotdepasse

ftp> ls
ftp> put fichier.txt
ftp> quit
```

### Logs du proxy

Le proxy affiche des logs détaillés pour le débogage :

```
(PROXY) Client connecté : PID = 12345)
(PROXY) commande reçue : USER anonymous@ftp.fr.debian.org
(PROXY) login = anonymous
(PROXY) serveur = ftp.fr.debian.org
(PROXY) Tentative de connexion au serveur ftp.fr.debian.org:21...
(PROXY) Connecté au serveur.
(PROXY) Client: PORT 127,0,0,1,195,80
(PROXY) IP client: 127.0.0.1, port client: 50000
(PROXY) Connecté au client
(PROXY) Envoi au serveur: PASV
(PROXY) Réponse PASV du serveur: 227 Entering Passive Mode (...)
(PROXY) Transfert de données entre serveur et client
(PROXY) Transfert terminé

````
## 📝 Structure du code

```
proxy-ftp/
│
├── proxy.c              # Code source principal
├── simpleSocketAPI.h    # Bibliothèque de gestion des sockets
├── README.md            # Ce fichier
```

---

## 🔒 Sécurité

⚠️ **Avertissement** : Ce proxy est développé à des fins éducatives. Pour un usage en production, considérez :

- Ajouter une authentification au niveau du proxy
- Implémenter le chiffrement (FTPS/SFTP)
- Limiter les commandes autorisées
- Ajouter des logs d'audit
- Gérer les timeout de connexion
- Valider toutes les entrées utilisateur

---

## 📚 Ressources

- [RFC 959 - FTP Protocol](https://www.rfc-editor.org/rfc/rfc959)
- [Guide sur les sockets en C](https://beej.us/guide/bgnet/)
- [Documentation FTP](https://www.ietf.org/rfc/rfc959.txt)

---

## 📄 Licence

Ce projet est développé dans le cadre d'un projet universitaire (R3.05 - Groupe B).

---

## 📧 Contact

Pour toute question ou suggestion :

- **Emmy OUMERRETANE**    https://github.com/emmyo-git
- **Phuong NGUYEN**       https://github.com/phoocore
- **Iris CORBILLE**       https://github.com/iriscrbl

---
