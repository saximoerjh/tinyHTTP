#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <fstream>
#include <nlohmann/json.hpp>
#include <mysql/mysql.h>
#include "../include/utils/db/sqlConnectionPool.h"
#include "../include/utils/db/sqlConnection.h"

struct DBConfig {
    std::string host;
    std::string user;
    std::string password;
    std::string database;
    unsigned int port{0};
};

// 读取配置文件（与连接池保持一致路径）
DBConfig loadConfig(const std::string& path = "../dbconfig.json") {
    DBConfig cfg;
    std::fstream cfgFile(path);
    if (!cfgFile.is_open()) {
        std::cerr << "[ERROR] Cannot open config file: " << path << std::endl;
        return cfg;
    }
    try {
        nlohmann::json j; cfgFile >> j;
        cfg.host = j["host"].get<std::string>();
        cfg.user = j["user"].get<std::string>();
        cfg.password = j["password"].get<std::string>();
        cfg.database = j["database"].get<std::string>();
        cfg.port = j["port"].get<unsigned int>();
    } catch (std::exception& e) {
        std::cerr << "[ERROR] Parse config failed: " << e.what() << std::endl;
    }
    return cfg;
}

// 使用连接池进行多线程查询
long long benchmarkWithPool(int threads, int queriesPerThread) {
    auto& pool = sqlConnectionPool::getInstance();
    std::atomic<int> okCount{0};
    auto start = std::chrono::steady_clock::now();
    std::vector<std::thread> ths;
    ths.reserve(threads);
    for (int t = 0; t < threads; ++t) {
        ths.emplace_back([queriesPerThread, &okCount]() {
            for (int i = 0; i < queriesPerThread; ++i) {
                auto conn = sqlConnectionPool::getInstance().getConnection();
                if (!conn || !conn->isValid()) continue;
                MYSQL_RES* res = conn->executeQuery("SELECT 1");
                if (res) {
                    mysql_free_result(res);
                    okCount.fetch_add(1, std::memory_order_relaxed);
                }
                conn->refreshTime();
            }
        });
    }
    for (auto& th : ths) th.join();
    auto end = std::chrono::steady_clock::now();
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[Pool] Executed " << okCount.load() << " successful queries in " << ms << " ms" << std::endl;
    return ms;
}

// 不使用连接池：每次查询都新建+销毁一个连接（最差情况）
long long benchmarkWithoutPool(const DBConfig& cfg, int threads, int queriesPerThread) {
    std::atomic<int> okCount{0};
    auto start = std::chrono::steady_clock::now();
    std::vector<std::thread> ths;
    ths.reserve(threads);
    for (int t = 0; t < threads; ++t) {
        ths.emplace_back([&cfg, queriesPerThread, &okCount]() {
            for (int i = 0; i < queriesPerThread; ++i) {
                sqlConnection conn(cfg.host, cfg.user, cfg.password, cfg.database, cfg.port);
                if (!conn.isValid()) continue;
                MYSQL_RES* res = conn.executeQuery("SELECT 1");
                if (res) {
                    mysql_free_result(res);
                    okCount.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& th : ths) th.join();
    auto end = std::chrono::steady_clock::now();
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[NoPool] Executed " << okCount.load() << " successful queries in " << ms << " ms" << std::endl;
    return ms;
}

int main() {
    int threads = 8;
    int queriesPerThread = 50;

    std::cout << "=== Connection Pool Benchmark ===" << std::endl;

    DBConfig cfg = loadConfig();
    if (cfg.host.empty()) {
        std::cerr << "[ERROR] Config invalid, abort benchmark" << std::endl;
        return 1;
    }

    auto warmConn = sqlConnectionPool::getInstance().getConnection();
    if (!warmConn || !warmConn->isValid()) {
        std::cerr << "[ERROR] Failed to get initial pooled connection" << std::endl;
        return 2;
    }
    MYSQL_RES* warmRes = warmConn->executeQuery("SELECT 1");
    if (warmRes) mysql_free_result(warmRes);

    // 归还连接（暂时解决死锁）
    // warmConn.reset();

    long long poolMs = benchmarkWithPool(threads, queriesPerThread);
    long long noPoolMs = benchmarkWithoutPool(cfg, threads, queriesPerThread);

    std::cout << "-------------------------------------" << std::endl;
    std::cout << "Threads: " << threads << ", Queries/Thread: " << queriesPerThread << std::endl;
    std::cout << "With Pool Time:    " << poolMs << " ms" << std::endl;
    std::cout << "Without Pool Time: " << noPoolMs << " ms" << std::endl;
    if (poolMs > 0) {
        std::cout << "Speedup (NoPool/Pool): " << std::fixed << (double)noPoolMs / (double)poolMs << "x" << std::endl;
    }

    return 0;
}