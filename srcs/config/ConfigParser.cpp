#include "../../includes/config/ConfigParser.hpp"
#include <iostream>
#include <set>
#include <sstream>

ConfigParser::ConfigParser() : _inServer(false), _inLocation(false) {}

ConfigParser::~ConfigParser() {}

// Utils

// Enlève les espaces au début et à la fin d'une string
// Exemple : "   hello world   " → "hello world"
std::string ConfigParser::trim(const std::string& str) {
	size_t start = 0;
	while (start < str.length() && (str[start] == ' ' || str[start] == '\t')) {
		start++;
	}
	if (start == str.length()) {
		return "";
	}
	size_t end = str.length() - 1;
	while (end > start && (str[end] == ' ' || str[end] == '\t')) {
		end--;
	}
	return str.substr(start, end - start + 1);
}

// getter
const std::vector<ServerConfig>& ConfigParser::getServers() const {
    return _servers;
}

// La fonction principale de parsing
// 1. Ouvrir le fichier
// 2. Lire ligne par ligne
// 3. Pour chaque ligne, appeler parseLine()
// 4. Valider à la fin
void ConfigParser::parse(const std::string& filename) {
	std::ifstream file(filename.c_str());
	if (!file.is_open()) {
		throw std::runtime_error("Error: Could not open config file " + filename);
	}
	std::string line;
	while (std::getline(file, line)) {
		parseLine(line);
	}
	file.close();
	validateConfig();
}

// Analyse une ligne du fichier de config
// Détecte si c'est une balise [server], [location], ou une directive
void ConfigParser::parseLine(const std::string& line) {
    // Nettoyer la ligne (enlever espaces)
    std::string trimmedLine = trim(line);  
    // Ignorer lignes vides
    if (trimmedLine.empty()) {
        return;
    }  
    // Ignorer commentaires
    if (trimmedLine[0] == '#') {
        return;
    }
    // Détecter [server]
    if (trimmedLine == "[server]") {
        _inServer = true;
        _currentServer = ServerConfig();  // Réinitialiser avec valeurs par défaut
        return;
    }   
    // Détecter [/server]
    if (trimmedLine == "[/server]") {
        if (!_inServer) {
            throw std::runtime_error("Unexpected [/server]");
        }
        _servers.push_back(_currentServer);  // Sauvegarder le serveur
        _inServer = false;
        return;
    }   
    // Détecter [location /path]
    if (trimmedLine.find("[location ") == 0 && trimmedLine[trimmedLine.length() - 1] == ']') {
        if (!_inServer) {
            throw std::runtime_error("[location] must be inside [server]");
        }
        _inLocation = true;
        _currentLocation = LocationConfig();  // Réinitialiser
        _currentLocation.path_url = extractLocationPath(trimmedLine);
        return;
    }
    // Détecter [/location]
    if (trimmedLine == "[/location]") {
        if (!_inLocation) {
            throw std::runtime_error("Unexpected [/location]");
        }
        _currentServer.locations.push_back(_currentLocation);  // Sauvegarder la location
        _inLocation = false;
        return;
    }
    // Sinon c'est une directive : "nom valeur;"
    // Trouver le premier espace pour séparer nom et valeur
    size_t spacePos = trimmedLine.find(' ');
    if (spacePos == std::string::npos) {
        throw std::runtime_error("Invalid directive: " + trimmedLine);
    }
    std::string name = trimmedLine.substr(0, spacePos);
    std::string value = trimmedLine.substr(spacePos + 1);
    // Enlever le ; à la fin de la valeur
    if (!value.empty() && value[value.length() - 1] == ';') {
        value = value.substr(0, value.length() - 1);
    }
    value = trim(value);
    // Router vers le bon parser selon où on est
    if (_inLocation) {
        parseLocationDirective(name, value);
    } else if (_inServer) {
        parseServerDirective(name, value);
    } else {
        throw std::runtime_error("Directive outside of [server] block: " + name);
    }
}

