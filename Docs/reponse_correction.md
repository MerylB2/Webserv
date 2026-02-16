# Réponses aux Questions de Correction

## 1. Les bases d'un serveur HTTP

Un serveur HTTP est un programme qui écoute les requêtes HTTP provenant de clients (navigateurs web, applications) et leur renvoie des réponses HTTP appropriées.

### Principe de fonctionnement

- Le serveur crée un socket et se met en écoute sur un port spécifique (ex: 80 pour HTTP, 443 pour HTTPS)
- Il accepte les connexions entrantes des clients
- Il reçoit et analyse les requêtes HTTP (méthode, URI, headers, body)
- Il traite la requête et génère une réponse appropriée
- Il envoie la réponse au client (code de statut, headers, body)

### Principales méthodes HTTP

- **GET : Récupérer une ressource** - Demande au serveur de renvoyer le contenu d'une page ou d'un fichier
- **POST : Envoyer des données** - Soumettre des informations au serveur (formulaires, uploads)
- **DELETE : Supprimer une ressource** - Demander la suppression d'une ressource sur le serveur

### Codes de statut HTTP

- **2xx (Succès)** : 200 OK, 201 Created, 204 No Content
- **3xx (Redirection)** : 301 Moved Permanently, 302 Found, 304 Not Modified
- **4xx (Erreur client)** : 400 Bad Request, 404 Not Found, 403 Forbidden
- **5xx (Erreur serveur)** : 500 Internal Server Error, 502 Bad Gateway, 503 Service Unavailable

---

## 2. Fonction d'I/O Multiplexing utilisée

Pour ce projet, j'ai utilisé la fonction **`poll()`** qui permet de surveiller plusieurs descripteurs de fichiers (sockets) simultanément sans bloquer le programme.

### Avantages de poll() par rapport à select()

- Pas de limitation sur le nombre de descripteurs (pas de FD_SETSIZE)
- Interface plus simple avec un tableau de structures pollfd
- Pas besoin de recalculer le maximum des descripteurs

### Alternatives possibles

- **`select()`** : Plus ancien, limité à FD_SETSIZE descripteurs
- **`epoll()`** : Spécifique à Linux, plus performant pour un très grand nombre de connexions

## 3. Fonctionnement de poll()

La fonction `poll()` permet de surveiller plusieurs descripteurs de fichiers et de savoir quand ils sont prêts pour une opération d'I/O sans bloquer le programme.

### Prototype de la fonction

```c
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

### Structure pollfd

```c
struct pollfd {
    int   fd;       // Descripteur de fichier à surveiller
    short events;   // Événements à surveiller (POLLIN, POLLOUT)
    short revents;  // Événements qui se sont produits (rempli par poll)
};
```

### Paramètres

- **fds** : Tableau de structures pollfd à surveiller
- **nfds** : Nombre d'éléments dans le tableau fds
- **timeout** : Durée maximale d'attente en millisecondes (-1 = attente infinie)

### Événements principaux

- **POLLIN** : Données disponibles en lecture
- **POLLOUT** : Le descripteur est prêt pour l'écriture
- **POLLERR** : Erreur sur le descripteur
- **POLLHUP** : Connexion fermée

### Fonctionnement étape par étape

1. **Initialisation** : On prépare un tableau de pollfd avec les descripteurs à surveiller et les événements souhaités (POLLIN, POLLOUT)
2. **Appel à poll()** : Le programme se bloque jusqu'à ce qu'au moins un événement se produise ou que le timeout expire
3. **Retour de poll()** : Le champ `revents` de chaque structure pollfd est rempli avec les événements qui se sont produits
4. **Vérification** : On parcourt le tableau et on vérifie `revents` pour savoir quels descripteurs sont prêts

---

### Architecture du serveur

- **Socket serveur (accept)** : Ajouté au tableau avec `events = POLLIN` car une nouvelle connexion se manifeste comme une donnée à lire
- **Sockets clients (read)** : Ajoutés avec `events = POLLIN` lorsqu'ils attendent de recevoir des données
- **Sockets clients (write)** : Ajoutés avec `events = POLLOUT` lorsqu'ils ont une réponse à envoyer

### Processus de traitement

Après le retour de poll(), je parcours le tableau de pollfd :

- Si le socket serveur a `revents & POLLIN` → j'appelle `accept()` pour accepter la nouvelle connexion
- Si un socket client a `revents & POLLIN` → j'appelle `read()` pour recevoir la requête
- Si un socket client a `revents & POLLOUT` → j'appelle `write()` pour envoyer la réponse

### Fonctionnalités principales implémentées

- Serveur HTTP non-bloquant avec poll()
- Gestion de plusieurs clients simultanés
- Support des méthodes GET, POST, DELETE
- Fichiers de configuration pour virtual hosts et routes
- Gestion d'erreurs robuste avec POLLERR et POLLHUP
- Respect strict de toutes les contraintes du sujet

---

## Résumé des Points Critiques

|                               Critère                             | Statut |
|-------------------------------------------------------------------|--------|
| poll() vérifie POLLIN et POLLOUT simultanément                    | ✅ OUI |
| Un seul read/write par client par poll()                          | ✅ OUI |
| Vérification complète des valeurs de retour (-1 ET 0)             | ✅ OUI |
| Aucune vérification d'errno après I/O (sauf pour poll() et EINTR) | ✅ OUI |
| Toutes les I/O passent par poll()                                 | ✅ OUI |
| Compilation sans re-link                                          | ✅ OUI |
