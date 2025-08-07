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
std::thread notification_thread;
mutex output_mutex;
string notifyaddress;
std::mutex notification_mutex;
bool inChatSession = false;
    string activeRecipient;
    string inputBuffer;
//std::mutex output_mutex;
std::queue<std::string> notification_queue;
std::condition_variable notification_cv;

// 线程控制
std::atomic<bool> notification_thread_running(false);
mutex thread_mutex;
 mutex outputMutex;

int sockfd;
std::string current_user;
void sendLengthPrefixed(int fd, const json& response) {
    std::string responseStr = response.dump();
    uint32_t len = htonl(responseStr.size());
    send(fd, &len, sizeof(len), 0);
    send(fd, responseStr.c_str(), responseStr.size(), 0);
}
std::string receiveLengthPrefixed() {
    // 接收长度
    uint32_t len;
    recv(sockfd, &len, sizeof(len), MSG_WAITALL);
    len = ntohl(len);
    // 接收数据
    std::vector<char> buffer(len);
    size_t totalReceived = 0;
    
    while (totalReceived < len) {
        ssize_t n = recv(sockfd, buffer.data() + totalReceived, len - totalReceived, 0);
        if (n <= 0) break;
        totalReceived += n;
    }
    
    return std::string(buffer.data(), len);
}
void notificationPollingThread()
{
    std::lock_guard<std::mutex> lock(output_mutex);
    const int POLL_INTERVAL = 1;     
    while (notification_thread_running) {
        // 构造轮询请求
        json request = {
            {"type", "poll_notifications"},
            {"username", current_user}
        };
        
        // 发送请求
        std::string request_str = request.dump();
        send(sockfd, request_str.c_str(), request_str.size(), 0);
        string buffer=receiveLengthPrefixed();
        // // 接收响应
        // char buffer[4096];
        // ssize_t bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        
        if (buffer.size() > 0) {
            //buffer[bytes_read] = '\0';
            //cout<<"123123"<<endl;
            json response = json::parse(buffer);
            if (response["success"]) {
                    // 使用互斥锁保护整个输出块
                    
                    
                    for (const auto& notification : response["notifications"]) {
                        std::string type = notification["type"];
                        
                        if (type == "friend_request") {
                            std::string requester = notification["requester"];
                            std::cout << "\n[系统通知] 收到好友请求来自: " << requester << std::endl;
                        } else if (type == "group_join_request") {
                            int group_name = notification["group_id"];
                            std::string inviter = notification["applicant"];
                            std::cout << "\n[系统通知] 收到群组申请消息: " <<"群号"<< group_name << " 来自: " << inviter << std::endl;
                        } 
                        else if (type == "new_message") {
                            std::string sender = notification["sender"];
                            //std::string content = notification["content"];
                            std::cout << "\n[新消息] " <<"来自"<<sender << std::endl;
                        } 
                        else if(type=="new_groupmessage"){
                            int groupid=notification["groupid"];
                            cout<<"[群消息]来自"<<groupid<<endl;
                        }
                        /////////////////////////////////////
                    //     if (type == "private_message") 
                    // {
                        
                    //     // 处理私聊消息
                    //     string sender = notification.value("sender", "");
                    //     string text = notification.value("message", "");
                    //     time_t timestamp = notification["timestamp"];
                    //     tm* localTime = localtime(&timestamp);
                    //     char timeStr[80];
                    //     strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", localTime);
            
                        
                    //     // 显示消息（使用互斥锁保护输出）
                    //     {
                    //         lock_guard<mutex> lock(outputMutex);
                    //         // 如果正在与发送者聊天，直接显示消息
                    //         if (inChatSession && activeRecipient == sender) {
                    //             cout << "\n[" << timeStr << "] " << sender << ": " << text << endl;
                    //             cout << "> " << inputBuffer << flush; // 重新显示输入提示
                    //         }
                    //         // //否则显示通知
                    //         // else {
                    //         //     cout << "\n您收到来自 " << sender << " 的新消息 (" 
                    //         //         << (text.length() > 20 ? text.substr(0, 20) + "..." : text) 
                    //         //         << ") [" << timeStr << "]" << endl;
                    //         //     if (inChatSession) {
                    //         //         cout << "> " << inputBuffer << flush; // 重新显示输入提示
                    //         //     }
                    //         // }
                    //     }

                    //     // 发送消息确认（如果提供了消息ID）                                                 
                    //     if (notification.contains("message_id")) {
                    //         json ack = {
                    //             {"type", "ack_private_message"},
                    //             //{"message_id", message["message_id"]},
                    //             {"user", current_user},
                    //             {"friend",sender}
                    //         };
                    //         //sendRequest(ack);
                    //         string str=ack.dump();
                    //         send(sockfd,&str,str.size(),0);
                    //     }
                    // }
                        ////////////////////////////////////
                    }
                    
                }
        }
        
        // 等待轮询间隔
        for (int i = 0; i < POLL_INTERVAL && notification_thread_running; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}
void notificationstart()
{
    
       std::lock_guard<std::mutex> lock(thread_mutex);
        notification_thread_running = true;
        notification_thread = std::thread(notificationPollingThread);
    
}
void notificationstop()
{
    
        
        std::lock_guard<std::mutex> lock(thread_mutex);
        if (notification_thread.joinable()) {
            notification_thread_running = false;
            notification_thread.join();
        }
    
}
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
    void setNonBlockingTerminal() {
        struct termios newTermios;
        tcgetattr(STDIN_FILENO, &origTermios);
        newTermios = origTermios;
        newTermios.c_lflag &= ~(ICANON | ECHO); // 禁用规范模式和回显
        newTermios.c_cc[VMIN] = 0;  // 非阻塞读取
        newTermios.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
    }
    void setRawTerminalMode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
    // 恢复终端设置
    void restoreTerminal() {
        tcsetattr(STDIN_FILENO, TCSANOW, &origTermios);
    }

    //发送请求到服务器
    json sendRequest(const json& request) {
        // 序列化请求为JSON字符串
        string requestStr = request.dump();
        
        //发送请求到服务器
        {
            lock_guard<mutex> lock(outputMutex);
            if (send(sock, requestStr.c_str(), requestStr.size(), 0) < 0) {
                cerr << "发送请求失败" << endl;
                return {{"success", false}, {"message", "发送请求失败"}};
            }
        }
        // std::vector<char> data(requestStr.size()+4);
        // int _len = htonl(requestStr.size());
        
        // memcpy(data.data(),&_len,4);
        // memcpy(data.data()+4,requestStr.c_str(),requestStr.size());
        // int ret = send(sock, data.data(),data.size(),0);
        
        // 对于不需要即时响应的请求直接返回
        return {{"success", true}};
    }
     json sendReq(const json& req) {
        std::string requestStr = req.dump();
        send(sock, requestStr.c_str(), requestStr.size(), 0);

        char buffer[4096] = {0};
        recv(sock, buffer, 4096, 0);
        return json::parse(buffer);
    }
    void mainMenu() 
    {
        while (running) {
            // 显示主菜单
            cout << "\n===== 聊天系统主菜单 =====" << endl;
            cout << "1. 开始聊天" << endl;
            cout << "2. 查看历史聊天记录" << endl;
            cout << "3. 退出系统" << endl;
            cout << "==========================" << endl;
            cout << "请选择操作: ";
            
            string choice;
            getline(cin, choice);
            
            if (choice == "1") {
                // 开始聊天
                cout << "请输入对方用户名: ";
                string friendName;
                getline(cin, friendName);
                if (!friendName.empty()) {
                    startChatSession(friendName);
                }
            }
            else if (choice == "2") 
            {
                cout << "=====查找聊天记录=====: "<<endl;
                cout<<"请输入对方用户名: ";
                string friendName;
                getline(cin, friendName);
                
                if (!friendName.empty()) {
                    queryChatHistory(friendName);
                }
            } 
            else if (choice == "3") {
                // 退出系统
                cout << "感谢使用，再见!" << endl;
                running = false;
                break;
            } else {
                cout << "无效选择，请重新输入!" << endl;
            }
        }
    }
    string readLineNonBlocking() 
    {
   
        char buffer[BUFFER_SIZE];
        string input;
        bool lineComplete = false;
        
        while (!lineComplete) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(STDIN_FILENO, &read_fds);
            
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 10000; // 10ms
            
            int ready = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &tv);
            
            if (ready > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) {
                ssize_t bytesRead = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);
                
                if (bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    
                    for (int i = 0; i < bytesRead; i++) {
                        if (buffer[i] == '\n' || buffer[i] == '\r') {
                            lineComplete = true;
                            break;
                        } else {
                            input += buffer[i];
                        }
                    }
                }
            } else if (ready == 0) {
                // 超时，没有输入
                break;
            }
        }
        
        return input;
    }
    void startChatSession(const string& friendName) 
    {
       displayUnreadMessagesFromFriend(friendName);
         // 1. 安全地停止通知线程
    // {
    //     std::lock_guard<std::mutex> lock(thread_mutex);
    //     if (notification_thread.joinable()) {
    //         notification_thread_running = false;
    //         notification_thread.join();
    //     }
    // }
    if (!isValidFriend(friendName)) {
            std::cerr << "✘ 无法与 " << friendName << " 聊天，请先添加好友或解除屏蔽\n";
            return;
        }
    
    // 2. 启动接收消息线程
    {   running=true;
        std::lock_guard<std::mutex> lock(thread_mutex);
        if (recvThread.joinable()) {
            recvThread.join(); // 确保之前的线程已结束
        }
        recvThread = std::thread([this]() { receiveMessages(); });
    }
    
        //检查好友关系和黑名单
        
        // 设置终端为非阻塞模式
        
        //setNonBlockingTerminal();
        //setRawTerminalMode();
        
        // 设置活动聊天状态
        inChatSession = true;
        activeRecipient = friendName;
        inputBuffer.clear();
        setvbuf(stdout, NULL, _IONBF, 0); // 禁用标准输出缓冲
        // 显示提示信息
        {
            lock_guard<mutex> lock(outputMutex);
            cout << "\n开始与 " << friendName << " 聊天 (输入 /exit 退出)" << endl;

            cout << "> " << flush;
        }
        while (inChatSession) 
        {
            // 读取整行输入
            //string inputLine = readLineNonBlocking();
            string inputLine;
            getline(cin,inputLine);
            
            if (!inputLine.empty()) {
                // 处理输入行
                if (inputLine == "/exit") {
                    running=false;
                    json request = {
                        {"type", "send_message"},
                        {"sender", currentUser},
                        {"recipient", "==="}
                    };
                    string str=request.dump();
                    send(sock,str.c_str(),str.size(),0);

                    break;
                }
                cout<<"\033[A"<< flush;
                cout << "\r\033[2K" << "> " << flush;
                // 发送消息
                sendMessage(friendName, inputLine);
                
                // 更新输入提示
                cout << "\r\033[2K" << "> " << flush;
            //     if (!isValidFriend(friendName)) {
            //     std::cerr << "✘ 无法与 " << friendName << " 聊天，请先添加好友或解除屏蔽\n";
            //     return;
            // }
            }
            // 短暂休眠避免过度占用CPU
            this_thread::sleep_for(chrono::milliseconds(50));
        }
        
    
        // 退出聊天会话
        running=false;
        inChatSession = false;
        activeRecipient = "";
        inputBuffer.clear();
        //restoreTerminal();
        
        
         // 12. 停止接收线程
    {
        
        std::lock_guard<std::mutex> lock(thread_mutex);
        
         cout<<"??"<<endl;
        if (recvThread.joinable()) {
            recvThread.join();
            
            //recvThread.detach();
         }
    }
    cout<<"?"<<endl;
    // 13. 重启通知线程
    // {
    //     std::lock_guard<std::mutex> lock(thread_mutex);
    //     notification_thread_running = true;
    //     notification_thread = std::thread(notificationPollingThread);
    // }
    
        cout << "\n退出与 " << friendName << " 的聊天" << endl;
    }
    // 发送消息给指定用户
    void sendMessage(const string& recipient, const string& text) 
    {
        
        time_t now = time(nullptr);
        
        json request = {
            {"type", "send_message"},
            {"sender", currentUser},
            {"recipient", recipient},
            {"message", text},
            {"timestamp", static_cast<uint64_t>(now)}
        };
        
        json response = sendRequest(request);
        
        if (response.value("success", false)) {
            // 显示发送的消息
            lock_guard<mutex> lock(outputMutex);
            
            string timeStr = "Now";
            tm* localTime = localtime(&now);
            if (localTime) {
                char timeBuffer[80];
                strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M", localTime);
                timeStr = string(timeBuffer);
            }
            
            cout << "[" << timeStr << "] 你 -> " << recipient << ": " << text << endl;
        } else {
            lock_guard<mutex> lock(outputMutex);
            cerr << "消息发送失败: " 
                << response.value("message", "未知错误") << endl;
        }
    }
    /////////////////////////////////////////////////////////////////
    void receiveMessages() 
    {
        //char buffer[4096];
        
        while (running) {
            // 清空缓冲区
            //memset(buffer, 0, sizeof(buffer));
            
            // // 接收消息（非阻塞模式）
            // ssize_t bytesRead = recv(sock, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
            uint32_t len;
            ssize_t bytesRead=0;
            //= recv(sock, &len, sizeof(len), MSG_WAITALL);
            if(running){
                bytesRead=recv(sock, &len, sizeof(len), MSG_WAITALL);
            }
            if (bytesRead != sizeof(len)) {
                throw std::runtime_error("接收长度失败");
            }
            
            len = ntohl(len);
            
            // 接收数据
            std::vector<char> buffer(len);
            size_t totalReceived = 0;
            
            while (totalReceived < len) {
                ssize_t n = recv(sock, buffer.data() + totalReceived, len - totalReceived, 0);
                if (n <= 0) {
                    throw std::runtime_error("接收数据失败");
                }
                totalReceived += n;
            }
        json  message= json::parse(string(buffer.data(), len));
            
            if (bytesRead > 0) {
                //buffer[bytesRead] = '\0'; // 确保字符串正确终止
                
                try {
                    ///json message = json::parse(buffer);
                    // 根据消息类型直接处理
                    if (!message.contains("type")) {
                        lock_guard<mutex> lock(outputMutex);
                        cerr << "无效消息格式，缺少类型字段" << endl;
                        continue;
                    }
                    
                    string type = message["type"];
                    cout<<type<<endl;
                    if (type == "private_message") 
                    {
                        
                        // 处理私聊消息
                        string sender = message.value("sender", "");
                        string text = message.value("message", "");
                        time_t timestamp = message["timestamp"];
                        tm* localTime = localtime(&timestamp);
                        char timeStr[80];
                        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", localTime);
            
                        
                        // 显示消息（使用互斥锁保护输出）
                        {
                            lock_guard<mutex> lock(outputMutex);
                            // 如果正在与发送者聊天，直接显示消息
                            if (inChatSession && activeRecipient == sender) {
                                cout << "\n[" << timeStr << "] " << sender << ": " << text << endl;
                                cout << "> " << inputBuffer << flush; // 重新显示输入提示
                            }
                            // //否则显示通知
                            // else {
                            //     cout << "\n您收到来自 " << sender << " 的新消息 (" 
                            //         << (text.length() > 20 ? text.substr(0, 20) + "..." : text) 
                            //         << ") [" << timeStr << "]" << endl;
                            //     if (inChatSession) {
                            //         cout << "> " << inputBuffer << flush; // 重新显示输入提示
                            //     }
                            // }
                        }

                        // 发送消息确认（如果提供了消息ID）                                                 
                        if (message.contains("message_id")) {
                            json ack = {
                                {"type", "ack_private_message"},
                                //{"message_id", message["message_id"]},
                                {"user", currentUser},
                                {"friend",sender}
                            };
                            sendRequest(ack);
                        }
                    }
                    if(type=="exit")
                    {
                        running=false;
                    }

                    
                } catch (json::parse_error& e) {
                    lock_guard<mutex> lock(outputMutex);
                    cerr << "消息解析错误: " << e.what() << endl;
                }
            } else if (bytesRead == 0) {
                // 连接关闭
                lock_guard<mutex> lock(outputMutex);
                cerr << "\n服务器关闭了连接" << endl;
                running = false;
                break;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                // 真正的错误（非非阻塞操作产生的错误）
                lock_guard<mutex> lock(outputMutex);
                perror("\n接收消息错误");
                running = false;
                break;
            }
            
            // 短暂休眠避免过度占用CPU
            this_thread::sleep_for(chrono::milliseconds(50));
        }
    }
    //////////////////////////////////////////////////////////////////////////
   
