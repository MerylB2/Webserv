#include "../../includes/http/Response.hpp"
#include <fstream>
#include <sstream>

namespace ResponseBuilder {

/* ========== Setters ========== */

void setCookie(Response& res, 
                   const std::string& name, 
                   const std::string& value,
                   int max_age,
                   const std::string& path,
                   bool http_only,
                   bool secure)
{
    std::ostringstream cookie;

    cookie << name << "=" << value;

    if (max_age > 0)
        cookie << "; Max-Age=" << max_age;
    else if (max_age < 0)
        cookie << "; Max-Age=0";
        
    cookie << "; Path=" << path;

    if (http_only)
        cookie << "; HttpOnly";
    
    if (secure)
        cookie << "; Secure";
        
    cookie << "; SameSite=Lax";

    res.set_cookies.push_back(cookie.str());
}

void setStatus(Response& res, int code) {
    res.status_code = code;
    res.status_message = HttpStatus::getMessage(code);
}

void setStatus(Response& res, int code, const std::string& message) {
    res.status_code = code;
    res.status_message = message;
}

void setHeader(Response& res, const std::string& name, const std::string& value) {
    res.headers[name] = value;
}

void setBody(Response& res, const std::string& body) {
    res.body = body;
}

void setBodyFromFile(Response& res, const std::string& filepath) {
    std::ifstream file(filepath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        setStatus(res, HttpStatus::NOT_FOUND);
        res.body = "";
        return;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    res.body = oss.str();
    file.close();

    std::string ext = MimeTypes::getExtension(filepath);
    setHeader(res, "Content-Type", MimeTypes::getType(ext));
}

/* ========== Build ========== */

std::string buildStatusLine(const Response& res) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << res.status_code << " " << res.status_message << "\r\n";
    return oss.str();
}

std::string buildHeaders(const Response& res) {
    std::ostringstream oss;

    for (std::map<std::string, std::string>::const_iterator it = res.headers.begin();
         it != res.headers.end(); ++it) {
        oss << it->first << ": " << it->second << "\r\n";
    }

    //Header set-cookie
    for (std::vector<std::string>::const_iterator it = res.set_cookies.begin();
        it != res.set_cookies.end(); ++it) {
        oss << "Set-Cookie: " << *it << "\r\n";
    }

    if (res.headers.find("Content-Length") == res.headers.end()) {
        oss << "Content-Length: " << res.body.size() << "\r\n";
    }

    oss << "\r\n";
    return oss.str();
}

void build(Response& res) {
    res.send_buffer = buildStatusLine(res) + buildHeaders(res) + res.body;
    res.bytes_sent = 0;
    res.is_ready = true;
    res.is_complete = false;
}

/* ========== Envoi ========== */

void addBytesSent(Response& res, size_t bytes) {
    res.bytes_sent += bytes;
    if (res.bytes_sent >= res.send_buffer.size()) {
        res.is_complete = true;
    }
}

/* ========== Réponses pré-construites ========== */

Response makeError(int code) {
    Response res;
    setStatus(res, code);
    setHeader(res, "Content-Type", "text/html");

    std::ostringstream oss;
    oss << "<!DOCTYPE html>\n"
        << "<html>\n"
        << "<head><title>" << code << " " << HttpStatus::getMessage(code) << "</title></head>\n"
        << "<body>\n"
        << "<h1>" << code << " " << HttpStatus::getMessage(code) << "</h1>\n"
        << "</body>\n"
        << "</html>\n";

    setBody(res, oss.str());
    build(res);
    return res;
}

Response makeRedirect(int code, const std::string& location) {
    Response res;
    setStatus(res, code);
    setHeader(res, "Location", location);
    setHeader(res, "Content-Type", "text/html");

    std::ostringstream oss;
    oss << "<!DOCTYPE html>\n"
        << "<html>\n"
        << "<head><title>Redirect</title></head>\n"
        << "<body>\n"
        << "<h1>" << code << " " << HttpStatus::getMessage(code) << "</h1>\n"
        << "<p>Redirecting to <a href=\"" << location << "\">" << location << "</a></p>\n"
        << "</body>\n"
        << "</html>\n";

    setBody(res, oss.str());
    build(res);
    return res;
}

} // namespace ResponseBuilder
