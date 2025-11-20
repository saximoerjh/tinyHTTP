#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <chrono>
#include <cppconn/connection.h>
#include <mysql/mysql.h>
#include <muduo/base/Logging.h>

/*
 * 连接类 提供连接的初始化以及数据库的查询、更新操作
 */
class sqlConnection
{
public:
    sqlConnection(const std::string& host,
                const std::string& user,
                const std::string& password,
                const std::string& database,
                const unsigned int& port);
    ~sqlConnection();

    // 禁止拷贝
    sqlConnection(const sqlConnection&) = delete;
    sqlConnection& operator=(const sqlConnection&) = delete;

    // 对数据库进行查询并返回结果集
    MYSQL_RES& executeQuery(const std::string& sql) const;

    // 对数据库进行更新操作，返回受影响的行数
    int executeUpdate(const std::string& sql) const;

    // 返回连接闲置时间
    int getConnIdleTime() const;

    // 重置内置时钟
    void refreshTime();

private:
    MYSQL* conn_;
    std::chrono::system_clock::time_point executeTime_;
};