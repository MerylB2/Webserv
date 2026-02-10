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

    public :
        //Obtenir l'instance unique
        static SessionManager* getInstance();

        //Creer une nouvelle session
        std::string createSession(const std::string& user);

        //Recuperer une session existante
        SessionData* getSession(const std::string& session_id);

        //Mettre a jour l'activite d'une session
        void updateActivity(const std::string session_id);

        //Detruire une session
        void destroySession(const std::string session_id);

        //Nettoyer les sessions expirees
        void cleanupExpiredSessions(int timeout_secons = 3600);

        //Generer un ID de session unique
        static std::string generateSessionId();
};

#endif