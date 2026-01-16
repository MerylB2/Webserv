#include "../../includes/http/Request.hpp"
#include <sstream>
#include <algorithm>

/* ========== Constructeurs / Destructeur ========== */

Request::Request() {}

Request::~Request() {}

Request::Request(const Request& other) : _data(other._data) {}

Request& Request::operator=(const Request& other) {
    if (this != &other) {
        _data = other._data;
    }
    return *this;
}

/* ========== Parsing principal ========== */

/*
Parse les données brutes reçues du client.
Les données arrivent par morceaux, donc on accumule dans read_buffer.
Retourne true si la requête est complète.

Format d'une requête HTTP :
    GET /path?query HTTP/1.1\r\n
    Host: localhost\r\n
    Content-Length: 5\r\n
    \r\n
    Hello
*/
bool Request::parse(const std::string& raw_data) {
    // Ajouter les nouvelles données au buffer
    _data.read_buffer += raw_data;

    // Machine à états pour le parsing
    while (_data.state != COMPLETE && _data.state != ERROR) {
        if (_data.state == REQUEST_LINE) {
            // Chercher la fin de la request line (\r\n)
            size_t pos = _data.read_buffer.find("\r\n");
            if (pos == std::string::npos)
                return false;  // Pas encore assez de données

            std::string line = _data.read_buffer.substr(0, pos);
            _data.read_buffer.erase(0, pos + 2);

            if (!parseRequestLine(line)) {
                _data.state = ERROR;
                return false;
            }
            _data.state = HEADERS;
        }
        else if (_data.state == HEADERS) {
            // Chercher la fin d'un header (\r\n)
            size_t pos = _data.read_buffer.find("\r\n");
            if (pos == std::string::npos)
                return false;  // Pas encore assez de données

            // Ligne vide = fin des headers
            if (pos == 0) {
                _data.read_buffer.erase(0, 2);

                // Déterminer s'il y a un body
                if (_data.content_length > 0) {
                    _data.state = BODY;
                } else if (_data.is_chunked) {
                    _data.state = CHUNKED_BODY;
                } else {
                    _data.state = COMPLETE;
                }
            } else {
                std::string line = _data.read_buffer.substr(0, pos);
                _data.read_buffer.erase(0, pos + 2);

                if (!parseHeader(line)) {
                    _data.state = ERROR;
                    return false;
                }
            }
        }
        else if (_data.state == BODY) {
            if (!parseBody())
                return false;  // Pas encore assez de données
        }
        else if (_data.state == CHUNKED_BODY) {
            if (!parseChunkedBody())
                return false;  // Pas encore assez de données
        }
    }

    return (_data.state == COMPLETE);
}

/* ========== Parsing de la Request Line ========== */

/*
Parse la première ligne : "GET /path?query HTTP/1.1"
*/
bool Request::parseRequestLine(const std::string& line) {
    // Vérifier la taille max de l'URI
    if (line.size() > WebservConfig::MAX_URI_LENGTH) {
        _data.error_code = HttpStatus::URI_TOO_LONG;
        return false;
    }

    std::istringstream iss(line);
    std::string method, uri, version;

    if (!(iss >> method >> uri >> version)) {
        _data.error_code = HttpStatus::BAD_REQUEST;
        return false;
    }

    // Vérifier la méthode
    if (method != "GET" && method != "POST" && method != "DELETE") {
        _data.error_code = HttpStatus::METHOD_NOT_ALLOWED;
        return false;
    }

    // Vérifier la version HTTP
    if (version != "HTTP/1.0" && version != "HTTP/1.1") {
        _data.error_code = HttpStatus::BAD_REQUEST;
        return false;
    }

    _data.method = method;
    _data.version = version;

    // Séparer URI et query string
    size_t pos = uri.find('?');
    if (pos != std::string::npos) {
        _data.uri = uri.substr(0, pos);
        _data.query_string = uri.substr(pos + 1);
    } else {
        _data.uri = uri;
        _data.query_string = "";
    }

    return true;
}

/* ========== Parsing des Headers ========== */

