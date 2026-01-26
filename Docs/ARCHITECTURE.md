# Webserv - Architecture du Projet

## Vue d'ensemble

Serveur HTTP en C++98 capable de gérer GET, POST, DELETE, servir des fichiers statiques, exécuter des CGI et gérer l'upload de fichiers. Utilise I/O non-bloquant avec poll()/epoll().

---

## Structure des fichiers

```
webserv/
│
├── Makefile                              # Compilation du projet
├── README.md                             # Documentation du projet
│
├── config/                               # FICHIERS DE CONFIGURATION
│   ├── default.conf                      # Config par défaut (si aucun argument)
│   ├── multiple_servers.conf             # Config multi-serveurs (test multi-ports)
│   └── test.conf                         # Config minimale pour debug
│
├── includes/                             # HEADERS (.hpp)
│   │
│   ├── core/                             # Coeur du serveur
│   │   ├── Dico.hpp                      # Structures partagées (FAIT)
│   │   ├── Server.hpp                    # Classe principale, boucle poll()
│   │   ├── Client.hpp                    # Gestion d'une connexion
│   │   └── Socket.hpp                    # Gestion des sockets
│   │
│   ├── config/                           # Parsing configuration
│   │   └── ConfigParser.hpp              # Parser du fichier .conf
│   │
│   ├── http/                             # Protocole HTTP
│   │   ├── Request.hpp                   # Parsing des requêtes
│   │   ├── Response.hpp                  # Construction des réponses
│   │   └── Router.hpp                    # Matching des routes
│   │
│   ├── cgi/                              # Gestion CGI
│   │   └── CGIHandler.hpp                # fork/execve pour scripts
│   │
│   └── utils/                            # Utilitaires
│       ├── Utils.hpp                     # Fonctions helper
│       └── Logger.hpp                    # Logs du serveur (optionnel)
│
├── srcs/                                 # CODE SOURCE (.cpp)
│   │
│   ├── main.cpp                          # Point d'entrée
│   │
│   ├── core/                             # Coeur du serveur
│   │   ├── Server.cpp                    # Boucle principale
│   │   ├── Client.cpp                    # Gestion connexion
│   │   └── Socket.cpp                    # Gestion des sockets
│   │
│   ├── config/                           # Parsing configuration
│   │   ├── ConfigParser.cpp              # Parser principal
│   │   ├── ServerConfig.cpp              # Parsing bloc server
│   │   └── LocationConfig.cpp            # Parsing bloc location
│   │
│   ├── http/                             # Protocole HTTP
│   │   ├── Request.cpp                   # Parsing requêtes
│   │   ├── Response.cpp                  # Construction réponses
│   │   └── Router.cpp                    # Matching routes
│   │
│   ├── cgi/                              # Gestion CGI
│   │   ├── CGIHandler.cpp                # Implémentation CGI
│   │   └── FileHandler.cpp               # Gestion fichiers CGI
│   │
│   └── utils/                            # Utilitaires
│       ├── Utils.cpp                     # Fonctions helper
│       └── Logger.cpp                    # Logs (optionnel)
│
├── www/                                  # CONTENU WEB À SERVIR
│   │
│   ├── index.html                        # Page d'accueil par défaut
│   ├── style.css                         # Feuille de style
│   │
│   ├── pages/                            # Pages statiques
│   │   ├── about.html
│   │   └── contact.html
│   │
│   ├── errors/                           # Pages d'erreur personnalisées
│   │   ├── 400.html                      # Bad Request
│   │   ├── 403.html                      # Forbidden
│   │   ├── 404.html                      # Not Found
│   │   ├── 405.html                      # Method Not Allowed
│   │   ├── 413.html                      # Payload Too Large
│   │   └── 500.html                      # Internal Server Error
│   │
│   ├── uploads/                          # Dossier pour fichiers uploadés
│   │   └── .gitkeep                      # Garde le dossier vide dans git
│   │
│   └── cgi-bin/                          # Scripts CGI de test
│       ├── test.py                       # Script Python de test
│       ├── form.py                       # Traitement formulaire
│       └── info.php                      # Script PHP (si php-cgi dispo)
│
└── tests/                                # TESTS
    │
    ├── test_config/                      # Configs pour tests automatisés
    │   ├── valid.conf
    │   └── invalid.conf
    │
    ├── test_files/                       # Fichiers de différentes tailles
    │   ├── small.txt
    │   └── large.bin
    │
    └── scripts/                          # Scripts de test
        ├── test_get.sh                   # Tests requêtes GET
        ├── test_post.sh                  # Tests requêtes POST
        ├── test_cgi.sh                   # Tests CGI
        └── stress_test.sh                # Tests de charge
```

