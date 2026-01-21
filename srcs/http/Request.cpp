#include "../../includes/http/Request.hpp"
#include <sstream>
#include <algorithm>
#include <cstdlib>

namespace RequestParser {

/*
Parse les données brutes reçues du client.
Les données arrivent par morceaux, donc on accumule dans read_buffer.
Retourne true si la requête est complète.
*/
bool parse(Request& req, const std::string& raw_data) {
    // Ajouter les nouvelles données au buffer
    req.read_buffer += raw_data;

    // Machine à états pour le parsing
    while (req.state != COMPLETE && req.state != ERROR) {
        if (req.state == REQUEST_LINE) {
            size_t pos = req.read_buffer.find("\r\n");
            if (pos == std::string::npos)
                return false;

            std::string line = req.read_buffer.substr(0, pos);
            req.read_buffer.erase(0, pos + 2);

            if (!parseRequestLine(req, line)) {
                req.state = ERROR;
                return false;
            }
            req.state = HEADERS;
        }
        else if (req.state == HEADERS) {
            size_t pos = req.read_buffer.find("\r\n");
            if (pos == std::string::npos)
                return false;

            if (pos == 0) {
                req.read_buffer.erase(0, 2);
                if (req.content_length > 0) {
                    req.state = BODY;
                } else if (req.is_chunked) {
                    req.state = CHUNKED_BODY;
                } else {
                    req.state = COMPLETE;
                }
            } else {
                std::string line = req.read_buffer.substr(0, pos);
                req.read_buffer.erase(0, pos + 2);

                if (!parseHeader(req, line)) {
                    req.state = ERROR;
                    return false;
                }
            }
        }
        else if (req.state == BODY) {
            if (!parseBody(req))
                return false;
        }
        else if (req.state == CHUNKED_BODY) {
            if (!parseChunkedBody(req))
                return false;
        }
    }

    return (req.state == COMPLETE);
}

/*
Parse la première ligne : "GET /path?query HTTP/1.1"
*/
bool parseRequestLine(Request& req, const std::string& line) {
    if (line.size() > WebservConfig::MAX_URI_LENGTH) {
        req.error_code = HttpStatus::URI_TOO_LONG;
        return false;
    }

    std::istringstream iss(line);
    std::string method, uri, version;

    if (!(iss >> method >> uri >> version)) {
        req.error_code = HttpStatus::BAD_REQUEST;
        return false;
    }

    if (method != "GET" && method != "POST" && method != "DELETE") {
        req.error_code = HttpStatus::METHOD_NOT_ALLOWED;
        return false;
    }

    if (version != "HTTP/1.0" && version != "HTTP/1.1") {
        req.error_code = HttpStatus::BAD_REQUEST;
        return false;
    }

    req.method = method;
    req.version = version;

    size_t pos = uri.find('?');
    if (pos != std::string::npos) {
        req.uri = uri.substr(0, pos);
        req.query_string = uri.substr(pos + 1);
    } else {
        req.uri = uri;
        req.query_string = "";
    }

    return true;
}

/*
Parse un header : "Content-Type: text/html"
*/
bool parseHeader(Request& req, const std::string& line) {
    size_t pos = line.find(':');
    if (pos == std::string::npos) {
        req.error_code = HttpStatus::BAD_REQUEST;
        return false;
    }

    std::string name = line.substr(0, pos);
    std::string value = line.substr(pos + 1);

    size_t start = value.find_first_not_of(" \t");
    if (start != std::string::npos)
        value = value.substr(start);

    std::string name_lower = name;
    for (size_t i = 0; i < name_lower.size(); ++i)
        name_lower[i] = std::tolower(name_lower[i]);

    req.headers[name] = value;

    if (name_lower == "content-length") {
        req.content_length = static_cast<size_t>(std::atol(value.c_str()));
    }
    else if (name_lower == "transfer-encoding") {
        std::string value_lower = value;
        for (size_t i = 0; i < value_lower.size(); ++i)
            value_lower[i] = std::tolower(value_lower[i]);
        if (value_lower.find("chunked") != std::string::npos) {
            req.is_chunked = true;
        }
    }

    return true;
}

/*
Parse le body avec Content-Length connu.
*/
bool parseBody(Request& req) {
    size_t remaining = req.content_length - req.body_bytes_received;
    size_t available = req.read_buffer.size();
    size_t to_read = (remaining < available) ? remaining : available;

    req.body += req.read_buffer.substr(0, to_read);
    req.read_buffer.erase(0, to_read);
    req.body_bytes_received += to_read;

    if (req.body_bytes_received >= req.content_length) {
        req.state = COMPLETE;
        return true;
    }

    return false;
}

/*
Parse le body en mode chunked.
*/
bool parseChunkedBody(Request& req) {
    while (true) {
        size_t pos = req.read_buffer.find("\r\n");
        if (pos == std::string::npos)
            return false;

        std::string size_str = req.read_buffer.substr(0, pos);
        size_t chunk_size = static_cast<size_t>(std::strtol(size_str.c_str(), NULL, 16));

        if (chunk_size == 0) {
            req.read_buffer.erase(0, pos + 2);
            if (req.read_buffer.size() >= 2 && req.read_buffer.substr(0, 2) == "\r\n") {
                req.read_buffer.erase(0, 2);
            }
            req.state = COMPLETE;
            return true;
        }

        if (req.read_buffer.size() < pos + 2 + chunk_size + 2)
            return false;

        req.body += req.read_buffer.substr(pos + 2, chunk_size);
        req.read_buffer.erase(0, pos + 2 + chunk_size + 2);
    }
}

} // namespace RequestParser
