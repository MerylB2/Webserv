#include "../../includes/core/Server.hpp"
#include "../../includes/http/Request.hpp"
#include "../../includes/http/Response.hpp"
#include "../../includes/http/Router.hpp"
#include "../../includes/cgi/CGIHandler.hpp"
#include "SessionHandler.hpp"
#include "Client.hpp"
#include <fstream>
#include <cstdio>
#include <sstream>

Server::Server() : _running(false)
{}

Server::~Server()
{
    //Fermer tous les sockets
    for (size_t i = 0; i < _serverSockets.size(); i++)
    {
        close(_serverSockets[i]);
    }

    //Supprimer tous les clients
    std::map<int, Client*>::iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it)
    {
        close(it->first);
        delete it->second;
    }
    _clients.clear();
    _clientsData.clear();
}

//Fonction qui va permet la creation du socket
int Server::createServerSocket(int port)
{
    std::cout << "Création du socket pour port " << port << std::endl;

    // 1. Creation du socket : AF_INET = IPv4 / SOCK_STREAM = TCP / 0 = auto
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
    {
        std::cerr << "ERREUR: Impossible de créer le socket" << std::endl;
        return -1;
    }

    // 2. SO_REUSEADDR => Permet de relancer sur le meme port immediatement
    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "ERREUR: setsockopt() failed" << std::endl;
        close(serverFd);
        return -1;
    }

    // 3. Mode non-bloquant permet de ne jamais bloquer si un client est lent
    // poll() me dit quand un fd est prêt,
    //le mode non-bloquant garantit que je ne bloque jamais.
    int flags = fcntl(serverFd, F_GETFL, 0);
    if (fcntl(serverFd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        std::cerr << "ERREUR: fcntl() failed" << std::endl;
        close(serverFd);
        return -1;
    }

    // 4. Bind (Attachement au port)
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        std::cerr << "ERREUR: Impossible de bind au port " << port << std::endl;
        close(serverFd);
        return -1;
    }

    // 5. Listen
    if (listen(serverFd, 128) < 0)
    {
        std::cerr << "ERREUR: Impossible d'écouter" << std::endl;
        close(serverFd);
        return -1;
    }

    std::cout << "Socket serveur sur port " << port << " (FD = " << serverFd << ")" << std::endl;
    return serverFd;
}

//Fonction qui va permettre de creer chaque socket pour
//le tableau de configs. Exemple un socket pour le port 8080
//un socket pour le port 8081 etc.
void Server::setup(const std::vector<ServerConfig>& config)
{
    //Stocker la config
    _config = config;

    //Creer un socket pour chaque port
    for (size_t i = 0; i < _config.size(); i++)
    {
        int port = _config[i].listen_port;
        int fd = createServerSocket(port);
        if (fd < 0)
        {
            std::cerr << "ERREUR: Echec creation socket pour port " << port << std::endl;
            continue;
        }
        _serverSockets.push_back(fd);
        _socketToConfig[fd] = &_config[i];
    }

    if (_serverSockets.empty())
    {
        std::cerr << "ERREUR: Aucun socket serveur créé !" << std::endl;
        return;
    }
    std::cout << _serverSockets.size() << " socket(s) serveur prêt(s)" << std::endl;
}

//Cette fonction permet de creer le vecteur de fds
//que va devoir surveiller poll()
//D'abord les fds concernant les sockets serveurs
//Puis les fds concernant les fds sockets clients
void Server::buildPollFds()
{
    _pollFds.clear();
    _cgiPipeToClient.clear();

    //socket serveur
    for (size_t i = 0; i < _serverSockets.size(); i++)
    {
        struct pollfd pfd;
        pfd.fd = _serverSockets[i];
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollFds.push_back(pfd);
    }

    //socket clients
    std::map<int, Client*>::iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it)
    {
        struct pollfd pfd;
        pfd.fd = it->first;
        pfd.events = POLLIN | POLLOUT;
        pfd.revents = 0;
        _pollFds.push_back(pfd);
    }

    //pipes CGI (pour les clients en attente de CGI)
    _cgiStartIndex = _pollFds.size();
    for (it = _clients.begin(); it != _clients.end(); ++it)
    {
        Client* client = it->second;
        if (client->getClientState() == CLIENT_WAITING_CGI)
        {
            CGIData* cgi = client->getCGIData();
            if (cgi->pipe_out >= 0)
            {
                struct pollfd pfd;
                pfd.fd = cgi->pipe_out;
                pfd.events = POLLIN;
                pfd.revents = 0;
                _pollFds.push_back(pfd);
                _cgiPipeToClient[cgi->pipe_out] = it->first;
            }
        }
    }
}

