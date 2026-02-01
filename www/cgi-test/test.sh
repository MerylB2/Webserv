#!/bin/bash

# Headers CGI (obligatoire)
echo "Content-Type: text/html"
echo ""

# Body HTML
echo "<!DOCTYPE html>"
echo "<html>"
echo "<head><title>CGI Bash - Webserv</title></head>"
echo "<body style='font-family: Arial; margin: 40px;'>"
echo "<h1>🐚 CGI Bash fonctionne !</h1>"
echo "<p>Ce contenu a été généré par un script Bash.</p>"
echo "<table border='1' style='border-collapse: collapse;'>"
echo "<tr><th>Info</th><th>Valeur</th></tr>"
echo "<tr><td>Date</td><td>$(date)</td></tr>"
echo "<tr><td>User</td><td>$(whoami)</td></tr>"
echo "<tr><td>Hostname</td><td>$(hostname)</td></tr>"
echo "<tr><td>REQUEST_METHOD</td><td>$REQUEST_METHOD</td></tr>"
echo "<tr><td>QUERY_STRING</td><td>$QUERY_STRING</td></tr>"
echo "<tr><td>SERVER_NAME</td><td>$SERVER_NAME</td></tr>"
echo "</table>"
echo "<p><a href='/'>← Retour</a></p>"
echo "</body>"
echo "</html>"