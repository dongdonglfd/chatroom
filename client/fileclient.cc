#include"friend.h"
string address;
// struct FileInfo {
//     int id;
//     std::string sender;
//     std::string file_name;
//     uint64_t file_size;
//     uint64_t created_at;
//     std::string file_path;
// };
json FileTransferClient::sendRequest(int sock, const json& request) {
        // string requestStr = request.dump();
        // ssize_t size=send(sock, requestStr.c_str(), requestStr.size(), 0);
        sendLengthPrefixed(sock,request);
        // if(size<0)
        // {
        //     cout<<"send fail"<<endl;
        // }
        char buffer[4096];
        ssize_t bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead > 0) {
            // return {{"success",true}, {"message", "接收响应失败"}};
            return {{"success",true}};
        }
        else{
            return {{"success",false}};
        }
        //return json::parse(buffer);
        
    }
    json FileTransferClient::sendreq(int sock, const json& request) 
    {
        // // 序列化请求
        // std::string requestStr = request.dump();
        
        // // 发送请求
        // send(sock, requestStr.c_str(), requestStr.size(), 0);
        sendLengthPrefixed(sock,request);
        // 接收响应
        char buffer[4096] = {0};
        ssize_t bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            throw std::runtime_error("接收响应失败");
        }
        
        return json::parse(std::string(buffer, bytes));
    }
    void FileTransferClient::sendFile(int sockfd,string currentuser)
    {
        sock=sockfd;
        currentUser=currentuser;
        
        // cout << "请输入对方用户名: ";
        // getline(cin, friendName);
        // cout<<"请输入文件路径"<<endl;
        // getline(cin, filePath);
        // if (!fs::exists(filePath)) {
        //     cerr << "文件不存在: " << filePath << endl;
        //     return;
        // }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    
        // 输入用户名
        cout << "请输入对方用户名: ";
        getline(cin, friendName);
        
        // 验证用户名
        if (friendName.empty()) {
            cerr << "用户名不能为空" << endl;
            return;
        }
        
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
            {"recipient", friendName},
            {"timestamp", static_cast<uint64_t>(time(nullptr))}
        };
        json response = sendRequest(sock, request);
        if (!response.value("success", false)) {
            cerr << "文件上传请求失败: " 
                 << response.value("message", "未知错误") << endl;
            return;
        }

    
        // 上传文件到服务器
        uploadFileToServer(filePath,sockfd);

    }
    
    void FileTransferClient::pasvclient()
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
    void FileTransferClient::uploadFileToServer(const std::string& filePath, int sock) 
    {
        string fileName = fs::path(filePath).filename().string();
        std::ifstream file(filePath, std::ios::binary);
        if (!file) throw std::runtime_error("文件不存在");
        uint64_t fileSize = fs::file_size(filePath);
        // 3. 发送开始消息
        json startMsg = {
            {"type", "file_start"},
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
            {"type", "file_end"},
            {"success", true}
            //{"bytes_sent", total_sent}
        };
        json endResponse = sendRequest(sock, endMsg);
        close(cfd);
        
        std::cout << "文件上传完成: " << filePath << std::endl;
    }
    void FileTransferClient::getUndeliveredFiles(int client_sock, const std::string& username)
    {
        sock=client_sock;
        currentUser=username;
        json request = {
            {"type", "get_undelivered_files"},
            {"username", username}
        };
        
        // 2. 发送请求
        //json response = sendreq(sock, request);// 3. 检查响应状态
        // std::string requestStr = request.dump();
        
        // // 发送请求
        // send(sock, requestStr.c_str(), requestStr.size(), 0);
        sendLengthPrefixed(sock,request);
        uint32_t len;
        recv(sock, &len, sizeof(len), MSG_WAITALL);
        len = ntohl(len);
        // 接收数据
        std::vector<char> buffer(len);
        size_t totalReceived = 0;
        
        while (totalReceived < len) {
            ssize_t n = recv(sock, buffer.data() + totalReceived, len - totalReceived, 0);
            if (n <= 0) break;
            totalReceived += n;
        }
        json response=json::parse(string(buffer.data(), len));
        if (!response.value("success", false)) {
            throw std::runtime_error("服务器错误: " + response.value("message", ""));
        }

        std::vector<FileInfo> filesstore;
        for (const auto & files :response["messages"])
        {
            FileInfo file;
            file.id = files["id"];
            file.sender = files["sender"];
            file.file_name = files["file_name"];
            file.file_size = files["file_size"];
            file.created_at = files["timestamp"];
            filesstore.push_back(file);
        
        }
        std::cout << "======================================================================" << std::endl;
        std::cout << "ID"<<"\t"<<"发送者"<<"\t"<<"文件名"<<"\t"<<"大小"<<"\t"<<"时间"<<"\t" << std::endl;
        std::cout << "----------------------------------------------------------------------" << std::endl;
        for(const auto & info :filesstore)
        {
            time_t timestamp = info.created_at;
            tm* localTime = localtime(&timestamp);
            char timeBuffer[80];
            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M", localTime);
            cout<<info.id<<"\t"<<info.sender<<"\t"<<info.file_name<<"\t"<<info.file_size<<"\t"<<timeBuffer<<"\t"<<endl;
        }
        cout<<"选择接收文件的id:"<<endl;
        int id;
        cin>>id;
        getchar();
        FileInfo selectedFile;
        if (id > 0) 
        {
            // 1. 在本地存储中查找文件信息
            bool found = false;
            
            for (const auto& info : filesstore) {
                if (info.id == id) {
                    selectedFile = info;
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                std::cout << "未找到ID为 " << id << " 的文件" << std::endl;
                return;
            }
            
        }
        receivefile(selectedFile);
    }
    void FileTransferClient::receivefile(FileInfo &selectedFile)
    {   
        json downloadRequest = {
            {"type", "download_file"},
            {"file_id", selectedFile.id},
            {"filename",selectedFile.file_name}

        };
        json response = sendRequest(sock, downloadRequest);
        pasvclient();
        char buffer[4096];
        ssize_t total = 0;
        cout<<"开始接收"<<endl;
        std::ofstream file(selectedFile.file_name, std::ios::binary);
        cout<<"ofstream"<<endl;
        if(!file) {
            cout<<"fail"<<endl;
            return;
        }
        while (selectedFile.file_size>total) {
            ssize_t bytes = recv(cfd, buffer, sizeof(buffer), 0);
            if (bytes < 0) {
                if (errno == EINTR) continue; // 处理中断
                perror("recv error");
                break;
            }
            if (bytes == 0) break; // 客户端关闭连接
            
            file.write(buffer, bytes);
            if (!file) {
                //send_response("451 本地文件写入错误");
                break;
            }
            total += bytes;
            
            file.flush(); // 确保写入磁盘
        }
        cout<<"接收完成"<<endl;
        close(cfd);
        json endMsg = {
            {"type", "fileclient_end"},
            {"fileid",selectedFile.id},
            {"success", true}
            //{"bytes_sent", total_sent}
        };
        json endResponse = sendRequest(sock, endMsg);
        
    }