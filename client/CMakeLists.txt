cmake_minimum_required(VERSION 3.13)
project(chat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件
include_directories(/usr/local/include)

# 链接库路径
link_directories(/usr/local/lib)
# 查找库
find_package(nlohmann_json 3.2.0 REQUIRED)
# 查找 OpenSSL 和 libcurl
find_package(OpenSSL REQUIRED)
# 添加可执行文件
set(SERVER_HEADERS
   fileclient.h
   friend.h
   group.h
   groupchat.h
   groupfile.h 
   chatclient.h
)
set(SERVER_SOURCES
    fileclient.cc
    friend.cc
    group.cc
    groupchat.cc
    groupfile.cc 
    chatclient.cc
    client.cpp
)
add_executable(client ${SERVER_SOURCES})
# 链接 cpp_redis 和 tacopie
target_link_libraries(server
    PRIVATE
    pthread
    nlohmann_json::nlohmann_json
    OpenSSL::SSL 
    OpenSSL::Crypto

)
target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)