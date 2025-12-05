#pragma once

#include <muduo/net/TcpServer.h>


namespace tinyHttp
{
    class HttpResponse
    {
    public:
        enum HttpStatusCode
        {
            kUnknown,
            k200Ok = 200,
            k204NoContent = 204,
            k301MovedPermanently = 301,
            k400BadRequest = 400,
            k401Unauthorized = 401,
            k403Forbidden = 403,
            k404NotFound = 404,
            k409Conflict = 409,
            k500InternalServerError = 500,
        };

        HttpResponse(bool close = true)
            : statusCode_(kUnknown)
            , closeConnection_(close)
        {}

        void setVersion(std::string version)
        { httpVersion_ = version; }
        void setStatusCode(HttpStatusCode code)
        { statusCode_ = code; }

        HttpStatusCode getStatusCode() const
        { return statusCode_; }

        void setStatusMessage(const std::string message)
        { statusMessage_ = message; }

        void setCloseConnection(bool on)
        { closeConnection_ = on; }

        bool closeConnection() const
        { return closeConnection_; }

        void setContentType(const std::string& contentType)
        { addHeader("Content-Type", contentType); }

        void setContentLength(uint64_t length)
        { addHeader("Content-Length", std::to_string(length)); }

        void addHeader(const std::string& key, const std::string& value)
        { headers_[key] = value; }

        void setBody(const std::string& body)
        {
            body_ = body;
            // body_ += "\0";
        }

        void setStatusLine(const std::string& version,
                             HttpStatusCode statusCode,
                             const std::string& statusMessage);

        void setErrorHeader(){}

        void appendToBuffer(muduo::net::Buffer* outputBuf) const;
    private:
        // 协议版本
        std::string                        httpVersion_;
        // 状态码
        HttpStatusCode                     statusCode_;
        // 状态消息
        std::string                        statusMessage_;
        // 是否关闭连接
        bool                               closeConnection_;
        // 响应头
        std::map<std::string, std::string> headers_;
        // 响应体
        std::string                        body_;
        bool                               isFile_;
    };
}