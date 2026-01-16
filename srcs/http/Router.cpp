#include "../../includes/http/Router.hpp"
#include <sys/stat.h>
#include <algorithm>

/* ========== Constructeur / Destructeur ========== */

Router::Router() {}

Router::~Router() {}

/* ========== Route principale ========== */

/*
Route une requête et détermine le type de réponse à générer.
*/
RouteResult Router::route(const Request& request, ServerConfig* server) {
    RouteResult result;

    // 1. Trouver la location correspondante
    LocationConfig* loc = matchLocation(request.getUri(), server);
    if (!loc) {
        result.type = ROUTE_ERROR;
        result.error_code = HttpStatus::NOT_FOUND;
        return result;
    }

    // 2. Vérifier si c'est une redirection
    if (!loc->redirect_url.empty()) {
        result.type = ROUTE_REDIRECT;
        result.redirect_url = loc->redirect_url;
        result.redirect_code = loc->redirect_code > 0 ? loc->redirect_code : 302;
        return result;
    }

    // 3. Vérifier que la méthode est autorisée
    if (!isMethodAllowed(request.getMethod(), loc)) {
        result.type = ROUTE_ERROR;
        result.error_code = HttpStatus::METHOD_NOT_ALLOWED;
        return result;
    }

    // 4. Résoudre le chemin du fichier
    std::string filepath = resolvePath(request.getUri(), loc);
    result.filepath = filepath;

    // 5. Vérifier si c'est un CGI
    if (isCGI(filepath, loc)) {
        result.type = ROUTE_CGI;
        result.cgi_interpreter = getCGIInterpreter(filepath, loc);
        return result;
    }

    // 6. Vérifier si le fichier/répertoire existe
    if (isDirectory(filepath)) {
        // Chercher le fichier index
        std::string index_path = filepath;
        if (index_path[index_path.size() - 1] != '/')
            index_path += "/";
        index_path += loc->index;

        if (fileExists(index_path)) {
            result.type = ROUTE_FILE;
            result.filepath = index_path;
        } else if (loc->autoindex) {
            result.type = ROUTE_DIRECTORY;
        } else {
            result.type = ROUTE_ERROR;
            result.error_code = HttpStatus::FORBIDDEN;
        }
    } else if (fileExists(filepath)) {
        result.type = ROUTE_FILE;
    } else {
        result.type = ROUTE_ERROR;
        result.error_code = HttpStatus::NOT_FOUND;
    }

    return result;
}

/* ========== Matching de location ========== */

/*
Trouve la LocationConfig qui matche le mieux l'URI.
Utilise le matching par préfixe le plus long.

Exemple :
    URI = "/images/photo.jpg"
    Locations : "/", "/images", "/images/thumbnails"
    Match : "/images" (préfixe le plus long qui matche)
*/
LocationConfig* Router::matchLocation(const std::string& uri, ServerConfig* server) {
    if (!server)
        return NULL;

    return findBestMatch(uri, server);
}

LocationConfig* Router::findBestMatch(const std::string& uri, ServerConfig* server) {
    LocationConfig* best_match = NULL;
    size_t best_length = 0;

    for (size_t i = 0; i < server->locations.size(); ++i) {
        LocationConfig& loc = server->locations[i];
        const std::string& path = loc.path_url;

        // Vérifier si l'URI commence par le path de la location
        if (uri.compare(0, path.size(), path) == 0) {
            // Vérifier que c'est une correspondance complète
            // (soit fin de l'URI, soit suivi de '/')
            if (uri.size() == path.size() ||
                path == "/" ||
                uri[path.size()] == '/') {
                // Garder le match le plus long
                if (path.size() > best_length) {
                    best_length = path.size();
                    best_match = &loc;
                }
            }
        }
    }

    return best_match;
}

/* ========== Vérification de méthode ========== */

/*
Vérifie si la méthode HTTP est autorisée pour cette location.
*/
bool Router::isMethodAllowed(const std::string& method, const LocationConfig* loc) const {
    if (!loc)
        return false;

    for (size_t i = 0; i < loc->methods.size(); ++i) {
        if (loc->methods[i] == method)
            return true;
    }

    return false;
}

/* ========== Résolution de chemin ========== */

/*
Résout le chemin complet du fichier sur le disque.

Exemple :
    URI = "/images/photo.jpg"
    Location path = "/images"
    Location root = "./www/img"
    Résultat = "./www/img/photo.jpg"
*/
std::string Router::resolvePath(const std::string& uri, const LocationConfig* loc) const {
    if (!loc)
        return "";

    std::string root = loc->root_dir;
    if (root.empty())
        root = "./www";

    // Enlever le slash final du root si présent
    if (!root.empty() && root[root.size() - 1] == '/')
        root = root.substr(0, root.size() - 1);

    // Calculer le chemin relatif (URI sans le préfixe de la location)
    std::string relative_path;
    if (loc->path_url == "/") {
        relative_path = uri;
    } else if (uri.size() > loc->path_url.size()) {
        relative_path = uri.substr(loc->path_url.size());
    } else {
        relative_path = "/";
    }

    // S'assurer que le chemin relatif commence par /
    if (relative_path.empty() || relative_path[0] != '/')
        relative_path = "/" + relative_path;

    return root + relative_path;
}

/* ========== Vérifications de fichier ========== */

bool Router::isDirectory(const std::string& path) const {
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}

bool Router::fileExists(const std::string& path) const {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
}

/* ========== CGI ========== */

/*
Vérifie si le fichier est un script CGI basé sur son extension.
*/
bool Router::isCGI(const std::string& path, const LocationConfig* loc) const {
    if (!loc || loc->cgi_handlers.empty())
        return false;

    std::string ext = MimeTypes::getExtension(path);
    return (loc->cgi_handlers.find(ext) != loc->cgi_handlers.end());
}

/*
Retourne le chemin de l'interpréteur CGI pour cette extension.
*/
std::string Router::getCGIInterpreter(const std::string& filepath, const LocationConfig* loc) const {
    if (!loc)
        return "";

    std::string ext = MimeTypes::getExtension(filepath);
    std::map<std::string, std::string>::const_iterator it = loc->cgi_handlers.find(ext);

    if (it != loc->cgi_handlers.end())
        return it->second;

    return "";
}
