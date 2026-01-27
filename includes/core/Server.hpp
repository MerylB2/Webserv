#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <sys/socket.h> //socket, bind, listen, accept
#include <netinet/in.h> //sockaddr_in
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <cerrno>
#include <fcntl.h>
#include <errno.h>
#include <vector>
#include <map>
#include <poll.h>

#include "Dico.hpp" // pour ServerConfig, ClientData

class Client;
//class Config;

class Server
{
    private :
        std::vector<int> _serverSockets; //tableau FD sockets
        std::map<int, Client*> _clients; // FD rattache a un client
        std::vector<struct pollfd> _pollFds; // cf struct pollfd
        bool _running; //bool pour arreter proprement

        // Ajout de nouveaux attributs pour la config (lire le fichier de config, implémenter redirections 301/302, CGI etc)
        std::vector<ServerConfig> _configs; // Stockage des configurations parsées depuis le fichier .conf
        std::map<int, ServerConfig*> _socketToConfig; // Association socket serveur -> sa configuration : permet de savoir quelle config utiliser quand un client se connecte
        std::map<int, ClientData> _clientsData; // Association socket client -> configuration du serveur : quand un client se connecte sur le port 8080, on sait quelle config utiliser

        void buildPollFds();
        void handleNewConnections();
        void handleClientEvents();
        void checkTimeouts();
    public :
        Server();
        ~Server();

        int createServerSocket(int port);
        void setup(const std::vector<int>& ports); // version1 ne connait que les ports pas la config complète
        void setup(const std::vector<ServerConfig>& configs); // nouvelle version pour accéder à toute la configuration pour redirections, CGI etc 

        void run();
        void stop();
};

#endif