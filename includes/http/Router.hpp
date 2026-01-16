#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "../core/Dico.hpp"
#include "Request.hpp"
#include "Response.hpp"

/*
Classe Router : Route les requêtes vers le bon handler.

Responsabilités :
- Matcher l'URI avec la LocationConfig la plus spécifique
- Vérifier que la méthode est autorisée
- Résoudre le chemin du fichier sur le disque
- Déterminer le type de réponse (fichier, CGI, redirect, autoindex, erreur)

Utilisation :
    Router router;
    LocationConfig* loc = router.matchLocation(request, server_config);
    if (router.isMethodAllowed(request, loc)) {
        std::string filepath = router.resolvePath(request, loc);
        // ...
    }
*/

// Type de réponse à générer
enum RouteType {
    ROUTE_FILE,         // Servir un fichier statique
    ROUTE_DIRECTORY,    // Répertoire (autoindex ou index file)
    ROUTE_CGI,          // Exécuter un script CGI
    ROUTE_REDIRECT,     // Redirection HTTP
    ROUTE_ERROR         // Page d'erreur
};

// Résultat du routing
struct RouteResult {
    RouteType type;
    std::string filepath;       // Chemin du fichier à servir
    std::string cgi_interpreter; // Chemin de l'interpréteur CGI (si CGI)
    int error_code;             // Code d'erreur (si erreur)
    std::string redirect_url;   // URL de redirection (si redirect)
    int redirect_code;          // Code de redirection (301/302)

    RouteResult() : type(ROUTE_ERROR), error_code(500), redirect_code(0) {}
};

class Router {
private:
    // Trouve la location la plus spécifique qui matche l'URI
    LocationConfig* findBestMatch(const std::string& uri, ServerConfig* server);

    // Vérifie si le chemin est un répertoire
    bool isDirectory(const std::string& path) const;

    // Vérifie si le fichier existe
    bool fileExists(const std::string& path) const;

    // Vérifie si c'est un fichier CGI
    bool isCGI(const std::string& path, const LocationConfig* loc) const;

public:
    Router();
    ~Router();

    // Route une requête et retourne le résultat
    RouteResult route(const Request& request, ServerConfig* server);

    // Trouve la LocationConfig correspondant à l'URI
    LocationConfig* matchLocation(const std::string& uri, ServerConfig* server);

    // Vérifie si la méthode est autorisée pour cette location
    bool isMethodAllowed(const std::string& method, const LocationConfig* loc) const;

    // Résout le chemin complet du fichier sur le disque
    std::string resolvePath(const std::string& uri, const LocationConfig* loc) const;

    // Récupère l'interpréteur CGI pour une extension
    std::string getCGIInterpreter(const std::string& filepath, const LocationConfig* loc) const;
};

#endif
