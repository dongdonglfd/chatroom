#pragma once
#include"chatclient.h"
#include"fileclient.h"

class Friend
{
    private:
    int sock;
    std::string currentUser;        // 当前登录用户
    std::vector<std::string> friendRequests; // 待处理的好友请求
    Chat chat;
    FileTransferClient file;
    // 发送请求并获取响应
    json sendRequest(const json& req);
    json sendReq(const json& req,int fd) ;
   public: json requestlength(const json& req);
    void friendMenu(int sockfd,std::string current_user) ;
    void block();
    void addFriend();
    void deleteFriend();
    void blockFriend();
     void disblockFriend();
    void showFriends();
    void processRequest();
    // 处理接收到的请求（后台线程）
    void checkNotifications();
    // 显示待处理请求
    void showPendingRequests() ;
    void checkUnreadMessages(std::string currentUser, int sockfd) ;
    void checkgroupUnreadMessages(std::string currentUser, int sockfd);

};
