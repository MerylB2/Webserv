#include "Client.hpp"
#include "Server.hpp"
#include "Dico.hpp"
#include "Request.hpp"
#include "Response.hpp"

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

int Client::readData()
{
    //client a quelque chose a lire
    //1. Creation d'un buffer temporaire
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));

    //2. Lire depuis socket
    int bytesRead = recv(_data.socket_fd, buffer, sizeof(buffer), 0);
    
    //3. Gestion des erreurs
    if (bytesRead < 0)
    {
        //cas ou il n'y a pas de donnees
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        //vraie erreur
        std::cout << "ERROR: recv() failed pour fd " << _data.socket_fd << std::endl;
        return -1;
    }
    //Fermeture de la connexion par le client
    if (bytesRead == 0)
    {
        std::cout << "Client fd " << _data.socket_fd << " a ferme la connexion" << std::endl;
        return -1;
    }

    //4. Accumuler dans le buffer du client pour ne rien
    //perdre entre les differentes lecture

    std::string rawData(buffer, bytesRead);
    _data.request.read_buffer += rawData;

    std::cout << bytesRead << "bytes lu du client FD = " << _data.socket_fd << std::endl;

    //5. Parser la requete (avec _data.request)
    RequestParser::parse(_data.request, _data.request.read_buffer);

    //6. Verification erreur de parsing
    if (_data.request.error_code != 0)
    {
        std::cerr << "ERREUR: parsing HTTP failed (code " << _data.request.error_code << ")" << std::endl;
        setState(CLIENT_ERROR);
        return -1; 
    }
    //7. Time mis a jour
    updateActivity();

    //8. Retourner le nombre de bytes lus
    return bytesRead;
}