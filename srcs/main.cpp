#include "../includes/config/ConfigParser.hpp"
#include <iostream>

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: ./test_config <config_file>" << std::endl;
        return 1;
    }

    try {
        ConfigParser parser;
        parser.parse(argv[1]);
        
        const std::vector<ServerConfig>& servers = parser.getServers();
        
        std::cout << "=== Parsing OK ===" << std::endl;
        std::cout << "Number of servers: " << servers.size() << std::endl;
        
        for (size_t i = 0; i < servers.size(); i++) {
            std::cout << "\n--- Server " << i + 1 << " ---" << std::endl;
            std::cout << "Listen: " << servers[i].ip_address << ":" << servers[i].listen_port << std::endl;
            std::cout << "Server name: " << servers[i].server_name << std::endl;
            std::cout << "Root: " << servers[i].root_dir << std::endl;
            std::cout << "Max body size: " << servers[i].max_body_size << std::endl;
            std::cout << "Locations: " << servers[i].locations.size() << std::endl;
            
            for (size_t j = 0; j < servers[i].locations.size(); j++) {
                const LocationConfig& loc = servers[i].locations[j];
                std::cout << "  [" << loc.path_url << "]" << std::endl;
                std::cout << "    root: " << loc.root_dir << std::endl;
                std::cout << "    index: " << loc.index << std::endl;
                std::cout << "    methods: ";
                for (size_t k = 0; k < loc.methods.size(); k++) {
                    std::cout << loc.methods[k] << " ";
                }
                std::cout << std::endl;
                std::cout << "    autoindex: " << (loc.autoindex ? "on" : "off") << std::endl;
                
                // Afficher les CGI handlers
                if (!loc.cgi_handlers.empty()) {
                    std::cout << "    cgi_handlers:" << std::endl;
                    std::map<std::string, std::string>::const_iterator it;
                    for (it = loc.cgi_handlers.begin(); it != loc.cgi_handlers.end(); ++it) {
                        std::cout << "      " << it->first << " -> " << it->second << std::endl;
                    }
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}