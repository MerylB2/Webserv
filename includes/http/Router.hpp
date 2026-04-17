#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "../core/Dico.hpp"
#include "SessionManager.hpp"

/*
Fonctions de routing pour les requêtes HTTP.
Utilise les structs définies dans Dico.hpp.

Utilisation :
    RouteResult result = Router::route(request, server_config);
    switch (result.type) {
        case ROUTE_FILE:    // Servir le fichier result.filepath
        case ROUTE_UPLOAD:  // Fichier uploade a result.upload_path
        case ROUTE_DELETE:  // Supprimer result.filepath
        case ROUTE_CGI:     // Executer CGI
        case ROUTE_REDIRECT:// Rediriger
        case ROUTE_DIRECTORY:// Listing repertoire
        case ROUTE_ERROR:   // Erreur HTTP
    }
*/

// Type de réponse à générer
enum RouteType {
    ROUTE_FILE,         // Servir un fichier statique (GET)
    ROUTE_UPLOAD,       // Sauvegarder un fichier (POST)
    ROUTE_DELETE,       // Supprimer un fichier (DELETE)
    ROUTE_DIRECTORY,    // Répertoire (autoindex ou index file)
    ROUTE_CGI,          // Exécuter un script CGI
    ROUTE_REDIRECT,     // Redirection HTTP
    ROUTE_ERROR,         // Page d'erreur
    ROUTE_LOGIN,
    ROUTE_LOGOUT,
    ROUTE_PROTECTED_PAGE
};

// Résultat du routing
struct RouteResult {
    RouteType type;
    std::string filepath;        // Chemin du fichier a servir/supprimer
    std::string upload_path;     // Chemin complet pour sauvegarder l'upload
    std::string cgi_interpreter; // Chemin de l'interpréteur CGI (si CGI)
    int error_code;              // Code d'erreur (si erreur)
    std::string redirect_url;    // URL de redirection (si redirect)
    int redirect_code;           // Code de redirection (301/302)
    std::string session_id;      // id de la session

    RouteResult() : type(ROUTE_ERROR), error_code(500), redirect_code(0) {}
};

namespace Router {
    // Route principale : dispatch selon la methode HTTP
    RouteResult route(const Request& request, ServerConfig* server);

    // Routes par methode
    RouteResult routeGET(const Request& request, LocationConfig* loc);
    RouteResult routePOST(const Request& request, LocationConfig* loc);
    RouteResult routeDELETE(const Request& request, LocationConfig* loc);

    // Trouve la LocationConfig correspondant à l'URI
    LocationConfig* matchLocation(const std::string& uri, ServerConfig* server);

    // Vérifie si la méthode est autorisée pour cette location
    bool isMethodAllowed(const std::string& method, const LocationConfig* loc);

    // Résout le chemin complet du fichier sur le disque
    std::string resolvePath(const std::string& uri, const LocationConfig* loc);

    // Récupère l'interpréteur CGI pour une extension
    std::string getCGIInterpreter(const std::string& filepath, const LocationConfig* loc);

    // Vérifie si le chemin est un répertoire
    bool isDirectory(const std::string& path);

    // Vérifie si le fichier existe
    bool fileExists(const std::string& path);

    // Vérifie si c'est un fichier CGI
    bool isCGI(const std::string& path, const LocationConfig* loc);

    // Gère l'autoindex pour un répertoire
    bool handleAutoindex(const std::string& dirpath, const std::string& uri, Response& response);

    // Génère une page d'erreur personnalisée
    void generateErrorPage(int error_code, const ServerConfig* server, Response& response);
}

#endif
