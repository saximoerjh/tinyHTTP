#pragma once
#include <atomic>

#include "sqlConnection.h"
#include <condition_variable>
#include <nlohmann/json.hpp>
#include <fstream>
#include <queue>

class sqlConnectionPool
{
public:
    static sqlConnectionPool& getInstance();

    sqlConnectionPool(const sqlConnectionPool&) = delete;
    sqlConnectionPool& operator=(const sqlConnectionPool&) = delete;

    ~sqlConnectionPool();

    std::shared_ptr<sqlConnection> getConnection();

    // 删除闲置连接
    void connDeletor();
    // 生产新连接
    void connProducer();

private:
    sqlConnectionPool();

    // 数据库连接参数
    std::string host_;
    std::string user_;
    std::string password_;
    std::string database_;
    unsigned int port_;

    // 连接池
    std::queue<std::shared_ptr<sqlConnection>> connQueue_;
    std::mutex mtx_;
    std::size_t poolSize_;
    std::size_t initPoolSize_;
    std::condition_variable cv_;

    int connTimeOut_;
    int maxIdleTime_;

    std::atomic<int> qSize_{0};

    std::atomic<bool> run_{true};
};