//     std::mutex receive;
//     void receiveMessages() 
//  {   cout<<"qqq"<<endl;
//                     lock_guard<mutex> lock(receive);

//     char buffer[4096];
    
//     while (running) {
//         // 清空缓冲区
//         memset(buffer, 0, sizeof(buffer));
//         // std::string res;
//         // char c;
//         // while (true) {
//         //     ssize_t n = recv(sock, &c, 1, 0);
//         //     if (n <= 0) {
//         //         throw std::runtime_error("接收失败");
//         //     }
//         //     if (c == '\n') {
//         //         break;
//         //     }
//         //     res.push_back(c);
//         // }
//         uint32_t len;
//         ssize_t bytesRead = recv(sock, &len, sizeof(len), MSG_WAITALL);
//         if (bytesRead != sizeof(len)) {
//             throw std::runtime_error("接收长度失败");
//         }
        
//         len = ntohl(len);
        
//         // 接收数据
//         std::vector<char> buffer(len);
//         size_t totalReceived = 0;
        
//         while (totalReceived < len) {
//             ssize_t n = recv(sock, buffer.data() + totalReceived, len - totalReceived, 0);
//             if (n <= 0) {
//                 throw std::runtime_error("接收数据失败");
//             }
//             totalReceived += n;
//         }
    
