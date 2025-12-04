#include "./gtest/gtest.h"
#include <chrono>
#include <iostream>
#include <atomic>
#include "router/Router.h"
#include "http/HttpResponse.h"
#include "http/HttpRequest.h"

// 注意：下面的 makeRequest 可能需根据你项目中 HttpRequest 的实际 API 调整。
// 假设存在默认构造 + setMethod/setPath 或构造函数 (Method, path)。
// 如果项目提供不同构造，请在此处替换为真实构造调用。
// 该函数用于测试过程中快速构造 HttpRequest 对象。
static HttpRequest makeRequest(HttpRequest::Method method, const std::string &path)
{
    HttpRequest req;

    req.setMethod(method);
    req.setPath(path);

    return req;
}

// 基本行为测试：精确匹配回调
TEST(RouterTest, ExactCallbackMatch)
{
    Router router;
    std::atomic<int> hitCount{0};

    // 注册一个简单的回调函数路由
    router.registerCallback(HttpRequest::Method::kGet, "/test/exact", [&](const HttpRequest &req, HttpResponse *resp) {
        (void)req; (void)resp;
        ++hitCount;
    });

    // 构造请求对象
    HttpRequest req = makeRequest(HttpRequest::Method::kGet, "/test/exact");

    const int iterations = 10;
    for (int i = 0; i < iterations; ++i)
    {
        bool routed = router.route(req, nullptr);
        (void)routed;
    }

    std::cout << "ExactCallbackMatch: expected=" << iterations
              << ", actual=" << hitCount.load() << "\n";

    ASSERT_EQ(hitCount.load(), iterations);
}

// 精确匹配对象式处理器（RouterHandler）
// 需要项目中已定义 RouterHandler 基类且接口为 virtual void handle(...)
class TestHandler : public RouterHandler
{
public:
    std::atomic<int> &counter_;
    explicit TestHandler(std::atomic<int> &c) : counter_(c) {}
    void handle(const HttpRequest &req, HttpResponse *resp) override
    {
        (void)req; (void)resp;
        ++counter_;
    }
};

TEST(RouterTest, ExactObjectHandlerMatch)
{
    Router router;
    std::atomic<int> hitCount{0};

    auto handler = std::make_shared<TestHandler>(hitCount);
    router.registerHandler(HttpRequest::Method::kPost, "/obj/handle", handler);

    HttpRequest req = makeRequest(HttpRequest::Method::kPost, "/obj/handle");

    const int iterations = 7;
    for (int i = 0; i < iterations; ++i)
        router.route(req, nullptr);

    std::cout << "ExactObjectHandlerMatch: expected=" << iterations
              << ", actual=" << hitCount.load() << "\n";

    ASSERT_EQ(hitCount.load(), iterations);
}

// 正则路由匹配（动态路由）和失败分支测试
TEST(RouterTest, RegexCallbackMatchAndMiss)
{
    Router router;
    std::atomic<int> hitCount{0};

    // 动态路由模式示例：/user/:id/profile/:section
    router.addRegexCallback(HttpRequest::Method::kGet, "/user/:id/profile/:section",
                            [&](const HttpRequest &req, HttpResponse *resp) {
                                (void)req; (void)resp;
                                ++hitCount;
                            });

    HttpRequest good = makeRequest(HttpRequest::Method::kGet, "/user/123/profile/overview");
    HttpRequest bad1 = makeRequest(HttpRequest::Method::kGet, "/user//profile/overview");
    HttpRequest bad2 = makeRequest(HttpRequest::Method::kPost, "/user/123/profile/overview"); // method mismatch

    bool r1 = router.route(good, nullptr);
    bool r2 = router.route(bad1, nullptr);
    bool r3 = router.route(bad2, nullptr);

    std::cout << "RegexCallbackMatchAndMiss: good_routed=" << r1
              << ", bad1_routed=" << r2 << ", bad2_routed=" << r3
              << ", callback_hits=" << hitCount.load() << "\n";

    ASSERT_TRUE(r1);
    ASSERT_FALSE(r2);
    ASSERT_FALSE(r3);
    ASSERT_EQ(hitCount.load(), 1);
}

// 性能与吞吐量测试（测量路由平均延迟、总耗时、吞吐量）
// 说明：此测试为性能基准，运行大量次以获取数值指标，作为回归检测可调低次数。
TEST(RouterTest, ThroughputAndLatency)
{
    Router router;
    std::atomic<int> hitCount{0};

    router.registerCallback(HttpRequest::Method::kGet, "/perf/test", [&](const HttpRequest &req, HttpResponse *resp) {
        (void)req; (void)resp;
        ++hitCount;
    });

    HttpRequest req = makeRequest(HttpRequest::Method::kGet, "/perf/test");

    const int totalRequests = 10000; // 如 CI 时间有限可减少到 1000
    using clock = std::chrono::high_resolution_clock;
    auto t0 = clock::now();
    for (int i = 0; i < totalRequests; ++i)
    {
        router.route(req, nullptr);
    }
    auto t1 = clock::now();

    auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    double avg_us = static_cast<double>(total_us) / totalRequests;
    double throughput = (totalRequests * 1e6) / static_cast<double>(total_us); // req/s
    double success_rate = (100.0 * hitCount.load()) / totalRequests;

    std::cout << "ThroughputAndLatency: total_requests=" << totalRequests
              << ", hits=" << hitCount.load()
              << ", total_us=" << total_us
              << ", avg_us=" << avg_us
              << ", req_per_s=" << throughput
              << ", success_rate_percent=" << success_rate << "\n";

    ASSERT_EQ(hitCount.load(), totalRequests);
    ASSERT_GT(throughput, 0.0);
}

// 路由失败率与边界测试：不存在路由
TEST(RouterTest, NotFoundRoute)
{
    Router router;
    HttpRequest req = makeRequest(HttpRequest::Method::kGet, "/does/not/exist");
    bool routed = router.route(req, nullptr);

    std::cout << "NotFoundRoute: routed=" << routed << "\n";
    ASSERT_FALSE(routed);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
