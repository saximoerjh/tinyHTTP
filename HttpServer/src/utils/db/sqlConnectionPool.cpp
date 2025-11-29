#include "utils/db/sqlConnectionPool.h"

sqlConnectionPool::sqlConnectionPool()
{
    LOG_INFO << "create sqlConnectionPool";
    // 读取配置json文件 在项目目录下
    std::fstream cfgFile("../dbconfig.json");

    if (!cfgFile.is_open())
    {
        LOG_ERROR << "database config file not found";
    }

    try
    {
        nlohmann::json configJson;
        cfgFile >> configJson;
        std::cout << configJson << std::endl;

        host_ = configJson["host"].get<std::string>();
        port_ = configJson["port"].get<unsigned int>();
        user_ = configJson["user"].get<std::string>();
        password_ = configJson["password"].get<std::string>();
        initPoolSize_ = configJson["initPoolSize"].get<int>();
        poolSize_ = configJson["poolSize"].get<int>();
        connTimeOut_ = configJson["connTimeout"].get<int>();
        maxIdleTime_ = configJson["maxIdleTime"].get<int>();
        database_ = configJson["database"].get<std::string>();
    }
    catch (std::exception& e)
    {
        LOG_ERROR << e.what();
    }

    for (int i = 0; i < initPoolSize_; i++)
    {
        auto conn = std::make_shared<sqlConnection>(host_, user_, password_, database_, port_);
        connQueue_.push(conn);
        qSize_.fetch_add(1);
    }

    producer_ = std::thread(&sqlConnectionPool::connProducer, this);
    deletor_ = std::thread(&sqlConnectionPool::connDeletor, this);
}

std::shared_ptr<sqlConnection> sqlConnectionPool::getConnection()
{
    if (!run_.load()) return nullptr;
    std::unique_lock<std::mutex> lock(mtx_);

    // 等待队列非空或线程要退出
    cv_.wait(lock, [this] { return !connQueue_.empty() || !run_.load(); });

    if (!run_.load() && connQueue_.empty()) return nullptr;

    auto conn = connQueue_.front();
    connQueue_.pop();

    // 返回连接，将其删除器改为向队列中归还连接
    std::shared_ptr<sqlConnection> sp(conn.get(),
           [this, conn](sqlConnection*) {
               std::lock_guard<std::mutex> lock(mtx_);
               connQueue_.push(conn);
               cv_.notify_one();
           });
    // 通知其他线程，不然会造成死锁
    cv_.notify_all();
    return sp;
}

sqlConnectionPool& sqlConnectionPool::getInstance()
{
    static sqlConnectionPool instance;
    return instance;
}

void sqlConnectionPool::connDeletor()
{
    while (run_.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(maxIdleTime_));
        if (!run_.load()) break;

        std::lock_guard<std::mutex> lock(mtx_);
        while (qSize_.load() > initPoolSize_ && !connQueue_.empty())
        {
            auto conn = connQueue_.front();
            auto end = conn->getConnIdleTime();
            if (end >= maxIdleTime_)
            {
                connQueue_.pop();
                qSize_.fetch_sub(1);
                conn.reset();
            }
            else break;
        }
    }
}

void sqlConnectionPool::connProducer()
{
    while (run_.load())
    {
        std::unique_lock<std::mutex> lock(mtx_);
        // 等待队列为空或线程要退出
        cv_.wait(lock, [this] { return connQueue_.empty() || !run_.load(); });

        if (!run_.load()) break;

        if (qSize_.load() < poolSize_)
        {
            auto conn = std::make_shared<sqlConnection>(host_, user_, password_, database_, port_);
            connQueue_.push(conn);
            qSize_.fetch_add(1);
        }

        // 通知消费者线程，可以消费连接了
        cv_.notify_all();
    }
}

sqlConnectionPool::~sqlConnectionPool()
{
    LOG_INFO << "destroy sqlConnections";
    run_.store(false);

    // 唤醒所有等待的线程/消费者
    cv_.notify_all();

    // 等待后台线程结束
    if (producer_.joinable()) producer_.join();
    if (deletor_.joinable()) deletor_.join();

    // 清理连接队列
    std::unique_lock<std::mutex> lock(mtx_);
    while (!connQueue_.empty())
    {
        connQueue_.pop();
    }

    // 通知线程结束
    cv_.notify_all();
}
