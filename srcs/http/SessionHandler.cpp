#include "SessionHandler.hpp"
#include "SessionManager.hpp"
#include "Response.hpp"
#include "Dico.hpp"
#include <fstream>

namespace SessionHandler {

/* ========= Extraire username depuis body ==--=====*/
std::string extractUsername(const std::string& body)
{
    size_t pos = body.find("username=");
    if (pos == std::string::npos)
        return "";

    pos += 9; //longueur username
    size_t end = body.find('&', pos);
    if (end == std::string::npos)
        end = body.size();

    return body.substr(pos, end - pos);
}

/* ======= Gerer le login ======= */
void handleLogin(const Request& req, Response& res)
{
    std::string username = extractUsername(req.body);

    if (username.empty())
    {
        ResponseBuilder::setStatus(res, HttpStatus::BAD_REQUEST);
        ResponseBuilder::setBody(res, "<h1>400 Bad Request</h1><p>Username manquant</p>");
        return;
    }

    //Creer la session
    SessionManager* sm = SessionManager::getInstance();
    std::string session_id = sm->createSession(username);

    std::cout << "[LOGIN] Session creee pour " << username << " : " << session_id << std::endl;

    // Reponse avec redirection + cookie
    ResponseBuilder::setStatus(res, HttpStatus::FOUND);
    ResponseBuilder::setHeader(res, "Location", "/dashboard");
    ResponseBuilder::setCookie(res, "session_id", session_id, 3600, "/", true, false);
    ResponseBuilder::setBody(res, "");
}

// GErer le logout
void handleLogout(const Request& req, Response& res, const std::string& session_id )
{
    if (!session_id.empty())
    {
        (void)req;
    
    std::cout << "[LOGOUT] Session détruite: " << session_id << std::endl;
    
    SessionManager* sm = SessionManager::getInstance();
    sm->deleteSession(session_id); 
    }

    // Reponse avec redirection + suppression du cookie
    ResponseBuilder::setStatus(res, HttpStatus::FOUND);  // 302
    ResponseBuilder::setHeader(res, "Location", "/login.html");
    ResponseBuilder::setCookie(res, "session_id", "", -1, "/", true, false);
    ResponseBuilder::setBody(res, "");
}

//Generer une page protege
void handleProtectedPage(const Request& req, Response& res, const std::string& session_id)
{
    (void)req;
    SessionManager* sm = SessionManager::getInstance();
    SessionData* session = sm->getSession(session_id);

    if (!session)
    {
        ResponseBuilder::setStatus(res, HttpStatus::UNAUTHORIZED);
        ResponseBuilder::setBody(res, "<h1>401 Unauthorized</h1>");
        return;
    }

    std::string username = session->user;

    // Lire le fichier dashboard.html
    std::ifstream file("./www/dashboard.html");
    if (!file.is_open())
    {
        ResponseBuilder::setStatus(res, HttpStatus::INTERNAL_SERVER_ERROR);
        ResponseBuilder::setBody(res, "<h1>500 Internal Server Error</h1>");
        return;
    }

    std::ostringstream buf;
    buf << file.rdbuf();
    std::string html = buf.str();

    // Remplacer les placeholders
    size_t pos;
    while ((pos = html.find("{{USERNAME}}")) != std::string::npos)
        html.replace(pos, 12, username);
    while ((pos = html.find("{{SESSION_ID}}")) != std::string::npos)
        html.replace(pos, 14, session_id);

    ResponseBuilder::setStatus(res, HttpStatus::OK);
    ResponseBuilder::setHeader(res, "Content-Type", "text/html");
    ResponseBuilder::setBody(res, html);
}
}