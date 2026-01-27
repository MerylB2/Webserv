#include "../../includes/core/Server.hpp"
#include "../../includes/http/Request.hpp"
#include "../../includes/http/Response.hpp"

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
}

//Fonction qui va permet la creation du socket
int Server::createServerSocket(int port)
{
    std::cout << "Creation du socket pour port " << port << std::endl;
    
    // 1. Ccreation du socket : AF_INET = IPv4 / SOCK_STREAM = TCP / 0 = auto
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
    {
        std::cerr << "ERREUR: Impossible de creer le socket" << std::endl;
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
        std::cerr << "ERREUR: Impossible d'ecouter" << std::endl;
        close(serverFd);
        return -1;
    }

    std::cout << "Socket serveur sur port " << port << " (FD = " << serverFd << ")" << std::endl;
    return serverFd;
}

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
            std::cerr << "ERREUR: Echec creation socket pour port " << ports[i] << std::endl;
            continue;
        }
        _serverSockets.push_back(fd);
    }

    if (_serverSockets.empty())
    {
        std::cerr << "ERREUR: Aucun socket serveur cree !" << std::endl;
        return;
    }
    std::cout << _serverSockets.size() << " socket(s) serveur pret(s)" << std::endl;
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

            std::cout << "Nouveau client connecte ! (FD = " << clientFd << ")" << std::endl;

            //TODO : CREER OBJET CLIENT
            //Client* client = new Client(clientFd)
            // _clients[clientFd] = client;
            
            // Pour l'instant, juste stocker le FD
            _clients[clientFd] = NULL; //temporaire
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
            std::cerr << "Client " << fd << " deconnecte" << std::endl;
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
            std::cout << "Recu " << bytesRead << " bytes du client " << fd << std::endl;

            // Parser la requete avec RequestParser
            Request req;
            std::string rawData(buffer);
            bool parseOk = RequestParser::parse(req, rawData);

            std::cout << "Methode: " << req.method << std::endl;
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

            // Envoyer la reponse immediatement
            std::cout << "Envoi reponse: " << res.status_code << " " << res.status_message << std::endl;

            int bytesSent = send(fd, res.send_buffer.c_str(), res.send_buffer.size(), 0);

            if (bytesSent < 0)
                std::cerr << "ERREUR: send() failed" << std::endl;
            else
                std::cout << "Envoye " << bytesSent << " bytes" << std::endl;

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

    std::cout << "Serveur arrete proprement" << std::endl;
}

void Server::stop()
{
    _running = false;
}