#include"groupchatserver.h"
namespace fs = std::filesystem;
struct FileSession {
    string filePath;          // 最终文件路径
    string filename;          // 临时文件路径
    uint64_t fileSize;        // 文件总大小
    uint64_t receivedSize;    // 已接收大小
    // ofstream fileStream;      // 文件输出流
     //int fileFd;               // 文件描述符
     FILE * fileFd;
    int clientFd;             // 客户端套接字
};
// 全局会话映射
extern mutex sessionMutex;
extern mutex data_mutex;
 extern   int data_sock;
  extern  int listen_sock;
//unordered_map<string, FileSession> fileSessions; // file_path -> session
uint16_t pasv();
class FileTransferServer
{
private:
    mutex uploadMutex;
    
    
    void sendResponse(int fd, const json& response) ;
public:
    Connection* getDBConnection();
    void handleFileUploadRequest(int fd, const json& request);
    void storeFileInfo(const string& sender, const string& recipient, 
                      const string& fileName, uint64_t fileSize, const string& filePath,time_t timestamp);
    void handleFileStart(json& data, int client_sock) ;

    void handleFileEnd(json& data, int client_sock);
    void getUndeliveredFiles(int fd,json& data);
    void privatefilesend(json& data, int client_sock);
    void updateFileDeliveryStatus(json& data, int client_sock);





};