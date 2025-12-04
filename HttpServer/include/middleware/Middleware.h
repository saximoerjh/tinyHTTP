#pragma once
#include "../http/HttpRequest.h"
#include "http/HttpResponse.h"

class Middleware
{
public:
    virtual ~Middleware() = default;

    virtual void before(HttpRequest& request) = 0;
    virtual void after(HttpResponse& response) = 0;

    void setNext(std::shared_ptr<Middleware> next)
    {
        nextMiddleware_ = next;
    }

    // 可选：提供链式调用的接口（方便组装中间件链）
    virtual std::shared_ptr<Middleware> getNext() const {
        return nextMiddleware_;
    }

protected:
    std::shared_ptr<Middleware> nextMiddleware_;
};