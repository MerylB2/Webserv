#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "../core/Dico.hpp"

/*
Classe Response : Construit les réponses HTTP à envoyer aux clients.

Responsabilités :
- Construire la status line (HTTP/1.1 200 OK)
- Ajouter les headers
- Ajouter le body
- Générer le buffer prêt à envoyer

Utilisation :
    Response res;
    res.setStatus(200);
    res.setHeader("Content-Type", "text/html");
    res.setBody("<html>...</html>");
    res.build();
    std::string data = res.getSendBuffer();
*/

class Response {
private:
    ::Response _data;  // Structure de données (définie dans Dico.hpp)

    // Génère la status line
    std::string buildStatusLine() const;
    // Génère les headers
    std::string buildHeaders() const;

public:
    Response();
    ~Response();
    Response(const Response& other);
    Response& operator=(const Response& other);

    // Setters
    void setStatus(int code);
    void setStatus(int code, const std::string& message);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    void setBodyFromFile(const std::string& filepath);

    // Construit le send_buffer complet
    void build();

    // Getters
    int getStatusCode() const;
    const std::string& getStatusMessage() const;
    const std::string& getBody() const;
    const std::string& getSendBuffer() const;
    size_t getBytesSent() const;
    bool isReady() const;
    bool isComplete() const;

    // Pour l'envoi partiel
    void addBytesSent(size_t bytes);
    void markComplete();

    // Reset pour keep-alive
    void reset();

    // Réponses pré-construites
    static Response error(int code);
    static Response redirect(int code, const std::string& location);
};

#endif
