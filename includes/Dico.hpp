#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <sys/types.h>

/* LOCATION CONFIG
Représente un bloc "location" dans le fichier de config.
C'est ici qu'on définit les règles pour chaque URL du serveur.

Exemple dans le fichier de config :
    location /upload {
       allowed_methods POST GET;
       upload_store ./www/uploads;
    }
*/

struct LocationConfig {

	std::string path_url; // Chemin URL configuré dans le fichier de config

    std::string root_dir; // Dossier sur le disque où chercher les fichiers
					      // Si la location est "/images" et root "./www",
					      // une requête vers "/images/pic.jpg" cherche "./www/pic.jpg"

    std::vector<std::string> methods; // Liste des méthodes HTTP autorisées : "GET", "POST", "DELETE"

    std::string index; // Fichier par défaut quand on demande un dossier
                       // Si le client demande "/" on lui renvoie "/index.html"

    bool autoindex;  // Si true et pas de fichier index, on liste le contenu du dossier
					  // Si false et pas de fichier index, on renvoie erreur 404

    std::string cgi_path; // Chemin vers l'interpréteur CGI (ex: "/usr/bin/python3")
						 // Si défini, les fichiers sont exécutés au lieu d'être renvoyés

    std::string cgi_extension;  // Extension des fichiers CGI (ex: ".py", ".php")
							   // Seuls les fichiers avec cette extension sont traités en CGI			

    std::string upload_dir;  // Dossier où sauvegarder les fichiers uploadés via POST

    std::string redirect_url; // URL vers laquelle rediriger (ex: "/new-page")
							 // Si défini, le serveur renvoie une redirection HTTP

    int redirect_code;  // Code de redirection : 301 (permanent) ou 302 (temporaire)

    size_t max_body_size; // Taille max du body acceptée en bytes
					      // Si le client envoie plus, on renvoie erreur 413

    // Constructeur avec valeurs initialisées par défaut
    LocationConfig() :
		path_url(""),
		root_dir(""),
   		index("index.html"),
    	autoindex(false),
    	cgi_path(""),
    	cgi_extension(""),
    	upload_dir(""),
		redirect_url(""),
    	redirect_code(0),
    	max_body_size(1048576) // 1 MB par défaut
    {
        methods.push_back("GET"); // Par défaut, méthode GET est autorisée
    }
};


/*  SERVER CONFIG
Représente un bloc "server" dans le fichier de config.
Un serveur écoute sur un port et contient plusieurs locations.

Exemple dans le fichier de config :
    server {
       listen 8080;
       server_name localhost;
       root ./www;
       location / { ... }
   }
*/

struct ServerConfig {

    std::string ip_address; // Adresse IP sur laquelle écouter
    					    // "0.0.0.0" = toutes les interfaces (accessible de l'extérieur)
                            // "127.0.0.1" = seulement en local

    int listen_port; // Port d'écoute (ex: 8080)
    				// Le client se connecte avec http://localhost:8080/

    std::string server_name; // Nom de domaine du serveur (ex: "localhost", "example.com")

    std::string root_dir; // Dossier racine par défaut
    					 // Utilisé si une location ne définit pas son propre root
 
    std::map<int, std::string> error_pages; // Pages d'erreur personnalisées
    										// Clé = code d'erreur, Valeur = chemin du fichier HTML
    										// Exemple : error_pages[404] = "/errors/404.html"

    size_t max_body_size; // Taille max du body par défaut pour ce serveur

    std::vector<LocationConfig> locations; // Liste des locations (routes) configurées

    // Constructeur avec valeurs initialisées par défaut
    ServerConfig() :
		ip_address("0.0.0.0"),
        listen_port(8080),
        server_name(""),
        root_dir("./www"),
        max_body_size(1048576)
    {}
};


/* REQUEST STATE
États du parsing d'une requête HTTP.
Le parsing se fait en plusieurs étapes car les données arrivent par morceaux.

Une requête HTTP ressemble à ça :

POST /upload HTTP/1.1      <- REQUEST_LINE
Host: localhost            <- HEADERS
Content-Length: 5          <- HEADERS
                           <- Ligne vide = fin des headers
Hello                      <- BODY
*/

enum RequestState {
    REQUEST_LINE,    // On lit "GET /index.html HTTP/1.1"
    HEADERS,         // On lit les headers
    BODY,            // On lit le body
    CHUNKED_BODY,    // Body en mode chunked (par morceaux)
    COMPLETE,        // Requête complète, prête à traiter
    ERROR            // Requête invalide
};


