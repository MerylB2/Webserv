
#include "../includes/core/Server.hpp"
#include "../includes/core/Dico.hpp"
#include "../includes/http/Request.hpp"
#include "../includes/http/Response.hpp"

// Variable globale pour arreter proprement la boucle principale
bool g_running = true;

// Fonction appelee quand on fait Ctrl+C
void signalHandler(int signum)
{
    (void)signum;
    std::cout << "\nSignal recu, arret du serveur..." << std::endl;
    g_running = false;
}

int main()
{
    std::cout << "=== WEBSERV ===" << std::endl;

    signal(SIGINT, signalHandler);

    // ETAPE 1 : Creation du socket
    std::cout << "Creation du socket..." << std::endl;
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
    {
        std::cerr << "ERREUR: Impossible de creer le socket" << std::endl;
        return 1;
    }
    std::cout << "Socket cree (FD = " << serverFd << ")" << std::endl;

    // ETAPE 2 : Configuration SO_REUSEADDR (relance rapide)
    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "ERREUR: setsockopt() failed" << std::endl;
        close(serverFd);
        return 1;
    }

    // ETAPE 3 : Mode non-bloquant
    int flags = fcntl(serverFd, F_GETFL, 0);
    if (fcntl(serverFd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        std::cerr << "ERREUR: fcntl() failed" << std::endl;
        close(serverFd);
        return 1;
    }

    // ETAPE 4 : Configuration de l'adresse
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // ETAPE 5 : Bind (attache au port)
    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        std::cerr << "ERREUR: Impossible de bind au port 8080" << std::endl;
        close(serverFd);
        return 1;
    }

    // ETAPE 6 : Listen (mise en ecoute)
    if (listen(serverFd, 128) < 0)
    {
        std::cerr << "ERREUR: Impossible d'ecouter" << std::endl;
        close(serverFd);
        return 1;
    }

    std::cout << "Serveur en ecoute sur http://localhost:8080" << std::endl;
    std::cout << "Ctrl+C pour arreter" << std::endl;
    std::cout << "========================================" << std::endl;

    struct sockaddr_in clientAddress;
    socklen_t clientLen = sizeof(clientAddress);

    // BOUCLE PRINCIPALE
    while (g_running)
    {
        // ETAPE 7 : Accept (accepter un client)
        int clientFd = accept(serverFd, (struct sockaddr*)&clientAddress, &clientLen);

        if (clientFd < 0)
        {
            // Mode non-bloquant : EAGAIN = pas de client en attente
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                usleep(10000);  // Attendre 10ms
                continue;
            }
            if (!g_running)
                break;
            std::cerr << "ERREUR: accept() failed" << std::endl;
            continue;
        }

        std::cout << "\n--- Nouveau client (FD = " << clientFd << ") ---" << std::endl;

        // ETAPE 8 : Lecture de la requete
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

        if (bytesRead <= 0)
        {
            if (bytesRead < 0)
                std::cerr << "ERREUR: recv() failed" << std::endl;
            else
                std::cout << "Client a ferme la connexion" << std::endl;
            close(clientFd);
            continue;
        }

        std::cout << "Recu " << bytesRead << " bytes" << std::endl;

        // ETAPE 9 : Parser la requete avec RequestParser
        Request req;
        std::string rawData(buffer);
        bool parseOk = RequestParser::parse(req, rawData);

        std::cout << "Methode: " << req.method << std::endl;
        std::cout << "URI: " << req.uri << std::endl;
        std::cout << "Parse OK: " << (parseOk ? "oui" : "non") << std::endl;

        // ETAPE 10 : Construire la reponse avec ResponseBuilder
        Response res;

        if (!parseOk || req.state == ERROR)
        {
            // Requete invalide -> erreur 400
            res = ResponseBuilder::makeError(400);
        }
        else if (req.method != "GET" && req.method != "POST" && req.method != "DELETE")
        {
            // Methode non supportee -> erreur 405
            res = ResponseBuilder::makeError(405);
        }
        else
        {
            // Requete valide -> reponse 200
            ResponseBuilder::setStatus(res, 200);
            ResponseBuilder::setHeader(res, "Content-Type", "text/html");

            std::string body = "<!DOCTYPE html>\n"
                "<html>\n"
                "<head><title>Webserv</title></head>\n"
                "<body>\n"
                "<h1>Bienvenue sur Webserv!</h1>\n"
                "<p>Methode: " + req.method + "</p>\n"
                "<p>URI: " + req.uri + "</p>\n"
                "<p>Version: " + req.version + "</p>\n"
                "</body>\n"
                "</html>\n";

            ResponseBuilder::setBody(res, body);
            ResponseBuilder::build(res);
        }

        // ETAPE 11 : Envoyer la reponse
        std::cout << "Envoi reponse: " << res.status_code << " " << res.status_message << std::endl;

        int bytesSent = send(clientFd, res.send_buffer.c_str(), res.send_buffer.size(), 0);

        if (bytesSent < 0)
            std::cerr << "ERREUR: send() failed" << std::endl;
        else
            std::cout << "Envoye " << bytesSent << " bytes" << std::endl;

        // ETAPE 12 : Fermer la connexion client
        close(clientFd);
    }

    // ETAPE 13 : Fermer le socket serveur
    close(serverFd);
    std::cout << "\nServeur arrete proprement" << std::endl;

    return 0;
}
