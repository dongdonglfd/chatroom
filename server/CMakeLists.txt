cmake_minimum_required(VERSION 3.13)
project(chat)
find_package(CURL REQUIRED)
# 查找 OpenSSL 和 libcurl
find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件
include_directories(/usr/local/include)

# 链接库路径
link_directories(/usr/local/lib)
# 查找库
find_package(nlohmann_json 3.2.0 REQUIRED)
# 添加可执行文件
set(SERVER_HEADERS
    chatserver.h
    fileserver.h
    Friendserver.h
    groupchatserver.h
    groupfileserver.h
    groupserver.h
    threadpool.h
    notification.h
    )
set(SERVER_SOURCES
    server.cpp
)
add_executable(server ${SERVER_SOURCES})
# 链接 cpp_redis 和 tacopie
target_link_libraries(server
    PRIVATE
    pthread
    nlohmann_json::nlohmann_json
    CURL::libcurl
    mysqlcppconn       # 手动指定（假设已安装）
    OpenSSL::SSL       # 由 find_package(OpenSSL) 提供
    OpenSSL::Crypto   
)
target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)