/* REQUEST
Représente une requête HTTP envoyée par le client.

Quand le client envoie :

GET /search?q=hello HTTP/1.1
Host: localhost

On obtient :
method = "GET"
uri = "/search"
query_string = "q=hello"
headers["Host"] = "localhost"
*/

struct Request {

    
    std::string method; // Méthode HTTP : "GET", "POST" ou "DELETE"
						// une seule méthode par requête

    std::string uri; // URL demandé par le client
	                 //Chemin demandé sans les paramètres
    				// Si l'URL est "/search?q=hello", uri = "/search"

    std::string query_string;  // Paramètres après le "?" dans l'URL (paramètres de recherche)
							   // Si l'URL est "/search?q=hello", query_string = "q=hello"

    std::string version;  // Version HTTP, généralement "HTTP/1.1"
    
    std::map<std::string, std::string> headers; // Headers HTTP clé/valeur
												// Ex: headers["Host"] = "localhost"
												// Ex: headers["Content-Length"] = "1234"

    std::string body;  // Corps de la requête (données POST)
					   // Vide pour GET, contient les données pour POST

    size_t content_length;   // Taille du body annoncée dans Content-Length header
							// Permet de savoir quand on a fini de lire

    bool is_chunked; // Le body arrive par morceaux, taille inconnue à l'avance

    RequestState state; // État actuel du parsing

    std::string read_buffer; // Buffer des données pas encore parsées

    size_t body_bytes_received; // Bytes du body déjà reçus
						  // Quand body_received == content_length, c'est fini

    int error_code;   // Code d'erreur si requête invalide (0 = pas d'erreur)

    Request():
		method(""),
        uri(""),
        query_string(""),
        version("HTTP/1.1"),
        content_length(0),
        is_chunked(false),
        state(REQUEST_LINE),
        body_bytes_received(0),
        error_code(0)
    {}

    // Réinitialise pour une nouvelle requête (keep-alive)
    // Avec keep-alive, un client peut envoyer plusieurs requêtes sur la même connexion :
    // Connexion TCP établie (socket_fd = 5, server_config = config du port 8080)
    //│
    //├── Requête 1 : GET /index.html → Réponse 1
    //│   reset() ← on vide request/response mais on garde la connexion
    //│
    //├── Requête 2 : GET /style.css → Réponse 2
    //│   reset() ← on vide request/response mais on garde la connexion
    //│
    //├── Requête 3 : GET /image.png → Réponse 3
    //│   reset() ← on vide request/response mais on garde la connexion
    //│
    //└── Connexion fermée

    void reset() {
        method.clear();          // Vider la méthode
        uri.clear();             // Vider l'URI
        query_string.clear();    // Vider les paramètres
        version = "HTTP/1.1";    // Remettre la version par défaut
        headers.clear();         // Vider tous les headers
        body.clear();            // Vider le body
        content_length = 0;      // Remettre la longueur du body à 0, pas de body attendu
        is_chunked = false;      // Pas de mode chunked
        state = REQUEST_LINE;    // Recommencer le parsing au début par la ligne de requête
        read_buffer.clear();     // Vider le buffer de lecture
        body_bytes_received = 0;  // Rien reçu encore
        error_code = 0;           // Pas d'erreur
    }
};


/* RESPONSE
Représente une réponse HTTP à envoyer au client.

Format d'une réponse :

HTTP/1.1 200 OK             <- Status line
Content-Type: text/html     <- Headers
Content-Length: 13          <- Headers
                            <- Ligne vide
Hello World!                <- Body
*/

struct Response {

    int status_code;  // Code de statut HTTP (200, 404, 500, etc.)

    std::string status_message;  // Message associé au code ("OK", "Not Found", etc.)

    std::map<std::string, std::string> headers; // Headers de la réponse
    											// Ex: headers["Content-Type"] = "text/html"

    std::string body; // Corps de la réponse (HTML, fichier, JSON, etc.)

    std::string send_buffer; // Réponse complète prête à envoyer avec write()
                            // Contient : status line + headers + ligne vide + body

    size_t bytes_sent;  // Bytes déjà envoyés (write peut ne pas tout envoyer d'un coup)

    bool is_ready; // True si send_buffer est construit et prêt

    bool is_complete; // True si tout a été envoyé

    Response():
        status_code(200),
        status_message("OK"),
        bytes_sent(0),
        is_ready(false),
        is_complete(false)
    {}

    void reset() {
        status_code = 200;
        status_message = "OK";
        headers.clear();
        body.clear();
        send_buffer.clear();
        bytes_sent = 0;
        is_ready = false;
        is_complete = false;
    }
};


