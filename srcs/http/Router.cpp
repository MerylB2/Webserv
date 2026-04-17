#include "../../includes/http/Router.hpp"
#include "../../includes/http/Response.hpp"
#include "Request.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <ctime>

namespace Router {

/* ========== Route principale ========== */

RouteResult route(const Request& request, ServerConfig* server)
{
    RouteResult result;

    // ===== Routes speciales pour les sessions =====

    // 1. Route LOGIN (POST /login)
    if (request.uri == "/login" && request.method == "POST")
    {
        result.type = ROUTE_LOGIN;
        return result;
    }

    // 2. Route LOGOUT (GET/POST /logout)
    if (request.uri == "/logout")
    {
        result.type = ROUTE_LOGOUT;
        return result;
    }

    // 3. Route protegees (necessitent authentification)
    if (request.uri == "/dashboard" || request.uri == "/profile")
    {
        //verifier l'authentification
        SessionManager* sm = SessionManager::getInstance();
        std::string session_id = RequestParser::getSessionId(request);

        if (session_id.empty() || !sm->validateSession(session_id))
        {
            result.type = ROUTE_ERROR;
            result.error_code = HttpStatus::UNAUTHORIZED;
            return result;
        }

        //Authentifie -> continuer normalement
        result.type = ROUTE_PROTECTED_PAGE;
        result.session_id = session_id;
        return result;
    }
    // 1. Trouver la location correspondante
    LocationConfig* loc = matchLocation(request.uri, server);
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
    if (!isMethodAllowed(request.method, loc)) {
        result.type = ROUTE_ERROR;
        result.error_code = HttpStatus::METHOD_NOT_ALLOWED;
        return result;
    }

    // 4. Dispatcher selon la methode HTTP
    if (request.method == "GET")
        return routeGET(request, loc);
    else if (request.method == "POST")
        return routePOST(request, loc);
    else if (request.method == "DELETE")
        return routeDELETE(request, loc);

    // Methode non geree
    result.type = ROUTE_ERROR;
    result.error_code = HttpStatus::METHOD_NOT_ALLOWED;
    return result;
}

/* ========== GET : servir fichier / repertoire / CGI ========== */

RouteResult routeGET(const Request& request, LocationConfig* loc) {
    RouteResult result;
    std::string filepath = resolvePath(request.uri, loc);
    result.filepath = filepath;

    // CGI ?
    if (isCGI(filepath, loc)) {
        result.type = ROUTE_CGI;
        result.cgi_interpreter = getCGIInterpreter(filepath, loc);
        return result;
    }

    // Repertoire ?
    if (isDirectory(filepath)) {
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
        return result;
    }

    // Fichier ?
    if (fileExists(filepath)) {
        result.type = ROUTE_FILE;
        return result;
    }

    result.type = ROUTE_ERROR;
    result.error_code = HttpStatus::NOT_FOUND;
    return result;
}

/* ========== POST : upload ou CGI ========== */

RouteResult routePOST(const Request& request, LocationConfig* loc) {
    RouteResult result;
    std::string filepath = resolvePath(request.uri, loc);
    result.filepath = filepath;

    // CGI en priorite (ex: script.py qui recoit un POST)
    if (isCGI(filepath, loc)) {
        result.type = ROUTE_CGI;
        result.cgi_interpreter = getCGIInterpreter(filepath, loc);
        return result;
    }

    // Upload si la location a un upload_dir
    if (!loc->upload_dir.empty()) {
        // Extraire le nom du fichier depuis l'URI
        std::string filename;
        size_t lastSlash = request.uri.rfind('/');
        if (lastSlash != std::string::npos && lastSlash + 1 < request.uri.size())
            filename = request.uri.substr(lastSlash + 1);
        else
            filename = "upload";

        std::string uploadPath = loc->upload_dir;
        if (!uploadPath.empty() && uploadPath[uploadPath.size() - 1] != '/')
            uploadPath += "/";
        uploadPath += filename;

        result.type = ROUTE_UPLOAD;
        result.upload_path = uploadPath;
        return result;
    }

    // Pas de CGI ni d'upload_dir -> erreur
    result.type = ROUTE_ERROR;
    result.error_code = HttpStatus::FORBIDDEN;
    return result;
}

/* ========== DELETE : supprimer un fichier ========== */

RouteResult routeDELETE(const Request& request, LocationConfig* loc) {
    RouteResult result;
    std::string filepath = resolvePath(request.uri, loc);
    result.filepath = filepath;

    if (fileExists(filepath)) {
        result.type = ROUTE_DELETE;
        return result;
    }

    result.type = ROUTE_ERROR;
    result.error_code = HttpStatus::NOT_FOUND;
    return result;
}

/* ========== Matching de location ========== */

LocationConfig* matchLocation(const std::string& uri, ServerConfig* server) {
    if (!server)
        return NULL;

    LocationConfig* best_match = NULL;
    size_t best_length = 0;

    for (size_t i = 0; i < server->locations.size(); ++i) {
        LocationConfig& loc = server->locations[i];
        const std::string& path = loc.path_url;

        if (uri.compare(0, path.size(), path) == 0) {
            if (uri.size() == path.size() ||
                path == "/" ||
                uri[path.size()] == '/') {
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

bool isMethodAllowed(const std::string& method, const LocationConfig* loc) {
    if (!loc)
        return false;

    for (size_t i = 0; i < loc->methods.size(); ++i) {
        if (loc->methods[i] == method)
            return true;
    }

    return false;
}

/* ========== Résolution de chemin ========== */

std::string resolvePath(const std::string& uri, const LocationConfig* loc) {
    if (!loc)
        return "";

    std::string root = loc->root_dir;
    if (root.empty())
        root = "./www";

    if (!root.empty() && root[root.size() - 1] == '/')
        root = root.substr(0, root.size() - 1);

    std::string relative_path;
    if (loc->path_url == "/") {
        relative_path = uri;
    } else if (uri.size() > loc->path_url.size()) {
        relative_path = uri.substr(loc->path_url.size());
    } else {
        relative_path = "/";
    }

    if (relative_path.empty() || relative_path[0] != '/')
        relative_path = "/" + relative_path;

    return root + relative_path;
}

/* ========== Vérifications de fichier ========== */

bool isDirectory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}

bool fileExists(const std::string& path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
}

/* ========== CGI ========== */

bool isCGI(const std::string& path, const LocationConfig* loc) {
    if (!loc || loc->cgi_handlers.empty())
        return false;

    std::string ext = MimeTypes::getExtension(path);
    return (loc->cgi_handlers.find(ext) != loc->cgi_handlers.end());
}

std::string getCGIInterpreter(const std::string& filepath, const LocationConfig* loc) {
    if (!loc)
        return "";

    std::string ext = MimeTypes::getExtension(filepath);
    std::map<std::string, std::string>::const_iterator it = loc->cgi_handlers.find(ext);

    if (it != loc->cgi_handlers.end())
        return it->second;

    return "";
}

} // namespace Router
