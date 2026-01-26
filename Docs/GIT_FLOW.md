# Git Flow - Webserv

## Structure des Branches

```
main (production)
  │
  └── develop (intégration)
        │
        ├── feature/config-parser      (Personne 1)
        ├── feature/server-socket      (Personne 1)
        ├── feature/http-request       (Personne 2)
        ├── feature/http-response      (Personne 2)
        ├── feature/http-router        (Personne 2)
        ├── feature/client             (Personne 3)
        ├── feature/cgi                (Personne 3)
        └── hotfix/critical-bug        (si nécessaire)
```

---

## I. Branches Principales

### `main`

- **Rôle** : Code stable, prêt pour évaluation
- **Protection** : Interdire les push directs
- **Merge uniquement depuis** : `develop` via Pull Request
- **Tag** : Chaque version évaluée (`v1.0`, `v1.1`, etc.)

### `develop`

- **Rôle** : Branche d'intégration, dernières features complétées
- **Merge depuis** : branches `feature/*`
- **Tests** : Validation avant merge dans `main`

---

## II. Branches de Features

### Convention de nommage

```
feature/nom-de-la-feature
```

### Features par Phase

#### Phase 1 - Fondations

```
feature/project-setup
  - Makefile
  - Structure de dossiers
  - Classes de base (vides)
  - .gitignore

feature/config-parser
  - Parser le fichier de configuration
  - Structures de données Config
  - Validation
  - Tests unitaires

feature/config-validation
  - Vérifier cohérence de la config
  - Messages d'erreur clairs
```

#### Phase 2 - Réseau de Base

```
feature/socket-management
  - Création sockets
  - Bind, listen
  - Mode non-bloquant
  - Classe Socket/Server

feature/multiplexing
  - Implémentation poll/epoll/kqueue
  - Classe Multiplexer
  - Gestion des events
  - Accept de connexions

feature/client-connection
  - Classe Client
  - Buffer lecture/écriture
  - État de connexion
  - Timeout
```

#### Phase 3 - HTTP

```
feature/http-request-parser
  - Parser request line
  - Parser headers
  - Gérer body avec Content-Length
  - Classe Request

feature/http-chunked-encoding
  - Parser Transfer-Encoding: chunked
  - Dé-chunker pour CGI

feature/http-response-builder
  - Classe Response
  - Construction headers
  - Status codes
  - Body

feature/error-pages
  - Pages d'erreur par défaut
  - Pages personnalisées
  - Tous les codes d'erreur
```

#### Phase 4 - Routing & Méthodes

```
feature/router
  - Matching routes
  - Application règles de config
  - Classe Router

feature/get-method
  - Implémentation GET
  - Servir fichiers statiques
  - Gestion MIME types

feature/post-method
  - Implémentation POST
  - Réception formulaires
  - Parsing application/x-www-form-urlencoded
  - Parsing multipart/form-data

feature/delete-method
  - Implémentation DELETE
  - Sécurité (vérifier permissions)

feature/redirections
  - 301, 302
  - Location header
```

#### Phase 5 - Fichiers

```
feature/file-handler
  - Lecture fichiers
  - MIME types
  - Cache control
  - Classe FileHandler

feature/directory-listing
  - Autoindex
  - Génération HTML
  - Liens clickables

feature/file-upload
  - Réception fichiers
  - Stockage sur disque
  - Limites de taille
  - Sécurité
```

#### Phase 6 - CGI

```
feature/cgi-handler
  - Fork + exec
  - Variables d'environnement
  - Communication pipes
  - Classe CGIHandler

feature/cgi-multi-language
  - Support PHP
  - Support Python
  - Support Perl
  - Configuration par extension
```

#### Phase 7 - Robustesse

```
feature/timeout-management
  - Timeout connexions
  - Timeout CGI
  - Nettoyage ressources

feature/keep-alive
  - Connexions persistantes
  - Gestion timeout

feature/error-handling
  - Gestion robuste erreurs
  - Logging
  - Pas de crash

feature/stress-testing
  - Tests de charge
  - Fuites mémoire
  - Valgrind clean
```

#### Phase 8 - Bonus (optionnel)

```
feature/cookies-sessions
  - Parser cookies
  - Set-Cookie
  - Gestion sessions simple
```

---

## III. Workflow Détaillé

### 1. Créer une feature

```bash
git checkout develop
git pull origin develop
git checkout -b feature/nom-feature
```

### 2. Développer

```bash
# Commits fréquents, atomiques
git add .
git commit -m "feat: description claire"
```

### Convention de commits

| Type | Usage |
|------|-------|
| `feat` | Nouvelle fonctionnalité |
| `fix` | Correction de bug |
| `refactor` | Refactoring sans changement de comportement |
| `test` | Ajout/modification de tests |
| `docs` | Documentation |
| `style` | Formatage, pas de changement de code |

### Scopes

```
config, server, client, request, response, router, cgi, utils
```

### Exemples de commits