//     //returnstd::string(buffer.data(), len);
//         //recv(sock, buffer, 4096, 0);
   
//         json response= json::parse(string(buffer.data(), len));
        
//         // 接收消息（非阻塞模式）
//         //ssize_t bytesRead = recv(sock, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
        
//         if (bytesRead > 0) {
//             //buffer[bytesRead] = '\0'; // 确保字符串正确终止
            
//             //try {
//                 json message = json::parse(response);
                
//                 // 验证消息类型
//                 if (!message.contains("type") || !message["type"].is_string()) {
//                     lock_guard<mutex> lock(outputMutex);
//                     cerr << "无效消息格式，缺少类型字段" << endl;
//                     continue;
//                 }
                
//                 string type = message["type"].get<string>();
                
//                 if (type == "private_message") 
//                 {
//                     // 安全获取字段值
//                     string sender = message.contains("sender") && message["sender"].is_string() 
//                                   ? message["sender"].get<string>() : "未知";
//                     string text = message.contains("message") && message["message"].is_string()
//                                 ? message["message"].get<string>() : "";
                    
//                     // 安全获取时间戳
//                     time_t timestamp = time(nullptr);
//                     if (message.contains("timestamp") && message["timestamp"].is_number()) {
//                         timestamp = message["timestamp"].get<time_t>();
//                     }
                    