void Server::handleNewConnections()
{
    for (size_t i = 0; i < _serverSockets.size(); i++)
    {
        if (_pollFds[i].revents & POLLIN)
        {
            struct sockaddr_in clientAddress;
            socklen_t clientLen = sizeof(clientAddress);

            int clientFd = accept(_serverSockets[i], (struct sockaddr*)&clientAddress, &clientLen);

            if (clientFd < 0)
            {
                // En mode non-bloquant, EAGAIN signifie "pas de client"
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    std::cerr << "ERREUR: accept() failed" << std::endl;
                continue;
            }
            //Mettre de nouveau en non-bloquant concernant le client
            int flags = fcntl(clientFd, F_GETFL, 0);
            fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

            std::cout << "Nouveau client connecté ! (FD = " << clientFd << ")" << std::endl;

            //CREER OBJET CLIENT
            Client* client = new Client(clientFd, _serverSockets[i]);
            _clients[clientFd] = client;

            ClientData data;
            data.socket_fd = clientFd;
            data.server_config = _socketToConfig[_serverSockets[i]];
            data.last_activity = time(NULL);
            _clientsData[clientFd] = data;
        }
    }
}

void Server::handleClientEvents()
{
    //Commencer apres les sockets serveur
    size_t clientStartIndex = _serverSockets.size();

    std::vector<int> toRemove;

    for (size_t i = clientStartIndex; i < _cgiStartIndex; i++)
    {
        int fd = _pollFds[i].fd;

        //Recuperation du client
        Client* client = _clients[fd];

        if (!client)
            continue;

        //client deconnecte ou erreur
        if (_pollFds[i].revents & (POLLHUP | POLLERR))
        {
            std::cerr << "Client " << fd << " déconnecté" << std::endl;
            toRemove.push_back(fd);
            continue;
        }

        //client a des donnees a lire
        if (_pollFds[i].revents & POLLIN)
        {
            if (client->getClientState() == CLIENT_READING)
            {
                int result = client->readData();

                if (result < 0)
                {
                    std::cout << "Client " << fd << " erreur lecture" << std::endl;
                    toRemove.push_back(fd);
                    continue;
                }

                if (result == 0)
                {
                    std::cout << "Client " << fd << " termine normalement" << std::endl;
                    toRemove.push_back(fd);
                    continue;
                }

                if (client->getRequest()->state == COMPLETE)
                {
                    std::cout << "Requete prete a traiter" << std::endl;

                    Request* req = client->getRequest();
                    Response* res = client->getResponse();

                    if (req->error_code != 0 || req->state == ERROR)
                    {
                        *res = ResponseBuilder::makeError(req->error_code ? req->error_code : 400);
                    }
                    else
                    {
                        // Recuperer la config du serveur pour ce client
                        ServerConfig* config = NULL;
                        if (_clientsData.find(fd) != _clientsData.end())
                            config = _clientsData[fd].server_config;

                        if (!config)
                        {
                            *res = ResponseBuilder::makeError(500);
                        }
                        else if (req->body.size() > config->max_body_size)
                        {
                            *res = ResponseBuilder::makeError(413);
                        }
                        else
                        {
                            RouteResult result = Router::route(*req, config);

                            switch (result.type)
                            {
                                case ROUTE_LOGIN:
                                {
                                    SessionHandler::handleLogin(*req, *res);
                                    ResponseBuilder::build(*res);
                                    break;
                                }
                                case ROUTE_LOGOUT:
                                {
                                    std::string session_id = RequestParser::getCookie(*req, "session_id");
                                    SessionHandler::handleLogout(*req, *res, session_id);
                                    ResponseBuilder::build(*res);
                                    break;
                                }
                                case ROUTE_PROTECTED_PAGE:
                                {
                                    SessionHandler::handleProtectedPage(*req, *res, result.session_id);
                                    ResponseBuilder::build(*res);
                                    break;
                                }
                                case ROUTE_FILE:
                                {
                                    std::cout << "FILE: " << result.filepath << std::endl;
                                    ResponseBuilder::setStatus(*res, 200);
                                    ResponseBuilder::setBodyFromFile(*res, result.filepath);
                                    ResponseBuilder::build(*res);
                                    break;
                                }
                                case ROUTE_UPLOAD:
                                {
                                    std::ofstream outFile(result.upload_path.c_str(), std::ios::binary);
                                    if (outFile.is_open())
                                    {
                                        outFile.write(req->body.c_str(), req->body.size());
                                        outFile.close();
                                        std::cout << "UPLOAD: " << result.upload_path << " (" << req->body.size() << " bytes)" << std::endl;
                                        ResponseBuilder::setStatus(*res, 201);
                                        ResponseBuilder::setHeader(*res, "Content-Type", "text/html");
                                        std::ostringstream oss;
                                        oss << "<!DOCTYPE html><html><body>"
                                            << "<h1>201 Created</h1>"
                                            << "<p>File uploaded (" << req->body.size() << " bytes)</p>"
                                            << "</body></html>";
                                        ResponseBuilder::setBody(*res, oss.str());
                                        ResponseBuilder::build(*res);
                                    }
                                    else
                                    {
                                        std::cerr << "UPLOAD ERREUR: " << result.upload_path << std::endl;
                                        *res = ResponseBuilder::makeError(500);
                                    }
                                    break;
                                }
                                case ROUTE_DELETE:
                                {
                                    if (std::remove(result.filepath.c_str()) == 0)
                                    {
                                        std::cout << "DELETE: " << result.filepath << std::endl;
                                        ResponseBuilder::setStatus(*res, 204);
                                        ResponseBuilder::setBody(*res, "");
                                        ResponseBuilder::build(*res);
                                    }
                                    else
                                        *res = ResponseBuilder::makeError(500);
                                    break;
                                }
                                case ROUTE_REDIRECT:
                                {
                                    std::cout << "REDIRECT " << result.redirect_code << " -> " << result.redirect_url << std::endl;
                                    *res = ResponseBuilder::makeRedirect(result.redirect_code, result.redirect_url);
                                    break;
                                }
                                case ROUTE_CGI:
                                {
                                    std::cout << "CGI: " << result.filepath << std::endl;
                                    CGIData cgiData = startCGI(*req, result.filepath, result.cgi_interpreter, config);
                                    if (cgiData.pid < 0)
                                    {
                                        int code = (cgiData.errorCode > 0) ? cgiData.errorCode : 500;
                                        *res = ResponseBuilder::makeError(code);
                                    }
                                    else
                                    {
                                        *(client->getCGIData()) = cgiData;
                                        client->setState(CLIENT_WAITING_CGI);
                                    }
                                    break;
                                }
                                case ROUTE_DIRECTORY:
                                {
                                    std::cout << "AUTOINDEX: " << result.filepath << std::endl;
                                    if (!Router::handleAutoindex(result.filepath, req->uri, *res))
                                        *res = ResponseBuilder::makeError(403);
                                    break;
                                }
                                case ROUTE_ERROR:
                                default:
                                {
                                    std::map<int, std::string>::iterator errIt = config->error_pages.find(result.error_code);
                                    if (errIt != config->error_pages.end())
                                    {
                                        std::string errPath = config->root_dir + errIt->second;
                                        ResponseBuilder::setStatus(*res, result.error_code);
                                        ResponseBuilder::setBodyFromFile(*res, errPath);
                                        ResponseBuilder::build(*res);
                                    }
                                    else
                                        *res = ResponseBuilder::makeError(result.error_code);
                                    break;
                                }
                            }
                        }
                    }

                    // Ne pas ecraser CLIENT_WAITING_CGI (le CGI est en cours)
                    if (client->getClientState() != CLIENT_WAITING_CGI)
                        client->setState(CLIENT_WRITING);
                }
            }
        }

        if (_pollFds[i].revents & POLLOUT)
        {
            if (client->getClientState() == CLIENT_WRITING)
            {
                int result = client->writeData();

                if (result < 0)
                {
                    std::cout << "Client " << fd << " erreur d'ecriture" << std::endl;
                    toRemove.push_back(fd);
                    continue;
                }

                //Faut-il tout envoyer ?
                if (client->getResponse()->is_complete)
                {
                    std::cout << "Reponse completement envoyee" << std::endl;
                    if (client->shouldKeepAlive())
                    {
                        std::cout << "Keep-alive : pret pour nouvelle requete" << std::endl;
                        client->reset();
                    }
                    else
                    {
                        std::cout << "Client " << fd << " termine" << std::endl;
                        toRemove.push_back(fd);
                    }
                }

            }
        }
    }

    //Nettoyer clients deconnectes
    for (size_t i = 0; i < toRemove.size(); i++)
    {
        int fd = toRemove[i];
        // Nettoyer un CGI en cours si necessaire
        Client* c = _clients[fd];
        if (c)
        {
            CGIData* cgi = c->getCGIData();
            if (cgi->pid > 0)
            {
                kill(cgi->pid, SIGKILL);
                waitpid(cgi->pid, NULL, 0);
            }
            if (cgi->pipe_out >= 0)
                close(cgi->pipe_out);
        }
        close(fd);
        delete _clients[fd];
        _clients.erase(fd);
        _clientsData.erase(fd);
    }
}


