#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Dico.hpp"

class Client
{
    private :
        ClientData _data;
        int _serverPort;
    public :
        //Constructeur
        Client(int fd, int _serverPort);
        ~Client();

        //Getters
        int getFd() const;
        ClientState getClientState() const;
        Request* getRequest();
        Response* getResponse();
        

        //Setters

        //I/O

        //Lifecycle
};

#endif