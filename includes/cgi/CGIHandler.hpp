#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "../core/Dico.hpp"
#include <string>
#include <vector>
#include <map>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>

/* CGIHandler gère l'exécution des scripts CGI (Python, PHP, etc.)
Le CGI (Common Gateway Interface) permet au serveur d'exécuter des scripts externes et de renvoyer leur sortie au client.
Flux d'exécution :
1. Client envoie requête vers /script.py
2. Serveur crée des pipes pour communiquer avec le script
3. Serveur fork() un processus enfant
4. Enfant exécute le script avec execve()
5. Parent lit la sortie du script
6. Parent renvoie la sortie au client
*/

Response executeCGI(const Request& request, const std::string& scriptPath, const std::string& interpreter, const ServerConfig* serverConfig);

#endif