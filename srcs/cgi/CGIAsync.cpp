#include "../../includes/cgi/CGIHandler.hpp"
#include "../../includes/http/Response.hpp"
#include <iostream>
#include <fcntl.h>

/* ========== Version non-bloquante du CGI ========== */

// startCGI() : fork + execve, retourne immediatement
// Le pipe_out est mis en non-bloquant pour etre surveille par poll()

CGIData startCGI(const Request& request, const std::string& scriptPath,
    const std::string& interpreter, const ServerConfig* serverConfig)
{
    CGIData cgi;

    int pipe_in[2];
    int pipe_out[2];

    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1)
    {
        std::cerr << "CGI: Erreur creation pipes" << std::endl;
        cgi.pid = -1;
        return cgi;
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        std::cerr << "CGI: Erreur fork()" << std::endl;
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);
        cgi.pid = -1;
        return cgi;
    }

    if (pid == 0)
    {
        // Processus enfant : executer le script
        dup2(pipe_in[0], STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);

        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);

        std::vector<std::string> envVars = buildEnvVars(request, scriptPath, serverConfig);
        char** envp = vectorToEnvp(envVars);

        std::string filename = getFilename(scriptPath);
        char* args[3];
        args[0] = const_cast<char*>(interpreter.c_str());
        args[1] = const_cast<char*>(scriptPath.c_str());
        args[2] = NULL;

        execve(interpreter.c_str(), args, envp);

        std::cerr << "CGI: Erreur execve() - " << interpreter << std::endl;
        freeEnvp(envp);
        exit(1);
    }

    // Processus parent : preparer la lecture non-bloquante
    close(pipe_in[0]);
    close(pipe_out[1]);

    // Envoyer le body POST si present
    if (!request.body.empty())
        write(pipe_in[1], request.body.c_str(), request.body.size());
    close(pipe_in[1]);

    // Mettre pipe_out en non-bloquant pour poll()
    int flags = fcntl(pipe_out[0], F_GETFL, 0);
    fcntl(pipe_out[0], F_SETFL, flags | O_NONBLOCK);

    cgi.pid = pid;
    cgi.pipe_in = -1;
    cgi.pipe_out = pipe_out[0];
    cgi.start_time = time(NULL);
    cgi.buffer.clear();

    std::cout << "CGI lance (PID=" << pid << ", pipe=" << pipe_out[0] << ")" << std::endl;
    return cgi;
}


// finishCGI() : waitpid + parse sortie accumulee -> Response

Response finishCGI(CGIData& cgi)
{
    Response response;

    // Fermer le pipe
    if (cgi.pipe_out >= 0)
    {
        close(cgi.pipe_out);
        cgi.pipe_out = -1;
    }

    // Attendre la fin du processus (devrait etre deja termine)
    int status;
    waitpid(cgi.pid, &status, 0);

    // Verifier le statut de sortie
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
    {
        std::cerr << "CGI: Erreur exit code " << WEXITSTATUS(status) << std::endl;
        cgi.reset();
        return ResponseBuilder::makeError(HttpStatus::BAD_GATEWAY);
    }

    if (WIFSIGNALED(status))
    {
        std::cerr << "CGI: Tue par signal " << WTERMSIG(status) << std::endl;
        cgi.reset();
        return ResponseBuilder::makeError(HttpStatus::GATEWAY_TIMEOUT);
    }

    if (cgi.buffer.empty())
    {
        std::cerr << "CGI: Sortie vide" << std::endl;
        cgi.reset();
        return ResponseBuilder::makeError(HttpStatus::BAD_GATEWAY);
    }

    // Parser la sortie accumulee
    parseCGIOutput(cgi.buffer, response);
    cgi.reset();

    return response;
}
