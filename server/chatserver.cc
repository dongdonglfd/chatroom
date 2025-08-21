#include "chatserver.h"
unordered_map<string, int> online_users; // 在线用户表
unordered_map<string, bool> onlineing;
 
void Chat::sendResponse(int fd, const json& response) {
    string responseStr = response.dump();
    send(fd, responseStr.c_str(), responseStr.size(), 0);
}
Connection* Chat:: getDBConnection() {
        try {
            sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
            if(!driver) {
                cerr << "获取驱动实例失败" << endl;
                return nullptr;
            }

            sql::Connection *con = driver->connect(DB_HOST, DB_USER, DB_PASS);
            if(!con) {
                cerr << "创建连接失败" << endl;
                return nullptr;
            }

            con->setSchema(DB_NAME);
            cout << "数据库连接成功" << endl;
            return con;
        } catch (sql::SQLException &e) {
            cerr << "MySQL错误[" << e.getErrorCode() << "]: " << e.what() << endl;
        } catch (...) {
            cerr << "未知错误" << endl;
        }
        return nullptr;
    }
    // 添加用户到在线列表
    void Chat::userOnline(const std::string& username, int socket) {
        std::lock_guard<std::mutex> lock(online_mutex);
        cout<<"==============================="<<username<<endl;
        online_users[username] = socket;
    }
    void Chat::handleCheckFriendValid(int fd, const json& req) 
    {
        string user = req["user"];
        string friendName = req["friend"];
        
        unique_ptr<sql::Connection> con(getDBConnection());
        json response;
        response["valid"] = 1;
        
        try {
            unique_ptr<sql::PreparedStatement> Stmt(
                con->prepareStatement(
                    "SELECT id FROM friends "
                    "WHERE (user1 = ? AND user2 = ?)  "
                    
                )
            );
            Stmt->setString(1, user);
            Stmt->setString(2, friendName);
            unique_ptr<sql::ResultSet> Res(Stmt->executeQuery());
            if (!Res->next()) {
                response["valid"] = 0;
                response["type"] = "valid";
                response["reason"] = "存在屏蔽关系";
                response["success"] = true;
                //send(fd, response.dump().c_str(), response.dump().size(), 0);
                sendLengthPrefixed(fd,response);
                return;
            }           
            unique_ptr<sql::PreparedStatement> blockStmt(
                con->prepareStatement(
                    "SELECT id FROM blocks "
                    "WHERE (user = ? AND blocked_user = ?) OR "
                    "(user = ? AND blocked_user = ?)"
                )
            );
            blockStmt->setString(1, user);
            blockStmt->setString(2, friendName);
            blockStmt->setString(3, friendName);
            blockStmt->setString(4, user);
            
            unique_ptr<sql::ResultSet> blockRes(blockStmt->executeQuery());
            if (blockRes->next()) {
                response["valid"] = 0;
                response["type"] = "valid";
                response["reason"] = "存在屏蔽关系";
                response["success"] = true;
                //send(fd, response.dump().c_str(), response.dump().size(), 0);
                sendLengthPrefixed(fd,response);
                return;
            }
            
            // 所有检查通过
            response["valid"] = 1;
            response["reason"] = "有效好友";
            response["type"] = "valid";
            response["success"] = true;
            
        } catch (sql::SQLException &e) {
            response["success"] = false;
            response["message"] = "数据库错误: " + string(e.what());
        }
        
        //send(fd, response.dump().c_str(), response.dump().size(), 0);
        sendLengthPrefixed(fd,response);
    }
    void Chat::handlePrivateMessage(int fd,const json& request)
    {
        Message msg;
        msg.sender = request["sender"];
        msg.receiver = request["recipient"];
        if(msg.receiver=="===")
        {   onlineing[msg.sender]=false;
            json exitmsg;
            exitmsg["type"]="exit";
            std::string responseStr = exitmsg.dump();
                uint32_t len = htonl(responseStr.size());
                json notification;
                notification["type"]="exit";
                notification["success"]= true;
                notification["sender"]=msg.sender;
                // 先发送长度
                send(fd, &len, sizeof(len), 0);
                // 再发送数据
                send(fd, responseStr.c_str(), responseStr.size(), 0);
                return;

        }
        msg.content = request["message"];
        msg.timestamp = static_cast<time_t>(request["timestamp"]);
        msg.message_id = time(nullptr) * 1000 + rand() % 1000; // 简单实现的消息ID
        
        unique_ptr<sql::Connection> con(getDBConnection());
        unique_ptr<sql::PreparedStatement> blockStmt(
                con->prepareStatement(
                    "SELECT id FROM blocks "
                    "WHERE (user = ? AND blocked_user = ?) OR "
                    "(user = ? AND blocked_user = ?)"
                )
            );
            blockStmt->setString(1, msg.sender);
            blockStmt->setString(2, msg.receiver);
            blockStmt->setString(3, msg.receiver);
            blockStmt->setString(4, msg.sender);
            
            unique_ptr<sql::ResultSet> blockRes(blockStmt->executeQuery());
            if (blockRes->next()) {
                return;
            }
       // 3. 存储消息
        unique_ptr<sql::PreparedStatement> msgStmt(
            con->prepareStatement(
                "INSERT INTO private_messages "
                "(sender, recipient, message, timestamp) "
                "VALUES (?, ?, ?, ?)"
            )
        );
        msgStmt->setString(1, msg.sender);
        msgStmt->setString(2, msg.receiver);
        msgStmt->setString(3, msg.content);
        msgStmt->setUInt64(4, msg.timestamp);
        msgStmt->executeUpdate();
        unique_ptr<sql::Statement> idStmt(con->createStatement());
        unique_ptr<sql::ResultSet> idRes(
            idStmt->executeQuery("SELECT LAST_INSERT_ID() AS id")
        );
        
        uint32_t msgId = 0;
        if (idRes->next()) {
            msg.message_id = idRes->getInt("id");
        }
        // 尝试直接发送给接收者
        bool delivered = false;
        {
            std::lock_guard<std::mutex> lock(online_users_mutex);
            auto it = online_users.find(msg.receiver);
            if (it != online_users.end()&&onlineing[msg.receiver]) {
                cout<<"www"<<endl;
                json realtime_msg;
                realtime_msg["type"] = "private_message";
                realtime_msg["success"]= false;
                realtime_msg["message_id"] = msg.message_id;
                realtime_msg["sender"] = msg.sender;
                realtime_msg["message"] = msg.content;
                realtime_msg["timestamp"] = msg.timestamp;
                // string msg=realtime_msg.dump()+ "\n";
                // // sendResponse(it->second, realtime_msg);
                // send(it->second,msg.c_str(),msg.size(),0);
                std::string responseStr = realtime_msg.dump();
                uint32_t len = htonl(responseStr.size());
                json notification;
                notification["type"]="new_message";
                notification["sender"]=msg.sender;
                addNotification(msg.receiver,notification);
                // 先发送长度
                send(it->second, &len, sizeof(len), 0);
                // 再发送数据
                send(it->second, responseStr.c_str(), responseStr.size(), 0);
                
                delivered = true;
            }
            else if(it != online_users.end())
            {
                json notification;
                notification["type"]="new_message";
                notification["sender"]=msg.sender;
                addNotification(msg.receiver,notification);
            }
        }
    }
    void Chat::handleAckPrivateMessage(int fd, const json& req)
    {   
        //uint32_t messageId = req["message_id"];
        string user = req["user"];
        string sender = req["friend"];

        unique_ptr<sql::Connection> con(getDBConnection());
        if (!con) {
            return; // 不需要响应
        }
        unique_ptr<sql::PreparedStatement> stmt(
            con->prepareStatement(
                "UPDATE private_messages "
                "SET delivered = 1 "
                //"WHERE id = ? AND recipient = ?"
                "WHERE sender = ? AND recipient = ?"
            )
        );
        //stmt->setInt(1, messageId);
        stmt->setString(1, sender);
        stmt->setString(2, user);
        stmt->executeUpdate();
    }   
    void Chat::checkUnreadMessages(int fd, const json& req)
    {

        // 确保请求包含用户字段
        if (!req.contains("user") || !req["user"].is_string()) {
            json errorResponse = {
                {"type", "error"},
                {"message", "无效请求: 缺少用户名字段"}
            };
            string responseStr = errorResponse.dump();
            send(fd, responseStr.c_str(), responseStr.size(), 0);
            return;
        }

        string username = req["user"];
        unique_ptr<sql::Connection> con(getDBConnection());
        
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

        try {
            // 查询未读消息
            unique_ptr<sql::PreparedStatement> msgStmt(
                con->prepareStatement(
                    "SELECT id, sender, message, timestamp "
                    "FROM private_messages "
                    "WHERE recipient  = ? AND delivered = 0 "
                    "ORDER BY timestamp ASC"
                )
            );
            msgStmt->setString(1, username);
            unique_ptr<sql::ResultSet> res(msgStmt->executeQuery());
            
            // 构建响应对象
            json response;
            response["type"] = "unread_messages";
            response["user"] = username;
            int messageCount = 0;
            // 检查是否有未读消息
            if (!res->next()) {
                
            json messagesArray = json::array();
                // 没有未读消息
                response["success"] = true;
                response["message"] = "没有未读消息";
                response["count"] = 0;
                response["messages"] = json::array(); // 空数组
            } else {
                // 有未读消息
                json messagesArray = json::array();
                

                // 重置结果集指针到开头
                res->beforeFirst();

                // 遍历所有未读消息
                while (res->next()) {
                    json messageObj;
                    messageObj["type"]="private";
                    messageObj["sender"] = res->getString("sender");
                    messageObj["timestamp"] = res->getInt64("timestamp");
                    
                    messagesArray.push_back(messageObj);
                    messageCount++;
                }

                response["success"] = true;
                response["count"] = messageCount;
                response["messages"] = messagesArray;
            }
             
            // response["success"] = true;
            // response["messages"] = messagesArray; 
            // 序列化响应为JSON字符串
           // string responseStr = response.dump();
            
            // 添加调试输出
            //cout << "发送响应: " << responseStr << endl;

            // 发送响应给客户端
            // if (send(fd, responseStr.c_str(), responseStr.size(), 0) < 0) {
            //     cerr << "发送未读消息响应失败: " << strerror(errno) << endl;
            // } else {
            //     cout << "已发送未读消息给用户 " << username << endl;
            // }
            std::string responseStr = response.dump();
            cout<<responseStr <<endl;
            uint32_t len = htonl(responseStr.size());
            
            // 先发送长度
            send(fd, &len, sizeof(len), 0);
            // 再发送数据
            send(fd, responseStr.c_str(), responseStr.size(), 0);
            
        } catch (sql::SQLException &e) {
            // 数据库错误处理
            json errorResponse = {
                {"type", "error"},
                {"message", "数据库查询失败: " + string(e.what())}
            };
            string responseStr = errorResponse.dump();
            send(fd, responseStr.c_str(), responseStr.size(), 0);
        } catch (const exception& e) {
            // 其他异常处理
            json errorResponse = {
                {"type", "error"},
                {"message", "处理请求失败: " + string(e.what())}
            };
            string responseStr = errorResponse.dump();
            send(fd, responseStr.c_str(), responseStr.size(), 0);
        }
    }
    
    void Chat::handleGetFriendUnreadMessages(int fd, const json& req)
    {   
        string username = req["user"];
        string friendName = req["friend"];
        unique_ptr<sql::Connection> con(getDBConnection());
        unique_ptr<sql::PreparedStatement> msgStmt(
            con->prepareStatement(
                "SELECT id, message, timestamp "
                "FROM private_messages "
                "WHERE recipient = ? AND sender = ? AND delivered = 0 "
                "ORDER BY timestamp ASC"
            )
        );
        msgStmt->setString(1, username);
        msgStmt->setString(2, friendName);
        unique_ptr<sql::ResultSet> res(msgStmt->executeQuery());
        
        json response;
        response["type"] = "friend_unread_messages";
        response["user"] = username;
        response["friend"] = friendName;
        if (!res->next()) {
            // 没有未读消息
           
            response["success"] = false;
            response["message"] = "没有未读消息";
            response["count"] = 0;
            response["messages"] = json::array();
            
        }
        else
        {
            json messagesArray = json::array();
            // 重置结果集指针到开头
            res->beforeFirst();
            // 遍历所有未读消息
            while (res->next()) {
                json messageObj;
                messageObj["id"] = res->getInt("id");
                messageObj["message"] = res->getString("message");
                messageObj["timestamp"] = res->getInt64("timestamp");
                
                messagesArray.push_back(messageObj);
            }

            response["success"] = true;
            response["messages"] = messagesArray;
        }
        
        // // 发送响应
        // string responseStr = response.dump()+"\n";
        // send(fd, responseStr.c_str(), responseStr.size(), 0);
        std::string responseStr = response.dump();
        uint32_t len = htonl(responseStr.size());
        
        // 先发送长度
        send(fd, &len, sizeof(len), 0);
        // 再发送数据
        send(fd, responseStr.c_str(), responseStr.size(), 0);
        onlineing[username]=true;
        
    }
    void Chat::handleChatHistoryRequest(int fd, const json& request)
    {
        // 验证请求参数
        if (!request.contains("user") || !request["user"].is_string() ||
            !request.contains("friend") || !request["friend"].is_string()) {
            json error = {
                {"type", "error"},
                {"message", "无效请求: 缺少必要字段"}
            };
            send(fd, error.dump().c_str(), error.dump().size(), 0);
            return;
        }
        
        string user = request["user"];
        string friendName = request["friend"];
        
        unique_ptr<sql::Connection> con(getDBConnection());
        if (!con) {
            json error = {
                {"type", "error"},
                {"message", "数据库连接失败"}
            };
            send(fd, error.dump().c_str(), error.dump().size(), 0);
            return;
        }
        
        try {
            // 准备SQL查询
            unique_ptr<sql::PreparedStatement> msgStmt(
                con->prepareStatement(
                    "SELECT sender, recipient, message, timestamp "
                    "FROM private_messages "
                    "WHERE (sender = ? AND recipient = ?) OR (sender = ? AND recipient = ?) "
                    "ORDER BY timestamp ASC" // 按时间升序（从旧到新）
                )
            );
            msgStmt->setString(1, user);
            msgStmt->setString(2, friendName);
            msgStmt->setString(3, friendName);
            msgStmt->setString(4, user);
            
            // 执行查询
            unique_ptr<sql::ResultSet> res(msgStmt->executeQuery());
            
            // 构建响应
            json response;
            response["user"] = user;
            response["friend"] = friendName;
            
            // 检查是否有消息
            if (!res->next()) {
                // 没有消息
                response["success"] = true;
                response["count"] = 0;
                response["messages"] = json::array();
            } else {
                // 有消息
                json messagesArray = json::array();
                int messageCount = 0;
                
                // 重置结果集指针到开头
                res->beforeFirst();
                
                // 遍历所有消息
                while (res->next()) {
                    json messageObj;
                    
                    // 获取字段值
                    string sender = res->getString("sender");
                    string recipient = res->getString("recipient");
                    string message = res->getString("message");
                    time_t timestamp = res->getInt64("timestamp"); // 关键修复：使用getInt64获取时间戳
                    
                    // 构建消息对象
                    messageObj["sender"] = sender;
                    messageObj["recipient"] = recipient;
                    messageObj["message"] = message;
                    messageObj["timestamp"] = timestamp; // 存储为整数
                    
                    messagesArray.push_back(messageObj);
                    
                }
                
                response["success"] = true;
                response["messages"] = messagesArray;
            }
            
            // 发送响应
            string responseStr = response.dump();
            send(fd, responseStr.c_str(), responseStr.size(), 0);
            
        } catch (sql::SQLException &e) {
            // 数据库错误处理
            json error = {
                {"type", "error"},
                {"message", "数据库查询失败: " + string(e.what())}
            };
            string responseStr = error.dump();
            send(fd, responseStr.c_str(), responseStr.size(), 0);
        } catch (const exception& e) {
            // 其他异常处理
            json error = {
                {"type", "error"},
                {"message", "处理请求失败: " + string(e.what())}
            };
            string responseStr = error.dump();
            send(fd, responseStr.c_str(), responseStr.size(), 0);
        }
    }