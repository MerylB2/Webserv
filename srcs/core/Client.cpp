#include "Client.hpp"
#include "Server.hpp"
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

Client::Client(int fd, int serverPort) : _serverPort(serverPort)
{
    _data.socket_fd = fd;
    _data.state = CLIENT_READING;
    _data.last_activity = time(NULL);
    _data.server_config = NULL;
    _data.location_config = NULL;

    std::cout << "Client cree (FD = " << fd << ", port = " << serverPort << ")" << std::endl;
}

Client::~Client()
{}

// GETTERS

int Client::getFd() const
{
    return _data.socket_fd;
}

ClientState Client::getClientState() const
{
    return _data.state;
}

Request* Client::getRequest()
{
    return &_data.request;
}

Response* Client::getResponse()
{
    return &_data.response;
}

// SETTERS

void Client::setState(ClientState state)
{
    _data.state = state;
}

void Client::updateActivity()
{
    _data.last_activity = time(NULL);
}