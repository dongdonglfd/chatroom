#include "notification.h"

using namespace std;
using json = nlohmann::json;
using namespace sql;

// 数据库配置
const string DB_HOST = "tcp://127.0.0.1:3306";
const string DB_USER = "chatuser";   // 数据库账户名
const string DB_PASS = "123";  // 数据库账户密码
const string DB_NAME = "chat";
struct Message {
    std::string sender;
    std::string receiver;
    std::string content;
    time_t timestamp;
    uint64_t message_id;
    bool delivered = false;
};
extern unordered_map<string, int> online_users; // 在线用户表
extern unordered_map<string, bool> onlineing;
 
//  void sendLengthPrefixed(int fd, const json& response) {
//     std::string responseStr = response.dump();
//     uint32_t len = htonl(responseStr.size());
//     send(fd, &len, sizeof(len), 0);
//     send(fd, responseStr.c_str(), responseStr.size(), 0);
// }
class Chat
{
    public:
     mutex online_mutex;
    //  unordered_map<string, int> online_users; // 在线用户表
     //std::unordered_map<std::string, UserSession> online_users;   // 在线用户
    std::mutex online_users_mutex;
    
    std::map<std::string, std::deque<Message>> messages_store;   // 按会话ID存储的消息
    std::map<std::string, uint64_t> unread_counts;               // 用户未读消息计数
    std::mutex data_mutex;

    // 连接管理
    std::unordered_map<int, std::string> connection_user_map;    // socket->username
    std::mutex connection_mutex;
    public:
    void sendResponse(int fd, const json& response) ;
    Connection* getDBConnection();
    // 添加用户到在线列表
    void userOnline(const std::string& username, int socket) ;
    void handleCheckFriendValid(int fd, const json& req) ;
    void handlePrivateMessage(int fd,const json& request);
    void handleAckPrivateMessage(int fd, const json& req);
    void checkUnreadMessages(int fd, const json& req);
    void handleGetFriendUnreadMessages(int fd, const json& req);
    
    void handleChatHistoryRequest(int fd, const json& request);
};
