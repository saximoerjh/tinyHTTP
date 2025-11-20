#include "utils/db/sqlConnectionPool.h"

#include <thread>

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

    std::thread produce(std::bind(&sqlConnectionPool::connProducer, this));
    produce.detach();

    // 启动一个新的定时线程，扫描超过maxIdleTime时间的空闲连接，进行对于的连接回收
    std::thread deletor(std::bind(&sqlConnectionPool::connDeletor, this));
    deletor.detach();

}

std::shared_ptr<sqlConnection> sqlConnectionPool::getConnection()
{
    if (!run_) return nullptr;
    std::unique_lock<std::mutex> lock(mtx_);
    LOG_INFO << "sqlConnectionPool::getConnection";

    while (connQueue_.empty())
    {
        cv_.wait(lock);
    }

    // 获取到队列控制权，取出连接
    auto conn = connQueue_.front();
    connQueue_.pop();

    // 返回连接，将其删除器改为向队列中归还连接
    return std::shared_ptr<sqlConnection>(conn.get(),
           [this, conn](sqlConnection*) {
               std::lock_guard<std::mutex> lock(mtx_);
               connQueue_.push(conn);
               cv_.notify_one();
           });
}

sqlConnectionPool& sqlConnectionPool::getInstance()
{
    static sqlConnectionPool instance;
    return instance;
}

void sqlConnectionPool::connDeletor()
{
    // 扫描超过最大空闲时间的连接并删除
    for (;;)
    {
        // 休眠一定时间后再扫描
        std::this_thread::sleep_for(std::chrono::milliseconds(maxIdleTime_));

        std::lock_guard<std::mutex> lock(mtx_);
        // 发现当前线程数量多于初始化数量
        while (qSize_.load() > initPoolSize_)
        {
            LOG_INFO << "delete idle connection";
            auto conn = connQueue_.front();
            // 计算idle时间
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
    for (;;)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        while (!connQueue_.empty())
        {
            cv_.wait(lock); // 队列不空，此处生产线程进入等待状态
        }

        // 连接数量没有到达上限，继续创建新的连接
        if (qSize_ < poolSize_)
        {
            LOG_INFO << "Create new connection";
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
    run_ = false;
    // 等待所有连接任务执行完成
    std::unique_lock<std::mutex> lock(mtx_);

    while (connQueue_.size() != qSize_)
    {
        cv_.wait(lock);
    }

    // 拿到控制权，开始释放资源
    while (connQueue_.size() > 0)
    {
        connQueue_.pop();
    }
}
