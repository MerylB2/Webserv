#!/usr/bin/env python3
import os

print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>Test chdir() CGI</h1>")
print(f"<p><b>Current Working Directory:</b> {os.getcwd()}</p>")
print(f"<p><b>Script Location:</b> {os.path.dirname(os.path.abspath(__file__))}</p>")

if os.path.exists("test_chdir.py"):
    print("<p style='color: green;'><b>SUCCESS:</b> chdir() fonctionne!</p>")
else:
    print("<p style='color: red;'><b>FAIL:</b> chdir() ne fonctionne pas.</p>")

print("<h2>Fichiers dans le repertoire courant:</h2>")
print("<ul>")
for f in os.listdir("."):
    print(f"<li>{f}</li>")
print("</ul>")
print("</body></html>")