# Webserv - État d'avancement

## Résumé

**Progression estimée : ~45%**

Ce document résume ce qui a été implémenté et ce qui reste à faire selon le sujet

---

## Vue d'ensemble par composant

| Composant | Avancement | Notes |
|-----------|------------|-------|
| Configuration | 80% | Parser OK, intégration manquante |
| HTTP Parsing | 90% | GET/POST/DELETE/Chunked OK |
| HTTP Response | 70% | Construction OK, envoi incomplet |
| Server Core | 60% | Sockets + poll() OK, Client class manquante |
| CGI | 5% | Structures définies seulement |
| **TOTAL** | **~45%** | |

---

## Implémenté ✅

### Build & Compilation
- [x] Makefile avec règles : `$(NAME)`, `all`, `clean`, `fclean`, `re`
- [x] Compilation C++98 avec flags `-Wall -Wextra -Werror -std=c++98`
- [x] Détection OS (Linux/macOS)
- [x] Règles additionnelles : `make test`, `make debug`, `make valgrind`

### Serveur Socket (server.cpp - 330 lignes)
- [x] Création socket (`socket()`)
- [x] Configuration `SO_REUSEADDR`
- [x] Mode non-bloquant (`fcntl()` avec `O_NONBLOCK`)
- [x] Bind sur port (`bind()`)
- [x] Écoute (`listen()`)
- [x] Accept clients (`accept()`)
- [x] **Boucle poll() fonctionnelle** (`run()` ligne 287-326)
- [x] `buildPollFds()` - prépare les FDs à surveiller
- [x] `handleNewConnections()` - accepte nouveaux clients
- [x] `handleClientEvents()` - recv/send avec les clients
- [x] Parsing requête + construction réponse intégrés
- [x] Gestion signal `SIGINT`/`SIGTERM` (Ctrl+C)

### Module HTTP - Parsing (Request.cpp)
- [x] Parsing requête HTTP (Request line, Headers, Body)
- [x] Support méthodes GET, POST, DELETE
- [x] Parsing body avec `Content-Length`
- [x] Parsing body chunked (`Transfer-Encoding: chunked`)
- [x] Extraction query string (`/path?param=value`)
- [x] Machine à états pour parsing incrémental
- [x] Détection erreurs de parsing (400 Bad Request)

### Module HTTP - Réponse (Response.cpp)
- [x] Construction réponse HTTP (status line, headers, body)
- [x] Codes status HTTP avec messages (namespace HttpStatus)
- [x] Types MIME automatiques (namespace MimeTypes)
- [x] Helper `makeError()` pour pages d'erreur
- [x] Helper `makeRedirect()` pour redirections
- [x] Tracking des bytes envoyés (pour envois partiels)

### Module HTTP - Routing (Router.cpp)
- [x] Matching de location (longest prefix match)
- [x] Vérification méthodes autorisées
- [x] Résolution de chemin (URI → filesystem)
- [x] Détection type de route (FILE, DIRECTORY, CGI, REDIRECT, ERROR)
- [x] Structure `RouteResult`

### Structures de données (Dico.hpp - 499 lignes)
- [x] `struct Request` - requête HTTP complète
- [x] `struct Response` - réponse HTTP complète
- [x] `struct ServerConfig` - configuration serveur
- [x] `struct LocationConfig` - configuration location
- [x] `struct ClientData` - données client avec états
- [x] `struct CGIData` - données CGI (pid, pipes, buffer, timeout)
- [x] `enum RequestState` - états du parsing (REQUEST_LINE → COMPLETE)
- [x] `enum ClientState` - états du client (READING → DONE)
- [x] `enum RouteType` - types de routes
- [x] `namespace HttpStatus` - tous les codes HTTP
- [x] `namespace MimeTypes` - types MIME courants
- [x] `namespace WebservConfig` - constantes (timeouts, buffer sizes)

