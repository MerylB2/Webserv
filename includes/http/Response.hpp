#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "../core/Dico.hpp"

/*
Fonctions pour construire les réponses HTTP.
Utilise la struct Response définie dans Dico.hpp.

Utilisation :
    Response res;
    ResponseBuilder::setStatus(res, 200);
    ResponseBuilder::setHeader(res, "Content-Type", "text/html");
    ResponseBuilder::setBody(res, "<html>...</html>");
    ResponseBuilder::build(res);
    // res.send_buffer est prêt à envoyer
*/

namespace ResponseBuilder {
    // Setters
    void setCookie(Response& res, 
                   const std::string& name, 
                   const std::string& value,
                   int max_age = 0,
                   const std::string& path = "/",
                   bool http_only = true,
                   bool secure = false);
    void setStatus(Response& res, int code);
    void setStatus(Response& res, int code, const std::string& message);
    void setHeader(Response& res, const std::string& name, const std::string& value);
    void setBody(Response& res, const std::string& body);
    void setBodyFromFile(Response& res, const std::string& filepath);

    // Construit le send_buffer complet
    void build(Response& res);

    // Génère la status line
    std::string buildStatusLine(const Response& res);

    // Génère les headers
    std::string buildHeaders(const Response& res);

    // Pour l'envoi partiel
    void addBytesSent(Response& res, size_t bytes);

    // Réponses pré-construites
    Response makeError(int code);
    Response makeRedirect(int code, const std::string& location);

    // autres utilitairesd de reponses...
}

#endif
