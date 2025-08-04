#include <iostream>
#include <string>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <mysql/mysql.h>
#include <nlohmann/json.hpp>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include"threadpool.h"
#include </usr/include/mysql_driver.h>       // MySQL驱动头文件
#include <mysql_connection.h>  // 连接类头文件
#include <cppconn/prepared_statement.h> // 预处理语句
#include </usr/include/x86_64-linux-gnu/curl/curl.h>
#include <time.h>
#include <fstream>
#include <openssl/sha.h>  // 提供 SHA256_CTX 定义
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>
#include <openssl/buffer.h>
#include <sys/stat.h>
using json = nlohmann::json;
using namespace std;


// 全局通知存储
std::mutex notifications_mutex;
std::unordered_map<std::string, std::vector<json>> user_notifications;
void sendLengthPrefixed(int fd, const json& response) {
    std::string responseStr = response.dump();
    uint32_t len = htonl(responseStr.size());
    send(fd, &len, sizeof(len), 0);
    send(fd, responseStr.c_str(), responseStr.size(), 0);
}
void handlePollNotifications(const json& request,int client_sock) {
    
    std::string username = request["username"];
    
    json response;
    response["type"] = "poll_notifications_response";
    
    // 获取并清空用户通知
    std::vector<json> notifications;
    {
        std::lock_guard<std::mutex> lock(notifications_mutex);
        
        if (user_notifications.find(username) != user_notifications.end()) {
            notifications = user_notifications[username];
            user_notifications[username].clear();
        }
    }
    
    if (notifications.empty()) {
        response["success"] = true;
        response["message"] = "没有新通知";
    } else {
        response["success"] = true;
        response["notifications"] = notifications;
    }
    
    // 发送响应
    // std::string response_str = response.dump();
    // send(client_sock, response_str.c_str(), response_str.size(), 0);
    sendLengthPrefixed(client_sock,response);
}
void addNotification(const std::string& username, const json& notification) 
{
    cout<<"notify"<<endl;
    std::lock_guard<std::mutex> lock(notifications_mutex);
    
    // 确保用户通知队列存在
    if (user_notifications.find(username) == user_notifications.end()) {
        user_notifications[username] = std::vector<json>();
    }
    
    // 添加时间戳
    // json notif_with_timestamp = notification;
    //notif_with_timestamp["timestamp"] = static_cast<uint64_t>(time(nullptr));
    
    user_notifications[username].push_back(notification);
}