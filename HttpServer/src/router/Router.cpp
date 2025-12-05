#include "../../include/router/Router.h"
#include <muduo/base/Logging.h>

namespace tinyHttp
{
    // 注册静态路由处理器
    void Router::registerHandler(HttpRequest::Method method, const std::string &path, HandlerPtr handler)
    {
        // key由请求方法和路径组成
        RouteKey key{method, path};
        handlers_[key] = std::move(handler);
    }

    void Router::registerCallback(HttpRequest::Method method, const std::string &path, const HandlerCallback &callback)
    {
        RouteKey key{method, path};
        callbacks_[key] = std::move(callback);
    }

    bool Router::route(const HttpRequest &req, HttpResponse *resp)
    {
        RouteKey key{req.method(), req.path()};

        // 先在注册的静态路由中查找
        // 查找处理器
        auto handlerIt = handlers_.find(key);
        if (handlerIt != handlers_.end())
        {
            // 找到处理器则执行
            handlerIt->second->handle(req, resp);
            return true;
        }

        // 查找回调函数
        auto callbackIt = callbacks_.find(key);
        if (callbackIt != callbacks_.end())
        {
            callbackIt->second(req, resp);
            return true;
        }

        // 查找动态路由处理器 使用之前注册的正则表达式进行匹配
        /*
         * 注册动态路由时，路径会被转换为正则表达式进行匹配
         * 动态路由使查询url相应的回调时，即使url中带有路径参数(user/:123)也能正确匹配到对应的回调
         */
        for (const auto &[method, pathRegex, handler] : regexHandlers_)
        {
            std::smatch match;
            // 待匹配字符串
            std::string pathStr(req.path());
            // 如果方法匹配并且动态路由匹配，则执行处理器
            // match用于存储正则表达式匹配结果，返回一个结果数组
            if (method == req.method() && std::regex_match(pathStr, match, pathRegex))
            {
                // 复制构造一个新的请求对象以便修改
                HttpRequest newReq(req);
                extractPathParameters(match, newReq);

                handler->handle(newReq, resp);
                return true;
            }
        }

        // 查找动态路由回调函数
        for (const auto &[method, pathRegex, callback] : regexCallbacks_)
        {
            std::smatch match;
            std::string pathStr(req.path());
            // 如果方法匹配并且动态路由匹配，则执行回调函数
            if (method == req.method() && std::regex_match(pathStr, match, pathRegex))
            {
                // Extract path parameters and add them to the request
                HttpRequest newReq(req); // 因为这里需要用这一次所以是可以改的
                extractPathParameters(match, newReq);

                callback(req, resp);
                return true;
            }
        }

        return false;
    }
}