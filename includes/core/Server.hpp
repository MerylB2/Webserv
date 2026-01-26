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

class Client;
class Config;

class Server
{
    private :
        std::vector<int> _serverSockets;
        std::map<int, Client*> _clients;
        std::vector<struct pollfd> _pollFds;
        bool _running;

        void buildPollFds();
        void handleNewConnections();
        void handleClientEvents();
        void checkTimeouts();
    public :
        Server();
        ~Server();

        int createServerSocket(int port);
        void setup(const std::vector<int>& ports);

        void run();
        void stop();
};

#endif