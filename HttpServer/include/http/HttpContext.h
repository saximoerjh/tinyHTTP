#pragma once

#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include "HttpRequest.h"


namespace tinyHttp
{
    class HttpContext
    {
    public:
        HttpContext()
        : complete_(false)
        {

        }

        // 解析HTTP请求
        bool parseRequest(muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

        bool parseComplete() const
        { return complete_ == true;  }

        void reset()
        {
            complete_ = false;
            HttpRequest dummyData;
            request_.swap(dummyData);
        }

        const HttpRequest& request() const
        { return request_;}

        HttpRequest& request()
        { return request_;}

        void showRequest() const
        {
            request_.showDetails();
        }

    private:
        // 解析请求行
        bool processRequestLine(const char* begin, const char* end);
        // 解析请求头
        bool processHeaders(const char* begin, const char* end);
        // 解析请求体
        bool processBody(const char* begin, const char* end);

        bool complete_;
        HttpRequest request_;
    };
}