void Server::handleCGIEvents()
{
    std::vector<int> toFinish;

    for (size_t i = _cgiStartIndex; i < _pollFds.size(); i++)
    {
        int pipeFd = _pollFds[i].fd;

        if (_cgiPipeToClient.find(pipeFd) == _cgiPipeToClient.end())
            continue;

        int clientFd = _cgiPipeToClient[pipeFd];
        Client* client = _clients[clientFd];
        if (!client)
            continue;

        CGIData* cgi = client->getCGIData();
        bool done = false;

        // Lire les donnees disponibles du pipe CGI
        if (_pollFds[i].revents & (POLLIN | POLLHUP))
        {
            char buffer[4096];
            ssize_t n;
            while ((n = read(pipeFd, buffer, sizeof(buffer) - 1)) > 0)
            {
                cgi->buffer.append(buffer, n);
            }
            if (n == 0)
                done = true;
            if (_pollFds[i].revents & (POLLHUP | POLLERR))
                done = true;
        }

        if (done)
            toFinish.push_back(clientFd);
    }

    // Finaliser les CGI termines
    for (size_t i = 0; i < toFinish.size(); i++)
    {
        int clientFd = toFinish[i];
        Client* client = _clients[clientFd];
        if (!client)
            continue;

        std::cout << "CGI termine pour client " << clientFd << std::endl;

        Response* res = client->getResponse();
        CGIData* cgi = client->getCGIData();

        *res = finishCGI(*cgi);
        client->setState(CLIENT_WRITING);
    }
}


