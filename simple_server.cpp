#include "includes/core/Server.hpp"

//Variable globale pour arreter proprement la boucle principale
bool g_running = true;

//Fonction appele quand on fait Ctrl+C
void signalHandler(int signum)
{
    (void)signum;
    std::cout << "\n Signal recu, arret du serveur" << std::endl;
    g_running = false;
}

int main()
{
    std::cout << "=== Serveur simple ===" << std::endl;

    signal(SIGINT, signalHandler);
    std::cout << "Creation du socket" << std::endl;
    //creation du socket : AF_INET = IPv4 / SOCK_STREAM = TCP / 0 = auto
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd < 0)
    {
        std::cerr << "ERREUR: Impossible de creer le socket" << std::endl;
        return -1;
    }

    std::cout << "Socket cree (FD = " << serverFd << ")" << std::endl;

    //Permer de relancer immediatement le serveur sans attendre 60 secondes.
    std::cout << "Configuration SO_REUSEADDR..." << std::endl;
    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "ERREUR: setsockopt failed" << std::endl;
        close(serverFd);
        return -1;
    }
    std::cout << "SO_REUSEADDR active" << std::endl;

    std::cout << "Configuration mode non-bloquant..." << std::endl;
    int flags = fcntl(serverFd, F_GETFL, 0);
    fcntl(serverFd, F_SETFL, flags | O_NONBLOCK);
    //Rajouter message d'erreur -1
    std::cout << "✓ Mode non-bloquant active" << std::endl;

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
        return -1;
    }

    std::cout << "Socket attache au port 8080" << std::endl;
    
    std::cout << "Mise en ecoute" << std::endl;

    if (listen(serverFd, 128) < 0)
    {
        std::cerr << "ERREUR: Impossible d'ecouter" << std::endl;
        close(serverFd);
        return -1;
    }

    std::cout << "Serveur en ecoute sur http://localhost:8080" << std::endl;
    std::cout << "En attente d'un client" << std::endl;
    
    struct sockaddr_in clientAddress;
    socklen_t clientLen = sizeof(clientAddress);

    while (g_running)
    {
        int clientFd = accept(serverFd, (struct sockaddr*)&clientAddress, &clientLen);

        if (clientFd < 0)
        {
            // En mode non-bloquant, EAGAIN signifie "pas de client"
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                usleep(100000);  // Attendre 100ms avant de réessayer
                continue;
            }
            
            // Si signal reçu, sortir proprement
            if (!g_running)
                break;
            
            // Autre erreur
            std::cerr << "ERREUR: accept() failed" << std::endl;
            continue;  // Essayer le prochain client
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
            "HTTP/1.1 200 OK\r\n"
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
    }
    
    close(serverFd);

    std::cout << "Serveur termine proprement" << std::endl;
    return 0;
}

/* ===========Creation d'un serveur=============

ETAPE 1 : Creer le socket => int serverFd = socket(...)

ETAPE 2 : Configurer l'adresse (bind) => Ecouter sur le
port 8080. Utilisation de la structure sockaddr_in 

ETAPE 3 : Attacher le socket au port (bind) => Utilisation 
de la fonction bind avec struct sockaddr

ETAPE 4 : Ecouter listen => fonction listen ecoute et
attend des clients.

ETAPE 5 : Accepter un client => accept struct sockaddr_in

ETAPE 6 : Lire les donnees => recv. memset sur un buffer
ressemblance avec read gnl.

ETAPE 7 : Envoi de la reponse => send fonctionnement
similaire a recv.

ETAPE 8 : Nettoyer (fermer les sockets) => close le Fd client et serveur
*/


//=======================================================
//=======================================================
//=======================================================

int Server::createServerSocket(int port)
{
    std::cout << "Creation du socket" << std::endl;
    //creation du socket : AF_INET = IPv4 / SOCK_STREAM = TCP / 0 = auto
   
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
    {
        std::cerr << "ERREUR: Impossible de creer le socket" << std::endl;
        return -1;
    }

    //SO_REUSEADDR => Permet de relancer sur le meme port immediatement
    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "ERREUR: setsockopt() failed" << std::endl;
        close(serverFd);
        return -1;
    }

    //Mode non-bloquant permet de passer d'un 
    std::cout << "Configuration mode non-bloquant..." << std::endl;
    int flags = fcntl(serverFd, F_GETFL, 0);
    fcntl(serverFd, F_SETFL, flags | O_NONBLOCK);
    //Rajouter message d'erreur -1
    std::cout << "✓ Mode non-bloquant active" << std::endl;

    std::cout << "Configuration de l'adresse" << std::endl;

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    // Utilise le parametre port au lieu de 8080 en dur
    address.sin_port = htons(port);

    std::cout << "Adresse configure (0.0.0.0:" << port << ")" << std::endl;

    std::cout << "Attachement au port " << port << std::endl;

    // Bind : attache le socket au port
    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        std::cerr << "ERREUR: Impossible de bind au port " << port << std::endl;
        close(serverFd);
        return -1;
    }

    std::cout << "Socket attache au port " << port << std::endl;

    std::cout << "Mise en ecoute" << std::endl;

    // Listen : met le socket en mode ecoute (128 = backlog max)
    if (listen(serverFd, 128) < 0)
    {
        std::cerr << "ERREUR: Impossible d'ecouter" << std::endl;
        close(serverFd);
        return -1;
    }

    std::cout << "Serveur pret sur le port " << port << std::endl;
    // Retourne le file descriptor du socket serveur
    return serverFd;
}