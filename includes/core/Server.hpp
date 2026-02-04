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
#include "Dico.hpp"

class Client;
class Config;

class Server
{
    private :
        std::vector<ServerConfig> _config;
        std::vector<int> _serverSockets; //tableau FD sockets
        std::map<int, Client*> _clients; // FD rattache a un client
        std::vector<struct pollfd> _pollFds; // cf struct pollfd
        bool _running; //bool pour arreter proprement

        void buildPollFds();
        void handleNewConnections();
        void handleClientEvents();
        void checkTimeouts();
    public :
        Server();
        ~Server();

        int createServerSocket(int port);
        void setup(const std::vector<ServerConfig>& config);

        void run();
        void stop();
};

#endif