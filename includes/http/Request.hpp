#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "../core/Dico.hpp"

/*
Fonctions de parsing pour les requêtes HTTP.
Utilise la struct Request définie dans Dico.hpp.

Utilisation :
    Request req;
    bool complete = RequestParser::parse(req, raw_data);
    if (req.state == COMPLETE) {
        std::cout << req.method << " " << req.uri << std::endl;
    }
*/

namespace RequestParser {
    // Parse les données brutes et remplit la struct Request
    // Retourne true si la requête est complète
    bool parse(Request& req, const std::string& raw_data);

    // Parse la request line : "GET /path?query HTTP/1.1"
    bool parseRequestLine(Request& req, const std::string& line);

    // Parse un header : "Content-Type: text/html"
    bool parseHeader(Request& req, const std::string& line);

    // Parse le body avec Content-Length
    bool parseBody(Request& req);

    // Parse le body en mode chunked
    bool parseChunkedBody(Request& req);

    void parseCookies(Request& req);

    std::string getCookie(const Request& req, const std::string& name);

    std::string getSessionId(const Request& req);
    // autres utilitaires de parsing...

}


#endif
