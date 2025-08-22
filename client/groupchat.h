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
#include <thread>
#include <atomic>
#include <mutex>

 
using json = nlohmann::json;
using namespace std;
class groupchat
{
private:
    string userName;
    int sock;
    bool grouprunning= true;
    thread recvThread;
    bool inChatSession = false;
    int activeid;
    mutex outputMutex;
    mutex inputMutex;
    string currentGroup;
    int groupid;
    string inputBuffer;
    json sendRequest(const json& request) ;
    json sendReq(const json& request);
public:
    void sendLength(int fd, const json& response);
    void groupChat(int fd,string name);
    // 发送消息给指定用户
    void sendMessage(const string& recipient, const string& text) ;
    void mainMenu() ;
    string readLineNonBlocking() ;
    void startgroupChat(int id);
    void receivegroupMessages();
    void displayUnreadMessagesFromgroup(string username);
    void querygroupHistory(int groupid);
    void processgroupHistory(const json& response) ;
};