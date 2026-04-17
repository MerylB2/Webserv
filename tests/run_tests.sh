#!/bin/bash

# ============================================================
# Webserv - Script de tests d'integration
# Usage : ./tests/run_tests.sh
# Le script compile, lance le serveur, teste, puis arrete
# ============================================================

RED='\033[0;31m'
GREEN='\033[1;32m'
YELLOW='\033[0;33m'
CYAN='\033[1;36m'
RESET='\033[0m'

HOST="http://localhost:8080"
HOST2="http://localhost:8081"
PASS=0
FAIL=0
TOTAL=0
SERV_PID=0

# ============================================================
# Fonctions utilitaires
# ============================================================

run_test() {
    local num="$1"
    local name="$2"
    local expected_code="$3"
    shift 3
    local curl_args=("$@")

    TOTAL=$((TOTAL + 1))
    code=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 "${curl_args[@]}")

    if [ "$code" = "$expected_code" ]; then
        echo -e "  ${GREEN}✓${RESET} Test $num: $name ${GREEN}[$code]${RESET}"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}✗${RESET} Test $num: $name ${RED}[got $code, expected $expected_code]${RESET}"
        FAIL=$((FAIL + 1))
    fi
}

# Test qui verifie aussi un header specifique
run_test_header() {
    local num="$1"
    local name="$2"
    local expected_code="$3"
    local header_name="$4"
    local header_contains="$5"
    shift 5
    local curl_args=("$@")

    TOTAL=$((TOTAL + 1))
    local tmpfile=$(mktemp)
    code=$(curl -s -D "$tmpfile" -o /dev/null -w "%{http_code}" --max-time 5 "${curl_args[@]}")
    local header_value=$(grep -i "^${header_name}:" "$tmpfile" | tr -d '\r' | head -1)
    rm -f "$tmpfile"

    if [ "$code" = "$expected_code" ]; then
        if [ -n "$header_contains" ] && echo "$header_value" | grep -qi "$header_contains"; then
            echo -e "  ${GREEN}✓${RESET} Test $num: $name ${GREEN}[$code]${RESET}"
            echo -e "    ${CYAN}→ $header_value${RESET}"
            PASS=$((PASS + 1))
        elif [ -n "$header_contains" ]; then
            echo -e "  ${RED}✗${RESET} Test $num: $name ${RED}[header $header_name missing '$header_contains']${RESET}"
            echo -e "    ${RED}→ got: $header_value${RESET}"
            FAIL=$((FAIL + 1))
        else
            echo -e "  ${GREEN}✓${RESET} Test $num: $name ${GREEN}[$code]${RESET}"
            PASS=$((PASS + 1))
        fi
    else
        echo -e "  ${RED}✗${RESET} Test $num: $name ${RED}[got $code, expected $expected_code]${RESET}"
        FAIL=$((FAIL + 1))
    fi
}

# Test qui verifie le body
run_test_body() {
    local num="$1"
    local name="$2"
    local expected_code="$3"
    local body_contains="$4"
    shift 4
    local curl_args=("$@")

    TOTAL=$((TOTAL + 1))
    local response
    response=$(curl -s -w "\n%{http_code}" --max-time 5 "${curl_args[@]}")
    code=$(echo "$response" | tail -1)
    body=$(echo "$response" | sed '$d')

    if [ "$code" = "$expected_code" ]; then
        if [ -n "$body_contains" ] && echo "$body" | grep -q "$body_contains"; then
            echo -e "  ${GREEN}✓${RESET} Test $num: $name ${GREEN}[$code]${RESET}"
            PASS=$((PASS + 1))
        elif [ -n "$body_contains" ]; then
            echo -e "  ${RED}✗${RESET} Test $num: $name ${RED}[body missing '$body_contains']${RESET}"
            FAIL=$((FAIL + 1))
        else
            echo -e "  ${GREEN}✓${RESET} Test $num: $name ${GREEN}[$code]${RESET}"
            PASS=$((PASS + 1))
        fi
    else
        echo -e "  ${RED}✗${RESET} Test $num: $name ${RED}[got $code, expected $expected_code]${RESET}"
        FAIL=$((FAIL + 1))
    fi
}

cleanup() {
    curl -s -X DELETE "$HOST/uploads/test_upload.txt" > /dev/null 2>&1
    curl -s -X DELETE "$HOST/uploads/test_upload2.txt" > /dev/null 2>&1
}

stop_server() {
    if [ "$SERV_PID" -ne 0 ] && kill -0 "$SERV_PID" 2>/dev/null; then
        kill "$SERV_PID" 2>/dev/null
        wait "$SERV_PID" 2>/dev/null
    fi
}

# ============================================================
# Compilation et lancement du serveur
# ============================================================

echo ""
echo -e "${CYAN}╔══════════════════════════════════════════════════╗${RESET}"
echo -e "${CYAN}║        WEBSERV - Tests d'integration             ║${RESET}"
echo -e "${CYAN}╚══════════════════════════════════════════════════╝${RESET}"
echo ""

# Compiler
echo -e "${YELLOW}Compilation...${RESET}"
make -C "$(dirname "$0")/.." > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo -e "${RED}Erreur de compilation${RESET}"
    exit 1