/*
Parse un header : "Content-Type: text/html"
*/
bool Request::parseHeader(const std::string& line) {
    size_t pos = line.find(':');
    if (pos == std::string::npos) {
        _data.error_code = HttpStatus::BAD_REQUEST;
        return false;
    }

    std::string name = line.substr(0, pos);
    std::string value = line.substr(pos + 1);

    // Trim les espaces au début de la valeur
    size_t start = value.find_first_not_of(" \t");
    if (start != std::string::npos)
        value = value.substr(start);

    // Convertir le nom en minuscules pour comparaison case-insensitive
    std::string name_lower = name;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

    // Stocker le header
    _data.headers[name] = value;

    // Traiter les headers spéciaux
    if (name_lower == "content-length") {
        _data.content_length = static_cast<size_t>(std::atol(value.c_str()));
    }
    else if (name_lower == "transfer-encoding") {
        std::string value_lower = value;
        std::transform(value_lower.begin(), value_lower.end(), value_lower.begin(), ::tolower);
        if (value_lower.find("chunked") != std::string::npos) {
            _data.is_chunked = true;
        }
    }

    return true;
}

/* ========== Parsing du Body ========== */

/*
Parse le body avec Content-Length connu.
*/
bool Request::parseBody() {
    size_t remaining = _data.content_length - _data.body_bytes_received;
    size_t available = _data.read_buffer.size();
    size_t to_read = std::min(remaining, available);

    _data.body += _data.read_buffer.substr(0, to_read);
    _data.read_buffer.erase(0, to_read);
    _data.body_bytes_received += to_read;

    if (_data.body_bytes_received >= _data.content_length) {
        _data.state = COMPLETE;
        return true;
    }

    return false;  // Pas encore tout le body
}

/*
Parse le body en mode chunked.
Format :
    SIZE\r\n
    DATA\r\n
    SIZE\r\n
    DATA\r\n
    0\r\n
    \r\n
*/
bool Request::parseChunkedBody() {
    while (true) {
        // Chercher la taille du chunk
        size_t pos = _data.read_buffer.find("\r\n");
        if (pos == std::string::npos)
            return false;  // Pas encore la taille

        std::string size_str = _data.read_buffer.substr(0, pos);
        size_t chunk_size = static_cast<size_t>(std::strtol(size_str.c_str(), NULL, 16));

        // Chunk de taille 0 = fin
        if (chunk_size == 0) {
            _data.read_buffer.erase(0, pos + 2);
            // Consommer le \r\n final
            if (_data.read_buffer.size() >= 2 && _data.read_buffer.substr(0, 2) == "\r\n") {
                _data.read_buffer.erase(0, 2);
            }
            _data.state = COMPLETE;
            return true;
        }

        // Vérifier qu'on a tout le chunk + \r\n
        if (_data.read_buffer.size() < pos + 2 + chunk_size + 2)
            return false;  // Pas encore tout le chunk

        // Extraire les données du chunk
        _data.body += _data.read_buffer.substr(pos + 2, chunk_size);
        _data.read_buffer.erase(0, pos + 2 + chunk_size + 2);
    }
}

/* ========== Getters ========== */

const std::string& Request::getMethod() const { return _data.method; }
const std::string& Request::getUri() const { return _data.uri; }
const std::string& Request::getQueryString() const { return _data.query_string; }
const std::string& Request::getVersion() const { return _data.version; }
const std::string& Request::getBody() const { return _data.body; }
const std::map<std::string, std::string>& Request::getHeaders() const { return _data.headers; }
RequestState Request::getState() const { return _data.state; }
int Request::getErrorCode() const { return _data.error_code; }
size_t Request::getContentLength() const { return _data.content_length; }
bool Request::isChunked() const { return _data.is_chunked; }

std::string Request::getHeader(const std::string& name) const {
    std::map<std::string, std::string>::const_iterator it = _data.headers.find(name);
    if (it != _data.headers.end())
        return it->second;
    return "";
}

/* ========== Reset ========== */

void Request::reset() {
    _data.reset();
}
