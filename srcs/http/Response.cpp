#include "../../includes/http/Response.hpp"
#include <fstream>
#include <sstream>

/* ========== Constructeurs / Destructeur ========== */

Response::Response() {}

Response::~Response() {}

Response::Response(const Response& other) : _data(other._data) {}

Response& Response::operator=(const Response& other) {
    if (this != &other) {
        _data = other._data;
    }
    return *this;
}

/* ========== Setters ========== */

void Response::setStatus(int code) {
    _data.status_code = code;
    _data.status_message = HttpStatus::getMessage(code);
}

void Response::setStatus(int code, const std::string& message) {
    _data.status_code = code;
    _data.status_message = message;
}

void Response::setHeader(const std::string& name, const std::string& value) {
    _data.headers[name] = value;
}

void Response::setBody(const std::string& body) {
    _data.body = body;
}

/*
Charge le body depuis un fichier.
Retourne false si le fichier n'existe pas.
*/
void Response::setBodyFromFile(const std::string& filepath) {
    std::ifstream file(filepath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        setStatus(HttpStatus::NOT_FOUND);
        _data.body = "";
        return;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    _data.body = oss.str();
    file.close();

    // Déterminer le Content-Type
    std::string ext = MimeTypes::getExtension(filepath);
    setHeader("Content-Type", MimeTypes::getType(ext));
}

/* ========== Build ========== */

/*
Construit la status line : "HTTP/1.1 200 OK\r\n"
*/
std::string Response::buildStatusLine() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << _data.status_code << " " << _data.status_message << "\r\n";
    return oss.str();
}

/*
Construit les headers.
*/
std::string Response::buildHeaders() const {
    std::ostringstream oss;

    // Headers définis par l'utilisateur
    for (std::map<std::string, std::string>::const_iterator it = _data.headers.begin();
         it != _data.headers.end(); ++it) {
        oss << it->first << ": " << it->second << "\r\n";
    }

    // Content-Length si pas déjà défini
    if (_data.headers.find("Content-Length") == _data.headers.end()) {
        oss << "Content-Length: " << _data.body.size() << "\r\n";
    }

    // Fin des headers
    oss << "\r\n";

    return oss.str();
}

/*
Construit le send_buffer complet prêt à envoyer.
Format :
    HTTP/1.1 200 OK\r\n
    Content-Type: text/html\r\n
    Content-Length: 13\r\n
    \r\n
    <html>...</html>
*/
void Response::build() {
    _data.send_buffer = buildStatusLine() + buildHeaders() + _data.body;
    _data.bytes_sent = 0;
    _data.is_ready = true;
    _data.is_complete = false;
}

/* ========== Getters ========== */

int Response::getStatusCode() const { return _data.status_code; }
const std::string& Response::getStatusMessage() const { return _data.status_message; }
const std::string& Response::getBody() const { return _data.body; }
const std::string& Response::getSendBuffer() const { return _data.send_buffer; }
size_t Response::getBytesSent() const { return _data.bytes_sent; }
bool Response::isReady() const { return _data.is_ready; }
bool Response::isComplete() const { return _data.is_complete; }

/* ========== Envoi ========== */

void Response::addBytesSent(size_t bytes) {
    _data.bytes_sent += bytes;
    if (_data.bytes_sent >= _data.send_buffer.size()) {
        _data.is_complete = true;
    }
}

void Response::markComplete() {
    _data.is_complete = true;
}

/* ========== Reset ========== */

void Response::reset() {
    _data.reset();
}

/* ========== Réponses pré-construites ========== */

/*
Crée une réponse d'erreur avec le code spécifié.
Utilise la page d'erreur par défaut.
*/
Response Response::error(int code) {
    Response res;
    res.setStatus(code);
    res.setHeader("Content-Type", "text/html");

    // Body par défaut
    std::ostringstream oss;
    oss << "<!DOCTYPE html>\n"
        << "<html>\n"
        << "<head><title>" << code << " " << HttpStatus::getMessage(code) << "</title></head>\n"
        << "<body>\n"
        << "<h1>" << code << " " << HttpStatus::getMessage(code) << "</h1>\n"
        << "</body>\n"
        << "</html>\n";

    res.setBody(oss.str());
    res.build();
    return res;
}

/*
Crée une réponse de redirection.
*/
Response Response::redirect(int code, const std::string& location) {
    Response res;
    res.setStatus(code);
    res.setHeader("Location", location);
    res.setHeader("Content-Type", "text/html");

    std::ostringstream oss;
    oss << "<!DOCTYPE html>\n"
        << "<html>\n"
        << "<head><title>Redirect</title></head>\n"
        << "<body>\n"
        << "<h1>" << code << " " << HttpStatus::getMessage(code) << "</h1>\n"
        << "<p>Redirecting to <a href=\"" << location << "\">" << location << "</a></p>\n"
        << "</body>\n"
        << "</html>\n";

    res.setBody(oss.str());
    res.build();
    return res;
}
