#include "../includes/core/Server.hpp"
#include "../includes/config/ConfigParser.hpp" 

Server* g_server = NULL;

//Fonction appele quand on fait Ctrl+C
void signalHandler(int signum)
{
    (void)signum;
    std::cout << "\n Signal reçu, arrêt du serveur" << std::endl;
    if (g_server)
        g_server->stop();
}

int main(int argc, char **argv)
{
    //Installer les signaux
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Parser la configuration depuis le fichier .conf
    // Usage: ./webserv [config_file]
    ConfigParser parser;
    try
    {
        if (argc > 1)
        {
            std::cout << "Chargement config: " << argv[1] << std::endl;
            parser.parse(argv[1]);
        }
        else
        {
            std::cout << "Usage: ./webserv [config_file]" << std::endl;
            std::cout << "Utilisation config par défaut: config/default.conf" << std::endl;
            parser.parse("config/default.conf");
        }
        std::cout << "Config chargée: " << parser.getServers().size() << " serveur(s)" << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << "ERREUR config: " << e.what() << std::endl;
        return 1;
    }

    //Creer le serveur
    Server server;
    g_server = &server;

    //Configurer les ports (plusieurs ports possibles)
    //std::vector<int> ports;
    //ports.push_back(8080);
    //ports.push_back(8081);
    //ports.push_back(8082);

    //Setup version 1
    //server.setup(ports);

    // Nouvelle version qui permet redirections, CGI, etc.
    server.setup(parser.getServers());

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


