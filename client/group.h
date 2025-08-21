#pragma once
#include "groupchat.h"
#include "groupfile.h"
class Group :public  groupchat,public groupfile
{
    private:
    int sock;
    std::string currentUser;

    json sendRequest(const json& req);
    public:
    void groupMenu(int sockfd, std::string current_user) ;
    void createGroup();
    bool is_number(const std::string& s);
    void disbandGroup();
    void joinGroup() ;
    void showMyGroups();
    void viewGroupInfo();
    void leaveGroup();
    // 管理群成员 (群主/管理员权限)
    void manageGroupMembers() ;
    void processJoinRequests(int groupId) ;
    // 添加群管理员 (群主权限)
    void addGroupAdmin(int groupId) ;
    // 移除群管理员 (群主权限)
    void removeGroupAdmin(int groupId) ;
    // 移除群成员 (群主/管理员权限)
    void removeGroupMember(int groupId) ;


};