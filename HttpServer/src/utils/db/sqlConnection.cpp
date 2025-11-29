#include "utils/db/sqlConnection.h"


sqlConnection::sqlConnection(
    const std::string& host,
    const std::string& user,
    const std::string& password,
    const std::string& database,
    const unsigned int& port)
{
    // 初始化 MYSQL 句柄
    MYSQL* handle = mysql_init(nullptr);
    if (handle == nullptr)
    {
        LOG_ERROR << "mysql_init failed";
        conn_ = nullptr;
        return;
    }
    // 建立数据库连接
    conn_ = mysql_real_connect(handle, host.c_str(), user.c_str(),
                                password.c_str(), database.c_str(), port, nullptr, 0);
    if (conn_ == nullptr)
    {
        LOG_ERROR << "Failed to connect to database: " << mysql_error(handle);
        mysql_close(handle);
        return;
    }
    // 设置当前时间
    refreshTime();
}

sqlConnection::~sqlConnection()
{
    if (conn_ != nullptr)
    {
        mysql_close(conn_);
        conn_ = nullptr;
    }
}

MYSQL_RES* sqlConnection::executeQuery(const std::string& sql) const
{
    if (conn_ == nullptr)
    {
        LOG_ERROR << "executeQuery called on invalid connection";
        return nullptr;
    }
    if (mysql_query(conn_, sql.c_str()) != 0)
    {
        LOG_ERROR << "Query failed: " << mysql_error(conn_);
        return nullptr;
    }
    MYSQL_RES* result = mysql_store_result(conn_);
    return result; // 由调用方释放 mysql_free_result(result)
}

int sqlConnection::executeUpdate(const std::string& sql) const
{
    if (conn_ == nullptr)
    {
        LOG_ERROR << "executeUpdate called on invalid connection";
        return -1;
    }
    if (mysql_query(conn_, sql.c_str()) != 0)
    {
        LOG_ERROR << "Update failed: " << mysql_error(conn_);
        return -1;
    }
    return mysql_affected_rows(conn_);
}

int sqlConnection::getConnIdleTime() const
{
    // 返回距上次刷新时间的毫秒数
    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - executeTime_).count();
    return static_cast<int>(diff);
}

void sqlConnection::refreshTime()
{
    executeTime_ = std::chrono::system_clock::now();
}
