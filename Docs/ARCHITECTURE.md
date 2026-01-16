# Webserv - Architecture du Projet

## Vue d'ensemble

Serveur HTTP en C++98 capable de gérer GET, POST, DELETE, servir des fichiers statiques, exécuter des CGI et gérer l'upload de fichiers. Utilise I/O non-bloquant avec poll()/epoll().

---

## Structure des fichiers

```
webserv/
├── Makefile
├── config/
│   └── default.conf              # Configuration par défaut
│
├── includes/                     # Headers
│   ├── Types.hpp                 # Structures de données (FAIT)
│   ├── Server.hpp                # Classe serveur principal
│   ├── Config.hpp                # Parser de configuration
│   ├── Client.hpp                # Gestion des connexions client
│   ├── Request.hpp               # Parsing des requêtes HTTP
│   ├── Response.hpp              # Construction des réponses HTTP
│   ├── Router.hpp                # Routing des requêtes vers les handlers
│   ├── CGI.hpp                   # Gestion des scripts CGI
│   └── Utils.hpp                 # Fonctions utilitaires
│
├── srcs/
│   ├── main.cpp                  # Point d'entrée
│   │
│   ├── config/                   # Parsing de configuration
│   │   ├── ConfigParser.cpp      # Parse le fichier .conf
│   │   └── ConfigValidator.cpp   # Valide la configuration
│   │
│   ├── server/                   # Coeur du serveur
│   │   ├── Server.cpp            # Boucle principale poll()
│   │   ├── Socket.cpp            # Création/gestion des sockets
│   │   └── EventLoop.cpp         # Gestion des événements I/O
│   │
│   ├── client/                   # Gestion des clients
│   │   ├── Client.cpp            # Cycle de vie d'une connexion
│   │   └── ClientManager.cpp     # Pool de connexions
│   │
│   ├── http/                     # Protocole HTTP
│   │   ├── RequestParser.cpp     # Parse ligne de requête + headers + body
│   │   ├── ResponseBuilder.cpp   # Construit la réponse HTTP
│   │   └── HttpUtils.cpp         # MIME types, status codes, etc.
│   │
│   ├── handlers/                 # Traitement des requêtes
│   │   ├── Router.cpp            # Match URL -> LocationConfig
│   │   ├── GetHandler.cpp        # Méthode GET (fichiers statiques)
│   │   ├── PostHandler.cpp       # Méthode POST (upload)
│   │   ├── DeleteHandler.cpp     # Méthode DELETE
│   │   ├── DirectoryListing.cpp  # Autoindex
│   │   └── ErrorHandler.cpp      # Pages d'erreur
│   │
│   ├── cgi/                      # CGI
│   │   ├── CGIHandler.cpp        # Fork + execve du script
│   │   ├── CGIEnv.cpp            # Variables d'environnement CGI
│   │   └── CGIPipes.cpp          # Communication via pipes
│   │
│   └── utils/                    # Utilitaires
│       ├── StringUtils.cpp       # Manipulation de strings
│       ├── FileUtils.cpp         # Opérations sur fichiers
│       └── Logger.cpp            # Logging (optionnel)
│
├── www/                          # Fichiers web à servir
│   ├── index.html
│   ├── style.css
│   ├── uploads/                  # Dossier d'upload
│   └── error/
│       ├── 400.html
│       ├── 403.html
│       ├── 404.html
│       ├── 405.html
│       ├── 413.html
│       ├── 500.html
│       └── 502.html
│
├── cgi-bin/                      # Scripts CGI
│   ├── test.py
│   └── test.php
│
└── tests/                        # Tests
    ├── test_config.py
    ├── test_get.py
    ├── test_post.py
    ├── test_delete.py
    ├── test_cgi.py
    └── stress_test.py
```

---

## Modules et responsabilités

### 1. Config (Parsing de configuration)

**Fichiers**: `ConfigParser.cpp`, `ConfigValidator.cpp`

**Responsabilités**:
- Lire et parser le fichier de configuration style NGINX
- Remplir les structures `ServerConfig` et `LocationConfig`
- Valider les valeurs (ports, chemins, méthodes)

**Entrée**: Fichier `.conf`
**Sortie**: `std::vector<ServerConfig>`

---

### 2. Server (Boucle principale)

**Fichiers**: `Server.cpp`, `Socket.cpp`, `EventLoop.cpp`

**Responsabilités**:
- Créer les sockets d'écoute pour chaque port configuré
- Boucle infinie avec `poll()` / `epoll()`
- Accepter les nouvelles connexions
- Dispatcher les événements read/write aux clients
- Gérer les timeouts

**Cycle principal**:
```
while (running) {
    poll(fds, nfds, timeout);

    for (each fd with event) {
        if (fd is server socket)
            accept_new_client();
        else if (event is POLLIN)
            client.read_data();
        else if (event is POLLOUT)
            client.send_data();
    }

    check_timeouts();
    cleanup_closed_clients();
}
```

---

### 3. Client (Gestion des connexions)

**Fichiers**: `Client.cpp`, `ClientManager.cpp`

