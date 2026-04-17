#include "../../includes/cgi/CGIHandler.hpp"
#include "../../includes/http/Response.hpp"
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <sys/stat.h> 
// Fonctions utilitaires (declarees dans CGIHandler.hpp, partagees avec CGIAsync.cpp)


// Fonction principale executeCGI()

Response executeCGI(const Request& request, const std::string& scriptPath, const std::string& interpreter, const ServerConfig* serverConfig)
{
    Response response;

    struct stat fileStat;
    if (stat(scriptPath.c_str(), &fileStat) != 0)
    {
        std::cerr << "CGI: Script introuvable: " << scriptPath << std::endl;
        return ResponseBuilder::makeError(HttpStatus::NOT_FOUND);
    }
    
    // ÉTAPE 1 : Créer les pipes
    int pipe_in[2];   // Serveur -> CGI (body POST)
    int pipe_out[2];  // CGI -> Serveur (sortie du script)
    
    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1)
    {
        std::cerr << "CGI: Erreur création pipes" << std::endl;
        return ResponseBuilder::makeError(HttpStatus::INTERNAL_SERVER_ERROR);
    }
    
    // ÉTAPE 2 : Fork 
    pid_t pid = fork();
    
    if (pid == -1)
    {
        std::cerr << "CGI: Erreur fork()" << std::endl;
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);
        return ResponseBuilder::makeError(HttpStatus::INTERNAL_SERVER_ERROR);
    }
    
    if (pid == 0) // Processus enfant : on exécute le script
    {
        // Rediriger stdin/stdout vers les pipes
        dup2(pipe_in[0], STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);
        
        // Fermer les descripteurs non utilisés
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);
        
        // Se placer dans le répertoire du script
        size_t lastSlash = scriptPath.rfind('/');
        if (lastSlash != std::string::npos)
        {
            std::string scriptDir = scriptPath.substr(0, lastSlash);
            if (!scriptDir.empty())
                chdir(scriptDir.c_str());
        }
  
        // Construire les variables d'environnement
        std::vector<std::string> envVars = buildEnvVars(request, scriptPath, serverConfig);
        char** envp = vectorToEnvp(envVars);
        
        // Construire les arguments pour execve
        std::string filename = getFilename(scriptPath);
        char* args[3];
        args[0] = const_cast<char*>(interpreter.c_str());
        args[1] = const_cast<char*>(filename.c_str());
        args[2] = NULL;
        
        // Exécuter le script
        execve(interpreter.c_str(), args, envp);
        
        // Si execve échoue
        std::cerr << "CGI: Erreur execve() - " << interpreter << std::endl;
        freeEnvp(envp);
        exit(1);
    }
    else  // Processus parent : on lit la sortie du CGI
    {
      close(pipe_in[0]);
      close(pipe_out[1]);
        
        // Envoyer le body POST si présent
        if (!request.body.empty())
            write(pipe_in[1], request.body.c_str(), request.body.size());
        close(pipe_in[1]);
        
        // Lire la sortie du script
        std::string cgiOutput;
        char buffer[4096];
        ssize_t bytesRead;
        
        time_t startTime = time(NULL);
        int timeout = WebservConfig::TIMEOUT_CGI;
        
        while ((bytesRead = read(pipe_out[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bytesRead] = '\0';
            cgiOutput += buffer;
            
            // Vérifier le timeout
            if (time(NULL) - startTime > timeout)
            {
                std::cerr << "CGI: Timeout" << std::endl;
                kill(pid, SIGKILL);
                break;
            }
        }
        close(pipe_out[0]);
        
        // Attendre la fin du processus
        int status;
        waitpid(pid, &status, 0);
        
        // Vérifier le statut de sortie
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            std::cerr << "CGI: Erreur " << WEXITSTATUS(status) << std::endl;
            return ResponseBuilder::makeError(HttpStatus::BAD_GATEWAY);
        }
        
        if (WIFSIGNALED(status))
        {
            std::cerr << "CGI: Tué par signal" << std::endl;
            return ResponseBuilder::makeError(HttpStatus::GATEWAY_TIMEOUT);
        }
        
        if (cgiOutput.empty())
        {
            std::cerr << "CGI: Sortie vide" << std::endl;
            return ResponseBuilder::makeError(HttpStatus::BAD_GATEWAY);
        }
        
        // Parser la sortie
        parseCGIOutput(cgiOutput, response);
    }
    
    return response;
}


// Variables d'environnement CGI buildEnvVars() 

