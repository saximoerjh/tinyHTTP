#pragma once
#include <string>
#include <memory>
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

// 抽象路由处理器类，所有自定义路由处理器需继承该类并实现handle方法
class RouterHandler
{
public:
    virtual ~RouterHandler() = default;

    // 纯虚函数，处理HTTP请求并生成响应
    virtual void handle(const HttpRequest& req, HttpResponse* resp) = 0;
};