**Responsabilités**:
- Stocker l'état de chaque connexion (`ClientData`)
- Gérer le cycle: READING -> PROCESSING -> WRITING -> DONE
- Support du keep-alive (reset entre requêtes)
- Timeouts par client

---

### 4. HTTP Request (Parsing)

**Fichiers**: `RequestParser.cpp`

**Responsabilités**:
- Parser la request line: `GET /path HTTP/1.1`
- Parser les headers
- Gérer le body (Content-Length ou chunked)
- Décoder l'URI et query string

**États de parsing** (définis dans `Types.hpp`):
```
REQUEST_LINE -> HEADERS -> BODY/CHUNKED_BODY -> COMPLETE
```

---

### 5. HTTP Response (Construction)

**Fichiers**: `ResponseBuilder.cpp`, `HttpUtils.cpp`

**Responsabilités**:
- Construire la status line: `HTTP/1.1 200 OK`
- Ajouter les headers (Content-Type, Content-Length, etc.)
- Déterminer le MIME type selon l'extension
- Remplir le `send_buffer`

---

### 6. Handlers (Traitement des requêtes)

**Fichiers**: `Router.cpp`, `GetHandler.cpp`, `PostHandler.cpp`, `DeleteHandler.cpp`

#### Router
- Matcher l'URI avec les `LocationConfig`
- Trouver la location la plus spécifique

#### GetHandler
- Vérifier que GET est autorisé
- Résoudre le chemin du fichier
- Servir le fichier ou déclencher l'autoindex
- Gérer les redirections

#### PostHandler
- Vérifier que POST est autorisé
- Vérifier la taille du body vs `max_body_size`
- Sauvegarder le fichier uploadé
- Parser multipart/form-data si nécessaire

#### DeleteHandler
- Vérifier que DELETE est autorisé
- Supprimer le fichier demandé
- Retourner 204 No Content

---

### 7. CGI (Common Gateway Interface)

**Fichiers**: `CGIHandler.cpp`, `CGIEnv.cpp`, `CGIPipes.cpp`

**Responsabilités**:
- Détecter si l'URI matche une extension CGI (.py, .php)
- Préparer les variables d'environnement CGI
- Fork + execve du script
- Envoyer le body via pipe (stdin du CGI)
- Lire la sortie via pipe (stdout du CGI)
- Gérer le timeout CGI

**Variables d'environnement CGI**:
```
REQUEST_METHOD, QUERY_STRING, CONTENT_TYPE, CONTENT_LENGTH,
PATH_INFO, PATH_TRANSLATED, SCRIPT_NAME, SERVER_NAME,
SERVER_PORT, SERVER_PROTOCOL, HTTP_* (headers)
```

---

## Flux de données

```
Client HTTP                     Webserv
    |                              |
    |---- TCP connect ------------>| accept()
    |                              |
    |---- "GET /index.html..." --->| recv() -> RequestParser
    |                              |           |
    |                              |     Router.match()
    |                              |           |
    |                              |     GetHandler.handle()
    |                              |           |
    |                              |     ResponseBuilder.build()
    |                              |           |
    |<--- "HTTP/1.1 200 OK..." ----| send()
    |                              |
    |---- (keep-alive) ----------->| client.reset()
    |                              |
```

---

## Checklist des fonctionnalités

### Obligatoire

- [ ] Parsing de configuration (server, location, listen, root, index...)
- [ ] Écoute sur plusieurs ports
- [ ] I/O non-bloquant avec poll()/epoll()
- [ ] Gestion de multiples clients simultanés
- [ ] Méthode GET (fichiers statiques)
- [ ] Méthode POST (upload de fichiers)
- [ ] Méthode DELETE
- [ ] Pages d'erreur par défaut et personnalisées
- [ ] Redirections HTTP (301, 302)
- [ ] Autoindex (listing de répertoire)
- [ ] CGI (au moins Python ou PHP)
- [ ] Limitation de taille du body (client_max_body_size)
- [ ] Timeout client
- [ ] Keep-alive
- [ ] Ne jamais crasher / bloquer

### Bonus

- [ ] Cookies et sessions
- [ ] Plusieurs types de CGI

---

## Ordre d'implémentation suggéré

1. **Config Parser** - Lire le fichier de configuration
2. **Socket + Server** - Écouter sur un port, accepter des connexions
3. **Request Parser** - Parser les requêtes HTTP basiques
4. **Response Builder** - Construire des réponses HTTP
5. **GET Handler** - Servir des fichiers statiques
6. **Router** - Matcher les locations
7. **Error Pages** - Pages d'erreur
8. **POST Handler** - Upload de fichiers
9. **DELETE Handler** - Suppression de fichiers
10. **CGI** - Exécution de scripts
11. **Autoindex** - Listing de répertoires
12. **Redirections** - 301/302
13. **Multi-server** - Plusieurs blocs server
14. **Stress testing** - S'assurer de la robustesse

---

## Références

- RFC 2616 (HTTP/1.1)
- RFC 3875 (CGI)
- Configuration NGINX
- man pages: poll, epoll, socket, accept, send, recv, fork, execve, pipe
