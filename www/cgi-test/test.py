#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys

print("Content-Type: text/html")
print("")

method = os.environ.get('REQUEST_METHOD', 'GET')
query_string = os.environ.get('QUERY_STRING', '')

print("""<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CGI Python - Webserv</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Arial, sans-serif;
            background: #0f0f0f;
            color: #e0e0e0;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        header {
            width: 100%;
            padding: 30px 0 20px;
            text-align: center;
            background: linear-gradient(135deg, #1a1a2e, #16213e);
            border-bottom: 2px solid #0f3460;
        }
        header h1 { font-size: 1.8em; font-weight: 300; letter-spacing: 4px; color: #e94560; }
        header .tag {
            display: inline-block;
            margin-top: 8px;
            padding: 3px 12px;
            background: #e94560;
            color: #fff;
            border-radius: 12px;
            font-size: 0.75em;
            letter-spacing: 1px;
        }
        main { max-width: 700px; width: 90%; margin: 30px auto; }
        .section {
            background: #1a1a1a;
            border: 1px solid #2a2a2a;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 16px;
        }
        .section h2 { color: #e94560; font-size: 1em; margin-bottom: 12px; font-weight: 500; }
        table { width: 100%; border-collapse: collapse; }
        th { text-align: left; color: #e94560; padding: 8px 12px; border-bottom: 1px solid #333; font-size: 0.85em; }
        td { padding: 8px 12px; border-bottom: 1px solid #222; color: #aaa; font-family: monospace; font-size: 0.85em; }
        tr:hover td { color: #e0e0e0; }
        .method-badge {
            display: inline-block;
            padding: 2px 10px;
            border-radius: 4px;
            font-size: 0.85em;
            font-weight: 600;
            font-family: monospace;
        }
        .method-get { background: #1e6f3e; color: #2ecc71; }
        .method-post { background: #6f4c1e; color: #f39c12; }
        pre {
            background: #111;
            border: 1px solid #333;
            border-radius: 4px;
            padding: 12px;
            font-size: 0.85em;
            color: #5dade2;
            overflow-x: auto;
        }
        .params li { padding: 6px 0; color: #aaa; font-size: 0.9em; list-style: none; }
        .params li strong { color: #5dade2; }
        .params li span { color: #2ecc71; }
        a.back {
            display: inline-block; color: #5dade2; text-decoration: none;
            border: 1px solid #5dade2; padding: 8px 20px; border-radius: 4px;
            font-size: 0.85em; transition: all 0.2s; margin-top: 10px;
        }
        a.back:hover { background: #5dade2; color: #0f0f0f; }
        footer { margin-top: auto; padding: 20px; color: #555; font-size: 0.8em; }
    </style>
</head>
<body>
    <header>
        <h1>CGI PYTHON</h1>""")

if method == 'POST':
    print('        <span class="tag">POST</span>')
else:
    print('        <span class="tag">GET</span>')

print("""    </header>
    <main>""")

# Variables d'environnement CGI
print('        <div class="section">')
print('            <h2>Variables d\'environnement CGI</h2>')
print('            <table>')
print('                <tr><th>Variable</th><th>Valeur</th></tr>')

cgi_vars = [
    'REQUEST_METHOD', 'QUERY_STRING', 'CONTENT_LENGTH', 'CONTENT_TYPE',
    'SCRIPT_FILENAME', 'SCRIPT_NAME', 'PATH_INFO',
    'SERVER_NAME', 'SERVER_PORT', 'SERVER_PROTOCOL',
    'HTTP_HOST', 'HTTP_USER_AGENT',
]

for var in cgi_vars:
    value = os.environ.get(var, '-')
    if not value:
        value = '-'
    print(f'                <tr><td>{var}</td><td>{value}</td></tr>')

print('            </table>')
print('        </div>')

# Parametres GET
if query_string:
    print('        <div class="section">')
    print('            <h2>Parametres GET</h2>')
    print('            <ul class="params">')
    for param in query_string.split('&'):
        if '=' in param:
            key, value = param.split('=', 1)
            print(f'                <li><strong>{key}</strong> = <span>{value}</span></li>')
    print('            </ul>')
    print('        </div>')

# Body POST
if method == 'POST':
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length > 0:
        body = sys.stdin.read(content_length)
        print('        <div class="section">')
        print('            <h2>Body POST</h2>')
        print(f'            <pre>{body}</pre>')
        print('        </div>')

print("""        <a class="back" href="/">Retour a l'accueil</a>
    </main>
    <footer>Webserv CGI &mdash; Python</footer>
</body>
</html>""")
