#include "../../includes/core/Server.hpp"
#include "../../includes/http/Request.hpp"
#include "../../includes/http/Response.hpp"
#include "../../includes/http/Router.hpp"  // Ajout pour les redirections
#include "../../includes/cgi/CGIHandler.hpp"

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
        //delete it->second;
    }
    _clients.clear();
    _clientsData.clear(); // Ajout pour nettoyer les données clients
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

// Version 1 de setup
//Fonction qui va permettre de creer chaque socket pour
//le tableau de ports. Exemple un socket pour le port 8080
//un socket pour le port 8081 etc.
void Server::setup(const std::vector<int>& ports)
{
    //Creer un socket pour chaque port

    for (size_t i = 0; i < ports.size(); i++)
    {
        int fd = createServerSocket(ports[i]);
        if (fd < 0)
        {
            std::cerr << "ERREUR: Echec création socket pour port " << ports[i] << std::endl;
            continue;
        }
        _serverSockets.push_back(fd);
    }

    if (_serverSockets.empty())
    {
        std::cerr << "ERREUR: Aucun socket serveur créé !" << std::endl;
        return;
    }
    std::cout << _serverSockets.size() << " socket(s) serveur prêt(s)" << std::endl;
}
// Nouvelle version de setup pour implémenter toutes les fonctionnalités de webserv
void Server::setup(const std::vector<ServerConfig>& configs)
{
    // Sauvegarder les configurations pour y accéder plus tard
    _configs = configs;
   for (size_t i = 0; i < _configs.size(); i++)
   {
        int fd = createServerSocket(_configs[i].listen_port);
        if (fd < 0)
        {
            std::cerr << "ERREUR: Echec création socket pour port " << _configs[i].listen_port << std::endl;
            continue;
        }
        _serverSockets.push_back(fd);
        
        _socketToConfig[fd] = &_configs[i]; // Associer ce socket à sa configuration. Quand un client se connecte sur ce socket, on saura quelle config utiliser
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
    //Il faut clear car entre le dernier appel et celui
    //certains fd ne sont plus valides (clients partis par exemple)
    _pollFds.clear();

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

            //TODO : CREER OBJET CLIENT
            //Client* client = new Client(clientFd)
            // _clients[clientFd] = client;
            
            // Pour l'instant, juste stocker le FD
            _clients[clientFd] = NULL; //temporaire

            ClientData data; // Initialiser ClientData et on associe le client à la configuration du serveur sur lequel il s'est connecté.
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

    for (size_t i = clientStartIndex; i < _pollFds.size(); i++)
    {
        int fd = _pollFds[i].fd;

        //client a ferme ou erreur
        if (_pollFds[i].revents & (POLLHUP | POLLERR))
        {
            std::cerr << "Client " << fd << " déconnecté" << std::endl;
            toRemove.push_back(fd);
            continue;
        }

        //client a quelque chose a lire
        if (_pollFds[i].revents & POLLIN)
        {
            char buffer[4096];
            memset(buffer, 0, sizeof(buffer));

            int bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead <= 0)
            {
                toRemove.push_back(fd);
                continue;
            }

            buffer[bytesRead] = '\0';
            std::cout << "Reçu " << bytesRead << " bytes du client " << fd << std::endl;

            // Parser la requete avec RequestParser
            Request req;
            std::string rawData(buffer);
            bool parseOk = RequestParser::parse(req, rawData);

            std::cout << "Méthode: " << req.method << std::endl;
            std::cout << "URI: " << req.uri << std::endl;
            std::cout << "Parse OK: " << (parseOk ? "oui" : "non") << std::endl;

            // Construire la reponse avec ResponseBuilder
            Response res;

            if (!parseOk || req.state == ERROR)
            {
                // Requete invalide -> erreur 400
                res = ResponseBuilder::makeError(400);
            }
            else if (req.method != "GET" && req.method != "POST" && req.method != "DELETE")
            {
                // Methode non supportee -> erreur 405
                res = ResponseBuilder::makeError(405);
            }
            else
            {
                // Ajout pour utiliser le Router si la config existe
                // Sinon, fallback sur l'ancien comportement (page basique)
                ServerConfig* config = NULL;
                if (_clientsData.find(fd) != _clientsData.end())
                    config = _clientsData[fd].server_config;

                if (config)
                {
                    // Config disponible : utiliser le Router
                    RouteResult result = Router::route(req, config);

                    switch (result.type)
                    {
                        case ROUTE_REDIRECT:
                            // Redirection 301/302 configurée dans le fichier .conf
                            std::cout << "Redirection " << result.redirect_code << " -> " << result.redirect_url << std::endl;
                            res = ResponseBuilder::makeRedirect(result.redirect_code, result.redirect_url);
                            break;

                        case ROUTE_FILE:
                            // Servir un fichier statique
                            ResponseBuilder::setStatus(res, 200);
                            ResponseBuilder::setBodyFromFile(res, result.filepath);
                            ResponseBuilder::build(res);
                            break;

                        case ROUTE_DIRECTORY:
                            // TODO: autoindex (listing du répertoire)
                            res = ResponseBuilder::makeError(403);
                            break;

                        case ROUTE_CGI:
                            std::cout << "CGI: " << result.filepath << std::endl;
                            res = executeCGI(req, result.filepath, result.cgi_interpreter, config);
                            break;

                        case ROUTE_ERROR:
                        default:
                            res = ResponseBuilder::makeError(result.error_code);
                            break;
                    }
                }
                else
                {
                    // Pas de config : ancien comportement (réponse basique)
                    // Requete valide -> reponse 200
                    ResponseBuilder::setStatus(res, 200);
                    ResponseBuilder::setHeader(res, "Content-Type", "text/html");

                    std::string body = "<!DOCTYPE html>\n"
                        "<html>\n"
                        "<head><title>Webserv</title></head>\n"
                        "<body>\n"
                        "<h1>Bienvenue sur Webserv!</h1>\n"
                        "<p>Methode: " + req.method + "</p>\n"
                        "<p>URI: " + req.uri + "</p>\n"
                        "<p>Version: " + req.version + "</p>\n"
                        "</body>\n"
                        "</html>\n";

                    ResponseBuilder::setBody(res, body);
                    ResponseBuilder::build(res);
                }
            }

            // Envoyer la reponse immediatement
            std::cout << "Envoi réponse: " << res.status_code << " " << res.status_message << std::endl;

            int bytesSent = send(fd, res.send_buffer.c_str(), res.send_buffer.size(), 0);

            if (bytesSent < 0)
                std::cerr << "ERREUR: send() failed" << std::endl;
            else
                std::cout << "Envoyé " << bytesSent << " bytes" << std::endl;

            // Fermer la connexion (pas de keep-alive pour l'instant)
            toRemove.push_back(fd);
        }
    }

    //Nettoyer clients deconnectes
    for (size_t i = 0; i < toRemove.size(); i++)
    {
        int fd = toRemove[i];
        close(fd);
        // delete _clients[fd];  // Quand Client existera
        _clients.erase(fd);
        _clientsData.erase(fd);  // Ajout pour nettoyer aussi ClientData
    }
}


void Server::checkTimeouts()
{
    // TODO: Implémenter quand Client aura lastActivity
}

//Fonction generale qui appelle les autres fonctions
void Server::run()
{
    _running = true;
    std::cout << "Lancement du serveur" << std::endl;
    std::cout << "Appuyer sur CTRL+C pour terminer" << std::endl;

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

        if (activity == 0)
        {
            //Timeout
            continue;
        }

        // 3. Traiter nouveaux clients
        handleNewConnections();

        // 4. Traiter les clients existants
        handleClientEvents();

        // 5. Verifier timeouts
        checkTimeouts();
    }

    std::cout << "Serveur arrêté proprement" << std::endl;
}

void Server::stop()
{
    _running = false;
}