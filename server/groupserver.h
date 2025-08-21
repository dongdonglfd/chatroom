#include "Friendserver.h"
// #include "groupchatserver.h"
class groupserver: public groupchat
{
    public:
    Friendserver mysql;
    
    void handleCreateGroup(int fd, const json& req);
    void handleGetMyGroups(int fd, const json& req);
    void handleDisbandGroup(int fd,const json& req);
    void handleLeaveGroup(int fd,const json& req);
    void handleGetUserRole(int fd, const json& req);
    void handleJoinGroupRequest(int fd, const json& req);
    void handleGetGroupJoinRequests(int fd, const json& req) ;
    void handleProcessJoinRequest(int fd, const json& req);
    void handleGetGroupInfo(int fd, const json& req) ;
    void handleAddGroupAdmin(int fd, const json& req);
    void handleRemoveGroupAdmin(int fd, const json& req);
    void handleRemoveGroupMember(int fd, const json& req);
    void handleAckGroupMessage(int client_sock, const json& data);
        

    

};