fi
echo -e "${GREEN}OK${RESET}"
echo ""

# Tuer un eventuel serveur existant
pkill -f "./webserv" 2>/dev/null
sleep 0.5

# Lancer le serveur
cd "$(dirname "$0")/.." || exit 1
./webserv config/default.conf > /tmp/webserv_test.log 2>&1 &
SERV_PID=$!
sleep 1

# Verifier que le serveur repond
check=$(curl -s -o /dev/null -w "%{http_code}" --max-time 3 "$HOST/")
if [ "$check" != "200" ]; then
    echo -e "${RED}Le serveur ne repond pas (code: $check)${RESET}"
    stop_server
    exit 1
fi
echo -e "${GREEN}Serveur demarre (PID $SERV_PID)${RESET}"
echo ""

trap 'stop_server; cleanup' EXIT

# ============================================================
# 1. GET - Fichiers statiques
# ============================================================
echo -e "${YELLOW}━━━ 1. GET - Fichiers statiques ━━━${RESET}"

run_test 1  "GET / (page d'accueil)"       "200" "$HOST/"
run_test 2  "GET /index.html"               "200" "$HOST/index.html"
run_test 3  "GET fichier inexistant"        "404" "$HOST/cette-page-nexiste-pas"
run_test 4  "GET /errors/404.html"          "200" "$HOST/errors/404.html"

echo ""

# ============================================================
# 2. Redirections
# ============================================================
echo -e "${YELLOW}━━━ 2. Redirections ━━━${RESET}"

run_test_header 5 "GET /redirect (301 + Location)" "301" \
    "Location" "google" "$HOST/redirect"

echo ""

# ============================================================
# 3. POST - Upload
# ============================================================
echo -e "${YELLOW}━━━ 3. POST - Upload ━━━${RESET}"

run_test 6  "POST upload fichier texte" "201" \
    -X POST "$HOST/uploads/test_upload.txt" \
    -H "Content-Type: text/plain" \
    -d "Hello Webserv"

run_test_body 7  "GET fichier uploade (contenu)" "200" \
    "Hello Webserv" "$HOST/uploads/test_upload.txt"

# POST ecrase le fichier
run_test 8  "POST ecrase fichier existant" "201" \
    -X POST "$HOST/uploads/test_upload.txt" \
    -H "Content-Type: text/plain" \
    -d "Nouveau contenu"

run_test_body 9  "GET contenu apres ecrasement" "200" \
    "Nouveau contenu" "$HOST/uploads/test_upload.txt"

echo ""

# ============================================================
# 4. DELETE
# ============================================================
echo -e "${YELLOW}━━━ 4. DELETE - Suppression ━━━${RESET}"

run_test 10 "DELETE fichier uploade"        "204" -X DELETE "$HOST/uploads/test_upload.txt"
run_test 11 "GET apres DELETE (404)"        "404" "$HOST/uploads/test_upload.txt"
run_test 12 "DELETE fichier inexistant"     "404" -X DELETE "$HOST/uploads/fichier_inexistant.txt"

echo ""

# ============================================================
# 5. Autoindex
# ============================================================
echo -e "${YELLOW}━━━ 5. Autoindex ━━━${RESET}"

run_test_body 13 "GET /uploads/ autoindex ON (HTML table)" "200" \
    "<table>" "$HOST/uploads/"

run_test 14 "GET /cgi-test/ autoindex OFF (403)" "403" "$HOST/cgi-test/"

echo ""

# ============================================================
# 6. CGI
# ============================================================
echo -e "${YELLOW}━━━ 6. CGI ━━━${RESET}"

run_test_body 15 "CGI Python GET" "200" \
    "CGI PYTHON" "$HOST/cgi-test/test.py"

run_test_body 16 "CGI Python GET + query string" "200" \
    "REQUEST_METHOD" "$HOST/cgi-test/test.py?name=webserv&lang=cpp"

run_test_body 17 "CGI Bash GET" "200" \
    "CGI BASH" "$HOST/cgi-test/test.sh"

run_test 18 "CGI Python POST" "200" \
    -X POST "$HOST/cgi-test/test.py" \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "user=test&action=login"

echo ""

# ============================================================
# 7. Methodes non autorisees (405)
# ============================================================
echo -e "${YELLOW}━━━ 7. Methodes non autorisees ━━━${RESET}"

run_test 19 "DELETE sur / (405)" "405" -X DELETE "$HOST/index.html"

run_test 20 "POST sur /cgi-test/ sans script (403)" "403" \
    -X POST "$HOST/cgi-test/"

echo ""

# ============================================================
# 8. Headers HTTP
# ============================================================
echo -e "${YELLOW}━━━ 8. Headers HTTP ━━━${RESET}"

run_test_header 21 "Content-Type: text/html pour .html" "200" \
    "Content-Type" "text/html" "$HOST/index.html"

run_test_header 22 "Content-Type: text/html pour CGI" "200" \
    "Content-Type" "text/html" "$HOST/cgi-test/test.py"

