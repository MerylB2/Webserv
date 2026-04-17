#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include "Dico.hpp"
#include <string>
#include <map>
#include <ctime>
#include <cstdlib>

class SessionManager {
    private :
        std::map<std::string, SessionData> sessions; // session_id -> SessionData
        static SessionManager* instance;

        SessionManager(); //constructeur prive

        // Empêcher la copie (DÉCLARATION SEULEMENT, PAS D'IMPLÉMENTATION)
        SessionManager(const SessionManager&);
        SessionManager& operator=(const SessionManager&);

        //Generer un ID de session unique
        std::string generateSessionId();

    public :
        //Obtenir l'instance unique
        static SessionManager* getInstance();

        //Creer une nouvelle session
        std::string createSession(const std::string& user);

        //Recuperer une session existante
        SessionData* getSession(const std::string& session_id);

        //Verifie qu'une session existe
        bool validateSession(const std::string& session_id);

        //Detruire une session
        void deleteSession(const std::string& session_id);

        //Nettoyer les sessions expirees
        void cleanupExpiredSessions(time_t timeout_seconds = 3600);

        //Liberer le singleton
        static void destroy();
};

#endif