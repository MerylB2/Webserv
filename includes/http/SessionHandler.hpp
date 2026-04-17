#ifndef SESSION_HANDLER_HPP
#define SESSION_HANDLER_HPP

#include "Request.hpp"
#include "Response.hpp"

namespace SessionHandler 
{
    void handleLogin(const Request& req, Response& res);
    void handleLogout(const Request& req, Response& res, const std::string& session_id);
    void handleProtectedPage(const Request& req, Response& res, const std::string& session_id);
    std::string extractUsername(const std::string& body);

}

#endif