### Configuration - Parser (ConfigParser.cpp - 412 lignes)
- [x] Syntaxe fichier de config (`[server]...[/server]`, `[location]...[/location]`)
- [x] Directive `listen` (port ou host:port)
- [x] Directive `server_name` (multiple noms)
- [x] Directive `root` (serveur et location)
- [x] Directive `index` (multiple fichiers)
- [x] Directive `error_page` (code + chemin)
- [x] Directive `client_max_body_size` (avec K/M/G)
- [x] Directive `allowed_methods` (GET POST DELETE)
- [x] Directive `autoindex` (on/off)
- [x] Directive `return` (code + URL redirection)
- [x] Directive `cgi_extension` (extension + interpréteur)
- [x] Directive `upload_path`
- [x] Validation de la configuration
- [x] Fichier exemple `config/default.conf` (2 serveurs)
- [x] 15 fichiers de tests config (`tests/configs/test*.conf`)

### Pages d'erreur HTML (www/errors/)
- [x] 400 Bad Request
- [x] 403 Forbidden
- [x] 404 Not Found
- [x] 405 Method Not Allowed
- [x] 413 Payload Too Large
- [x] 500 Internal Server Error

### Documentation
- [x] `Docs/ARCHITECTURE.md` - structure du projet
- [x] `Docs/GIT_FLOW.md` - workflow Git
- [x] `Docs/en.subject.pdf` - sujet officiel

---

## Partiellement implémenté ⚠️

| Fonctionnalité | État | Ce qui manque |
|----------------|------|---------------|
| Boucle poll() | ✅ Fonctionnelle | Manque classe Client complète |
| Multi-clients | ⚠️ Basique OK | Map existe, Client* = NULL temporaire |
| Servir fichiers | Router OK | Lecture fichier et envoi non implémentés |
| Autoindex | Directive parsée | Génération HTML du listing manquante |
| Pages erreur custom | Fichiers existent | `generateErrorPage()` incomplet |
| Multi-ports | Config OK | Binding sur plusieurs ports incomplet |

---

## Non implémenté ❌ (Mandatory)

### Priorité CRITIQUE (P0) - Bloquant

| Fonctionnalité | Description | Sujet ref |
|----------------|-------------|-----------|
| ~~Boucle événementielle poll()~~ | ✅ Implémenté dans `server.cpp:run()` | IV.1 |
| ~~Non-blocking I/O complet~~ | ✅ fcntl O_NONBLOCK sur tous les sockets | IV.1 |
| **Classe Client complète** | Manque classe Client avec états, buffers | IV.1 |
| **Argument config file** | `./webserv [configuration file]` | IV |
| **Servir site statique** | GET fichiers HTML/CSS/JS/images | IV.1 |
| **Ne jamais crash** | Même OOM, erreur réseau, etc. | II |

### Priorité HAUTE (P1) - Fonctionnalités core

| Fonctionnalité | Description | Sujet ref |
|----------------|-------------|-----------|
| **Méthode GET complète** | Lire fichier, envoyer avec bon Content-Type | IV.1 |
| **Méthode POST complète** | Traiter body, upload fichiers | IV.1 |
| **Méthode DELETE** | Supprimer fichiers sur le serveur | IV.1 |
| **Upload fichiers** | Multipart/form-data, sauvegarder dans upload_path | IV.1 |
| **CGI execution** | fork() + execve() pour .php, .py | IV.3 |
| **Variables env CGI** | REQUEST_METHOD, QUERY_STRING, CONTENT_LENGTH, etc. | IV.3 |
| **Pipes CGI** | Communication stdin/stdout avec le script | IV.3 |
| **Un-chunk pour CGI** | Convertir chunked en body complet pour CGI | IV.3 |

### Priorité MOYENNE (P2) - Fonctionnalités secondaires

| Fonctionnalité | Description | Sujet ref |
|----------------|-------------|-----------|
| Autoindex | Générer listing HTML d'un répertoire | IV.3 |
| Redirections HTTP | 301/302 avec header Location | IV.3 |
| Timeouts clients | Déconnecter clients inactifs | IV.1 |
| `client_max_body_size` | Retourner 413 si dépassé | IV.3 |
| Multi-ports | Écouter sur plusieurs ports simultanément | IV.3 |
| Keep-alive | Réutiliser connexions TCP | IV.1 |

