#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Dico.hpp"

// Voici ce que la structure ClientData contient 

// struct ClientData {

//     int socket_fd; // File descriptor de la socket (numéro retourné par accept)

//     ClientState state; // État actuel de la connexion

//     Request request; // La requête reçue de ce client

//     Response response; // La réponse à envoyer à ce client

//     ServerConfig* server_config; // Pointeur vers la config du serveur sur lequel le client s'est connecté

//     LocationConfig* location_config; // Pointeur vers la location qui correspond à l'URL demandée

//     CGIData cgi; // Données du CGI (pid, pipes, buffer, timeout)

//     time_t last_activity;  // Timestamp de la dernière activité (pour timeout)

//     ClientData():
//         socket_fd(-1),
//         state(CLIENT_READING),
//         server_config(NULL),
//         location_config(NULL),
//         last_activity(0)
//     {}

class Client
{
    private :
        ClientData _data;
        int _serverPort;
    public :
        //Constructeur
        Client(int fd, int serverPort);
        ~Client();

        //Getters
        int getFd() const;
        ClientState getClientState() const;
        Request* getRequest();
        Response* getResponse();
        int getServerPort() const;
        time_t getLastActivity() const;
        CGIData* getCGIData();
        

        //Setters
        void setState(ClientState state);
        void updateActivity();

        //I/O
        int readData();
        int writeData();

        //Lifecycle
        void reset();
        bool shouldKeepAlive() const;
};

#endif