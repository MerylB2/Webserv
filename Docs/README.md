# Webserv
Créer un serveur web HTTP en C++98 gérant les requêtes GET, POST et DELETE. Servir des fichiers statiques, supporter CGI pour contenu dynamique, et gérer l'upload de fichiers. Traiter plusieurs connexions clients simultanées via I/O non-bloquant avec poll(), select(), epoll()... Configuration par fichier inspiré de NGINX. Ne doit jamais crasher.