---

## Répartition pour 3 personnes

### PERSONNE 1 : Configuration + Serveur

```
├── includes/config/ConfigParser.hpp
├── includes/core/Server.hpp
├── includes/core/Socket.hpp
├── srcs/config/ConfigParser.cpp
├── srcs/config/ServerConfig.cpp
├── srcs/config/LocationConfig.cpp
├── srcs/core/Server.cpp
├── srcs/core/Socket.cpp
├── srcs/main.cpp
└── config/*.conf
```

**Responsabilités** :
- Parser le fichier de configuration
- Créer les sockets d'écoute
- Boucle principale poll()
- Accepter les nouvelles connexions

---

### PERSONNE 2 : HTTP (Request/Response/Router)

```
├── includes/http/Request.hpp
├── includes/http/Response.hpp
├── includes/http/Router.hpp
├── srcs/http/Request.cpp
├── srcs/http/Response.cpp
├── srcs/http/Router.cpp
└── www/errors/*.html
```

**Responsabilités** :
- Parser les requêtes HTTP (méthode, URI, headers, body)
- Construire les réponses HTTP
- Matcher les URLs avec les locations
- Gérer GET, POST, DELETE
- Pages d'erreur

---

### PERSONNE 3 : Client + CGI

```
├── includes/core/Client.hpp
├── includes/cgi/CGIHandler.hpp
├── srcs/core/Client.cpp
├── srcs/cgi/CGIHandler.cpp
├── srcs/cgi/FileHandler.cpp
└── www/cgi-bin/*.py
```

**Responsabilités** :
- Gérer le cycle de vie d'une connexion client
- Gérer les états (READING → PROCESSING → WRITING)
- Fork + execve des scripts CGI
- Communication via pipes
- Timeouts

---

### PARTAGÉ (tout le monde)

```
├── includes/core/Dico.hpp       ← À définir ENSEMBLE au début
├── includes/utils/Utils.hpp
├── srcs/utils/Utils.cpp
└── tests/*
```

---

## Modules et responsabilités

### 1. Dico.hpp (FAIT)

**Contenu** :
- `LocationConfig` : config d'un bloc location
- `ServerConfig` : config d'un bloc server
- `Request` : requête HTTP parsée
- `Response` : réponse HTTP à envoyer
- `ClientData` : état d'une connexion
- `RequestState` : états du parsing
- `ClientState` : états du client
- `WebservConfig` : constantes (timeouts, buffers)
- `HttpStatus` : codes HTTP

---

### 2. ConfigParser

**Fichiers** : `ConfigParser.cpp`, `ServerConfig.cpp`, `LocationConfig.cpp`

**Entrée** : Fichier `.conf`
**Sortie** : `std::vector<ServerConfig>`

**Directives supportées** :
```
server {
    listen 8080;
    server_name localhost;
    root ./www;
    index index.html;
    client_max_body_size 10M;
    error_page 404 /errors/404.html;

    location /path {
        root ./www/path;
        allowed_methods GET POST;
        autoindex on;
        index index.html;
        upload_path ./www/uploads;
        cgi_extension .py /usr/bin/python3;
        return 301 /redirect;
    }
}
```

---

### 3. Server

**Fichiers** : `Server.cpp`, `Socket.cpp`