### Priorité BASSE (P3) - Robustesse

| Fonctionnalité | Description |
|----------------|-------------|
| Stress test | Rester disponible sous forte charge |
| Gestion mémoire | Pas de leaks, libérer ressources |
| Compatibilité navigateur | Tester avec Chrome/Firefox |
| Comparaison NGINX | Vérifier comportement similaire |

---

## Non implémenté (Bonus)

| Fonctionnalité | Description |
|----------------|-------------|
| Cookies | Parsing et envoi de cookies |
| Sessions | Gestion de sessions utilisateur |
| Multiple CGI types | PHP, Python, Perl, Ruby, etc. |

---

## Structure du projet (vérifiée)

```
Webserv/
├── Makefile                        # Build system C++98
├── config/
│   └── default.conf                # Configuration exemple (2 serveurs)
│
├── includes/
│   ├── core/
│   │   ├── Dico.hpp                # Structures de données (499 lignes)
│   │   └── Server.hpp              # Classe Server
│   ├── config/
│   │   └── ConfigParser.hpp        # Parser de configuration
│   └── http/
│       ├── Request.hpp             # Parsing requêtes HTTP
│       ├── Response.hpp            # Construction réponses HTTP
│       └── Router.hpp              # Routing et matching
│
├── srcs/
│   ├── main.cpp                    # Point d'entrée (68 lignes)
│   ├── core/
│   │   └── server.cpp              # Implémentation serveur (330 lignes)
│   ├── config/
│   │   └── ConfigParser.cpp        # Implémentation parser (412 lignes)
│   └── http/
│       ├── Request.cpp             # Implémentation parsing (199 lignes)
│       ├── Response.cpp            # Implémentation réponse (126 lignes)
│       └── Router.cpp              # Implémentation routing (178 lignes)
│
├── www/
│   └── errors/                     # Pages d'erreur HTML
│       ├── 400.html
│       ├── 403.html
│       ├── 404.html
│       ├── 405.html
│       ├── 413.html
│       └── 500.html
│
├── tests/
│   ├── test_http.cpp               # Tests unitaires HTTP (6475 lignes)
│   └── configs/                    # 15 fichiers de test config
│       ├── test1_minimal.conf
│       ├── test2_complete.conf
│       ├── test3_multi_server.conf
│       ├── ... (jusqu'à test15)
│
└── Docs/
    ├── en.subject.pdf              # Sujet officiel (v23.1)
    ├── ARCHITECTURE.md             # Architecture détaillée
    ├── GIT_FLOW.md                 # Workflow Git
    ├── README.md                   # Description rapide
    ├── exemple_struc.md            # Exemple de structure
    └── PROGRESS.md                 # Ce fichier

TOTAL: ~1812 lignes de code C++
```

---

## Fonctions externes autorisées

```
execve, pipe, strerror, gai_strerror, errno, dup, dup2, fork, socketpair,
htons, htonl, ntohs, ntohl, select, poll, epoll (epoll_create, epoll_ctl,
epoll_wait), kqueue (kqueue, kevent), socket, accept, listen, send, recv,
chdir, bind, connect, getaddrinfo, freeaddrinfo, setsockopt, getsockname,
getprotobyname, fcntl, close, read, write, waitpid, kill, signal, access,
stat, open, opendir, readdir, closedir
```

---

## Prochaines étapes (ordre recommandé)

### Phase 1 - Core fonctionnel
1. [ ] **Argument ligne de commande** - Parser argv pour config file
2. [ ] **Intégrer ConfigParser** - Charger config dans main.cpp
3. [x] ~~**Boucle poll() fonctionnelle**~~ ✅ Fait dans server.cpp
4. [x] ~~**Gestion multi-clients basique**~~ ✅ Map + accept OK
5. [ ] **Classe Client complète** - Remplacer `Client* = NULL`