void Server::checkTimeouts()
{
    time_t now = time(NULL);

    std::vector<int> toRemove;

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        Client* client = it->second;

        // Timeout CGI : verifier les clients en attente de CGI
        if (client->getClientState() == CLIENT_WAITING_CGI)
        {
            CGIData* cgi = client->getCGIData();
            if (cgi->pid > 0)
            {
                double cgiDiff = difftime(now, cgi->start_time);
                if (cgiDiff > WebservConfig::TIMEOUT_CGI)
                {
                    std::cerr << "CGI timeout pour client " << it->first
                        << " (" << cgiDiff << "s)" << std::endl;
                    kill(cgi->pid, SIGKILL);
                    waitpid(cgi->pid, NULL, 0);
                    if (cgi->pipe_out >= 0)
                    {
                        close(cgi->pipe_out);
                        cgi->pipe_out = -1;
                    }
                    cgi->reset();
                    Response* res = client->getResponse();
                    *res = ResponseBuilder::makeError(HttpStatus::GATEWAY_TIMEOUT);
                    client->setState(CLIENT_WRITING);
                }
            }
            continue;
        }

        // Timeout client classique
        double diff = difftime(now, client->getLastActivity());

        if (diff > 60)
        {
            std::cout << "Client " << it->first << " timeout (" << diff << "s d'inactivite)" << std::endl;
            toRemove.push_back(it->first);
        }
    }

    // Nettoyage des clients timeout
    for (size_t i = 0; i < toRemove.size(); i++)
    {
        int fd = toRemove[i];
        // Nettoyer un CGI en cours si necessaire
        Client* c = _clients[fd];
        if (c)
        {
            CGIData* cgi = c->getCGIData();
            if (cgi->pid > 0)
            {
                kill(cgi->pid, SIGKILL);
                waitpid(cgi->pid, NULL, 0);
            }
            if (cgi->pipe_out >= 0)
                close(cgi->pipe_out);
        }
        close(fd);
        delete(_clients[fd]);
        _clients.erase(fd);
    }
}

//Fonction generale qui appelle les autres fonctions
void Server::run()
{
    _running = true;
    std::cout << "Lancement du serveur" << std::endl;
    std::cout << "Appuyer sur CTRL+C pour terminer" << std::endl;

    SessionManager* sm = SessionManager::getInstance();
    time_t last_cleanup = time(NULL);

    while(_running)
    {
        // 1. Preparer poll
        buildPollFds();

        // 2. Attendre un evenement timeout 1000ms
        int activity = poll(_pollFds.data(), _pollFds.size(), 1000);

        if (activity < 0)
        {
            if (!_running)
                break;
            std::cerr << "ERREUR: poll() failed" << std::endl;
            continue;
        }

        // 3. Traiter nouveaux clients
        handleNewConnections();

        // 4. Traiter les clients existants
        handleClientEvents();

        if (activity > 0)
        {
            // 5. Traiter les pipes CGI non-bloquants
            handleCGIEvents();

            // 6. Verifier timeouts (clients + CGI)
            checkTimeouts();
        }

        // 7. Nettoyage periodique des sessions (actif ou inactif)
        time_t now = time(NULL);
        if (now - last_cleanup > 60) {
            sm->cleanupExpiredSessions(3600);  // Expire apres 1h
            last_cleanup = now;
            std::cout << "[SERVER] Nettoyage des sessions" << std::endl;
        }
    }

    SessionManager::destroy();
    std::cout << "Serveur arrêté proprement" << std::endl;
}

void Server::stop()
{
    _running = false;
}
