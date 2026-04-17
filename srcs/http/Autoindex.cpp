#include "../../includes/http/Router.hpp"
#include "../../includes/http/Response.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <vector>
#include <string>

namespace Router {

static std::string formatSize(off_t size) {
    std::ostringstream oss;
    if (size < 1024)
        oss << size << " B";
    else if (size < 1024 * 1024)
        oss << (size / 1024) << " KB";
    else
        oss << (size / (1024 * 1024)) << " MB";
    return oss.str();
}

static std::string formatTime(time_t t) {
    char buf[64];
    struct tm* tm_info = localtime(&t);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm_info);
    return std::string(buf);
}

bool handleAutoindex(const std::string& dirpath, const std::string& uri, Response& response) {
    DIR* dir = opendir(dirpath.c_str());
    if (!dir)
        return false;

    // S'assurer que l'URI finit par /
    std::string base_uri = uri;
    if (!base_uri.empty() && base_uri[base_uri.size() - 1] != '/')
        base_uri += "/";

    // Collecter les entrees
    std::vector<std::string> entries;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == ".")
            continue;
        entries.push_back(name);
    }
    closedir(dir);

    std::sort(entries.begin(), entries.end());

    // Generer le HTML
    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html lang=\"fr\">\n"
         << "<head>\n"
         << "<meta charset=\"UTF-8\">\n"
         << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
         << "<title>Index of " << uri << "</title>\n"
         << "<style>\n"
         << "* { margin: 0; padding: 0; box-sizing: border-box; }\n"
         << "body { font-family: 'Segoe UI', Arial, sans-serif; background: #0f0f0f; color: #e0e0e0;"
         << " min-height: 100vh; display: flex; flex-direction: column; align-items: center; }\n"
         << "header { width: 100%; padding: 30px 0 20px; text-align: center;"
         << " background: linear-gradient(135deg, #1a1a2e, #16213e); border-bottom: 2px solid #0f3460; }\n"
         << "header h1 { font-size: 1.8em; font-weight: 300; letter-spacing: 4px; color: #e94560; }\n"
         << "header p { margin-top: 8px; color: #888; font-size: 0.85em; font-family: monospace; }\n"
         << "main { max-width: 800px; width: 90%; margin: 30px auto; }\n"
         << ".section { background: #1a1a1a; border: 1px solid #2a2a2a; border-radius: 8px; padding: 20px; }\n"
         << "table { width: 100%; border-collapse: collapse; }\n"
         << "th { text-align: left; color: #e94560; padding: 10px 12px; border-bottom: 1px solid #333; font-size: 0.85em; }\n"
         << "td { padding: 10px 12px; border-bottom: 1px solid #222; font-size: 0.85em; }\n"
         << "td a { color: #5dade2; text-decoration: none; }\n"
         << "td a:hover { text-decoration: underline; }\n"
         << "td.sz, td.dt { color: #777; font-family: monospace; }\n"
         << "tr:hover td { background: #222; }\n"
         << ".dr a { color: #e94560; }\n"
         << "a.bk { display: inline-block; color: #5dade2; text-decoration: none;"
         << " border: 1px solid #5dade2; padding: 8px 20px; border-radius: 4px;"
         << " font-size: 0.85em; transition: all 0.2s; margin-top: 16px; }\n"
         << "a.bk:hover { background: #5dade2; color: #0f0f0f; }\n"
         << "footer { margin-top: auto; padding: 20px; color: #555; font-size: 0.8em; }\n"
         << "</style>\n"
         << "</head>\n"
         << "<body>\n"
         << "<header><h1>INDEX</h1><p>" << uri << "</p></header>\n"
         << "<main>\n"
         << "<div class=\"section\">\n"
         << "<table>\n"
         << "<tr><th>Nom</th><th>Taille</th><th>Derniere modification</th></tr>\n";

    for (size_t i = 0; i < entries.size(); i++) {
        std::string name = entries[i];
        std::string fullpath = dirpath;
        if (fullpath[fullpath.size() - 1] != '/')
            fullpath += "/";
        fullpath += name;

        struct stat st;
        if (stat(fullpath.c_str(), &st) != 0)
            continue;

        std::string display_name = name;
        std::string link = base_uri + name;
        bool isDir = S_ISDIR(st.st_mode);

        if (isDir) {
            display_name += "/";
            link += "/";
        }

        html << "<tr" << (isDir ? " class=\"dr\"" : "") << ">"
             << "<td><a href=\"" << link << "\">" << display_name << "</a></td>"
             << "<td class=\"sz\">" << (isDir ? "-" : formatSize(st.st_size)) << "</td>"
             << "<td class=\"dt\">" << formatTime(st.st_mtime) << "</td>"
             << "</tr>\n";
    }

    html << "</table>\n"
         << "</div>\n"
         << "<a class=\"bk\" href=\"/\">Retour a l'accueil</a>\n"
         << "</main>\n"
         << "<footer>Webserv &mdash; Autoindex</footer>\n"
         << "</body>\n"
         << "</html>\n";

    ResponseBuilder::setStatus(response, 200);
    ResponseBuilder::setHeader(response, "Content-Type", "text/html");
    ResponseBuilder::setBody(response, html.str());
    ResponseBuilder::build(response);
    return true;
}

} // namespace Router