```bash
git commit -m "feat(config): parse location blocks"
git commit -m "fix(request): handle chunked encoding"
git commit -m "refactor(client): use CGIData struct"
git commit -m "docs: update ARCHITECTURE.md"
```

### 3. Tester localement

```bash
make re
./webserv config/default.conf

# Tests manuels
curl http://localhost:8080/

# Tests automatisés
python3 tests/test_feature.py
```

### 4. Push et Pull Request

```bash
git push origin feature/nom-feature
```

Sur GitHub :
- Créer Pull Request vers `develop`
- Assigner un reviewer (pair)
- Passer les checks CI (si configurés)
- Discuter, améliorer
- Merger

### 5. Merger dans develop

```bash
# Option 1: Merge commit (garde historique)
git checkout develop
git merge --no-ff feature/nom-feature

# Option 2: Squash (1 commit propre)
git merge --squash feature/nom-feature
git commit -m "feat: nom-feature complete"

# Option 3: Rebase (historique linéaire)
git rebase develop feature/nom-feature
git checkout develop
git merge feature/nom-feature

git push origin develop
git branch -d feature/nom-feature
```

### 6. Release vers main

```bash
# Quand develop est stable
git checkout main
git merge --no-ff develop
git tag -a v1.0 -m "Version 1.0 - Ready for evaluation"
git push origin main --tags
```

---

## IV. Stratégie de Travail en Équipe

### Répartition pour 3 personnes

| Personne 1 | Personne 2 | Personne 3 |
|------------|------------|------------|
| Config + parsing | HTTP request/response | Socket + multiplexing |
| Router | Methods (GET/POST/DELETE) | CGI |
| File handler | Error handling | Upload |
| Tests config | Tests HTTP | Tests réseau |

### Synchronisation quotidienne

```bash
# Tous les jours, synchroniser develop
git checkout develop
git pull origin develop

# Avant chaque session de code
git checkout feature/ma-feature
git rebase develop  # ou git merge develop
```

---

## V. Règles d'équipe

| Règle | Pourquoi |
|-------|----------|
| Ne jamais push sur `main` directement | Garder main stable |
| Pull `develop` avant de créer une branche | Éviter les gros conflits |
| Commits atomiques | 1 commit = 1 changement logique |
| Tester avant de merge dans `develop` | Ne pas casser le build |
| Peer review obligatoire | Attraper les bugs tôt |

---

## VI. CI/CD (Optionnel)

### GitHub Actions (.github/workflows/build.yml)

```yaml
name: Build and Test

on:
  push:
    branches: [ develop, main ]
  pull_request:
    branches: [ develop, main ]

jobs:
  build:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v2

    - name: Build
      run: make

    - name: Run unit tests
      run: make test

    - name: Check memory leaks
      run: |
        valgrind --leak-check=full --error-exitcode=1 \
        ./webserv config/test.conf &
        sleep 5
        curl http://localhost:8080/
        killall webserv
```

---

## VII. Commandes Utiles

### Voir l'état des branches

```bash
git branch -a                      # Toutes les branches
git branch -vv                     # Avec tracking
git log --graph --oneline --all    # Graphique
```

### Nettoyer les branches

```bash
git branch -d feature/done                 # Supprimer locale
git push origin --delete feature/done      # Supprimer remote
git remote prune origin                    # Nettoyer les refs
```

### Résoudre conflits

```bash
# Lors d'un merge/rebase avec conflits
git status                         # Voir fichiers en conflit
# Éditer les fichiers, supprimer les marqueurs <<<<< ===== >>>>>
git add fichier-resolu
git rebase --continue              # ou git merge --continue
```

### Annuler des changements

```bash
git reset --soft HEAD~1            # Annuler dernier commit (garde changes)
git reset --hard HEAD~1            # Annuler + supprimer changes
git checkout -- fichier            # Restaurer fichier
```

### Stash temporaire

```bash
git stash                          # Sauvegarder temporairement
git stash pop                      # Récupérer
git stash list                     # Voir les stash
```

---

## VIII. Checklist avant merge

### Avant merge dans develop

- [ ] Le code compile sans warning (`make`)
- [ ] Les fonctionnalités de base fonctionnent
- [ ] Pas de crash évident
- [ ] Les commits sont propres et bien nommés
- [ ] Synchronisé avec la dernière version de `develop`
- [ ] Code reviewé par un pair

### Avant merge dans main

- [ ] Tous les tests passent
- [ ] Testé avec navigateur
- [ ] Testé avec `curl` / `telnet`
- [ ] Pas de fuite mémoire (Valgrind)
- [ ] Stress test passé

---

## Résumé GitFlow

1. **main** = Version stable pour évaluation
2. **develop** = Intégration continue
3. **feature/x** = Développement de features
4. **Pull Requests** obligatoires vers develop
5. **Peer review** avant merge
6. **Tests** avant chaque merge
7. **Tag** les versions sur main (`v1.0`, `v1.1`)
