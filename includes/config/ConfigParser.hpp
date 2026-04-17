/* Classe pour parser les fichiers de configuration 
Rôle :
Lire un fichier .conf et remplir les structures ServerConfig et LocationConfig définies dans Dico.hpp
*/

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "../core/Dico.hpp"
#include <string>
#include <vector>
#include <fstream> // pour lire le fichier
#include <sstream> // pour std::istringstream, parser les lignes
#include <stdexcept>
#include <cstdlib> // pour std::atoi() pour convertir string en int

class ConfigParser {

private:
	std::vector<ServerConfig> _servers; // Liste finale de tous les serveurs parsés qui contient tous les ServerConfig
	ServerConfig _currentServer; // Serveur en cours de parsing : quand [server], on commence à remplir _currentServer et quand [/server], on l'ajoute à _servers
	LocationConfig _currentLocation; // Location en cours de parsing : quand [location /path], on commence à remplir _currentLocation et quand [/location], on l'ajoute à _currentServer.locations
    
	// Flags pour savoir où on est dans le fichier
	bool _inServer;    // true = on est entre [server] et [/server]
	bool _inLocation;  // true = on est entre [location] et [/location]

	// Méthodes de parsing
	void parseLine(const std::string& line); // Analyse une ligne du fichier et détecte si c'est une balise [server], [location], ou une directive
	void parseServerDirective(const std::string& name, const std::string& value); // Parse une directive qui appartient au bloc server
	void parseLocationDirective(const std::string& name, const std::string& value); // Parse une directive qui appartient au bloc location

	// Parsers spécifiques pour certains types de directives
	void parseListen(const std::string& value); //Parse "listen 8080" ou "listen 127.0.0.1:8080" : remplit_currentServer.listen_port et _currentServer.ip_address
	void parseErrorPage(const std::string& value); // Parse "error_page 404 /error/404.html"  ou "error_page 500 502 503 /error/50x.html" (plusieurs codes)
	void parseAllowedMethods(const std::string& value); //Parse "allowed_methods GET POST DELETE"
	void parseCgiExtension(const std::string& value); // Parse "cgi_extension .py /usr/bin/python3"
	void parseReturn(const std::string& value); //Parse "return 301 https://www.google.com"
	size_t parseSize(const std::string& sizeStr); //Parse "10M" ou "1024" ou "5K" en bytes et retourne nombre de bytes (10M = 10485760)

	// Utils
	std::string trim(const std::string& str); // Enlève les espaces au début et à la fin d'une string
	std::string extractLocationPath(const std::string& line);  // Extrait le path de "[location /uploads]" → "/uploads"

	//Validation
	void validateConfig();   // Valide la configuration finale après parsing (ports valides, redirections correctes, etc.)
	void validateServer(const ServerConfig& server);  // Vérifie qu'un ServerConfig est valide ( port entre 1 et 65535, root défini, etc.)


public:
	ConfigParser(); // Constructeur par défaut   
	~ConfigParser();

	// Méthode principale
	// Ouvre le fichier, lit ligne par ligne, remplit _servers
	// Lance une exception si le fichier n'existe pas ou si erreur de parsing
    void parse(const std::string& filename);
    const std::vector<ServerConfig>& getServers() const; // Retourne la liste des serveurs parsés. À appeler après parse()
};

#endif
