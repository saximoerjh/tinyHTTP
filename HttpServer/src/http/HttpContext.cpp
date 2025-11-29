#include "http/HttpContext.h"

#include <regex>

/*
POST /api/login?debug=1 HTTP/1.1\r\n
Host: example.com\r\n
Content-Type: application/x-www-form-urlencoded\r\n
Content-Length: 27\r\n
\r\n
username=admin&password=123456
*/

// 解析HTTP请求报文
bool HttpContext::parseRequest(muduo::net::Buffer* buf, muduo::Timestamp receiveTime)
{
    // 解析请求行
    const char *crlf = buf->findCRLF();
    if (crlf)
    {
        if (!processRequestLine(buf->peek(), crlf))
        {
            return false;
        }
        buf->retrieveUntil(crlf + 2); // 移动读指针
        request_.setReceiveTime(receiveTime);
    }
    else
    {
        return false; // 请求行不完整
    }

    // 解析请求头 请求头的最后有两个crlf字符
    // 记录上一个CRLF位置
    const char* preCRLF = buf->findCRLF();
    while (preCRLF && preCRLF + 2 <= buf->peek() + buf->readableBytes())
    {
        const char* nextCRLF = buf->findCRLF(preCRLF + 2);
        if (nextCRLF == preCRLF + 2)
        {
            // 找到连续空行，解析请求头
            if (!processHeaders(buf->peek(), nextCRLF))
            {
                return false;
            }
            buf->retrieveUntil(nextCRLF + 2);
            break;
        }
        preCRLF = nextCRLF;
    }

    // 解析请求体
    if (buf->readableBytes() > 0)
    {
        // 读取剩余的请求体数据
        if (buf->readableBytes() < request_.contentLength())
        {
            LOG_ERROR << "Buffer overflow: Body length less than Content-Length";
            return false;
        }

        // 只读取 Content-Length 指定的长度
        auto body = std::string(buf->peek(), buf->peek() + request_.contentLength());
        request_.setBody(body);
        buf->retrieve(request_.contentLength());
    }

    // 最后request自校验
    complete_ =  request_.selfCheck();
    return complete_;
}

// 解析请求行
bool HttpContext::processRequestLine(const char* begin, const char* end)
{
    // 解析请求类型、路径、路径参数、HTTP版本
    // 按照空格分割，找到第一个空格位置作为请求类型的end
    const char* space = std::find(begin, end, ' ');
    if (space == end)
    {
        LOG_ERROR << "Invalid RequestLine";
        return false;
    }
    // 设置请求方法
    request_.setMethod(begin, space);
    // 移动起始指针
    const char* pathBegin = space + 1;
    space = std::find(pathBegin, end, ' ');
    const char* argumentBegin = std::find(pathBegin, space, '?');
    if (space == end)
    {
        LOG_ERROR << "Invalid RequestLine";
        return false;
    }
    // 设置请求路径
    request_.setPath(pathBegin, argumentBegin);
    // 设置查询参数
    if (argumentBegin != space)
    {
        request_.setQueryParameters(argumentBegin + 1, space);
    }
    const char* versionBegin = space + 1;
    // 检查HTTP版本格式
    if (std::string(versionBegin, end) != "HTTP/1.1" &&
        std::string(versionBegin, end) != "HTTP/1.0")
    {
        LOG_ERROR << "Invalid HTTP Version";
        return false;
    }
    // 设置HTTP版本
    request_.setVersion(std::string(versionBegin, end));
    return true;
}

// 解析请求头
bool HttpContext::processHeaders(const char* begin, const char* end)
{
    // 请求头格式 "sentence/r/n sentence/r/n"
    // 每次处理/r/n前面的部分
    const char* lineStart = begin;
    while (lineStart < end)
    {
        const char* lineEnd = std::find(lineStart, end, '\r');
        if (lineEnd == end || lineEnd + 1 == end || *(lineEnd + 1) != '\n')
        {
            LOG_ERROR << "Invalid Header Line";
            return false;
        }
        // 解析单个请求头
        request_.addHeader(lineStart, lineEnd);
        // 移动到下一行
        lineStart = lineEnd + 2;
    }
    return true;
}

// 解析请求体，get和delete方法一般没有body
bool HttpContext::processBody(const char* begin, const char* end)
{
    // 直接设置请求体内容
    try
    {
        request_.setBody(begin, end);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Invalid Request Body " << e.what();
    }
    return true;
}