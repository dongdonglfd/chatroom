#include "chatserver.h"
extern unordered_map<string, bool> grouping;
class groupchat:public Chat
{
    Chat group;
private:
    
    int sock;
    mutex groupMutex;
    mutex session;
    json sendRequest(const json& request);
public:
    void handleGroupMessage(int client_sock, const json& data);
    void broadcastToGroup(int group_id, const json& message, const string& excludeUser);
    void storeOfflineMessage(const string& recipient, int group_id, const json& message);
    void checkgroupUnreadMessages(int fd, const json& req);
    void handleGetGroupUnreadMessages(int client_sock, const json& data);
    void handleGetGroupHistory(int client_sock, const json& data);
    
};