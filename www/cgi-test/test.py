#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ============================================================
# Script CGI de test
# Affiche les informations reçues du serveur
# ============================================================

import os
import sys

# Le CGI doit d'abord envoyer les headers HTTP
print("Content-Type: text/html")
print("")  # Ligne vide = fin des headers

# Ensuite le body HTML
print("<!DOCTYPE html>")
print("<html>")
print("<head>")
print("    <title>CGI Test - Webserv</title>")
print("    <style>")
print("        body { font-family: Arial, sans-serif; margin: 40px; }")
print("        h1 { color: #333; }")
print("        table { border-collapse: collapse; width: 100%; }")
print("        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }")
print("        th { background-color: #4CAF50; color: white; }")
print("        tr:nth-child(even) { background-color: #f2f2f2; }")
print("    </style>")
print("</head>")
print("<body>")
print("    <h1>🎉 CGI fonctionne !</h1>")
print("    <p>Ce contenu a été généré par un script Python.</p>")
print("")
print("    <h2>Variables d'environnement CGI :</h2>")
print("    <table>")
print("        <tr><th>Variable</th><th>Valeur</th></tr>")

# Afficher les variables CGI importantes
cgi_vars = [
    'REQUEST_METHOD',
    'QUERY_STRING',
    'CONTENT_LENGTH',
    'CONTENT_TYPE',
    'SCRIPT_FILENAME',
    'SCRIPT_NAME',
    'PATH_INFO',
    'SERVER_NAME',
    'SERVER_PORT',
    'SERVER_PROTOCOL',
    'HTTP_HOST',
    'HTTP_USER_AGENT',
]

for var in cgi_vars:
    value = os.environ.get(var, '<non défini>')
    print(f"        <tr><td>{var}</td><td>{value}</td></tr>")

print("    </table>")

# Si c'est un POST, afficher le body reçu
if os.environ.get('REQUEST_METHOD') == 'POST':
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length > 0:
        body = sys.stdin.read(content_length)
        print("")
        print("    <h2>Body POST reçu :</h2>")
        print(f"    <pre>{body}</pre>")

# Afficher les paramètres GET
query_string = os.environ.get('QUERY_STRING', '')
if query_string:
    print("")
    print("    <h2>Paramètres GET :</h2>")
    print("    <ul>")
    for param in query_string.split('&'):
        if '=' in param:
            key, value = param.split('=', 1)
            print(f"        <li><strong>{key}</strong> = {value}</li>")
    print("    </ul>")

print("")
print("    <p><a href='/'>← Retour à l'accueil</a></p>")
print("</body>")
print("</html>")