run_test 23 "Connection: keep-alive" "200" -H "Connection: keep-alive" "$HOST/"
run_test 24 "Connection: close"      "200" -H "Connection: close" "$HOST/"

echo ""

# ============================================================
# 9. client_max_body_size (413)
# ============================================================
echo -e "${YELLOW}━━━ 9. client_max_body_size (413) ━━━${RESET}"

# Generer un fichier de 6MB (depasse 5M du serveur port 8081)
python3 -c "import sys; sys.stdout.buffer.write(b'X' * (1024 * 1024 * 6))" > /tmp/webserv_bigbody.bin

run_test 25 "POST body > max_body_size (413)" "413" \
    -X POST "$HOST2/" \
    -H "Content-Type: application/octet-stream" \
    --data-binary @/tmp/webserv_bigbody.bin

rm -f /tmp/webserv_bigbody.bin

# Petit body : ne doit PAS renvoyer 413
run_test 26 "POST petit body != 413" "200" \
    -X POST "$HOST/cgi-test/test.py" \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "data=ok"

echo ""

# ============================================================
# 10. CGI non-bloquant (concurrence)
# ============================================================
echo -e "${YELLOW}━━━ 10. CGI non-bloquant ━━━${RESET}"

echo -e "  ${CYAN}CGI + requete statique en parallele...${RESET}"

start_cgi=$(date +%s%N)
curl -s -o /dev/null --max-time 10 "$HOST/cgi-test/test.py" &
pid_cgi=$!

start_static=$(date +%s%N)
curl -s -o /dev/null --max-time 5 "$HOST/"
end_static=$(date +%s%N)

wait $pid_cgi
end_cgi=$(date +%s%N)

time_static=$(( (end_static - start_static) / 1000000 ))
time_cgi=$(( (end_cgi - start_cgi) / 1000000 ))

TOTAL=$((TOTAL + 1))
echo -e "  ${CYAN}→ Statique: ${time_static}ms | CGI: ${time_cgi}ms${RESET}"
if [ "$time_static" -lt 1000 ]; then
    echo -e "  ${GREEN}✓${RESET} Test 27: Serveur non-bloquant ${GREEN}[statique ${time_static}ms]${RESET}"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗${RESET} Test 27: Serveur non-bloquant ${RED}[statique trop lent: ${time_static}ms]${RESET}"
    FAIL=$((FAIL + 1))
fi

echo ""

# ============================================================
# 11. Second serveur (port 8081)
# ============================================================
echo -e "${YELLOW}━━━ 11. Second serveur (port 8081) ━━━${RESET}"

run_test 28 "GET / sur port 8081"           "200" "$HOST2/"
run_test 29 "POST sur port 8081 (GET only)" "405" \
    -X POST "$HOST2/" -d "data"

echo ""

# ============================================================
# 12. Edge cases
# ============================================================
echo -e "${YELLOW}━━━ 12. Edge cases ━━━${RESET}"

run_test 30 "URI longue (100 chars)"   "404" \
    "$HOST/$(python3 -c 'print("a"*100)')"

run_test 31 "Double slash dans URI"     "200" "$HOST//index.html"

run_test 32 "Requete sans Host header"  "200" -H "Host:" "$HOST/"

# Plusieurs requetes consecutives rapides (stabilite)
echo -e "  ${CYAN}10 requetes consecutives rapides...${RESET}"
rapid_ok=0
for i in $(seq 1 10); do
    c=$(curl -s -o /dev/null -w "%{http_code}" --max-time 3 "$HOST/")
    [ "$c" = "200" ] && rapid_ok=$((rapid_ok + 1))
done
TOTAL=$((TOTAL + 1))
if [ "$rapid_ok" -eq 10 ]; then
    echo -e "  ${GREEN}✓${RESET} Test 33: 10/10 requetes rapides OK ${GREEN}[stable]${RESET}"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗${RESET} Test 33: ${rapid_ok}/10 requetes OK ${RED}[instable]${RESET}"
    FAIL=$((FAIL + 1))
fi

echo ""

# ============================================================
# 13. Pages d'erreur personnalisees
# ============================================================
echo -e "${YELLOW}━━━ 13. Pages d'erreur ━━━${RESET}"

run_test_body 34 "404 renvoie page custom" "404" \
    "Not Found" "$HOST/nexiste-pas"

run_test_body 35 "405 renvoie page custom" "405" \
    "Method Not Allowed" -X DELETE "$HOST/index.html"

echo ""

# ============================================================
# Nettoyage
# ============================================================
cleanup

# ============================================================
# Resume
# ============================================================
echo -e "${CYAN}══════════════════════════════════════════════════${RESET}"
echo -e "  Total: ${TOTAL}  |  ${GREEN}Pass: ${PASS}${RESET}  |  ${RED}Fail: ${FAIL}${RESET}"
if [ "$FAIL" -eq 0 ]; then
    echo -e "  ${GREEN}✓ Tous les tests passent !${RESET}"
else
    echo -e "  ${RED}✗ $FAIL test(s) en echec${RESET}"
fi
echo -e "${CYAN}══════════════════════════════════════════════════${RESET}"
echo ""

stop_server
exit $FAIL
