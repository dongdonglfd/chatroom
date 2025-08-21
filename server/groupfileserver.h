#include"fileserver.h"
class groupfileserver
{private:
    mutex uploadMutex;
    mutex groupmutex;
    mutex data_mutex;
    
    FileTransferServer fileserver;
    void sendResponse(int fd, const json& response) ;
public:
    void handleFilegroupUploadRequest(int fd, const json& request);
    void broadcastToGroup(int fd,int group_id, const json& message, const string& excludeUser = "");
    void storeOfflinefile(const string& recipient, int group_id, const json& message,int fd);
    void handlegroupFileStart(json& data, int client_sock);
    void handlegroupFileEnd(json& data, int client_sock);
    void getUndeliveredgroupFiles(int fd,json& data);
    void updategroupFileDeliveryStatus(json& data, int client_sock);
    void groupfilesend(json& data, int client_sock);


};