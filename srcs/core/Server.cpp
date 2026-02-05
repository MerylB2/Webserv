#include "../../includes/core/Server.hpp"
#include "../../includes/http/Request.hpp"
#include "../../includes/http/Response.hpp"
#include "Client.hpp"

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

            //CREER OBJET CLIENT
            Client* client = new Client(clientFd, _serverSockets[i]);
             _clients[clientFd] = client;
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

        //Recuperation du client
        Client* client = _clients[fd];

        if (!client)
            continue;

        //client deconnecte ou erreur
        if (_pollFds[i].revents & (POLLHUP | POLLERR))
        {
            std::cerr << "Client " << fd << " deconnecte" << std::endl;
            toRemove.push_back(fd);
            continue;
        }

        //client a des donnees a lire
        if (_pollFds[i].revents & POLLIN)
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
                    // Requete invalide -> erreur 400
                    *res = ResponseBuilder::makeError(400);
                }
                else if (req->method != "GET" && req->method != "POST" && req->method != "DELETE")
                {
                    // Methode non supportee -> erreur 405
                    *res = ResponseBuilder::makeError(405);
                }
                else
                {
                    // Requete valide -> reponse 200
                    ResponseBuilder::setStatus(*res, 200);
                    ResponseBuilder::setHeader(*res, "Content-Type", "text/html");

                    std::string body = "<!DOCTYPE html>\n"
                        "<html>\n"
                        "<head><title>Webserv</title></head>\n"
                        "<body>\n"
                        "<h1>Bienvenue sur Webserv!</h1>\n"
                        "<p>Methode: " + req->method + "</p>\n"
                        "<p>URI: " + req->uri + "</p>\n"
                        "<p>Version: " + req->version + "</p>\n"
                        "</body>\n"
                        "</html>\n";

                    ResponseBuilder::setBody(*res, body);
                }

                client->setState(CLIENT_WRITING);
            }
        }

        if (_pollFds[i].revents & POLLOUT)
        {
            if (client->getClientState() == CLIENT_WRITING)
            {
                int result = client->writeData();

                if (result < 0)
                {
                    std::cout << "Client " << fd << " erreur d'ecritue" << std::endl;
                    toRemove.push_back(fd);
                    continue;
                }

                //Faut-il tout envoyer ?
                if (client->getResponse()->is_complete)
                {
                    std::cout << "Reponse completement envoye" << std::endl;
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
        close(fd);
        delete _clients[fd];
        _clients.erase(fd);
    }
}


void Server::checkTimeouts()
{
    time_t now = time(NULL);

    std::vector<int> toRemove;

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        Client* client = it->second;

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