### Phase 2 - HTTP basique
5. [ ] **GET fichiers statiques** - Lire fichier, envoyer réponse
6. [ ] **Pages d'erreur** - Servir les fichiers de www/errors/
7. [ ] **Autoindex** - Générer listing HTML

### Phase 3 - HTTP complet
8. [ ] **POST upload** - Parser multipart, sauvegarder fichiers
9. [ ] **DELETE** - Supprimer fichiers
10. [ ] **Redirections** - Implémenter return 301/302

### Phase 4 - CGI
11. [ ] **fork() + execve()** - Lancer scripts
12. [ ] **Variables environnement** - Passer infos au CGI
13. [ ] **Pipes** - Communication bidirectionnelle
14. [ ] **Timeout CGI** - Kill si trop long

### Phase 5 - Robustesse
15. [ ] **Timeouts clients** - Déconnecter inactifs
16. [ ] **Stress tests** - Siege, ab, wrk
17. [ ] **Tests navigateur** - Chrome, Firefox
18. [ ] **Comparaison NGINX** - Vérifier comportement

---

## Tests

```bash
# Compilation
make clean && make              # Doit compiler sans erreur

# Tests unitaires
make test                       # Tests HTTP parsing

# Test manuel (quand fonctionnel)
./webserv config/default.conf
curl http://localhost:8080/
curl -X POST -d "data=test" http://localhost:8080/upload
curl -X DELETE http://localhost:8080/file.txt

# Stress test (quand fonctionnel)
siege -c 100 -t 30s http://localhost:8080/
```

---

## Points d'attention du sujet

> ⚠️ **"I/O that can wait for data (sockets, pipes/FIFOs, etc.) must be non-blocking and driven by a single poll()"**

> ⚠️ **"Calling read/recv or write/send on these descriptors without prior readiness will result in a grade of 0"**

> ⚠️ **"Your program must not crash under any circumstances (even if it runs out of memory)"**

> ⚠️ **"You can't use fork for anything other than CGI"**

> ⚠️ **"Checking the value of errno after read or write is strictly forbidden"**

---

## Historique des mises à jour

| Date | Modification |
|------|--------------|
| 27/01/2025 | Création initiale du document |
| 27/01/2025 | Ajout includes Request/Response dans server.cpp |
| 27/01/2025 | Correction variables et portée dans server.cpp |
| 27/01/2025 | Mise à jour complète avec analyse du sujet v23.1 |

---

## Répartition du travail par personne

### Vue d'ensemble

| Domaine | Personne 1 | Personne 2 | Personne 3 |
|---------|------------|------------|------------|
| **Branches Git** | `feature/config-parser`<br>`feature/server-socket` | `feature/http-request`<br>`feature/http-response`<br>`feature/http-router` | `feature/client`<br>`feature/cgi` |
| **Responsabilité** | Config + Router + Files | HTTP complet | Réseau + CGI |

---

### Personne 1 - Configuration & Fichiers

| Tâche | Fichiers | Statut | Reste à faire |
|-------|----------|--------|---------------|
| Config parser | `ConfigParser.cpp/.hpp` | ✅ 80% | Intégration dans main |
| Structures config | `Dico.hpp` | ✅ | - |
| Router / Matching | `Router.cpp/.hpp` | ✅ 70% | - |
| File handler | À créer | ❌ | Lecture fichiers statiques |
| Autoindex | `Router.cpp` | ⚠️ | Génération HTML listing |
| Tests config | `tests/configs/` | ⚠️ | Compléter |

**TODO Personne 1:**
1. [ ] Intégrer ConfigParser dans `main.cpp` (argument `./webserv [config]`)
2. [ ] Créer FileHandler pour lire les fichiers statiques
3. [ ] Implémenter `handleAutoindex()` - génération HTML du listing
4. [ ] Implémenter `generateErrorPage()` - servir pages custom

---

### Personne 2 - HTTP Request/Response

