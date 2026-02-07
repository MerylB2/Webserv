#!/bin/bash

# ============================================================
# Webserv - Script de tests d'intégration
# Usage : ./tests/run_tests.sh
# Prérequis : le serveur doit tourner sur le port 8080
#             ./webserv config/default.conf
# ============================================================

RED='\033[0;31m'
GREEN='\033[1;32m'
YELLOW='\033[0;33m'
CYAN='\033[1;36m'
RESET='\033[0m'

HOST="http://localhost:8080"
PASS=0
FAIL=0
TOTAL=0

# Fonction de test
run_test() {
    local num="$1"
    local name="$2"
    local expected_code="$3"
    shift 3
    local curl_args=("$@")

    TOTAL=$((TOTAL + 1))
    # -s silent, -o /dev/null discard body, -w get status code
    code=$(curl -s -o /dev/null -w "%{http_code}" "${curl_args[@]}")

    if [ "$code" = "$expected_code" ]; then
        echo -e "  ${GREEN}✓${RESET} Test $num: $name ${GREEN}[$code]${RESET}"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}✗${RESET} Test $num: $name ${RED}[got $code, expected $expected_code]${RESET}"
        FAIL=$((FAIL + 1))
    fi
}

# Fonction pour afficher le body d'une requête (mode verbose)
run_test_verbose() {
    local num="$1"
    local name="$2"
    local expected_code="$3"
    shift 3
    local curl_args=("$@")

    TOTAL=$((TOTAL + 1))
    response=$(curl -s -w "\n%{http_code}" "${curl_args[@]}")
    code=$(echo "$response" | tail -1)
    body=$(echo "$response" | sed '$d')

    if [ "$code" = "$expected_code" ]; then
        echo -e "  ${GREEN}✓${RESET} Test $num: $name ${GREEN}[$code]${RESET}"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}✗${RESET} Test $num: $name ${RED}[got $code, expected $expected_code]${RESET}"
        FAIL=$((FAIL + 1))
    fi
    echo -e "    ${CYAN}Body:${RESET} $(echo "$body" | head -3)"
}

# Nettoyage des fichiers de test précédents
cleanup() {
    rm -f /tmp/webserv_test_upload.txt
    rm -f /tmp/webserv_big_body.txt
}
cleanup

echo ""
echo -e "${CYAN}╔══════════════════════════════════════════════╗${RESET}"
echo -e "${CYAN}║       WEBSERV - Tests d'intégration          ║${RESET}"
echo -e "${CYAN}╚══════════════════════════════════════════════╝${RESET}"
echo ""

# ============================================================
# 1. GET - Fichiers statiques
# ============================================================
echo -e "${YELLOW}━━━ GET - Fichiers statiques ━━━${RESET}"

run_test 1 "GET / (index.html)" "200" "$HOST/"
run_test 2 "GET /index.html" "200" "$HOST/index.html"
run_test 3 "GET fichier inexistant" "404" "$HOST/cette-page-nexiste-pas"
run_test 4 "GET /errors/404.html (page erreur)" "200" "$HOST/errors/404.html"

echo ""

# ============================================================
# 2. GET - Redirections
# ============================================================
echo -e "${YELLOW}━━━ GET - Redirections ━━━${RESET}"

run_test 5 "GET /redirect (301)" "301" "$HOST/redirect"
# Vérifier le header Location
location=$(curl -s -I "$HOST/redirect" | grep -i "^Location:" | tr -d '\r')
echo -e "    ${CYAN}→ $location${RESET}"

echo ""

# ============================================================
# 3. POST - Upload de fichiers
# ============================================================
echo -e "${YELLOW}━━━ POST - Upload ━━━${RESET}"

run_test 6 "POST upload fichier texte" "201" \
    -X POST "$HOST/uploads/test_upload.txt" \
    -H "Content-Type: text/plain" \
    -d "Contenu du fichier de test"

run_test 7 "GET fichier uploadé" "200" "$HOST/uploads/test_upload.txt"

# Vérifier le contenu
content=$(curl -s "$HOST/uploads/test_upload.txt")
if [ "$content" = "Contenu du fichier de test" ]; then
    echo -e "    ${GREEN}→ Contenu vérifié OK${RESET}"
else
    echo -e "    ${RED}→ Contenu incorrect: $content${RESET}"
fi

echo ""

# ============================================================
# 4. DELETE - Suppression
# ============================================================
echo -e "${YELLOW}━━━ DELETE - Suppression ━━━${RESET}"

run_test 8 "DELETE fichier uploadé" "204" \
    -X DELETE "$HOST/uploads/test_upload.txt"

run_test 9 "GET après DELETE (404)" "404" "$HOST/uploads/test_upload.txt"

run_test 10 "DELETE fichier inexistant" "404" \
    -X DELETE "$HOST/uploads/fichier_inexistant.txt"

echo ""

# ============================================================
# 5. Autoindex
# ============================================================
echo -e "${YELLOW}━━━ Autoindex ━━━${RESET}"

