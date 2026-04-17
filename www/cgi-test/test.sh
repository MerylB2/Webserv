#!/bin/bash

echo "Content-Type: text/html"
echo ""

cat << HTMLEOF
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CGI Bash - Webserv</title>
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
            display: inline-block; margin-top: 8px; padding: 3px 12px;
            background: #2ecc71; color: #fff; border-radius: 12px;
            font-size: 0.75em; letter-spacing: 1px;
        }
        main { max-width: 700px; width: 90%; margin: 30px auto; }
        .section {
            background: #1a1a1a; border: 1px solid #2a2a2a;
            border-radius: 8px; padding: 20px; margin-bottom: 16px;
        }
        .section h2 { color: #e94560; font-size: 1em; margin-bottom: 12px; font-weight: 500; }
        table { width: 100%; border-collapse: collapse; }
        th { text-align: left; color: #e94560; padding: 8px 12px; border-bottom: 1px solid #333; font-size: 0.85em; }
        td { padding: 8px 12px; border-bottom: 1px solid #222; color: #aaa; font-family: monospace; font-size: 0.85em; }
        tr:hover td { color: #e0e0e0; }
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
        <h1>CGI BASH</h1>
        <span class="tag">BASH</span>
    </header>
    <main>
        <div class="section">
            <h2>Informations systeme</h2>
            <table>
                <tr><th>Info</th><th>Valeur</th></tr>
                <tr><td>Date</td><td>$(date '+%Y-%m-%d %H:%M:%S')</td></tr>
                <tr><td>User</td><td>$(whoami)</td></tr>
                <tr><td>Hostname</td><td>$(hostname)</td></tr>
                <tr><td>Kernel</td><td>$(uname -r)</td></tr>
            </table>
        </div>
        <div class="section">
            <h2>Variables CGI</h2>
            <table>
                <tr><th>Variable</th><th>Valeur</th></tr>
                <tr><td>REQUEST_METHOD</td><td>${REQUEST_METHOD:--}</td></tr>
                <tr><td>QUERY_STRING</td><td>${QUERY_STRING:--}</td></tr>
                <tr><td>SERVER_NAME</td><td>${SERVER_NAME:--}</td></tr>
                <tr><td>SERVER_PORT</td><td>${SERVER_PORT:--}</td></tr>
                <tr><td>SCRIPT_NAME</td><td>${SCRIPT_NAME:--}</td></tr>
                <tr><td>HTTP_HOST</td><td>${HTTP_HOST:--}</td></tr>
            </table>
        </div>
        <a class="back" href="/">Retour a l'accueil</a>
    </main>
    <footer>Webserv CGI &mdash; Bash</footer>
</body>
</html>
HTMLEOF
