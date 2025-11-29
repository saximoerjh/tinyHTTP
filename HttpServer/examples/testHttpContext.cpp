// 单元测试 HttpContext 类
#include "include/http/HttpContext.h"

#include <iostream>
#include <cassert>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Buffer.h>

void testGet(HttpContext& context)
{
    // 创建 Buffer 并填充一个简单的 HTTP 请求报文
    muduo::net::Buffer buffer;
    std::string httpRequest =
        "GET /index.html?debug=1&judge=2 HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "User-Agent: TestAgent/1.0\r\n"
        "Accept: */*\r\n"
        "\r\n";
    buffer.append(httpRequest);

    // 解析请求
    muduo::Timestamp receiveTime = muduo::Timestamp::now();
    bool result = context.parseRequest(&buffer, receiveTime);
    assert(result == true);

    // 获取解析后的 HttpRequest 对象
    const HttpRequest& request = context.request();

    // 验证请求行解析结果
    assert(request.method() == HttpRequest::kGet);
    assert(request.path() == "/index.html");
    assert(request.getHeader("Host") == "www.example.com");
    assert(request.getHeader("User-Agent") == "TestAgent/1.0");
    assert(request.getHeader("Accept") == "*/*");

    context.showRequest();

    std::cout << "HttpContext test passed!" << std::endl;
}

// 测试post
void testPost(HttpContext& context)
{
    // 创建 Buffer 并填充一个简单的 HTTP POST 请求报文
    muduo::net::Buffer buffer;
    std::string httpRequest =
        "POST /submit HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "field1=value1&field2=value2";
    buffer.append(httpRequest);

    // 解析请求
    muduo::Timestamp receiveTime = muduo::Timestamp::now();
    bool result = context.parseRequest(&buffer, receiveTime);
    assert(result == true);

    // 获取解析后的 HttpRequest 对象
    const HttpRequest& request = context.request();

    // 验证请求行解析结果
    assert(request.method() == HttpRequest::kPost);
    assert(request.path() == "/submit");
    assert(request.getHeader("Host") == "www.example.com");
    assert(request.getHeader("Content-Type") == "application/x-www-form-urlencoded");
    assert(request.getHeader("Content-Length") == "27");

    context.showRequest();

    std::cout << "HttpContext POST test passed!" << std::endl;
}


int main() {
    // 创建 HttpContext 对象
    HttpContext context;

    // 测试 GET 请求解析
    testGet(context);
    // 重置 HttpContext 对象以测试 POST 请求
    context.reset();
    // 测试 POST 请求解析
    testPost(context);

    return 0;
}