// Extrait le path d'une ligne [location /path]
// Exemple : "[location /uploads]" → "/uploads"
std::string ConfigParser::extractLocationPath(const std::string& line) {
    size_t start = 10;  // Trouver la position après "[location " -> strlen("[location ") = 10
    
    size_t end = line.length() - 1;  // Trouver le ] final
    
    return line.substr(start, end - start);  // Extraire
}

// Routage des directives

// Parse une directive qui appartient au bloc server
void ConfigParser::parseServerDirective(const std::string& name, const std::string& value) {
    if (name == "listen") {
        parseListen(value);
    }
    else if (name == "server_name") {
        _currentServer.server_name = value;
    }
    else if (name == "root") {
        _currentServer.root_dir = value;
    }
    else if (name == "index") {
        // Ignoré au niveau server et géré par LocationConfig, chaque location a son propre index
        return;
    }
    else if (name == "client_max_body_size") {
        _currentServer.max_body_size = parseSize(value);
    }
    else if (name == "error_page") {
        parseErrorPage(value);
    }
    else {
        throw std::runtime_error("Unknown server directive: " + name);
    }
}

// Parse une directive qui appartient au bloc location
void ConfigParser::parseLocationDirective(const std::string& name, const std::string& value) {
    if (name == "root") {
        _currentLocation.root_dir = value;
    }
    else if (name == "index") {
        _currentLocation.index = value;
    }
    else if (name == "allowed_methods") {
        parseAllowedMethods(value);
    }
    else if (name == "autoindex") {
        _currentLocation.autoindex = (value == "on");
    }
    else if (name == "upload_path") {
        _currentLocation.upload_dir = value;
    }
    else if (name == "client_max_body_size") {
        _currentLocation.max_body_size = parseSize(value);
    }
    else if (name == "cgi_extension") {
        parseCgiExtension(value);
    }
    else if (name == "return") {
        parseReturn(value);
    }
    else {
        throw std::runtime_error("Unknown location directive: " + name);
    }
}

// Parsers spécifiques 

//Parse la directive "listen"
// Formats acceptés :
//   - "8080"           → port seul, ip = 0.0.0.0 (toutes interfaces)
//   - "127.0.0.1:8080" → ip:port
// Exemples :
//   parseListen("8080")           → ip="0.0.0.0", port=8080
//   parseListen("127.0.0.1:8080") → ip="127.0.0.1", port=8080
void ConfigParser::parseListen(const std::string& value) {
    // Chercher s'il y a un ":" (séparateur ip:port)
    size_t colonPos = value.find(':');
    
    if (colonPos == std::string::npos) {
        // Pas de ":" → c'est juste un port
        // Exemple : "8080"
        _currentServer.ip_address = "0.0.0.0";  // Écouter sur toutes les interfaces
        _currentServer.listen_port = std::atoi(value.c_str());
    }
    else {
        // Il y a un ":" → format ip:port
        // Exemple : "127.0.0.1:8080"
     _currentServer.ip_address = value.substr(0, colonPos);      // Avant le ":"
        std::string portStr = value.substr(colonPos + 1);        // Après le ":"
        _currentServer.listen_port = std::atoi(portStr.c_str());
    }
}

