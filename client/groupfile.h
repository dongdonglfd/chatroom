#pragma once
#include "chatclient.h"
namespace fs = std::filesystem;  
using json = nlohmann::json;
using namespace std;
#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_EVENTS 10
extern string addressfile;
struct FileInfoo {
    int id;
    int groupid;
    std::string sender;
    std::string file_name;
    uint64_t file_size;
    uint64_t created_at;
    std::string file_path;
};
class groupfile
{
private:

    int sock;
    int cfd;
    string currentUser;
    int groupid;
    string filePath;
    json sendRequest(int sock, const json& request) ;
    json sendreq(int sock, const json& request) ;
    json sendReq(int sock, const json& request) ;
public :
    void sendgroupfile(int sockfd,string currentuser);
    void pasvclient();
    void uploadgroupFileToServer(const std::string& filePath, int sock) ;
    void getUndeliveredgroupFiles(int client_sock, const std::string& username);
    void receivegroupfile(FileInfoo &selectedFile);
    

};