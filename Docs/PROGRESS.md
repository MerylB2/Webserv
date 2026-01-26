# Webserv - État d'avancement

## Résumé

**Progression estimée : ~25-30%**

Ce document résume ce qui a été implémenté et ce qui reste à faire selon le sujet `en.subject.pdf`.

---

## Implémenté

### Build & Compilation
- [x] Makefile avec règles : `$(NAME)`, `all`, `clean`, `fclean`, `re`
- [x] Compilation C++98 avec flags `-Wall -Wextra -Werror -std=c++98`
- [x] Détection OS (Linux/macOS)
- [x] Règle `make test` pour les tests unitaires

### Serveur Socket (Personne 3)
- [x] Création socket (`socket()`)
- [x] Configuration `SO_REUSEADDR`
- [x] Mode non-bloquant (`fcntl()` avec `O_NONBLOCK`)
- [x] Bind sur port (`bind()`)
- [x] Écoute (`listen()`)
- [x] Acceptation client (`accept()`)
- [x] Réception données (`recv()`)
- [x] Envoi données (`send()`)
- [x] Gestion signal `SIGINT` (Ctrl+C)

### Module HTTP (Personne 2)
- [x] Parsing requête HTTP (Request line, Headers, Body)
- [x] Parsing body chunked (`Transfer-Encoding: chunked`)
- [x] Extraction query string (`/path?param=value`)
- [x] Construction réponse HTTP (status line, headers, body)
- [x] Pages d'erreur par défaut (400, 403, 404, 405, 500, etc.)
- [x] Types MIME (`.html`, `.css`, `.js`, `.png`, `.jpg`, etc.)
- [x] Codes status HTTP avec messages
- [x] Router (matching de location) - *partiel*

### Structures de données (Dico.hpp)
- [x] `struct Request` - requête HTTP
- [x] `struct Response` - réponse HTTP
- [x] `struct ServerConfig` - configuration serveur
- [x] `struct LocationConfig` - configuration location
- [x] `struct ClientData` - données client
- [x] `struct CGIData` - données CGI
- [x] `enum RequestState` - états du parsing
- [x] `enum ClientState` - états du client
- [x] `namespace HttpStatus` - codes HTTP
- [x] `namespace MimeTypes` - types MIME
- [x] `namespace WebservConfig` - constantes

### Configuration (Personne 1)
- [x] Syntaxe fichier de config définie (`[server]...[/server]`)
- [x] Fichier exemple `config/default.conf`
- [x] ConfigParser - *en cours d'intégration*

### Documentation
- [x] `Docs/ARCHITECTURE.md` - structure du projet
- [x] `Docs/GIT_FLOW.md` - workflow Git
- [x] Pages d'erreur HTML (`www/error/`)

---

## Non implémenté (Mandatory)

### Critique - Bloquant pour l'évaluation

| Fonctionnalité | Priorité | Description |
|----------------|----------|-------------|
| `poll()` / `epoll()` | **P0** | Boucle événementielle unique pour toutes les I/O |
| Multi-clients | **P0** | Gérer plusieurs connexions simultanées |
| Argument `[config]` | **P0** | `./webserv [config_file]` |
| Servir fichiers statiques | **P0** | GET `/index.html` → lire et renvoyer le fichier |
| Intégration ConfigParser | **P1** | Appliquer la config au serveur |
| Méthode DELETE | **P1** | Supprimer des fichiers |
| Upload fichiers (POST) | **P1** | Multipart form-data |
| CGI | **P1** | Exécuter scripts PHP/Python avec `fork()`/`execve()` |
| Redirections HTTP | **P2** | 301/302 redirections |
| Directory listing | **P2** | Autoindex (lister contenu dossier) |
| Multi-ports | **P2** | Écouter sur plusieurs ports |
| Timeouts | **P2** | Déconnecter clients inactifs |
| `client_max_body_size` | **P2** | Limiter taille du body (413) |

### Important - Robustesse

| Fonctionnalité | Description |
|----------------|-------------|
| Ne jamais crash | Même en cas de OOM ou erreur |
| Keep-alive | Réutiliser les connexions TCP |
| Stress test | Rester disponible sous charge |

---

## Non implémenté (Bonus)

| Fonctionnalité | Description |
|----------------|-------------|
| Cookies / Sessions | Gestion de sessions utilisateur |
| Multiple CGI | Support PHP, Python, Perl, etc. |

---

## Fichiers du projet

```
Webserv/
├── Makefile                    # Build system
├── config/
│   └── default.conf            # Configuration exemple
├── includes/
│   ├── core/
│   │   ├── Dico.hpp            # Structures de données
│   │   └── Server.hpp          # Classe Server
│   └── http/
│       ├── Request.hpp         # Parsing requêtes
│       ├── Response.hpp        # Construction réponses
│       └── Router.hpp          # Routing
├── srcs/
│   ├── main.cpp                # Point d'entrée (serveur fusionné)
│   └── http/
│       ├── Request.cpp
│       ├── Response.cpp
│       └── Router.cpp
├── www/
│   └── error/                  # Pages d'erreur HTML
├── tests/
│   └── test_http.cpp           # Tests unitaires HTTP
└── Docs/
    ├── en.subject.pdf          # Sujet
    ├── ARCHITECTURE.md
    ├── GIT_FLOW.md
    └── PROGRESS.md             # Ce fichier
```

---

## Prochaines étapes

1. **Implémenter `poll()`** - Remplacer `usleep()` par une vraie boucle événementielle
2. **Gestion multi-clients** - Map de `ClientData` indexée par fd
3. **Intégrer ConfigParser** - Lire et appliquer `config/default.conf`
4. **Servir fichiers statiques** - `GET /file.html` → lire fichier et renvoyer
5. **CGI** - Fork + execve pour scripts

---

## Tests effectués

```bash
# Compilation
make clean && make          # OK

# Test basique
./webserv &
curl http://localhost:8080/ # OK - Retourne page HTML

# Test parsing
make test                   # OK - Tous les tests passent
```

---

## Contributeurs

- **Personne 1** : ConfigParser (`feature/config-parser`)
- **Personne 2** : HTTP Request/Response/Router (`feature/http-request-parser`)
- **Personne 3** : Server/Socket (`feature/serveur-socket`)

---

*Dernière mise à jour : Janvier 2025*
