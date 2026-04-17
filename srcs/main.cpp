#include "../includes/core/Server.hpp"
#include "../includes/config/ConfigParser.hpp"
#include "../includes/core/Dico.hpp"
#include "Response.hpp"
#include "Request.hpp"
#include "SessionManager.hpp"

Server* g_server = NULL;

//Fonction appele quand on fait Ctrl+C
void signalHandler(int signum)
{
    (void)signum;
    std::cout << "\nSignal recu, arret du serveur" << std::endl;
    if (g_server)
        g_server->stop();
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "Usage : ./webserv config_file" << std::endl;
        return -1;
    }
    //Installer les signaux
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    ConfigParser config;
    try
    {
        std::cout << "Chargement config: " << argv[1] << std::endl;
        config.parse(argv[1]);
        std::cout << "Config chargée: " << config.getServers().size() << " serveur(s)" << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << "ERREUR config: " << e.what() << std::endl;
        return 1;
    }

    std::vector<ServerConfig> servers = config.getServers();

    //Creer le serveur
    Server server;
    g_server = &server;

    //Setup
    server.setup(servers);

    //Run
    server.run();
    return 0;
}

//Pour tester plusieurs clients sur le port 8080
//Ouvrir un autre terminal et lancer la commande
//suivante plusieurs fois
//curl http://localhost:8080 &

/* ===========Creation d'un serveur simple=============

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