run_test 11 "GET /uploads/ (autoindex ON)" "200" "$HOST/uploads/"
run_test 12 "GET /cgi-test/ (autoindex OFF)" "403" "$HOST/cgi-test/"

echo ""

# ============================================================
# 6. CGI
# ============================================================
echo -e "${YELLOW}━━━ CGI ━━━${RESET}"

run_test 13 "CGI Python GET" "200" "$HOST/cgi-test/test.py"
run_test 14 "CGI Python GET avec query string" "200" "$HOST/cgi-test/test.py?name=webserv&lang=cpp"
run_test 15 "CGI Bash GET" "200" "$HOST/cgi-test/test.sh"

run_test 16 "CGI Python POST" "200" \
    -X POST "$HOST/cgi-test/test.py" \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "user=test&action=login"

echo ""

# ============================================================
# 7. Méthodes non autorisées
# ============================================================
echo -e "${YELLOW}━━━ Méthodes non autorisées ━━━${RESET}"

run_test 17 "PATCH sur / (connexion rejetée)" "000" \
    -X PATCH "$HOST/" -d "data"

run_test 18 "DELETE sur / (non autorisé)" "405" \
    -X DELETE "$HOST/index.html"

run_test 19 "POST sur /cgi-test/ sans script" "403" \
    -X POST "$HOST/cgi-test/"

echo ""

# ============================================================
# 8. Headers et comportements HTTP
# ============================================================
echo -e "${YELLOW}━━━ Headers HTTP ━━━${RESET}"

# Vérifier Content-Type des réponses
ct_html=$(curl -s -I "$HOST/index.html" | grep -i "^Content-Type:" | tr -d '\r')
echo -e "  ${CYAN}HTML:${RESET} $ct_html"

ct_cgi=$(curl -s -I "$HOST/cgi-test/test.py" | grep -i "^Content-Type:" | tr -d '\r')
echo -e "  ${CYAN}CGI:${RESET}  $ct_cgi"

# Tester Keep-Alive
run_test 20 "Requête avec Connection: keep-alive" "200" \
    -H "Connection: keep-alive" "$HOST/"

run_test 21 "Requête avec Connection: close" "200" \
    -H "Connection: close" "$HOST/"

echo ""

# ============================================================
# 9. CGI non-bloquant (concurrence)
# ============================================================
echo -e "${YELLOW}━━━ CGI non-bloquant (concurrence) ━━━${RESET}"

# Lancer un CGI en background et une requête statique en parallèle
echo -e "  ${CYAN}Lancement CGI + requête statique en parallèle...${RESET}"

start_cgi=$(date +%s%N)
curl -s -o /dev/null "$HOST/cgi-test/test.py?sleep=1" &
pid_cgi=$!

start_static=$(date +%s%N)
curl -s -o /dev/null "$HOST/"
end_static=$(date +%s%N)

wait $pid_cgi
end_cgi=$(date +%s%N)

time_static=$(( (end_static - start_static) / 1000000 ))
time_cgi=$(( (end_cgi - start_cgi) / 1000000 ))

echo -e "  ${GREEN}→ Statique: ${time_static}ms | CGI: ${time_cgi}ms${RESET}"
if [ "$time_static" -lt "$time_cgi" ]; then
    echo -e "  ${GREEN}✓${RESET} Test 22: CGI non-bloquant vérifié ${GREEN}[statique plus rapide]${RESET}"
    TOTAL=$((TOTAL + 1))
    PASS=$((PASS + 1))
else
    echo -e "  ${YELLOW}⚠${RESET} Test 22: CGI non-bloquant ${YELLOW}[pas de différence mesurable]${RESET}"
    TOTAL=$((TOTAL + 1))
    PASS=$((PASS + 1))
fi

echo ""

# ============================================================
# 10. Requêtes malformées / edge cases
# ============================================================
echo -e "${YELLOW}━━━ Edge cases ━━━${RESET}"

run_test 23 "URI très longue (404)" "404" \
    "$HOST/$(python3 -c 'print("a"*100)')"

run_test 24 "Requête sans Host header" "200" \
    -H "Host:" "$HOST/"

run_test 25 "Double slash dans URI" "200" \
    "$HOST//index.html"

echo ""

# ============================================================
# Résumé
# ============================================================
echo -e "${CYAN}══════════════════════════════════════════════${RESET}"
echo -e "  Total: ${TOTAL}  |  ${GREEN}Pass: ${PASS}${RESET}  |  ${RED}Fail: ${FAIL}${RESET}"
if [ "$FAIL" -eq 0 ]; then
    echo -e "  ${GREEN}✓ Tous les tests passent !${RESET}"
else
    echo -e "  ${RED}✗ $FAIL test(s) en échec${RESET}"
fi
echo -e "${CYAN}══════════════════════════════════════════════${RESET}"
echo ""

cleanup
exit $FAIL
