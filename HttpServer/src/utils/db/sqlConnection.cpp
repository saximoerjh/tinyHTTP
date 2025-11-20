#include "../../../include/utils/db/sqlConnection.h"


sqlConnection::sqlConnection(
    const std::string& host,
    const std::string& user,
    const std::string& password,
    const std::string& database,
    const unsigned int& port)
{
    // 建立数据库连接
    conn_ = mysql_real_connect(nullptr, host.c_str(), user.c_str(),
                                password.c_str(), database.c_str(), port, nullptr, 0);
    if (conn_ == nullptr)
    {
        LOG_ERROR << "Failed to connect to database: " << mysql_error(conn_);
    }
    // 设置当前时间
    refreshTime();
}

sqlConnection::~sqlConnection()
{
    if (conn_ != nullptr)
    {
        mysql_close(conn_);
        // LOG_INFO << "connection closed";
    }
}

MYSQL_RES& sqlConnection::executeQuery(const std::string& sql) const
{
    // 执行查询语句
    if (mysql_query(conn_, sql.c_str()) != 0)
    {
        // 返回非0值即为查询失败
        LOG_ERROR << "Query failed: " << mysql_error(conn_);
    }
    MYSQL_RES* result = mysql_store_result(conn_);
    return *result;
}

int sqlConnection::executeUpdate(const std::string& sql) const
{
    // 执行更新语句
    if (mysql_query(conn_, sql.c_str()) != 0)
    {
        // 返回非0值即为更新失败
        LOG_ERROR << "Update failed: " << mysql_error(conn_);
        return -1;
    }
    return mysql_affected_rows(conn_);
}

int sqlConnection::getConnIdleTime() const
{
    // 返回以milisecond为单位的idle时间
    auto idleTime = executeTime_.time_since_epoch().count();
    return idleTime;
}

void sqlConnection::refreshTime()
{
    executeTime_ = std::chrono::system_clock::now();
}


