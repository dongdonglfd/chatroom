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
class NotificationService
{
    public:
    int notifysock;
    std::mutex notifymutex;
    public:
    NotificationService(int port){
        std::lock_guard<std::mutex> lock(notifymutex);
        
        // 创建数据监听socket
        notifysock = socket(AF_INET, SOCK_STREAM, 0);
        if(notifysock < 0) {
            return ;
        }

        
        sockaddr_in data_addr{};
        data_addr.sin_family = AF_INET;
        data_addr.sin_addr.s_addr = INADDR_ANY;
        data_addr.sin_port = htons(8081); 

        if(bind(notifysock, (sockaddr*)&data_addr, sizeof(data_addr)) < 0) {
        
            close(notifysock);
            notifysock = -1;
            return ;
        }

        // 开始监听
        if(listen(notifysock, 1) < 0) {
          
            close(notifysock);
            notifysock = -1;
            return ;
        }

    }
};