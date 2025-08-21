#include"groupfileserver.h"
class Friendserver:public Chat,public FileTransferServer
{
// public:
//     mutex online_mutex;
//     unordered_map<string, int> online_users; // 在线用户表
public:
    Connection* getDBConnection() ;
    void handleAddFriend(int fd, const json& req) ;
    // 处理查看请求列表
    void handleCheckRequests(int fd, const json& req) ;
    // 处理请求操作
    void handleProcessRequest(int fd, const json& req) ;
    //展示好友列表
    void handleGetFriends(int fd, const json& req);
    void handleDeleteFriend(int fd, const json& req);
    void handleBlockUser(int fd, const json& req);
    void handledisBlockUser(int fd, const json& req);

};