// Parse une taille
// Formats acceptés :
//   - "1024"  → 1024 bytes
//   - "10K"   → 10 * 1024 = 10240 bytes
//   - "10M"   → 10 * 1024 * 1024 = 10485760 bytes
//   - "1G"    → 1 * 1024 * 1024 * 1024 bytes
// Retourne : nombre de bytes
size_t ConfigParser::parseSize(const std::string& sizeStr) {
    if (sizeStr.empty()) {
        throw std::runtime_error("Empty size value");
    }
    // Récupérer le dernier caractère pour voir si c'est un suffixe
    char lastChar = sizeStr[sizeStr.length() - 1];
    // Si le dernier caractère est un chiffre → pas de suffixe, c'est directement en bytes
    if (lastChar >= '0' && lastChar <= '9') {
        return static_cast<size_t>(std::atol(sizeStr.c_str()));
    }
    // Sinon, extraire le nombre (sans le suffixe)
    std::string numberPart = sizeStr.substr(0, sizeStr.length() - 1);
    size_t number = static_cast<size_t>(std::atol(numberPart.c_str()));
    // Appliquer le multiplicateur selon le suffixe
    switch (lastChar) {
        case 'K':
        case 'k':
            return number * 1024;                      // Kilo-octets
        case 'M':
        case 'm':
            return number * 1024 * 1024;               // Méga-octets
        case 'G':
        case 'g':
            return number * 1024 * 1024 * 1024;        // Giga-octets
        default:
            throw std::runtime_error("Invalid size suffix: " + sizeStr);
    }
}

// Parse la directive "allowed_methods"
// Format : "GET POST DELETE" (méthodes séparées par des espaces)
// Exemple :
//   parseAllowedMethods("GET POST") 
//   → _currentLocation.methods = ["GET", "POST"]
// Note : On vide d'abord le vecteur car LocationConfig a "GET" par défaut
void ConfigParser::parseAllowedMethods(const std::string& value) {
    // Vider les méthodes par défaut (GET est mis par défaut dans le constructeur)
    _currentLocation.methods.clear();

    // Utiliser istringstream pour séparer les mots par espaces
    std::istringstream iss(value);
    std::string method;

    // Lire chaque méthode séparée par espace
    while (iss >> method) {
        // Vérifier que c'est une méthode valide
        if (method == "GET" || method == "POST" || method == "DELETE") {
            _currentLocation.methods.push_back(method);
        }
        else {
            throw std::runtime_error("Invalid HTTP method: " + method);
        }
    }
    // Vérifier qu'on a au moins une méthode
    if (_currentLocation.methods.empty()) {
        throw std::runtime_error("No valid methods specified");
    }
}

// Parse la directive "error_page"
// Formats acceptés :
//   - "404 /error/404.html"           → une seule erreur
//   - "500 502 503 /error/50x.html"   → plusieurs erreurs, même page
// Exemples :
//   parseErrorPage("404 /error/404.html")
//   → _currentServer.error_pages[404] = "/error/404.html"
//
//   parseErrorPage("500 502 503 /error/50x.html")
//   → _currentServer.error_pages[500] = "/error/50x.html"
//   → _currentServer.error_pages[502] = "/error/50x.html"
//   → _currentServer.error_pages[503] = "/error/50x.html"
void ConfigParser::parseErrorPage(const std::string& value) {
    // Séparer les éléments par espaces
    std::istringstream iss(value);
    std::vector<std::string> tokens;
    std::string token;
    // Lire tous les tokens
    while (iss >> token) {
        tokens.push_back(token);
    }
    // Il faut au moins 2 éléments : un code d'erreur et un chemin
    if (tokens.size() < 2) {
        throw std::runtime_error("Invalid error_page directive: " + value);
    }
    // Le dernier élément est le chemin de la page d'erreur
    std::string errorPath = tokens[tokens.size() - 1];    
    // Tous les éléments avant sont des codes d'erreur
    for (size_t i = 0; i < tokens.size() - 1; i++) {
        int errorCode = std::atoi(tokens[i].c_str());   
        // Vérifier que c'est un code d'erreur valide (entre 400 et 599)
        if (errorCode < 400 || errorCode > 599) {
            throw std::runtime_error("Invalid error code: " + tokens[i]);
        }
        // Associer ce code d'erreur au chemin de la page
        _currentServer.error_pages[errorCode] = errorPath;
    }
}


