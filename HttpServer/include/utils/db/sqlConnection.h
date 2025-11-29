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

    // 执行查询操作，返回结果集指针。调用方需使用 mysql_free_result 释放。
    MYSQL_RES* executeQuery(const std::string& sql) const;

    // 执行更新操作，返回受影响行数，失败返回 -1。
    int executeUpdate(const std::string& sql) const;

    // 返回空闲时间(ms) —— 距离上次 refreshTime 的时间差
    int getConnIdleTime() const;

    // 重置内部计时器
    void refreshTime();

    // 判断连接是否有效
    bool isValid() const { return conn_ != nullptr; }

private:
    MYSQL* conn_;
    std::chrono::system_clock::time_point executeTime_;
};