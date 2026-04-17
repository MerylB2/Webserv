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

/* CGIHandler gere l'execution des scripts CGI (Python, Bash, etc.)

Version non-bloquante :
1. startCGI()  -> fork + execve, retourne CGIData avec pid et pipe_out
2. Le serveur surveille pipe_out dans poll() et accumule la sortie
3. finishCGI() -> waitpid + parse sortie -> retourne Response
*/

// Lance le CGI de facon non-bloquante
// Retourne CGIData avec pid et pipe_out remplis (pid = -1 si erreur)
CGIData startCGI(const Request& request, const std::string& scriptPath,
    const std::string& interpreter, const ServerConfig* serverConfig);

// Finalise le CGI : waitpid, parse la sortie accumulee, retourne la Response
Response finishCGI(CGIData& cgi);

// Version bloquante (ancienne, conservee pour reference)
Response executeCGI(const Request& request, const std::string& scriptPath,
    const std::string& interpreter, const ServerConfig* serverConfig);

// Fonctions utilitaires partagees entre CGIHandler.cpp et CGIAsync.cpp
std::vector<std::string> buildEnvVars(const Request& request, const std::string& scriptPath, const ServerConfig* serverConfig);
char** vectorToEnvp(const std::vector<std::string>& env);
void freeEnvp(char** envp);
void parseCGIOutput(const std::string& cgiOutput, Response& response);
std::string getFilename(const std::string& path);

#endif