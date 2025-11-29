#include "http/HttpRequest.h"

#include <muduo/base/Logging.h>

void HttpRequest::setReceiveTime(muduo::Timestamp t)
{
    // 设置请求接收时间
    receiveTime_ = t;
}

void HttpRequest::setPath(const char* start, const char* end)
{
    path_ = std::string(start, end);
}

void HttpRequest::setPathParameters(const std::string& key, const std::string& value)
{
    pathParameters_[key] = value;
}

std::string HttpRequest::getPathParameters(const std::string& key) const
{
    auto it = pathParameters_.find(key);
    if (it != pathParameters_.end())
    {
        return it->second;
    }
    return "";
}

// 通过头尾指针设置查询键值对
// username=admin&password=123456
void HttpRequest::setQueryParameters(const char* start, const char* end)
{
    std::string qs(start, end);
    // 构造正则表达式进行键值对匹配
    std::regex re(R"(([^&=]+)=([^&]*))");
    const auto itBegin = std::sregex_iterator(qs.begin(), qs.end(), re);
    const auto itEnd = std::sregex_iterator();
    for (auto it = itBegin; it != itEnd; ++it) {
        const std::smatch& m = *it;
        queryParameters_[m[1]] = m[2];
    }
}

bool HttpRequest::setMethod(const char* start, const char* end)
{
    const std::string method(start, end);

    std::unordered_map<std::string, Method> parseMethods
    {
        {"GET", kGet}, {"POST", kPost}, {"DELETE", kDelete},
        {"PUT", kPut}, {"OPTIONS", kOptions}, {"HEAD", kHead}
    };

    if (const auto it = parseMethods.find(method); it != parseMethods.end())
    {
        method_ = it->second;
        return true;
    }
    return false;
}

std::string HttpRequest::getQueryParameters(const std::string &key) const
{
    auto it = queryParameters_.find(key);
    if (it != queryParameters_.end())
    {
        return it->second;
    }
    return "";
}

void HttpRequest::addHeader(const char* start, const char* end)
{
    std::string qs(start, end);
    // 构造正则表达式进行键值对匹配，只匹配第一个":"
    std::regex re(R"(([^&:=]+):\s*([^&]*))");
    std::smatch m;
    std::regex_search(qs, m, re);
    headers_[m[1]] = m[2];

    // 特殊处理Content-Length头
    if (m[1] == "Content-Length")
    {
        contentLength_ = std::stoull(m[2]);
    }
}

std::string HttpRequest::getHeader(const std::string& field) const
{
    auto it = headers_.find(field);
    if (it != headers_.end())
    {
        return it->second;
    }
    return "";
}

// 交换所有元素
void HttpRequest::swap(HttpRequest& that) noexcept
{
    std::swap(method_, that.method_);
    std::swap(path_, that.path_);
    std::swap(queryParameters_, that.queryParameters_);
    std::swap(pathParameters_, that.pathParameters_);
    std::swap(headers_, that.headers_);
    std::swap(contentLength_, that.contentLength_);
    std::swap(receiveTime_, that.receiveTime_);
}

void HttpRequest::showDetails() const
{
    nlohmann::json j;

    // method -> string
    auto methodToString = [](Method m) -> std::string {
        switch (m) {
        case kGet:     return "GET";
        case kPost:    return "POST";
        case kHead:    return "HEAD";
        case kPut:     return "PUT";
        case kDelete:  return "DELETE";
        case kOptions: return "OPTIONS";
        default:       return "INVALID";
        }
    };

    j["method"] = methodToString(method_);
    j["version"] = version_;
    j["path"] = path_;

    // pathParameters (unordered_map)
    nlohmann::json pathParams = nlohmann::json::object();
    for (const auto &p : pathParameters_) pathParams[p.first] = p.second;
    j["pathParameters"] = std::move(pathParams);

    // queryParameters (unordered_map)
    nlohmann::json queryParams = nlohmann::json::object();
    for (const auto &p : queryParameters_) queryParams[p.first] = p.second;
    j["queryParameters"] = std::move(queryParams);

    // receiveTime as microseconds since epoch (muduo::Timestamp)
    j["receiveTime_us"] = static_cast<long long>(receiveTime_.microSecondsSinceEpoch());
    // headers (std::map)
    nlohmann::json hdrs = nlohmann::json::object();
    for (const auto &h : headers_) hdrs[h.first] = h.second;
    j["headers"] = std::move(hdrs);

    j["content"] = content_;
    j["contentLength"] = contentLength_;

    LOG_INFO << "HttpRequest Details:\n" << j.dump(4);
}

// 简单的自检函数，检查必要字段是否存在，比如get和delete没有body，post和put必须有body和content-length > 0 content-Type 为规定值
bool HttpRequest::selfCheck() const
{
    // 检查selfCheckwFunc_是否注册了对应method的检查函数
    auto it = selfCheckFunc_.find(method_);
    if (it != selfCheckFunc_.end())
    {
        return it->second(); // 调用对应的检查函数
    }
    // 如果没有注册对应的检查函数，默认返回true
    return true;
}

bool HttpRequest::checkGetLikeMethod() const
{
    if (!content_.empty())
    {
        LOG_WARN << "GET request should not have a body.";
        return false;
    }
    return true;
}

bool HttpRequest::checkPostLikeMethod()
{
    if (content_.empty() || contentLength_ == 0)
    {
        LOG_ERROR << "POST request must have a body and Content-Length > 0.";
        return false;
    }

    auto contentType = getHeader("Content-Type");
    if (contentType != "application/x-www-form-urlencoded" && contentType != "application/json")
    {
        LOG_ERROR << "Unsupported Content-Type for POST request: " << contentType;
        return false;
    }

    // 校验contentLength_是否与content_长度匹配
    if (contentLength_ != content_.size())
    {
        LOG_WARN << "Content-Length does not match actual content size.";
        // 优先修正contentLength_
        contentLength_ = content_.size();
    }
    return true;
}