//                     tm* localTime = localtime(&timestamp);
//                     char timeStr[80];
//                     strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", localTime);
                    
//                     // 显示消息
//                     {
//                         lock_guard<mutex> lock(outputMutex);
//                         if (inChatSession && activeRecipient == sender) {
//                             cout << "\n[" << timeStr << "] " << sender << ": " << text << endl;
//                             cout << "> " << inputBuffer << flush;
//                         } 
//                         // else {
//                         //     cout << "[新消息]"<<"\n您收到来自 " << sender << " 的新消息:"<<text<<endl; 
//                         //     //     << (text.length() > 20 ? text.substr(0, 20) + "..." : text) 
//                         //     //     << ") [" << timeStr << "]" << endl;
//                         //     // if (inChatSession) {
//                         //     //     cout << "> " << inputBuffer << flush;
//                         //     // }
//                         // }
//                     }

//                     // 发送消息确认
//                     if (message.contains("message_id")) {
//                         json ack = {
//                             {"type", "ack_private_message"},
//                             {"message_id", message["message_id"]},
//                             {"user", currentUser},
//                             {"friend", sender}
//                         };
//                         sendRequest(ack);
//                     }
//                 } 
                
//             } catch (const json::parse_error& e) {
//                 lock_guard<mutex> lock(outputMutex);
//                 cerr << "消息解析错误: " << e.what() << endl;
//                 //cerr << "原始数据: " << buffer << endl;
//             } catch (const json::type_error& e) {
//                 lock_guard<mutex> lock(outputMutex);
//                 cerr << "JSON 类型错误: " << e.what() << endl;
//                 cerr << "请检查消息格式是否正确" << endl;
//                 //cerr << "原始数据: " << buffer << endl;
//             } catch (const std::exception& e) {
//                 lock_guard<mutex> lock(outputMutex);
//                 cerr << "处理消息时发生错误: " << e.what() << endl;
//             }
//         } else if (bytesRead == 0) {
//             // 连接关闭
//             lock_guard<mutex> lock(outputMutex);
//             cerr << "\n服务器关闭了连接" << endl;
//             running = false;
//             break;
//         } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
//             // 真正的错误（非非阻塞操作产生的错误）
//             lock_guard<mutex> lock(outputMutex);
//             perror("\n接收消息错误");
//             running = false;
//             break;
//         }
        