**Boucle principale** :
```cpp
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

### 4. Client

**Fichiers** : `Client.cpp`

**Cycle de vie** :
```
CLIENT_READING      → recv() données
CLIENT_PROCESSING   → traiter la requête
CLIENT_WAITING_CGI  → attendre le CGI (si applicable)
CLIENT_WRITING      → send() réponse
CLIENT_DONE         → reset() ou fermer
```

---

### 5. Request

**Fichiers** : `Request.cpp`

**Parsing d'une requête** :
```
GET /search?q=hello HTTP/1.1\r\n     ← REQUEST_LINE
Host: localhost\r\n                   ← HEADERS
Content-Length: 5\r\n                 ← HEADERS
\r\n                                  ← Fin headers
Hello                                 ← BODY
```

**États** : `REQUEST_LINE` → `HEADERS` → `BODY` → `COMPLETE`

---

### 6. Response

**Fichiers** : `Response.cpp`

**Construction d'une réponse** :
```
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 13\r\n
\r\n
<html>...</html>
```

---

### 7. Router

**Fichiers** : `Router.cpp`

**Responsabilités** :
- Matcher l'URI avec la `LocationConfig` la plus spécifique
- Vérifier que la méthode est autorisée
- Résoudre le chemin du fichier sur le disque
- Déclencher le bon handler (fichier, CGI, redirect, autoindex)

---

### 8. CGIHandler

**Fichiers** : `CGIHandler.cpp`, `FileHandler.cpp`

**Processus** :
1. Détecter si l'extension matche (.py, .php)
2. Préparer les variables d'environnement
3. `fork()`
4. Dans l'enfant : `dup2()` + `execve()`
5. Dans le parent : écrire le body dans le pipe, lire la sortie

**Variables d'environnement CGI** :
```
REQUEST_METHOD, QUERY_STRING, CONTENT_TYPE, CONTENT_LENGTH,
PATH_INFO, SCRIPT_NAME, SERVER_NAME, SERVER_PORT,
SERVER_PROTOCOL, HTTP_* (headers convertis)
```

---

## Flux de données

```
Client HTTP                     Webserv
    |                              |
    |---- TCP connect ------------>| accept() → nouveau ClientData
    |                              |
    |---- "GET /index.html..." --->| recv() → Request.parse()
    |                              |           |
    |                              |     Router.match(uri)
    |                              |           |
    |                              |     [GetHandler / PostHandler / CGI]
    |                              |           |
    |                              |     Response.build()
    |                              |           |
    |<--- "HTTP/1.1 200 OK..." ----| send()
    |                              |
    |---- (keep-alive) ----------->| client.reset()
    |                              |
```

---

## Checklist des fonctionnalités

### Obligatoire

- [x] Structures de données (Dico.hpp)
- [ ] Parsing de configuration
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

| Étape | Module | Description |
|-------|--------|-------------|
| 1 | Dico.hpp | Structures de données (FAIT) |
| 2 | ConfigParser | Lire le fichier de configuration |
| 3 | Socket + Server | Écouter sur un port, accepter des connexions |
| 4 | Client | Gérer le cycle de vie d'une connexion |
| 5 | Request | Parser les requêtes HTTP |
| 6 | Response | Construire des réponses HTTP |
| 7 | Router | Matcher les locations |
| 8 | GET Handler | Servir des fichiers statiques |
| 9 | Error Pages | Pages d'erreur |
| 10 | POST Handler | Upload de fichiers |
| 11 | DELETE Handler | Suppression de fichiers |
| 12 | CGI | Exécution de scripts |
| 13 | Autoindex | Listing de répertoires |
| 14 | Redirections | 301/302 |
| 15 | Multi-server | Plusieurs blocs server |
| 16 | Stress testing | Robustesse |

---

## Références

- RFC 2616 (HTTP/1.1)
- RFC 3875 (CGI)
- Configuration NGINX
- `man poll`, `epoll`, `socket`, `accept`, `send`, `recv`, `fork`, `execve`, `pipe`
