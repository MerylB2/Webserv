#include "SessionManager.hpp"

SessionManager* SessionManager::instance = NULL;

SessionManager::SessionManager()
{
    srand(time(NULL));
    std::cout << "[SESSION] SessionManager initialise" << std::endl;
}

//Obtenir l'instance unique
SessionManager* SessionManager::getInstance()
{
    if (instance == NULL)
    {
        instance = new SessionManager();
    }
    return instance;
}

//Generer un ID de session unique
std::string SessionManager::generateSessionId()
{
    static const char chars[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

    std::stringstream ss;
    ss << "sess_";

    // Generer 32 caracteres aleatoires
    for (int i = 0; i < 32; i++)
    {
        ss << chars[rand() % 62];
    }

    return ss.str();
}

//Creer une nouvelle session
std::string SessionManager::createSession(const std::string& user)
{
    std::string session_id = generateSessionId();

    SessionData data;
    data.user = user;
    data.created_at = time(NULL);
    data.last_activity = time(NULL);

    sessions[session_id] = data;
    return session_id;
}

//Recuperer une session existante
SessionData* SessionManager::getSession(const std::string& session_id)
{
    std::map<std::string, SessionData>::iterator it = sessions.find(session_id);

    if (it != sessions.end())
    {
        it->second.last_activity = time(NULL);
        return &(it->second);
    }

    return NULL;
}

bool SessionManager::validateSession(const std::string& session_id)
{
    return sessions.find(session_id) != sessions.end();
}

//Detruire une session
void SessionManager::deleteSession(const std::string& session_id)
{
    sessions.erase(session_id);
}

//Liberer le singleton
void SessionManager::destroy()
{
    delete instance;
    instance = NULL;
}

//Nettoyer les sessions expirees
void SessionManager::cleanupExpiredSessions(time_t timeout_seconds)
{
    time_t now = time(NULL);
    std::map<std::string, SessionData>::iterator it = sessions.begin();
    
    while (it != sessions.end()) 
    {
        if (now - it->second.last_activity > timeout_seconds)
            sessions.erase(it++);  // Supprimer et avancer l'itérateur
        else
            ++it;
    }
}

