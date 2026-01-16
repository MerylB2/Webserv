#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "../core/Dico.hpp"

/*
Classe Request : Parse les requêtes HTTP reçues des clients.

Responsabilités :
- Parser la request line (GET /path HTTP/1.1)
- Parser les headers
- Parser le body (Content-Length ou chunked)
- Gérer les erreurs de parsing

Utilisation :
    Request req;
    req.parse(raw_data);  // Retourne true si parsing complet
    if (req.getState() == COMPLETE) {
        // Requête prête à traiter
    }
*/

class Request {
private:
    ::Request _data;  // Structure de données (définie dans Dico.hpp)

    // Méthodes de parsing internes
    bool parseRequestLine(const std::string& line);
    bool parseHeader(const std::string& line);
    bool parseBody();
    bool parseChunkedBody();

public:
    Request();
    ~Request();
    Request(const Request& other);
    Request& operator=(const Request& other);

    // Parse les données brutes reçues du client
    // Retourne true si la requête est complète
    bool parse(const std::string& raw_data);

    // Getters
    const std::string& getMethod() const;
    const std::string& getUri() const;
    const std::string& getQueryString() const;
    const std::string& getVersion() const;
    const std::string& getBody() const;
    const std::map<std::string, std::string>& getHeaders() const;
    std::string getHeader(const std::string& name) const;
    RequestState getState() const;
    int getErrorCode() const;
    size_t getContentLength() const;
    bool isChunked() const;

    // Reset pour keep-alive
    void reset();
};

#endif