// //         // 短暂休眠避免过度占用CPU
//         this_thread::sleep_for(chrono::milliseconds(50));
//      }
// }
    void privateChat(int sockfd,std::string current_user)
    {   running=true;
        sock=sockfd;
        currentUser=current_user;
        // 启动接收线程
        // recvThread = std::thread([this]() { receiveMessages(); });
    
        // 运行主菜单系统
            mainMenu();
            // 清理资源
        running = false;
        if (recvThread.joinable()) {
            recvThread.join();
        }
        
    }
    std::mutex valid;
    int isValidFriend(const std::string& friendName) 
    {
        //lock_guard<mutex>lock(valid) ;
        if (friendName == currentUser) {
            return 0;
        }
        
        //向服务器查询
        json req;
        req["type"] = "check_friend_valid";
        req["user"] = currentUser;
        req["friend"] = friendName;
        //json res = sendReq(req);
        std::string requestStr = req.dump();
        send(sock, requestStr.c_str(), requestStr.size(), 0);
        uint32_t len;
            ssize_t bytesRead = recv(sock, &len, sizeof(len), MSG_WAITALL);
            cout<<"lenlen="<<len<<endl;
            if (bytesRead != sizeof(len)) {
                throw std::runtime_error("接收长度失败");
            }
            
            len = ntohl(len);
            
            // 接收数据
            std::vector<char> buffer(len);
            size_t totalReceived = 0;
            //cout<<"123"<<endl;
            while (totalReceived < len) {
                ssize_t n = recv(sock, buffer.data() + totalReceived, len - totalReceived, 0);
                if (n <= 0) {
                    throw std::runtime_error("接收数据失败");
                }
                totalReceived += n;
            }
            cout<<"buffer"<<endl;
            for(auto & ch:buffer)
            {
                cout<<ch;
            }
            cout<<endl;
        json res= json::parse(string(buffer.data(), len));
        // char buffer[4096] = {0};
        // recv(sock, buffer, 4096, 0);
        // json res= json::parse(buffer);


        int isValid = 0;
        if (res["success"]) {
            isValid = res["valid"];
        } else {
            // 查询失败时保守处理
            std::cerr << "✘ 好友验证失败: " << res["message"] << std::endl;
            isValid = 0;
        }
        return isValid;
    }
    void displayUnreadMessagesFromFriend(const string& friendName)
    {
        
        json request = {
            {"type", "get_friend_unread_messages"},
            {"user", currentUser},
            {"friend", friendName}
        };
        
        // 发送请求并获取响应
        //json response = sendRequest(request);
        std::string requestStr = request.dump();
        send(sock, requestStr.c_str(), requestStr.size(), 0);
        // char buffer[4096] = {0};
        // char c;
        // string res;
        // while (true) {
        //     ssize_t n = recv(sock, &c, 1, 0);
        //     if (n <= 0) {
        //         throw std::runtime_error("接收失败");
        //     }
        //     if (c == '\n') {
        //         break;
        //     }
        //     res.push_back(c);
        // }
        // 接收长度
    uint32_t len;
    ssize_t bytesRead = recv(sock, &len, sizeof(len), MSG_WAITALL);
    if (bytesRead != sizeof(len)) {
        throw std::runtime_error("接收长度失败");
    }
    
    len = ntohl(len);
    
    // 接收数据
    std::vector<char> buffer(len);
    size_t totalReceived = 0;
    
    while (totalReceived < len) {
        ssize_t n = recv(sock, buffer.data() + totalReceived, len - totalReceived, 0);
        if (n <= 0) {
            throw std::runtime_error("接收数据失败");
        }
        totalReceived += n;
    }
    
    //returnstd::string(buffer.data(), len);
        //recv(sock, buffer, 4096, 0);
    //returnstd::string(buffer.data(), len);
        json response= json::parse(string(buffer.data(), len));
        if (!response["success"]) 
        {
            cout<<"success"<<endl;
            return ;

        }
        auto messages = response["messages"];
        for (const auto& msg : messages) {
                string text = msg.value("message", "");
                // string timestampStr = msg.value("timestamp", "");
                
                // // 转换时间戳为可读格式
                // struct tm tm = {};
                // strptime(timestampStr.c_str(), "%Y-%m-%d %H:%M:%S", &tm);
                // time_t timestamp = mktime(&tm);
                
                // char timeBuffer[80];
                // strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M", localtime(&timestamp));
                time_t timestamp = msg["timestamp"];
                tm* localTime = localtime(&timestamp);
                char timeBuffer[80];
                strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M", localTime);
            
                cout << "[" << timeBuffer << "] " << friendName << ": " << text << endl;
            }
            
            cout << "=============================" << endl;
            
            // 发送确认请求，标记消息为已读
            json ack = {
                {"type", "ack_private_message"},
                {"user", currentUser},
                {"friend", friendName}
            };
            sendRequest(ack);
            
    }
    
    void queryChatHistory(const string& friendName)
    {
        json request = {
            {"type", "get_chat_history"},
            {"user", currentUser},
            {"friend", friendName},
        };
        
        std::string requestStr = request.dump();
        
        // 发送请求
        if (send(sock, requestStr.c_str(), requestStr.size(), 0) < 0) {
            perror("发送请求失败");
            return;
        }
        
        // 接收响应
        const int initialBufferSize = 4096;
        vector<char> buffer(initialBufferSize);
        ssize_t totalReceived = 0;
        
        while (true) {
            // 检查是否需要扩展缓冲区
            if (totalReceived >= buffer.size() - 1) {
                buffer.resize(buffer.size() * 2);
            }
            
            // 接收数据
            ssize_t bytesRead = recv(sock, buffer.data() + totalReceived, 
                                    buffer.size() - totalReceived - 1, 0);
            
            if (bytesRead < 0) {
                perror("接收响应失败");
                return;
            }
            
            if (bytesRead == 0) {
                // 连接关闭
                break;
            }
            
            totalReceived += bytesRead;
            
            // 尝试解析 JSON
            try {
                // 添加终止符
                buffer[totalReceived] = '\0';
                
                // 尝试解析
                json response = json::parse(buffer.data());
                
                // 如果解析成功，处理响应
                processChatHistory(response);
                return;
                
            } catch (json::parse_error& e) {
                // 如果错误不是"unexpected end of input"，继续接收更多数据
                if (e.id != 101) {
                    cerr << "JSON解析错误: " << e.what() << endl;
                    cerr << "已接收数据: " << totalReceived << "字节" << endl;
                    return;
                }
            }
        }
        
        // 如果循环结束但未解析成功
        cerr << "无法解析服务器响应" << endl;
    }

    void processChatHistory(const json& response) 
    {
        if (!response.value("success", false)) {
            cout << "无历史聊天记录" << endl;
            return;
        }
        
        if (!response.contains("messages") || !response["messages"].is_array()) {
            cerr << "无效响应格式" << endl;
            return;
        }
        
        auto messages = response["messages"];
        for (const auto& msg : messages) {
            //验证消息格式
            if (!msg.is_object() || 
                !msg.contains("sender") || !msg["sender"].is_string() ||
                !msg.contains("message") || !msg["message"].is_string() ||
                !msg.contains("timestamp") || !msg["timestamp"].is_number()) {
                cerr << "无效消息格式" << endl;
                continue;
            }
            
            // 解析时间戳
            time_t timestamp = msg["timestamp"];
            tm* localTime = localtime(&timestamp);
            char timeBuffer[80];
            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M", localTime);
            
            // 确定消息方向
            string sender = msg["sender"];
            string arrow = (sender == currentUser) ? "->" : "<-";
            string displayName = (sender == currentUser) ? "你" : sender;
            
            cout << "[" << timeBuffer << "] " 
                << displayName << " " << arrow << " "
                << msg["message"] << endl;
        }
        
        cout << "========================" << endl;
    }
    

};