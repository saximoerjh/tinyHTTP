#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <muduo/base/Timestamp.h>
#include <regex>
#include <utility>
#include <nlohmann/json.hpp>

class HttpRequest
{
public:
    // HTTP请求方法枚举
    enum Method
    {
        kInvalid, kGet, kPost, kHead, kPut, kDelete, kOptions
    };

    // 构造函数初始化成员变量
    HttpRequest()
        : method_(kInvalid)
        , version_("Unknown")
        , selfCheckFunc_({
            {kGet,   [this] { return checkGetLikeMethod(); }},
            {kPost,   [this] { return checkPostLikeMethod(); }},
            {kPut,    [this] { return checkPostLikeMethod(); }},
            {kDelete, [this] { return checkGetLikeMethod(); }}
        })
    {

    }

    // 设置和获取接收时间
    void setReceiveTime(muduo::Timestamp t);

    // 获取私有成员 receiveTime_
    muduo::Timestamp receiveTime() const { return receiveTime_; }

    // 设置和获取请求方法
    bool setMethod(const char* start, const char* end);
    bool setMethod(Method method);

    // 获取请求方法私有变量
    Method method() const { return method_; }

    // 设置和获取请求路径
    void setPath(const char* start, const char* end);

    void setPath(const std::string& path)
    {
        path_ = path;
    }

    // 获取请求路径私有变量
    std::string path() const { return path_; }

    // 设置和获取路径参数
    void setPathParameters(const std::string &key, const std::string &value);

    /// 获取路径参数
    std::string getPathParameters(const std::string &key) const;

    // 设置和获取查询参数
    void setQueryParameters(const char* start, const char* end);

    // 获取查询参数
    std::string getQueryParameters(const std::string &key) const;

    // 设置HTTP版本
    void setVersion(std::string v)
    {
        version_ = std::move(v);
    }

    // 获取版本
    std::string getVersion() const
    {
        return version_;
    }

    // 设置和获取请求头 每次接收一行请求头调用一次
    void addHeader(const char* start, const char* end);
    // 获取请求头，根据字段名获取对应值
    std::string getHeader(const std::string& field) const;
    // 获取所有请求头
    const std::map<std::string, std::string>& headers() const
    { return headers_; }

    // 设置请求体
    void setBody(const std::string& body) { content_ = body; }
    // 通过头尾指针设置请求体
    void setBody(const char* start, const char* end)
    {
        if (end >= start)
        {
            content_.assign(start, end - start);
        }
    }

    // 获取请求体
    std::string getBody() const
    { return content_; }

    // 设置和获取请求体长度
    void setContentLength(uint64_t length)
    { contentLength_ = length; }
    uint64_t contentLength() const
    { return contentLength_; }

    // 交换所有成员
    void swap(HttpRequest& that) noexcept;

    // 将所有成员通过json格式打印在控制台中
    void showDetails() const;

    // 进行自检验
    bool selfCheck() const;

private:
    std::unordered_map<Method, std::function<bool()>> selfCheckFunc_;

    bool checkGetLikeMethod() const;

    bool checkPostLikeMethod();

    Method                                       method_; // 请求方法
    std::string                                  version_; // http版本
    std::string                                  path_; // 请求路径
    std::unordered_map<std::string, std::string> pathParameters_; // 路径参数
    std::unordered_map<std::string, std::string> queryParameters_; // 查询参数
    muduo::Timestamp                             receiveTime_; // 接收时间
    std::map<std::string, std::string>           headers_; // 请求头
    std::string                                  content_; // 请求体
    uint64_t                                     contentLength_ { 0 }; // 请求体长度
};