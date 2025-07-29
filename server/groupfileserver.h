#include"fileserver.h"
class groupfileserver
{private:
    mutex uploadMutex;
    mutex groupmutex;
    mutex data_mutex;
    int data_sock=-1;
    int listen_sock=-1;
    FileTransferServer fileserver;
    void sendResponse(int fd, const json& response) {
        string responseStr = response.dump();
        send(fd, responseStr.c_str(), responseStr.size(), 0);
    }
public:
    void handleFilegroupUploadRequest(int fd, const json& request)
    {   
        string user = request["user"];
        string fileName = request["file_name"];
        uint64_t fileSize = request["file_size"];
        int groupid = request["groupid"];
        time_t timestamp = static_cast<time_t>(request["timestamp"]);
        string filePath = request["filepath"];
        unique_ptr<sql::Connection> con(fileserver.getDBConnection());
       //判读sender是否在群内
       unique_ptr<sql::PreparedStatement> checkStmt(
            con->prepareStatement(
                "SELECT COUNT(*) AS count FROM group_members "
                // "SELECT * FROM group_members "
                "WHERE group_id = ? AND user = ?"
            )
        );
        checkStmt->setInt(1, groupid);
        checkStmt->setString(2, user);

        unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
        
        if (res->next()) {
            int count = res->getInt("count");
            if (count == 0) {
                cout<<"5555"<<endl;
                // 发送者不在群组内
                json errorResponse = {
                    {"success", false},
                    {"message", "您不在该群组中，无法发送消息"}
                };
                sendResponse(fd,errorResponse);
                return;
            }
        }

        json broadcast = {
            {"type", "group_messages"},
            {"group_id", groupid},
            {"sender", user},
            {"recipient", fileName},
            {"filename",fileName},
            {"filesize",fileSize},
            {"timestamp", timestamp}
        };
        broadcastToGroup(fd,groupid, broadcast, user);
    }
    void broadcastToGroup(int fd,int group_id, const json& message, const string& excludeUser = "")
    {
        unique_ptr<sql::Connection> con(fileserver.getDBConnection());
        // 准备查询群组成员的SQL语句
        unique_ptr<sql::PreparedStatement> membersStmt(
            con->prepareStatement(
                "SELECT user "
                "FROM group_members "
                "WHERE group_id = ?"
            )
        );
        membersStmt->setInt(1, group_id);
        // 执行查询
        unique_ptr<sql::ResultSet> membersRes(membersStmt->executeQuery());
        // 存储群组成员列表
        vector<string> members;
        while (membersRes->next()) {
            string username = membersRes->getString("user");
            members.push_back(username);
        }
        for (const string& member : members) 
        {   
            if (member == excludeUser)
            {
                continue;
            } 
                // 用户离线，存储消息
            storeOfflinefile(member, group_id, message,fd);
        }
    }
    void storeOfflinefile(const string& recipient, int group_id, const json& message,int fd)
    {
        unique_ptr<sql::Connection> con(fileserver.getDBConnection());
        unique_ptr<sql::PreparedStatement> storeStmt(
            con->prepareStatement(
                "INSERT INTO group_files "
                "(recipient, groupid, sender,file_name , file_size,timestamp) "
                "VALUES (?, ?, ?, ?, ?, ?)"
            )
        );
        storeStmt->setString(1, recipient); 
        storeStmt->setInt(2, group_id);
        storeStmt->setString(3, message["sender"].get<string>());
        storeStmt->setString(4, message["filename"].get<string>());
        storeStmt->setInt(5,message["filesize"] );
        storeStmt->setUInt64(6, message["timestamp"].get<uint64_t>());
        storeStmt->executeUpdate();
        json response = {
            {"success", true}
        };
            
        sendResponse(fd, response);
    }
    uint16_t pasv()
    {
        std::lock_guard<std::mutex> lock(data_mutex);
        
        // 创建数据监听socket
        listen_sock = socket(AF_INET, SOCK_STREAM, 0);
        if(listen_sock < 0) {
            perror("socket failed");
            return 0;
        }

        
        sockaddr_in data_addr{};
        data_addr.sin_family = AF_INET;
        data_addr.sin_addr.s_addr = INADDR_ANY;
        // data_addr.sin_port = htons(8090);
        data_addr.sin_port = 0; 

        if(bind(listen_sock, (sockaddr*)&data_addr, sizeof(data_addr)) < 0) {
            perror("bind failed");
            close(listen_sock);
            listen_sock = -1;
            return 0;
        }

        // 开始监听
        if(listen(listen_sock, 1) < 0) {
            perror("listen failed");
            close(listen_sock);
            listen_sock = -1;
            return 0;
        }
        // 获取绑定的端口号
        socklen_t addr_len = sizeof(data_addr);
        getsockname(listen_sock, (sockaddr*)&data_addr, &addr_len);//getsockname 可以用于获取绑定到套接字的实际地址和端口。
        uint16_t port = ntohs(data_addr.sin_port);// 获取端口号（网络字节序转主机字节序）
        return port;
        
    }
    void handlegroupFileStart(json& data, int client_sock) 
    {
        std::lock_guard<std::mutex> lock(groupmutex);
        string filepath = data["file_path"];
        uint64_t fileSize=data["filesize"];
        string fileName = fs::path(filepath).filename().string();
        string filestore="/home/lfd/1/"+fileName;
        std::ofstream file(filestore, std::ios::binary);
        if(!file) {
            cout<<"fail"<<endl;
            return;
        }
        //uint16_t port=pasv();
        uint16_t port=pasv();
        cout<<"port = "<<port<<endl;
        if (port == 0) {
        json req = {{"success", false}, {"error", "PASV失败"}};
        string str = req.dump()+"\n";
        send(client_sock, str.c_str(), str.size(), 0);
        return;
    }
    
        // 发送响应（包含端口号）
        json req = {
            {"success", true},
            {"port", port} // 必须发送端口号给客户端
        };
        string str = req.dump()+"\n";
        send(client_sock, str.c_str(), str.size(), 0);
        // json req;
        // req["success"]=true;
        // //req["port"]=port;
        // string str=req.dump();
        // send(client_sock,str.c_str(),str.size(),0);
        
        cout << "开始接收文件: " << filepath  << endl;
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        data_sock = accept(listen_sock, 
                      (sockaddr*)&client_addr, &addr_len);
        char buffer[4096];
        ssize_t total = 0;
        
        
        while (fileSize>total) {
            ssize_t bytes = recv(data_sock, buffer, sizeof(buffer), 0);
            if (bytes < 0) {
                if (errno == EINTR) continue; // 处理中断
                cout<<"1234"<<endl;
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
        close(listen_sock);
        listen_sock=-1;
        data_sock=-1;
        file.close();
        cout<<"传输完成"<<endl;
    }
    void handlegroupFileEnd(json& data, int client_sock)
    {
        json response = {
            {"success", true}
        };
            
        sendResponse(client_sock, response);
    } 
    void getUndeliveredgroupFiles(int fd,json& data)
    {
        string username= data["username"];
        unique_ptr<sql::Connection> con(fileserver.getDBConnection());
        
        // 检查数据库连接
        if (!con) {
            json errorResponse = {
                {"type", "error"},
                {"message", "数据库连接失败"}
            };
            string responseStr = errorResponse.dump();
            send(fd, responseStr.c_str(), responseStr.size(), 0);
            return;
        }
        unique_ptr<sql::PreparedStatement> Stmt(
                con->prepareStatement(
                    "SELECT id, sender, file_name, file_size ,groupid ,timestamp "
                    "FROM group_files "
                    "WHERE recipient  = ?  "
                    "ORDER BY timestamp ASC"
                )
            );
        Stmt->setString(1, username);
        unique_ptr<sql::ResultSet> res(Stmt->executeQuery());
        json response;
            response["type"] = "unread_files";
            response["user"] = username;
            // 检查是否有未读消息
            if (!res->next()) {
                
            json filesArray = json::array();
                // 没有未读消息
                response["success"] = true;
                response["messages"] = json::array(); // 空数组
            } else {
                // 有未读消息
                json filesArray = json::array();
                

                // 重置结果集指针到开头
                res->beforeFirst();

                // 遍历所有未读消息
                while (res->next()) {
                    json messageObj;
                    messageObj["id"] = res->getInt("id");
                    messageObj["groupid"] = res->getInt("groupid");
                    messageObj["sender"] = res->getString("sender");
                    messageObj["file_name"] = res->getString("file_name");
                    messageObj["file_size"] = res->getInt("file_size");
                    messageObj["timestamp"] = res->getInt64("timestamp");
                    filesArray.push_back(messageObj);
                }

                response["success"] = true;
                response["messages"] = filesArray;
            }
            string responseStr = response.dump();
            // 添加调试输出
            cout << "发送响应: " << responseStr << endl;

            // 发送响应给客户端
            if (send(fd, responseStr.c_str(), responseStr.size(), 0) < 0) {
                cerr << "发送未读消息响应失败: " << strerror(errno) << endl;
            } else {
                cout << "已发送未读消息给用户 " << username << endl;
            }

    }
    void updategroupFileDeliveryStatus(json& data, int client_sock)
    {
        int id=data["fileid"];
        unique_ptr<sql::Connection> con(fileserver.getDBConnection());
         // 检查数据库连接
        if (!con) {
            json errorResponse = {
                {"type", "error"},
                {"message", "数据库连接失败"}
            };
            string responseStr = errorResponse.dump();
            send(client_sock, responseStr.c_str(), responseStr.size(), 0);
            return;
        }
        unique_ptr<sql::PreparedStatement> Stmt(
                con->prepareStatement(
                "DELETE group_files "
                "WHERE id = ?"
                )
            );
        Stmt->setInt(1, id);
        Stmt->executeUpdate();
        json response = {
            {"success", true}
        };   
        sendResponse(client_sock, response);

    }
    void groupfilesend(json& data, int client_sock)
    {
        string filename=data["filename"];
        string path="/home/lfd/1/"+filename;
        uint16_t port=pasv();
        cout<<"port = "<<port<<endl;
        if (port == 0) {
        json req = {{"success", false}, {"error", "PASV失败"}};
        string str = req.dump()+"\n";
        send(client_sock, str.c_str(), str.size(), 0);
        return;
    }
    
        // 发送响应（包含端口号）
        json req = {
            {"success", true},
            {"port", port} // 必须发送端口号给客户端
        };
        string str = req.dump()+"\n";
        send(client_sock, str.c_str(), str.size(), 0);
        // json response = {
        //     {"success", true}
        // };   
        // sendResponse(client_sock, response);
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        data_sock = accept(listen_sock, 
                      (sockaddr*)&client_addr, &addr_len);
        cout<<"开始发送文件"<<endl;
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("文件不存在");
        char buffer[4096];
        ssize_t total = 0;
        
        while (file) {
            file.read(buffer, sizeof(buffer));
            std::streamsize bytes_read = file.gcount();
            
            if (bytes_read > 0) {
                ssize_t total_sent = 0;
                while (total_sent < bytes_read) {
                    ssize_t sent = send(data_sock, 
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
        std::cout << "发送完成: "<< " (" << total << " bytes)" << std::endl;
        file.close();
        close(listen_sock);
        listen_sock=-1;
        data_sock=-1;

        
    }


};