/*CLIENT STATE

États d'une connexion client.

Cycle de vie normal :
CLIENT_READING -> CLIENT_PROCESSING -> CLIENT_WRITING -> CLIENT_DONE

Si CGI :
CLIENT_READING -> CLIENT_PROCESSING -> CLIENT_WAITING_CGI -> CLIENT_WRITING -> CLIENT_DONE
*/

enum ClientState {
    CLIENT_READING,      // On reçoit la requête
    CLIENT_PROCESSING,   // On traite la requête
    CLIENT_WAITING_CGI,  // On attend le script CGI
    CLIENT_WRITING,      // On envoie la réponse
    CLIENT_DONE,         // Terminé
    CLIENT_ERROR         // Erreur, on ferme
};


/*CLIENT DATA

Contient toutes les infos sur une connexion client.
Chaque client connecté a sa propre instance de ClientData.
*/

struct ClientData {

    int socket_fd; // File descriptor de la socket (numéro retourné par accept)

    ClientState state; // État actuel de la connexion

    Request request; // La requête reçue de ce client

    Response response; // La réponse à envoyer à ce client

    ServerConfig* server_config; // Pointeur vers la config du serveur sur lequel le client s'est connecté

    LocationConfig* location_config; // Pointeur vers la location qui correspond à l'URL demandée

    pid_t cgi_pid; // PID du processus CGI (-1 si pas de CGI en cours)

    int cgi_pipe_out; // Pipe pour lire la sortie du CGI (-1 si pas de CGI)

    std::string cgi_buffer;  // Buffer pour accumuler la sortie du CGI

    time_t last_activity;  // Timestamp de la dernière activité (pour timeout)

    ClientData(): 
        socket_fd(-1),
        state(CLIENT_READING),
        server_config(NULL),
        location_config(NULL),
        cgi_pid(-1),
        cgi_pipe_out(-1),
        last_activity(0)
    {}

    // Réinitialise pour une nouvelle requête (keep-alive)
    // On garde socket_fd et server_config car ne changent pas entre les requêtes (même connexion et même server)
    void reset() {
        state = CLIENT_READING;
        request.reset();
        response.reset();
        location_config = NULL;
        cgi_pid = -1;
        cgi_pipe_out = -1;
        cgi_buffer.clear();
        last_activity = time(NULL);
    }
};


/* CONSTANTES
Valeurs constantes utilisées partout dans le projet.
Regroupées ici pour les modifier facilement.
*/

namespace WebservConfig {
    const int TIMEOUT_CLIENT = 60;           // Secondes avant de déconnecter un client inactif
    const int TIMEOUT_CGI = 30;              // Secondes avant de kill un CGI qui ne répond pas
    const size_t BUFFER_SIZE = 4096;         // Taille des buffers read/write (4 KB)
    const size_t MAX_HEADER_SIZE = 8192;     // Taille max des headers (8 KB)
    const size_t MAX_URI_LENGTH = 2048;      // Longueur max de l'URI (2 KB)
    const size_t DEFAULT_MAX_BODY = 1048576; // Taille max du body par défaut (1 MB)
    const int DEFAULT_PORT = 8080;           // Port par défaut
    const int MAX_CONNECTIONS = 1024;        // Nombre max de clients simultanés
}


/*CODES HTTP
Codes de statut HTTP standards.
On utilise HttpStatus::NOT_FOUND au lieu de 404 pour plus de clarté.
*/

namespace HttpStatus {
    // Succès
    const int OK = 200;                      // Requête réussie
    const int CREATED = 201;                 // Ressource créée (POST réussi)
    const int NO_CONTENT = 204;              // Succès sans contenu (DELETE réussi)

    // Redirection
    const int MOVED_PERMANENTLY = 301;       // Redirection permanente
    const int FOUND = 302;                   // Redirection temporaire

    // Erreur client
    const int BAD_REQUEST = 400;             // Requête malformée
    const int FORBIDDEN = 403;               // Accès interdit
    const int NOT_FOUND = 404;               // Ressource introuvable
    const int METHOD_NOT_ALLOWED = 405;      // Méthode non autorisée
    const int REQUEST_TIMEOUT = 408;         // Client trop lent
    const int PAYLOAD_TOO_LARGE = 413;       // Body trop gros
    const int URI_TOO_LONG = 414;            // URI trop longue

    // Erreur serveur
    const int INTERNAL_SERVER_ERROR = 500;   // Bug du serveur
    const int NOT_IMPLEMENTED = 501;         // Fonctionnalité non implémentée
    const int BAD_GATEWAY = 502;             // Erreur avec le CGI
    const int GATEWAY_TIMEOUT = 504;         // CGI trop lent
}

#endif