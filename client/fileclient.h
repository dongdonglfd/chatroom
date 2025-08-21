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
extern string address;
struct FileInfo {
    int id;
    std::string sender;
    std::string file_name;
    uint64_t file_size;
    uint64_t created_at;
    std::string file_path;
};
class FileTransferClient
{
private:

    int sock;
    int cfd;
    string currentUser;
    string friendName;
    string filePath;
    json sendRequest(int sock, const json& request) ;
    json sendreq(int sock, const json& request) ;
public:
void sendFile(int sockfd,string currentuser);
    void pasvclient();
   
    void uploadFileToServer(const std::string& filePath, int sock) ;
    void getUndeliveredFiles(int client_sock, const std::string& username);
    void receivefile(FileInfo &selectedFile);
    
};