std::vector<std::string> buildEnvVars(const Request& request, const std::string& scriptPath, const ServerConfig* serverConfig)
{
    std::vector<std::string> env;
    std::ostringstream oss;
    
    // Variables obligatoires
    env.push_back("REQUEST_METHOD=" + request.method);
    env.push_back("QUERY_STRING=" + request.query_string);
    
    oss << request.body.size();
    env.push_back("CONTENT_LENGTH=" + oss.str());
    
    std::map<std::string, std::string>::const_iterator it;
    it = request.headers.find("Content-Type");
    if (it != request.headers.end())
        env.push_back("CONTENT_TYPE=" + it->second);
    else
        env.push_back("CONTENT_TYPE=");
    
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    env.push_back("SCRIPT_NAME=" + request.uri);
    env.push_back("PATH_INFO=" + request.uri);
    
    // Variables serveur
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    
    if (serverConfig)
    {
        env.push_back("SERVER_NAME=" + serverConfig->server_name);
        oss.str("");
        oss << serverConfig->listen_port;
        env.push_back("SERVER_PORT=" + oss.str());
    }
    else
    {
        env.push_back("SERVER_NAME=localhost");
        env.push_back("SERVER_PORT=8080");
    }
    
    env.push_back("SERVER_SOFTWARE=Webserv/1.0");
    env.push_back("REQUEST_URI=" + request.uri);
    
    // Headers HTTP -> HTTP_*
    for (it = request.headers.begin(); it != request.headers.end(); ++it)
    {
        std::string name = "HTTP_" + it->first;
        for (size_t i = 0; i < name.size(); ++i)
        {
            if (name[i] == '-')
                name[i] = '_';
            else if (name[i] >= 'a' && name[i] <= 'z')
                name[i] = name[i] - 'a' + 'A';
        }
        env.push_back(name + "=" + it->second);
    }
    
    env.push_back("REDIRECT_STATUS=200");
    
    return env;
}

// vectorToEnvp() -> convertit vector<string> en char**
char** vectorToEnvp(const std::vector<std::string>& env)
{
    char** envp = new char*[env.size() + 1];
    
    for (size_t i = 0; i < env.size(); ++i)
    {
        envp[i] = new char[env[i].size() + 1];
        std::strcpy(envp[i], env[i].c_str());
    }
    envp[env.size()] = NULL;
    
    return envp;
}


// freeEnvp() -> libère la mémoire
void freeEnvp(char** envp)
{
    if (!envp)
        return;
    
    for (size_t i = 0; envp[i] != NULL; ++i)
        delete[] envp[i];
    delete[] envp;
}

// parseCGIOutput() -> parse la sortie du script
void parseCGIOutput(const std::string& cgiOutput, Response& response)
{
    // Chercher la séparation headers/body
    size_t headerEnd = cgiOutput.find("\r\n\r\n");
    std::string separator = "\r\n\r\n";
    
    if (headerEnd == std::string::npos)
    {
        headerEnd = cgiOutput.find("\n\n");
        separator = "\n\n";
    }
    
    if (headerEnd == std::string::npos)
    {
        // Pas de headers, tout est body
        ResponseBuilder::setStatus(response, HttpStatus::OK);
        ResponseBuilder::setHeader(response, "Content-Type", "text/html");
        ResponseBuilder::setBody(response, cgiOutput);
        ResponseBuilder::build(response);
        return;
    }
    
    std::string headersPart = cgiOutput.substr(0, headerEnd);
    std::string bodyPart = cgiOutput.substr(headerEnd + separator.size());
    
    ResponseBuilder::setStatus(response, HttpStatus::OK);
    
    std::istringstream iss(headersPart);
    std::string line;
    
    while (std::getline(iss, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line = line.substr(0, line.size() - 1);
        
        if (line.empty())
            continue;
        
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
            continue;
        
        std::string name = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        
        size_t start = value.find_first_not_of(" \t");
        if (start != std::string::npos)
            value = value.substr(start);
        
        if (name == "Status")
        {
            int code = std::atoi(value.c_str());
            if (code > 0)
                ResponseBuilder::setStatus(response, code);
        }
        else
        {
            ResponseBuilder::setHeader(response, name, value);
        }
    }
    
    ResponseBuilder::setBody(response, bodyPart);
    ResponseBuilder::build(response);
}


std::string getFilename(const std::string& path)
{
    size_t lastSlash = path.rfind('/');
    if (lastSlash == std::string::npos)
        return path;
    return path.substr(lastSlash + 1);
}