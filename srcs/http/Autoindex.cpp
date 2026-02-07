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
         << "<html>\n"
         << "<head><title>Index of " << uri << "</title></head>\n"
         << "<body>\n"
         << "<h1>Index of " << uri << "</h1>\n"
         << "<hr>\n"
         << "<table>\n"
         << "<tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>\n";

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

        if (S_ISDIR(st.st_mode)) {
            display_name += "/";
            link += "/";
        }

        html << "<tr>"
             << "<td><a href=\"" << link << "\">" << display_name << "</a></td>"
             << "<td>" << (S_ISDIR(st.st_mode) ? "-" : formatSize(st.st_size)) << "</td>"
             << "<td>" << formatTime(st.st_mtime) << "</td>"
             << "</tr>\n";
    }

    html << "</table>\n"
         << "<hr>\n"
         << "</body>\n"
         << "</html>\n";

    ResponseBuilder::setStatus(response, 200);
    ResponseBuilder::setHeader(response, "Content-Type", "text/html");
    ResponseBuilder::setBody(response, html.str());
    ResponseBuilder::build(response);
    return true;
}

} // namespace Router
