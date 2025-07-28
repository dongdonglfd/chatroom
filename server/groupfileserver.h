#include"fileserver.h"
class groupfileserver
{private:
    mutex uploadMutex;
    mutex data_mutex;
    int data_sock=-1;
    int listen_sock;
    FileTransferServer file;
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
        unique_ptr<sql::Connection> con(file.getDBConnection());
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
            {"message", fileName},
            {"timestamp", timestamp}
        };
        broadcastToGroup(groupid, broadcast, user);
    }
    void broadcastToGroup(int group_id, const json& message, const string& excludeUser = "")
    {
        unique_ptr<sql::Connection> con(file.getDBConnection());
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
            for (const auto& [username, id] : online_users) {
            cout << "Username: " << username << ", ID: " << id << endl;
        }
        
    }
    


};