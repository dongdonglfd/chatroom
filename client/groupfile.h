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
#include </usr/include/x86_64-linux-gnu/curl/curl.h>
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
#include <openssl/sha.h>  // 提供 SHA256_CTX 定义
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>
#include <openssl/buffer.h>
#include <sys/sendfile.h>
#include <fcntl.h> 
#include <sys/stat.h>
namespace fs = std::filesystem;  
using json = nlohmann::json;
using namespace std;
#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_EVENTS 10
string addressfile;
class groupfile
{
private:

    int sock;
    int cfd;
    string currentUser;
    int groupid;
    string filePath;
    json sendRequest(int sock, const json& request) {
        string requestStr = request.dump();
        ssize_t size=send(sock, requestStr.c_str(), requestStr.size(), 0);
        if(size<0)
        {
            cout<<"send fail"<<endl;
        }
        char buffer[4096];
        ssize_t bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead > 0) {
            // return {{"success",true}, {"message", "接收响应失败"}};
            return {{"success",true}};
        }
        //return json::parse(buffer);
        
    }
    json sendreq(int sock, const json& request) 
    {
        // 序列化请求
        std::string requestStr = request.dump();
        
        // 发送请求
        send(sock, requestStr.c_str(), requestStr.size(), 0);
        
        // 接收响应
        char buffer[4096] = {0};
        ssize_t bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            throw std::runtime_error("接收响应失败");
        }
        
        return json::parse(std::string(buffer, bytes));
    }
public :
    void sendgroupfile(int sockfd,string currentuser)
    {
        sock=sockfd;
        currentUser=currentuser;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    
        // 输入用户名
        cout << "请输入群号: ";
        cin>>groupid;
        // 输入文件路径
        bool validPath = false;
        while (!validPath) {
            cout << "请输入文件路径: ";
            getline(cin, filePath);
            
            // 验证文件路径
            if (filePath.empty()) {
                cerr << "文件路径不能为空" << endl;
            } else {
                // 检查文件是否存在
                if (filesystem::exists(filePath)) {
                    validPath = true;
                } else {
                    cerr << "文件不存在，请重新输入" << endl;
                }
            }
        }
        //获取文件信息
        string fileName = fs::path(filePath).filename().string();
        uint64_t fileSize = fs::file_size(filePath);
        // 发送文件上传请求
        json request = {
            {"type", "file_upload_request"},
            {"user", currentUser},
            {"file_name", fileName},
            {"file_size", fileSize},
            {"filepath",filePath},
            {"groupid", groupid},
            {"timestamp", static_cast<uint64_t>(time(nullptr))}
        };
        json response = sendRequest(sock, request);
        if (!response.value("success", false)) {
            cerr << "文件上传请求失败: " 
                 << response.value("message", "未知错误") << endl;
            return;
        }
        uploadgroupFileToServer(filePath,sockfd);
    }
    void pasvclient()
    {
        cfd=socket(AF_INET,SOCK_STREAM,0);
        if(cfd==-1)
        {
            perror("socket");
            exit(0);
        }
        int opt = 1;
        setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        //2.连接服务器
        struct sockaddr_in addr;
        addr.sin_family=AF_INET;//IPV4
        addr.sin_port=htons(8090);//网络字节序
        //变成大端
        inet_pton(AF_INET,address.c_str(),&addr.sin_addr.s_addr);
        int ret=connect(cfd,(struct sockaddr*)&addr,sizeof(addr));
        if(ret==-1)
        {
            perror("connect");
            exit(0);
        }
    }
    void uploadgroupFileToServer(const std::string& filePath, int sock) 
    {
        string fileName = fs::path(filePath).filename().string();
        std::ifstream file(filePath, std::ios::binary);
        if (!file) throw std::runtime_error("文件不存在");
        uint64_t fileSize = fs::file_size(filePath);
        // 3. 发送开始消息
        json startMsg = {
            {"type", "groupfile_start"},
            {"file_path", filePath},
            {"filesize",fileSize}
        };
        std::string startStr = startMsg.dump();
        json response = sendRequest(sock, startMsg);
        //uint16_t port=response["port"];
        pasvclient();
         char buffer[4096];
        ssize_t total = 0;
        
        while (file) {
            file.read(buffer, sizeof(buffer));
            std::streamsize bytes_read = file.gcount();
            
            if (bytes_read > 0) {
                ssize_t total_sent = 0;
                while (total_sent < bytes_read) {
                    ssize_t sent = send(cfd, 
                                    buffer + total_sent, 
                                    bytes_read - total_sent, 
                                    MSG_NOSIGNAL);
                    if (sent <= 0) {
                        throw std::runtime_error("发送失败");
                    }
                    total_sent += sent;
                }
                total += total_sent;
            }
        }
        std::cout << "上传完成: "<< " (" << total << " bytes)" << std::endl;
        // 8. 发送结束消息
        json endMsg = {
            {"type", "groupfile_end"},
            {"success", true}
            //{"bytes_sent", total_sent}
        };
        json endResponse = sendRequest(sock, endMsg);
        close(cfd);
        
        std::cout << "文件上传完成: " << filePath << std::endl;
    }

};