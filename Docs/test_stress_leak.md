# Tests Stress & Fuites Memoire - Webserv

## 1. Siege (Stress Test)

### Lancer le serveur

```bash
./webserv config/default.conf &
```

### Commandes Siege

```bash
# Test leger : 10 users, 200 requetes
siege -c 10 -r 20 -b http://localhost:8080/

# Test moyen : 50 users, 2500 requetes
siege -c 50 -r 50 -b http://localhost:8080/

# Test charge : 100 users, 5000 requetes
siege -c 100 -r 50 -b http://localhost:8080/

# Test lourd : 200 users, 20000 requetes
siege -c 200 -r 100 -b http://localhost:8080/

# Test max : 255 users, 51000 requetes
siege -c 255 -r 200 -b http://localhost:8080/
```

### Options Siege

| Option | Description |
|--------|-------------|
| `-c N` | Nombre d'utilisateurs concurrents |
| `-r N` | Nombre de repetitions par utilisateur |
| `-b`   | Mode benchmark (sans delai entre requetes) |

### Resultats obtenus

| Test | Concurrence | Requetes | Availability | Req/s | Echecs | Temps reponse |
|------|-------------|----------|-------------|-------|--------|---------------|
| 1 | 10 users | 200 | **100.00%** | 2500 | 0 | 0.00s |
| 2 | 50 users | 2,500 | **100.00%** | 2272 | 0 | 0.02s |
| 3 | 100 users | 5,000 | **100.00%** | 1845 | 0 | 0.05s |
| 4 | 200 users | 20,000 | **100.00%** | 2317 | 0 | 0.06s |
| 5 | 255 users | 51,000 | **100.00%** | 2481 | 0 | 0.07s |

**Total : 78,700 requetes, 0 echec, 100% availability.**

Critere fiche de corrction : availability > 99.5% → **100% partout**.

---

## 2. Valgrind (Fuites Memoire)

### Lancer le serveur sous Valgrind

```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
  --log-file=/tmp/valgrind_webserv.log ./webserv config/default.conf &
```

### Options Valgrind

| Option | Description |
|--------|-------------|
| `--leak-check=full` | Detection detaillee de toutes les fuites |
| `--show-leak-kinds=all` | Affiche definite, indirect, possible, reachable |
| `--track-origins=yes` | Trace l'origine des valeurs non initialisees |
| `--log-file=...` | Ecrit le rapport dans un fichier |

### Envoyer des requetes (couvrir tous les chemins)

```bash
# GET page d'accueil
curl -s -o /dev/null -w "GET /: %{http_code}\n" http://localhost:8080/

# GET page 404
curl -s -o /dev/null -w "GET /404: %{http_code}\n" http://localhost:8080/nexiste-pas

# GET autoindex (uploads)
curl -s -o /dev/null -w "GET /uploads/: %{http_code}\n" http://localhost:8080/uploads/

# POST upload
curl -s -o /dev/null -w "POST upload: %{http_code}\n" -X POST -d "test=valgrind" http://localhost:8080/uploads/valgrind_test.txt

# DELETE fichier
curl -s -o /dev/null -w "DELETE: %{http_code}\n" -X DELETE http://localhost:8080/uploads/valgrind_test.txt

# CGI Python
curl -s -o /dev/null -w "CGI python: %{http_code}\n" http://localhost:8080/cgi-test/test.py

# CGI Bash
curl -s -o /dev/null -w "CGI bash: %{http_code}\n" http://localhost:8080/cgi-test/test.sh

# CGI Python POST
curl -s -o /dev/null -w "CGI POST: %{http_code}\n" -X POST -d "body=valgrind" http://localhost:8080/cgi-test/test.py

# Redirect
curl -s -o /dev/null -w "Redirect: %{http_code}\n" http://localhost:8080/redirect

# Siege rapide
siege -c 10 -r 10 -b http://localhost:8080/
```

### Arreter le serveur (SIGTERM, pas SIGKILL)

```bash
kill -TERM $(pgrep -f "valgrind.*webserv")
```

### Lire le rapport

```bash
cat /tmp/valgrind_webserv.log
```

### Resultats obtenus

```
HEAP SUMMARY:
    in use at exit: 0 bytes in 0 blocks
    total heap usage: 4,928 allocs, 4,928 frees, 4,050,568 bytes allocated

All heap blocks were freed -- no leaks are possible

ERROR SUMMARY: 0 errors from 0 contexts
```

| Verification | Resultat |
|---|---|
| Fuites memoire | **0 bytes** - aucune fuite |
| Blocs non liberes | **0 blocks** |
| Allocs / Frees | **4,928 / 4,928** (equilibre parfait) |
| Erreurs memoire | **0 erreurs** |

### Memoire du processus (sans valgrind)

Verifiee avec `ps -o pid,vsz,rss,comm -p <PID>` avant et apres siege :

| Moment | RSS |
|--------|-----|
| Avant siege | 3768 KB |
| Apres 78,700 requetes | 3776 KB |

Pas de fuite memoire, la memoire reste stable.