// Parse la directive "cgi_extension"
// Format : ".py /usr/bin/python3" (extension + chemin de l'interpréteur)
//
// Exemple :
//   parseCgiExtension(".py /usr/bin/python3")
//   → _currentLocation.cgi_handlers[".py"] = "/usr/bin/python3"
//   parseCgiExtension(".php /usr/bin/php-cgi")
//   → _currentLocation.cgi_handlers[".php"] = "/usr/bin/php-cgi"
void ConfigParser::parseCgiExtension(const std::string& value) {
    // Trouver l'espace qui sépare l'extension du chemin de l'interpréteur
    size_t spacePos = value.find(' ');

    if (spacePos == std::string::npos) {
        throw std::runtime_error("Invalid cgi_extension: " + value + " (format: .ext /path/to/interpreter)");
    }
    // Extraire l'extension (ex: ".py")
    std::string extension = value.substr(0, spacePos);
    // Extraire le chemin de l'interpréteur (ex: "/usr/bin/python3")
    std::string interpreterPath = trim(value.substr(spacePos + 1));
    // Vérifier que l'extension commence par un point
    if (extension.empty() || extension[0] != '.') {
        throw std::runtime_error("CGI extension must start with '.': " + extension);
    }
    // Vérifier que le chemin n'est pas vide
    if (interpreterPath.empty()) {
        throw std::runtime_error("CGI interpreter path is empty");
    }
    // Ajouter au map des handlers CGI
    _currentLocation.cgi_handlers[extension] = interpreterPath;
}

// Parse la directive "return" (redirection HTTP)
// Format : "301 https://www.example.com" (code + URL)
//
// Codes de redirection :
//   - 301 : Redirection permanente (le navigateur mémorise)
//   - 302 : Redirection temporaire
//
// Exemple :
//   parseReturn("301 https://www.google.com")
//   → _currentLocation.redirect_code = 301
//   → _currentLocation.redirect_url = "https://www.google.com"
void ConfigParser::parseReturn(const std::string& value) {
    // Trouver l'espace qui sépare le code de l'URL
    size_t spacePos = value.find(' ');

    if (spacePos == std::string::npos) {
        throw std::runtime_error("Invalid return directive: " + value + " (format: code url)");
    }
    // Extraire le code de redirection
    std::string codeStr = value.substr(0, spacePos);
    int code = std::atoi(codeStr.c_str());
    // Vérifier que c'est un code de redirection valide
    if (code != 301 && code != 302 && code != 303 && code != 307 && code != 308) {
        throw std::runtime_error("Invalid redirect code: " + codeStr + " (must be 301, 302, 303, 307 or 308)");
    }
    // Extraire l'URL de redirection
    std::string url = trim(value.substr(spacePos + 1));
    if (url.empty()) {
        throw std::runtime_error("Redirect URL is empty");
    }
    // Stocker dans la location courante
    _currentLocation.redirect_code = code;
    _currentLocation.redirect_url = url;
}

// Validation complète après parsing
void ConfigParser::validateConfig() {
    std::set<int> ports;
    if (_servers.empty()) {
        throw std::runtime_error("No server defined in config");
    } 
    for (size_t i = 0; i < _servers.size(); i++) {
        if (!ports.insert(_servers[i].listen_port).second) {
            std::ostringstream oss;
            oss << "Duplicate port: " << _servers[i].listen_port;
            throw std::runtime_error(oss.str());
        }
        validateServer(_servers[i]);
    }
}

// Valide un ServerConfig individuel
void ConfigParser::validateServer(const ServerConfig& server) {
    // Vérifier le port
    if (server.listen_port < 1 || server.listen_port > 65535) {
        throw std::runtime_error("Invalid port number");
    }
    // Vérifier que root est défini
    if (server.root_dir.empty()) {
        throw std::runtime_error("Server root directory not defined");
    }
    // Vérifier qu'il y a au moins une location
    if (server.locations.empty()) {
        throw std::runtime_error("Server must have at least one location");
    }
}