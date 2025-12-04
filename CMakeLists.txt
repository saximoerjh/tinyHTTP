# CMake
cmake_minimum_required(VERSION 3.16)
project(tinyHTTP)
set(CMAKE_CXX_STANDARD 17)

# 源文件
file(GLOB_RECURSE HTTP_SERVER_SRC
        "${PROJECT_SOURCE_DIR}/HttpServer/src/*.cpp"
)

# 单元测试文件
set(TEST_CONNPOOL_SRC "${PROJECT_SOURCE_DIR}/HttpServer/examples/testConnPool.cpp")
set(TEST_HTTPCONTEXT_SRC "${PROJECT_SOURCE_DIR}/HttpServer/examples/testHttpContext.cpp")
set(TEST_ROUTER_SRC "${PROJECT_SOURCE_DIR}/HttpServer/examples/testRouter.cpp")

add_executable(tinyHTTP
        ${TEST_ROUTER_SRC}
        ${HTTP_SERVER_SRC}
)
# 关键：把 HttpServer/include 加到目标的包含路径
target_include_directories(tinyHTTP PRIVATE
        ${PROJECT_SOURCE_DIR}/HttpServer/include
        ${PROJECT_SOURCE_DIR}/HttpServer
        /usr/include/mysql-cppconn-8
        /usr/include/mysql
)

target_link_libraries(tinyHTTP
        pthread
        muduo_net
        muduo_base
        mysqlcppconn
        mysqlclient
        ssl
        crypto
        gtest
)