| Tâche | Fichiers | Statut | Reste à faire |
|-------|----------|--------|---------------|
| HTTP Request parsing | `Request.cpp/.hpp` | ✅ 90% | - |
| HTTP Response | `Response.cpp/.hpp` | ✅ 70% | Envoi complet |
| Méthode GET | `Router.cpp` | ⚠️ | Lire + envoyer fichier |
| Méthode POST | À implémenter | ❌ | Multipart, upload |
| Méthode DELETE | À implémenter | ❌ | Supprimer fichier |
| Error pages | `www/errors/` | ✅ pages | Serving manquant |
| Tests HTTP | `test_http.cpp` | ✅ | - |

**TODO Personne 2:**
1. [ ] Implémenter GET complet (lire fichier + envoyer avec Content-Type)
2. [ ] Implémenter POST (parser multipart/form-data, sauvegarder)
3. [ ] Implémenter DELETE (supprimer fichier, vérifier permissions)
4. [ ] Implémenter redirections HTTP 301/302
5. [ ] Vérifier `client_max_body_size` → retourner 413

---

### Personne 3 - Réseau & CGI

| Tâche | Fichiers | Statut | Reste à faire |
|-------|----------|--------|---------------|
| Socket creation | `server.cpp` | ✅ | - |
| Boucle poll() | `server.cpp:run()` | ✅ | - |
| Multi-clients basique | `server.cpp` | ✅ | Map + accept OK |
| Mode non-bloquant | `server.cpp` | ✅ | fcntl O_NONBLOCK |
| **Classe Client** | À créer | ❌ | États, buffers, timeout |
| Gestion ClientData | `Dico.hpp` | ⚠️ struct OK | Utiliser dans server |
| CGI handler | À créer | ❌ 5% | fork + execve + pipes |
| Variables env CGI | À créer | ❌ | REQUEST_METHOD, etc. |
| Timeouts | `server.cpp` | ❌ | checkTimeouts() vide |
| Tests réseau | À créer | ❌ | - |

**TODO Personne 3:**
1. [x] ~~Implémenter boucle `poll()` fonctionnelle~~ ✅
2. [x] ~~Gestion multi-clients basique (accept + map)~~ ✅
3. [ ] Créer classe `Client` complète (remplacer `Client* = NULL`)
4. [ ] Utiliser `ClientData` de Dico.hpp
5. [ ] Implémenter CGI: `fork()` + `execve()` + pipes
6. [ ] Variables d'environnement CGI
7. [ ] Timeout CGI (kill si trop long)
8. [ ] Implémenter `checkTimeouts()` (déconnecter inactifs)

> ✅ **Bonne nouvelle**: La base réseau fonctionne! poll() + accept + recv/send OK.
> Il reste à créer la classe Client et implémenter CGI.

---

### Dépendances entre les tâches

```
Personne 3: poll() + multi-clients ✅ FAIT
            ↓
            ├── Personne 1: FileHandler + Autoindex ← EN COURS
            │               ↓
            └── Personne 2: GET/POST/DELETE ← EN COURS
                            ↓
                            Personne 3: CGI ← À FAIRE
```

---

### Planning suggéré

| Semaine | Personne 1 | Personne 2 | Personne 3 |
|---------|------------|------------|------------|
| **S1** | Intégrer config | Préparer GET | **poll() + clients** |
| **S2** | FileHandler | GET + POST | Tester avec P1/P2 |
| **S3** | Autoindex | DELETE + redirects | CGI base |
| **S4** | Tests | Error pages | CGI complet |
| **S5** | Review | Review | Timeouts |
| **S6** | Stress tests ensemble | Debug ensemble | Debug ensemble |

---

## Historique des mises à jour

| Date | Modification |
|------|--------------|
| 27/01/2025 | Création initiale du document |
| 27/01/2025 | Ajout includes Request/Response dans server.cpp |
| 27/01/2025 | Correction variables et portée dans server.cpp |
| 27/01/2025 | Mise à jour complète avec analyse du sujet v23.1 |
| 27/01/2025 | Ajout répartition du travail par personne |
| 27/01/2025 | Correction: poll() déjà implémenté dans server.cpp |
| 27/01/2025 | Vérification complète de la structure du projet |

---

*Dernière mise à jour : 27 janvier 2025*
