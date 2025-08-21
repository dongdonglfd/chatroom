#pragma once
#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<string>
#include <termios.h> // 密码输入处理
#include <iomanip>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include<vector>
#include <map>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>  
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <fcntl.h>
using json = nlohmann::json;
using namespace std;
#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_EVENTS 10
extern std::thread notification_thread;
extern mutex output_mutex;
extern string notifyaddress;
extern std::mutex notification_mutex;
extern bool inChatSession ;
 extern   string activeRecipient;
 extern   string inputBuffer;
//std::mutex output_mutex;
extern std::queue<std::string> notification_queue;
extern std::condition_variable notification_cv;

// 线程控制
extern std::atomic<bool> notification_thread_running;
extern mutex thread_mutex;
extern  mutex outputMutex;
extern std::mutex valid;

extern int sockfd;
extern std::string current_user;
extern void sendLengthPrefixed(int fd, const json& response) ;
extern std::string receiveLengthPrefixed();
extern void notificationPollingThread();
extern void notificationstart();
extern void notificationstop();
class Chat
{
private:
    
    // 全局变量
    int sock = -1;           // 已连接的套接字描述符
    string currentUser = "";    // 当前登录的用户名
    std::atomic<bool> running=true; // 控制程序运行的标志
    thread recvThread;          // 接收消息的线程
    //mutex outputMutex;          // 保护输出操作的互斥锁

    // 用于聊天会话的状态变量
    // bool inChatSession = false;
    // string activeRecipient;
    // string inputBuffer;
        
    
public:

    // 保存原始终端设置
    struct termios origTermios;

    // 设置终端为非阻塞模式
    void setNonBlockingTerminal() ;
    void setRawTerminalMode() ;
    // 恢复终端设置
    void restoreTerminal() ;
    //发送请求到服务器
    json sendRequest(const json& request) ;
     json sendReq(const json& req) ;
    void mainMenu() ;
    string readLineNonBlocking() ;
    std::string repairUtf8(const std::string& input);
bool isValidUtf8Char(const unsigned char* bytes, size_t len) ;
// 验证整个字符串是否为有效的 UTF-8
bool isValidUtf8(const std::string& str);

    void startChatSession(const string& friendName) ;
    // 发送消息给指定用户
    void sendMessage(const string& recipient, const string& text) ;
    /////////////////////////////////////////////////////////////////
    void receiveMessages(); 
    void privateChat(int sockfd,std::string current_user);
    
    int isValidFriend(const std::string& friendName) ;
    void displayUnreadMessagesFromFriend(const string& friendName);
    
    void queryChatHistory(const string& friendName);

    void processChatHistory(const json& response);

};