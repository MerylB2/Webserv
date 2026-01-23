#include <iostream>
#include <sys/socket.h> //socket, bind, listen, accept
#include <netinet/in.h> //sockaddr_in
#include <unistd.h>
#include <cstring>

int main()
{
    std::cout << "=== Serveur simple ===" << std::endl;

    std::cout << "Creation du socket" << std::endl;
    //creation du socket : AF_INET = IPv4 / SOCK_STREAM = TCP / 0 = auto
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd < 0)
    {
        std::cerr << "ERREUR: Impossible de creer le socket" << std::endl;
        return 1;
    }

    std::cout << "Socket cree (FD = " << serverFd << ")" << std::endl;

    std::cout << "Configuration de l'adresse" << std::endl;

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    std::cout << "Adresse configure (0.0.0.0:8080)" << std::endl;
    
    std::cout << "Attachement au port 8080" << std::endl;

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        std::cerr << "ERREUR: Impossible de bind au port 8080"  << std::endl;
        close(serverFd);
        return 1;
    }

    std::cout << "Socket attache au port 8080" << std::endl;
    
    std::cout << "Mise en ecoute" << std::endl;

    if (listen(serverFd, 5) < 0)
    {
        std::cerr << "ERREUR: Impossible d'ecouter" << std::endl;
        close(serverFd);
        return 1;
    }

    std::cout << "Serveur en ecoute sur http://localhost:8080" << std::endl;
    std::cout << "En attente d'un client" << std::endl;
    
    struct sockaddr_in clientAddress;
    socklen_t clientLen = sizeof(clientAddress);

    int clientFd = accept(serverFd, (struct sockaddr*)&clientAddress, &clientLen);

    if (clientFd < 0)
    {
        std::cerr << "ERREUR: Impossible d'accepter le client" << std::endl;
        close(serverFd);
        return 1;
    }

    std::cout << "Client connecte ! (FD = " << clientFd << ")" << std::endl;

    std::cout << "Lecture des donnees" << std::endl;

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    int bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

    if(bytesRead < 0)
        std::cerr << "ERREUR: Lecture impossible" << std::endl;
    else if (bytesRead == 0)
        std::cout << "Client a ferme la connexion" << std::endl;
    else
    {
        std::cout << "Recu " << bytesRead << " bytes" << std::endl;
        std::cout << "--- Contenu ---" << std::endl;
        std::cout << buffer << std::endl;
        std::cout << "--------------------" << std::endl;
    }

    std::cout << "Envoi de la reponse" << std::endl;

    const char* response =
        "HTTP/101 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 6\r\n"
        "\r\n"
        "Hello!";

    int byteSent = send(clientFd, response, strlen(response), 0);

    if (byteSent < 0)
        std::cerr << "ERREUR: Envoi impossible" << std::endl;
    else
        std::cout << "Envoye " << byteSent << "bytes" << std::endl;
    
    std::cout << "Fermeture des connexions" << std::endl;

    close(clientFd);
    close(serverFd);

    std::cout << "Serveur termine proprement" << std::endl;
    return 0;
}