#pragma once

#include <vector>

#include <memory>

#include <algorithm>
#include "Middleware.h"

namespace tinyHttp
{
    class MiddlewareChain
    {
    public:
        // 关键修改：参数直接接收 shared_ptr<Middleware>（子类指针可隐式传入）
        void registerMiddleware(std::shared_ptr<Middleware> middleware)
        {
            if (!middleware) {
                return; // 避免传入空指针
            }

            // 直接存储子类的 shared_ptr（指向基类，保持多态）
            middlewares_.emplace_back(std::move(middleware));

            // 可选：自动串联下一个中间件（如果需要链式调用）
            if (middlewares_.size() >= 2) {
                auto& prev = middlewares_[middlewares_.size() - 2];
                auto& curr = middlewares_[middlewares_.size() - 1];
                prev->setNext(curr);
            }
        }

        // 示例：执行所有中间件的 before 方法（按注册顺序）
        void handleRequest(HttpRequest& request) const
        {
            for (auto& mid : middlewares_) {
                mid->before(request);
            }
        }

        // 示例：执行所有中间件的 after 方法（按注册逆序，符合中间件逻辑）
        void handleResponse(HttpResponse& response) {
            for (auto it = middlewares_.rbegin(); it != middlewares_.rend(); ++it) {
                (*it)->after(response);
            }
        }

    private:
        // 存储基类的 shared_ptr（实际指向子类实例）
        std::vector<std::shared_ptr<Middleware>> middlewares_;
    };
}
