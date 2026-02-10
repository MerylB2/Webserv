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

    // ========== TEST COOKIES (a enlever apres) ==========
    std::cout << "\n=== TEST COOKIES ===\n";
    
    Response res;
    ResponseBuilder::setStatus(res, 200);
    ResponseBuilder::setBody(res, "<h1>Cookie Test</h1>");
    ResponseBuilder::setCookie(res, "session_id", "abc123xyz", 3600);
    ResponseBuilder::build(res);
    
    std::cout << "--- Cookie créé ---\n" << res.send_buffer << std::endl;
    
    Response res2;
    ResponseBuilder::setStatus(res2, 200);
    ResponseBuilder::setBody(res2, "<h1>Logout</h1>");
    ResponseBuilder::setCookie(res2, "session_id", "", -1);
    ResponseBuilder::build(res2);
    
    std::cout << "--- Cookie supprimé ---\n" << res2.send_buffer << std::endl;

    // ========== TEST SESSION MANAGER ==========
    std::cout << "\n=== TEST SESSION MANAGER ===\n";

    // 1. Obtenir l'instance
    SessionManager* sm = SessionManager::getInstance();

    // 2. Créer des sessions
    std::string alice_session = sm->createSession("Alice");
    std::string bob_session = sm->createSession("Bob");
    std::string charlie_session = sm->createSession("Charlie");

    std::cout << "\n--- Sessions créées ---" << std::endl;
    std::cout << "Alice   : " << alice_session << std::endl;
    std::cout << "Bob     : " << bob_session << std::endl;
    std::cout << "Charlie : " << charlie_session << std::endl;

    // 3. Récupérer une session
    SessionData* alice_data = sm->getSession(alice_session);
    if (alice_data) {
        std::cout << "\n--- Données d'Alice ---" << std::endl;
        std::cout << "User: " << alice_data->user << std::endl;
        std::cout << "Created: " << alice_data->created_at << std::endl;
        std::cout << "Last activity: " << alice_data->last_activity << std::endl;
    }

    // 4. Valider une session
    std::cout << "\n--- Validation ---" << std::endl;
    std::cout << "Alice valide ? " << (sm->validateSession(alice_session) ? "✓ OUI" : "✗ NON") << std::endl;
    std::cout << "Bob valide ? " << (sm->validateSession(bob_session) ? "✓ OUI" : "✗ NON") << std::endl;
    std::cout << "ID bidon valide ? " << (sm->validateSession("sess_fakeid123") ? "✓ OUI" : "✗ NON") << std::endl;

    // 5. Supprimer une session
    sm->deleteSession(bob_session);
    std::cout << "\n--- Après suppression de Bob ---" << std::endl;
    std::cout << "Bob valide ? " << (sm->validateSession(bob_session) ? "✓ OUI" : "✗ NON") << std::endl;

    // 6. Test de nettoyage (optionnel, mais utile)
    std::cout << "\n--- Test de nettoyage (attendre 2 secondes...) ---" << std::endl;
    sleep(2);  // Attendre 2 secondes
    sm->cleanupExpiredSessions(1);  // Nettoyer les sessions > 1 seconde

    std::cout << "Alice valide après nettoyage ? " << (sm->validateSession(alice_session) ? "✓ OUI" : "✗ NON") << std::endl;
    std::cout << "Charlie valide après nettoyage ? " << (sm->validateSession(charlie_session) ? "✓ OUI" : "✗ NON") << std::endl;

    std::cout << "\n=== FIN TEST SESSION MANAGER ===\n" << std::endl;

    // ========== FIN TEST a enlever ==========

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
